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
import java.util.concurrent.Callable;
import java.util.concurrent.CancellationException;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Future;
import java.util.concurrent.Semaphore;

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
import io.joynr.messaging.bounceproxy.ControlledBounceProxyUrl;

/**
 * Reporter for bounce proxy startup events.<br>
 * 
 * @author christina.strobel
 * 
 */
public class BounceProxyStartupReporter {

    private static final Logger logger = LoggerFactory.getLogger(BounceProxyStartupReporter.class);

    private boolean startupEventReported = false;

    private CloseableHttpClient httpclient;

    private long sendReportRetryIntervalMs;

    private Future<Boolean> startupReportFuture;

    private final BounceProxyControllerUrl bounceProxyControllerUrl;
    private final ControlledBounceProxyUrl controlledBounceProxyUrl;

    private ExecutorService execService;

    private final Semaphore startupReportFutureAvailable = new Semaphore(0, true);

    @Inject
    public BounceProxyStartupReporter(CloseableHttpClient httpclient,
                                      BounceProxyControllerUrl bounceProxyControllerUrl,
                                      ControlledBounceProxyUrl controlledBounceProxyUrl,
                                      ExecutorService execService,
                                      @Named(BounceProxyPropertyKeys.PROPERTY_BOUNCE_PROXY_SEND_LIFECYCLE_REPORT_RETRY_INTERVAL_MS) long sendReportRetryIntervalMs) {
        this.httpclient = httpclient;
        this.sendReportRetryIntervalMs = sendReportRetryIntervalMs;
        this.bounceProxyControllerUrl = bounceProxyControllerUrl;
        this.controlledBounceProxyUrl = controlledBounceProxyUrl;
        this.execService = execService;
    }

    /**
     * Notifies the monitoring service that a bounce proxy has changed its
     * lifecycle state.<br>
     * This call starts a loop that tries to send reports to the bounce proxy
     * controller until the bounce proxy is registered at the bounce proxy
     * controller.<br>
     * Startup reporting is done in a separate thread, so that a possibly
     * infinite loop won't block the whole servlet. Especially, this allows the
     * servlet to be stopped at any time.
     */
    public void startReporting() {

        // make sure we report the lifecycle event only once
        if (startupEventReported == true) {
            logger.error("Startup event has already been reported or is being reported now");
            return;
        }

        startupEventReported = true;

        // the startup event has to be reported in its own thread, as this is
        // done forever. Otherwise a servlet will hang in startup mode forever
        // and can't be stopped by normal admin commands.
        Callable<Boolean> reportEventCallable = new Callable<Boolean>() {

            public Boolean call() {
                try {
                    // Notify BPC of BP startup event in a loop.
                    if (reportEventLoop()) {
                        // reporting successful within the allowed number of
                        // retries
                        return true;
                    }

                    // The BPC could not be reached, so return.
                    logger.error("Bounce Proxy lifecycle event notification failed. Exiting.");

                } catch (Exception e) {
                    logger.error("Uncaught exception in reporting lifecycle event.", e);
                }
                startupEventReported = false;
                return false;
            }
        };

        // don't wait for the callable to end so that shutting down is possible
        // at any time
        startupReportFuture = execService.submit(reportEventCallable);
        startupReportFutureAvailable.release();
    }

    /**
     * Gets whether the bounce proxy has been registered successfully at the
     * bounce proxy controller. Successful registration means that a startup
     * notification has been sent to the bounce proxy controller and the bounce
     * proxy controller responded that the bounce proxy was registered.<br>
     * This call is non-blocking, i.e. it won't wait for the bounce proxy to be
     * registered.
     * 
     * @return <code>true</code> if the bounce proxy has successfully been
     *         registered.<br>
     *         <code>false</code> if one of:
     *         <ul>
     *         <li>startup reporting hasn't started</li>
     *         <li>startup reporting is still ongoing</li>
     *         <li>startup reporting was cancelled before registration was
     *         successful</li>
     *         <li>the bounce proxy controller responds that the bounce proxy
     *         could not be registered for any reason</li>
     *         </ul>
     */
    public boolean hasBounceProxyBeenRegistered() {

        // startup reporting hasn't started
        if (!startupEventReported) {
            return false;
        }

        // startup reporting still ongoing - don't block this call!
        if (!startupReportFuture.isDone()) {
            return false;
        }

        // get result
        try {
            return startupReportFuture.get();
        } catch (InterruptedException e) {
            return false;
        } catch (ExecutionException e) {
            return false;
        } catch (CancellationException e) {
            // reporting was cancelled before startup could be reported
            return false;
        }
    }

