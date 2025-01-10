/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2025 BMW Car IT GmbH
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
package io.joynr.runtime;

import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledThreadPoolExecutor;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.TimeUnit;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Provider;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.util.JoynrThreadFactory;

@Singleton
public class DefaultScheduledExecutorServiceProvider implements Provider<ScheduledExecutorService>, ShutdownListener {
    private static final Logger logger = LoggerFactory.getLogger(DefaultScheduledExecutorServiceProvider.class);
    private static final int MQTT_THREADS = 4;
    private static final int MAX_SKELETON_THREADS = 5;
    private static final long TERMINATION_TIMEOUT = 5000;
    private ScheduledThreadPoolExecutor scheduler;

    @Inject
    public DefaultScheduledExecutorServiceProvider(@Named(ConfigurableMessagingSettings.PROPERTY_MESSAGING_MAXIMUM_PARALLEL_SENDS) int maximumParallelSends,
                                                   ShutdownNotifier shutdownNotifier) {
        ThreadFactory schedulerNamedThreadFactory = new JoynrThreadFactory("ScheduledExecutorService", true);

        /*
         * Number of required threads (numbers in parentheses mean: no dedicated thread is required here):
         *
         * MessageRouter: #maximumParallelSends (default: 20) messageWorkers
         *                (1) routingTableCleanup
         * ArbitratorFactory: 1 arbitratorRunnable
         * MessagingSkeletonFactory: 1 per skeleton (transport), only required during startup, can be executed one after the other
         * HivemqMqttClientFactory: 4 per Mqtt connection?
         * ExpiredDiscoveryEntryCacheCleaner: (1) cleanupAction
         * RequestReplyManagerImpl: (1) cleanupScheduler for queued requests
         * PublicationManagerImpl: (1) cleanupScheduler for subscriptions
         * SubscriptionManagerImpl: (1) cleanupScheduler for subscriptions
         * ReplyCallerDirectory: (1) cleanupScheduler for ReplyCallers
        */
        scheduler = new ScheduledThreadPoolExecutor(maximumParallelSends + MAX_SKELETON_THREADS + MQTT_THREADS,
                                                    schedulerNamedThreadFactory);
        scheduler.setKeepAliveTime(100, TimeUnit.SECONDS);
        scheduler.allowCoreThreadTimeOut(true);

        shutdownNotifier.registerToBeShutdownAsLast(this);
    }

    @Override
    public ScheduledExecutorService get() {
        return scheduler;
    }

    @Override
    public void shutdown() {
        logger.debug("shutdown invoked");
        scheduler.setContinueExistingPeriodicTasksAfterShutdownPolicy(false);
        scheduler.setExecuteExistingDelayedTasksAfterShutdownPolicy(false);
        scheduler.setRemoveOnCancelPolicy(true);
        scheduler.setContinueExistingPeriodicTasksAfterShutdownPolicy(false);
        scheduler.purge();
        scheduler.shutdown();
        try {
            if (!scheduler.awaitTermination(TERMINATION_TIMEOUT, TimeUnit.MILLISECONDS)) {
                logger.error("Message Scheduler did not shut down in time. Timed out waiting for executor service to shutdown after {}ms.",
                             TERMINATION_TIMEOUT);
                logger.debug("Attempting to shutdown scheduler {} forcibly.", scheduler);
                scheduler.shutdownNow();
            }
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            logger.error("Message Scheduler shutdown interrupted:", e);
        }
        logger.debug("shutdown finished");
    }
}
