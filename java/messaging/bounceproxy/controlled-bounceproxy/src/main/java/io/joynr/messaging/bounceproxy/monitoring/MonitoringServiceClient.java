package io.joynr.messaging.bounceproxy.monitoring;

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

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;

/**
 * Creation of requests to report startup, shutdown and performance measures to
 * the monitoring service a bounce proxy controller.
 * 
 * @author christina.strobel
 * 
 */
@Singleton
public class MonitoringServiceClient implements BounceProxyLifecycleMonitor {

    private static final Logger logger = LoggerFactory.getLogger(MonitoringServiceClient.class);

    private final BounceProxyStartupReporter startupReporter;
    private final BounceProxyShutdownReporter shutdownReporter;
    private final BounceProxyPerformanceReporter performanceReporter;

    /**
     * Creates a new reporter for monitoring data.
     * 
     * @param startupReporter
     *   bounce proxy startup reporter instance
     * @param shutdownReporter
     *   bounce proxy shutdown reporter instance
     * @param performanceReporter
     *   bounce proxy performance reporter instance
     */
    @Inject
    public MonitoringServiceClient(BounceProxyStartupReporter startupReporter,
                                   BounceProxyShutdownReporter shutdownReporter,
                                   BounceProxyPerformanceReporter performanceReporter) {
        this.startupReporter = startupReporter;
        this.shutdownReporter = shutdownReporter;
        this.performanceReporter = performanceReporter;
    }

    /**
     * Starts reporting the performance of a bounce proxy to the monitoring
     * service.<br>
     * This only triggers a scheduled reporting event. No performance reports
     * are actually sent until a startup report could be sent to the bounce
     * proxy controller.
     */
    public void startPerformanceReport() {
        logger.debug("Starting bounce proxy reporting loop");
        performanceReporter.startReporting();
    }

    /**
     * Notifies the monitoring service that a bounce proxy has started up and is
     * ready to handle messaging. <br>
     * This call only triggers startup reporting but does not wait until the
     * startup was reported successfully. A non-blocking call is used to avoid
     * that any servlets are in initializing state forever and can't be shut
     * down.<br>
     * If the Bounce Proxy Controller can't be notified of the startup, this
     * means that this bounce proxy won't get any channels assigned for
     * messaging. Thus startup reports are sent forever, i.e. until the servlet
     * is shut down.
     */
    public void startStartupReporting() {
        logger.debug("Notifying of bounce proxy instance startup");
        startupReporter.startReporting();
    }

    /**
     * Returns whether the bounce proxy has successfully reported startup and is
     * ready to process normally.
     * 
     * @return <code>true</code> if startup has been reported successfully,
     *         <code>false</code> if no attempts to report startup have been
     *         made or if it is still trying to report startup
     */
    public boolean hasReportedStartup() {
        return startupReporter.hasBounceProxyBeenRegistered();
    }

    /**
     * Notifies the bounce proxy controller that it is going to shutdown and
     * won't take any new channels for messaging. Performance reporting is quit
     * in here as well.<br>
     * This call blocks until either the startup was reported successfully or
     * the maximum number of reporting retries was used up.<br>
     * If the Bounce Proxy Controller can't be notified of the shutdown, this
     * means that this bounce proxy might still get channels assigned for
     * messaging.
     */
    public void reportShutdown() {
        logger.debug("Notifying of bounce proxy instance shutdown");

        // cancel any reporting first
        startupReporter.cancelReporting();

        // quit performance report loop
        performanceReporter.stopReporting();

        // send the shutdown message to the BPC
        shutdownReporter.reportShutdown();
    }

    @Override
    public boolean isInitialized() {
        return hasReportedStartup();
    }
}