    /**
     * Cancels startup reporting.
     */
    public void cancelReporting() {
        if (startupReportFuture != null) {
            startupReportFuture.cancel(true);
        }
        if (execService != null) {
            // interrupt reporting loop
            execService.shutdownNow();
        }
    }

    /**
     * Starts a loop to send a startup report to the monitoring service.<br>
     * The loop is executed forever, if startup reporting isn't successful.
     * 
     * @return <code>true</code> if a startup report could be sent successfully
     *         and the bounce proxy controller responded that registration was
     *         successful. If <code>false</code> is returned, then sending a
     *         report to the service failed.
     */
    private boolean reportEventLoop() {

        // try forever to reach the bounce proxy, as otherwise the bounce proxy
        // isn't useful anyway
        while (true) {
            try {
                startupReportFutureAvailable.acquire();
                break;
            } catch (InterruptedException e) {
                // ignore & retry
            }
        }
        while (startupReportFuture != null && !startupReportFuture.isCancelled()) {
            try {
                reportEventAsHttpRequest();
                // if no exception is thrown, reporting was successful and we
                // can return here
                startupReportFutureAvailable.release();
                return true;
            } catch (Exception e) {
                // TODO we might have to intercept the JoynrHttpMessage if the
                // bounce proxy responds that the client could not be
                // registered. It could be that two clients try to register with
                // the same ID. Then we won't have to try to register forever.
                // Do a more detailed error handling here!
                logger.error("Error notifying of Bounce Proxy startup. Error:", e);
            }
            try {
                Thread.sleep(sendReportRetryIntervalMs);
            } catch (InterruptedException e) {
                startupReportFutureAvailable.release();
                return false;
            }
        }
        startupReportFutureAvailable.release();
        return false;
    }

    /**
     * Reports the lifecycle event to the monitoring service as HTTP request.
     * 
     * @throws IOException
     *             if the connection with the bounce proxy controller could not
     *             be established
     * @throws JoynrHttpException
     *             if the bounce proxy responded that registering the bounce
     *             proxy did not succeed
     */
    private void reportEventAsHttpRequest() throws IOException {

        final String url = bounceProxyControllerUrl.buildReportStartupUrl(controlledBounceProxyUrl);
        logger.debug("Using monitoring service URL: {}", url);

        HttpPut putReportStartup = new HttpPut(url.trim());

        CloseableHttpResponse response = null;
        try {
            response = httpclient.execute(putReportStartup);
            StatusLine statusLine = response.getStatusLine();
            int statusCode = statusLine.getStatusCode();

            switch (statusCode) {
            case HttpURLConnection.HTTP_NO_CONTENT:
                // bounce proxy instance was already known at the bounce proxy
                // controller
                // nothing else to do
                logger.info("Bounce proxy was already registered with the controller");
                break;

            case HttpURLConnection.HTTP_CREATED:
                // bounce proxy instance was registered for the very first time
                // TODO maybe read URL
                logger.info("Registered bounce proxy with controller");
                break;

            default:
                logger.error("Failed to send startup notification: {}", response);
                throw new JoynrHttpException(statusCode,
                                             "Failed to send startup notification. Bounce Proxy won't get any channels assigned.");
            }

        } finally {
            if (response != null) {
                response.close();
            }
        }
    }

}
