/*
 * #%L
 * joynr::java::messaging::bounceproxy::controlled-bounceproxy
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
 * %%
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * #L%
 */
package io.joynr.messaging.bounceproxy.monitoring;

import java.io.IOException;
import java.net.HttpURLConnection;

import org.apache.http.StatusLine;
import org.apache.http.client.methods.CloseableHttpResponse;
import org.apache.http.client.methods.HttpPut;
import org.apache.http.impl.client.CloseableHttpClient;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.exceptions.JoynrHttpException;
import io.joynr.messaging.bounceproxy.BounceProxyControllerUrl;
import io.joynr.messaging.bounceproxy.BounceProxyPropertyKeys;
import io.joynr.messaging.system.TimestampProvider;

/**
 * Reporter for bounce proxy shutdown events.
 * 
 * @author christina.strobel
 * 
 */
public class BounceProxyShutdownReporter {

    private static final Logger logger = LoggerFactory.getLogger(BounceProxyShutdownReporter.class);

    private CloseableHttpClient httpclient;

    private long sendReportRetryIntervalMs;
    private long maxSendShutdownTimeSecs;

    private final BounceProxyControllerUrl bounceProxyControllerUrl;

    private final TimestampProvider timestampProvider;

    @Inject
    public BounceProxyShutdownReporter(CloseableHttpClient httpclient,
                                       BounceProxyControllerUrl bounceProxyControllerUrl,
                                       TimestampProvider timestampProvider,
                                       @Named(BounceProxyPropertyKeys.PROPERTY_BOUNCE_PROXY_SEND_LIFECYCLE_REPORT_RETRY_INTERVAL_MS) long sendReportRetryIntervalMs,
                                       @Named(BounceProxyPropertyKeys.PROPERTY_BOUNCE_PROXY_MAX_SEND_SHUTDOWN_TIME_SECS) long maxSendShutdownTimeSecs) {
        this.httpclient = httpclient;
        this.sendReportRetryIntervalMs = sendReportRetryIntervalMs;
        this.maxSendShutdownTimeSecs = maxSendShutdownTimeSecs;
        this.bounceProxyControllerUrl = bounceProxyControllerUrl;
        this.timestampProvider = timestampProvider;
    }

    /**
     * Notifies the monitoring service that a bounce proxy is about to shut
     * down.<br>
     * This call starts a loop that tries to send reports to the bounce proxy
     * controller until the bounce proxy is registered at the bounce proxy
     * controller.<br>
     * Shutdown reporting is done in the same thread for a certain amount of
     * time, so that the servlet won't be blocked forever.
     */
    public void reportShutdown() {

        // The shutdown event is reported in the main thread, as this will be
        // done only for a certain amount of time.
        try {
            // Notify BPC of BP shutdown event in a loop.
            reportEventLoop();

        } catch (Exception e) {
            logger.error("Uncaught exception in reporting lifecycle event.", e);
        }
    }

    /**
     * Starts a loop to send a shutdown report to the monitoring service.<br>
     * The loop is executed for a certain amount of time, if shutdown reporting
     * is not successful.
     */
    private void reportEventLoop() {

        long loopStartingTime = timestampProvider.getCurrentTime();
        boolean shutdownReportSuccessful = false;

        // try for at most maxSendShutdownTimeSecs secs to reach the bounce
        // proxy, otherwise just shutdown
        while (!shutdownReportSuccessful
                && (timestampProvider.getCurrentTime() - loopStartingTime) < maxSendShutdownTimeSecs * 1000) {
            try {
                reportEventAsHttpRequest();
                // if no exception is thrown, reporting was successful and we
                // can return here
                shutdownReportSuccessful = true;
            } catch (Exception e) {
                // TODO we might have to intercept the JoynrHttpMessage if the
                // bounce proxy responds that the client could not be
                // de-registered. Then we won't have to try to register forever.
                // Do a more detailed error handling here!
                logger.error("Error notifying Bounce Proxy shutdown. Error:", e);
            }
            try {
                Thread.sleep(sendReportRetryIntervalMs);
            } catch (InterruptedException e) {
                break;
            }
        }
    }

    protected void reportEventAsHttpRequest() throws IOException {

        final String url = bounceProxyControllerUrl.buildReportShutdownUrl();
        logger.debug("Using monitoring service URL: {}", url);

        HttpPut putReportShutdown = new HttpPut(url.trim());

        CloseableHttpResponse response = null;
        try {
            response = httpclient.execute(putReportShutdown);
            StatusLine statusLine = response.getStatusLine();
            int statusCode = statusLine.getStatusCode();

            if (statusCode != HttpURLConnection.HTTP_NO_CONTENT) {
                // unexpected response
                logger.error("Failed to send shutdown notification: {}", response);
                throw new JoynrHttpException(statusCode,
                                             "Failed to send shutdown notification. Bounce Proxy still registered at Monitoring Service.");
            } else {
                logger.debug("Successfully sent shutdown notification");
            }

        } finally {
            if (response != null) {
                response.close();
            }
        }
    }

}
