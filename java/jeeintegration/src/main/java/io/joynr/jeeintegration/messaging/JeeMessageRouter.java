package io.joynr.jeeintegration.messaging;

/*
 * #%L
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

import static java.lang.String.format;

import java.util.concurrent.DelayQueue;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.accesscontrol.AccessController;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingSkeletonFactory;
import io.joynr.runtime.ClusterControllerRuntimeModule;
import io.joynr.runtime.ShutdownNotifier;
import io.joynr.messaging.routing.AddressManager;
import io.joynr.messaging.routing.DelayableImmutableMessage;
import io.joynr.messaging.routing.MessagingStubFactory;
import io.joynr.messaging.routing.MulticastReceiverRegistry;
import io.joynr.messaging.routing.RoutingTable;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * The MessageRouter is responsible for routing messages to their destination, and internally queues message post
 * requests using an executor service.
 * <p>
 * This override of the normal {@link io.joynr.messaging.routing.CcMessageRouter} is necessary, because the standard
 * implementation calls {@link ScheduledExecutorService#isShutdown()}, which results in an exception in a JEE
 * environment. Hence, this implementation overrides the {@link #schedule(Runnable, String, long, TimeUnit)} method and provides an
 * implementation which doesn't call <code>isShutdown()</code>.
 *
 * @see io.joynr.messaging.routing.CcMessageRouter
 */
@edu.umd.cs.findbugs.annotations.SuppressWarnings(value = "JLM_JSR166_UTILCONCURRENT_MONITORENTER", justification = "ensure that no new messages are scheduled when scheduler is shuting down")
public class JeeMessageRouter extends io.joynr.messaging.routing.CcMessageRouter {

    private static final Logger LOG = LoggerFactory.getLogger(JeeMessageRouter.class);
    private ScheduledExecutorService scheduler;

    @Inject
    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 8 LINES
    public JeeMessageRouter(RoutingTable routingTable,
                            @Named(SCHEDULEDTHREADPOOL) ScheduledExecutorService scheduler,
                            @Named(ConfigurableMessagingSettings.PROPERTY_SEND_MSG_RETRY_INTERVAL_MS) long sendMsgRetryIntervalMs,
                            @Named(ConfigurableMessagingSettings.PROPERTY_MESSAGING_MAXIMUM_PARALLEL_SENDS) int maxParallelSends,
                            @Named(ConfigurableMessagingSettings.PROPERTY_ROUTING_TABLE_GRACE_PERIOD_MS) long routingTableGracePeriodMs,
                            @Named(ConfigurableMessagingSettings.PROPERTY_ROUTING_TABLE_CLEANUP_INTERVAL_MS) long routingTableCleanupIntervalMs,
                            MessagingStubFactory messagingStubFactory,
                            MessagingSkeletonFactory messagingSkeletonFactory,
                            AddressManager addressManager,
                            MulticastReceiverRegistry multicastReceiverRegistry,
                            AccessController accessController,
                            @Named(ClusterControllerRuntimeModule.PROPERTY_ACCESSCONTROL_ENABLE) boolean enableAccessControl,
                            DelayQueue<DelayableImmutableMessage> messageQueue,
                            ShutdownNotifier shutdownNotifier) {
        super(routingTable,
              scheduler,
              sendMsgRetryIntervalMs,
              maxParallelSends,
              routingTableGracePeriodMs,
              routingTableCleanupIntervalMs,
              messagingStubFactory,
              messagingSkeletonFactory,
              addressManager,
              multicastReceiverRegistry,
              accessController,
              enableAccessControl,
              messageQueue,
              shutdownNotifier);
        if (LOG.isDebugEnabled()) {
            LOG.debug(format("Initialising with:%n\troutingTable: %s%n\tscheduler: %s%n\tsendMsgRetryIntervalMs: %d%n\tmessageStubFactory: %s",
                             routingTable,
                             scheduler,
                             sendMsgRetryIntervalMs,
                             messagingStubFactory));
        }
        this.scheduler = scheduler;
    }

    @Override
    protected void schedule(Runnable runnable, String messageId, long delay, TimeUnit timeUnit) {
        LOG.trace("Scheduling {} on {} with delay {} {}", new Object[]{ runnable, scheduler, delay, timeUnit });
        scheduler.schedule(runnable, delay, timeUnit);
    }
}
