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
package io.joynr.messaging;

import java.util.HashMap;
import java.util.Map;
import java.util.Optional;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

import io.joynr.messaging.routing.MessageRouter;
import io.joynr.runtime.ShutdownListener;
import io.joynr.runtime.ShutdownNotifier;
import joynr.system.RoutingTypes.Address;

@Singleton
public class MessagingSkeletonFactory implements ShutdownListener {

    private static final Logger logger = LoggerFactory.getLogger(MessagingSkeletonFactory.class);

    public static final String MIDDLEWARE_MESSAGING_SKELETON_FACTORIES = "MIDDLEWARE_MESSAGING_SKELETON_FACTORIES";
    private Map<Class<? extends Address>, IMessagingSkeletonFactory> messagingSkeletonFactories;
    private ScheduledExecutorService scheduler;
    private boolean started;

    /**
     * Transport Middleware implementation may be registered for use with a given Address type using guice multibinders.
     * <pre>
     *  messagingSkeletonFactory = MapBinder.newMapBinder(binder(),
     *      new TypeLiteral {@code<Class<? extends Address>>}() {},
     *      new TypeLiteral {@code<IMessagingSkeleton>()} {},
     *      Names.named(MessagingSkeletonFactory.MIDDLEWARE_MESSAGING_SKELETONS));
     *  messagingSkeletonFactory.addBinding(InProcessAddress.class).to(InProcessMessagingSkeletonFactory.class);
     * </pre>
     *
     * @param messagingSkeletonFactories a map of all skeletons (message receivers) that are to be started
     * @param scheduler ExecutorService that schedules all messaging communication
     * @param shutdownNotifier ShutdownNotifier
     */
    @Inject
    public MessagingSkeletonFactory(@Named(MIDDLEWARE_MESSAGING_SKELETON_FACTORIES) Map<Class<? extends Address>, IMessagingSkeletonFactory> messagingSkeletonFactories,
                                    @Named(MessageRouter.SCHEDULEDTHREADPOOL) ScheduledExecutorService scheduler,
                                    ShutdownNotifier shutdownNotifier) {
        this.messagingSkeletonFactories = (messagingSkeletonFactories != null)
                ? new HashMap<>(messagingSkeletonFactories)
                : null;
        this.scheduler = scheduler;
        shutdownNotifier.registerForShutdown(this);
    }

    public synchronized void start() {
        if (!started) {
            for (final IMessagingSkeletonFactory messagingSkeletonFactory : messagingSkeletonFactories.values()) {
                scheduler.schedule(new Runnable() {

                    @Override
                    public void run() {
                        try {
                            messagingSkeletonFactory.init();
                        } catch (Exception e) {
                            logger.error("Unable to start skeleton: {}. Reason:",
                                         messagingSkeletonFactory.getClass().getSimpleName(),
                                         e);
                        }
                    }
                }, 0, TimeUnit.MILLISECONDS);
            }
        } else {
            logger.info("Already started - skipping.");
        }
        started = true;
    }

    @Override
    public void shutdown() {
        for (IMessagingSkeletonFactory messagingSkeletonFactory : messagingSkeletonFactories.values()) {
            messagingSkeletonFactory.shutdown();
        }
    }

    public Optional<IMessagingSkeleton> getSkeleton(Address address) {
        if (address != null) {
            return Optional.of(messagingSkeletonFactories.get(address.getClass()).getSkeleton(address));
        }
        return Optional.empty();
    }
}
