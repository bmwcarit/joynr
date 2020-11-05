/*
 * #%L
 * joynr::java::messaging::bounceproxy::controlled-bounceproxy
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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
import java.util.Map;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

import org.apache.http.HttpHeaders;
import org.apache.http.StatusLine;
import org.apache.http.client.methods.CloseableHttpResponse;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.entity.StringEntity;
import org.apache.http.impl.client.CloseableHttpClient;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.exceptions.JoynrHttpException;
import io.joynr.messaging.bounceproxy.BounceProxyControllerUrl;
import io.joynr.messaging.bounceproxy.BounceProxyPropertyKeys;
import io.joynr.util.ObjectMapper;

/**
 * Reporter for performance measures of the bounce proxy instance.
 * 
 * @author christina.strobel
 * 
 */
public class BounceProxyPerformanceReporter {

    private static final Logger logger = LoggerFactory.getLogger(BounceProxyPerformanceReporter.class);

    private ScheduledExecutorService executorService;

    ScheduledFuture<?> scheduledFuture;

    private int performanceReportingFrequencyMs;

    private final BounceProxyControllerUrl bounceProxyControllerUrl;
    private final CloseableHttpClient httpclient;

    private final BounceProxyPerformanceMonitor bounceProxyPerformanceMonitor;
    private final BounceProxyLifecycleMonitor bounceProxyLifecycleMonitor;

    private final ObjectMapper objectMapper;

    @Inject
    public BounceProxyPerformanceReporter(@Named(BounceProxyPropertyKeys.PROPERTY_BOUNCE_PROXY_MONITORING_FREQUENCY_MS) int performanceReportingFrequencyMs,
                                          BounceProxyControllerUrl bounceProxyControllerUrl,
                                          CloseableHttpClient httpclient,
                                          BounceProxyPerformanceMonitor bounceProxyPerformanceMonitor,
                                          BounceProxyLifecycleMonitor bounceProxyLifecycleMonitor,
                                          ObjectMapper objectMapper,
                                          ScheduledExecutorService executorService) {
        this.performanceReportingFrequencyMs = performanceReportingFrequencyMs;
        this.bounceProxyControllerUrl = bounceProxyControllerUrl;
        this.httpclient = httpclient;
        this.bounceProxyPerformanceMonitor = bounceProxyPerformanceMonitor;
        this.bounceProxyLifecycleMonitor = bounceProxyLifecycleMonitor;
        this.objectMapper = objectMapper;
        this.executorService = executorService;
    }

    /**
     * Starts a reporting loop that sends performance measures to the monitoring
     * service in a certain frequency.
     */
    public void startReporting() {

        if (executorService == null) {
            throw new IllegalStateException("MonitoringServiceClient already shutdown");
        }

        if (scheduledFuture != null) {
            logger.error("Only one performance reporting thread can be started");
            return;
        }

        Runnable performanceReporterCallable = new Runnable() {

            @Override
            public void run() {
                try {

                    // only send a report if the bounce proxy is initialized
                    if (bounceProxyLifecycleMonitor.isInitialized()) {

                        // Don't do any retries here. If we miss on performance
                        // report, we'll just wait for the next loop.
                        sendPerformanceReportAsHttpRequest();
                    }

                } catch (Exception e) {
                    logger.error("Uncaught exception in reportPerformance.", e);
                }
            }

        };

        scheduledFuture = executorService.scheduleWithFixedDelay(performanceReporterCallable,
                                                                 performanceReportingFrequencyMs,
                                                                 performanceReportingFrequencyMs,
                                                                 TimeUnit.MILLISECONDS);
    }

    /**
     * Sends an HTTP request to the monitoring service to report performance
     * measures of a bounce proxy instance.
     * 
     * @throws IOException
     */
    private void sendPerformanceReportAsHttpRequest() throws IOException {

        final String url = bounceProxyControllerUrl.buildReportPerformanceUrl();
        logger.debug("Using monitoring service URL: {}", url);

        Map<String, Integer> performanceMap = bounceProxyPerformanceMonitor.getAsKeyValuePairs();
        String serializedMessage = objectMapper.writeValueAsString(performanceMap);

        HttpPost postReportPerformance = new HttpPost(url.trim());
        // using http apache constants here because JOYNr constants are in
        // libjoynr which should not be included here
        postReportPerformance.addHeader(HttpHeaders.CONTENT_TYPE, "application/json");
        postReportPerformance.setEntity(new StringEntity(serializedMessage, "UTF-8"));

        CloseableHttpResponse response = null;
        try {
            response = httpclient.execute(postReportPerformance);
            StatusLine statusLine = response.getStatusLine();
            int statusCode = statusLine.getStatusCode();

            if (statusCode != HttpURLConnection.HTTP_NO_CONTENT) {
                logger.error("Failed to send performance report: {}", response);
                throw new JoynrHttpException(statusCode, "Failed to send performance report.");
            }

        } finally {
            if (response != null) {
                response.close();
            }
        }
    }

    /**
     * Stops the performance reporting loop.
     * 
     * @return <code>false</code> if the reporing task could not be cancelled
     *         (maybe because it has already completed normally),
     *         <code>true</code> otherwise
     */
    public boolean stopReporting() {

        // quit the bounce proxy heartbeat but let a possibly running request
        // finish
        if (scheduledFuture != null) {
            return scheduledFuture.cancel(false);
        }

        executorService.shutdown();
        executorService = null;
        return false;
    }
}
