package io.joynr.messaging;

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

import java.util.Map;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import javax.annotation.CheckForNull;

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

    public static final String MIDDLEWARE_MESSAGING_SKELETONS = "MIDDLEWARE_MESSAGING_SKELETONS";
    private Map<Class<? extends Address>, IMessagingSkeleton> messagingSkeletons;
    private ScheduledExecutorService scheduler;

    /**
     * Transport Middleware implementation may be registered for use with a given Address type using guice multibinders.
     * <pre>
     *  messagingSkeletonFactory = MapBinder.newMapBinder(binder(),
     *      new TypeLiteral {@code<Class<? extends Address>>}() {},
     *      new TypeLiteral {@code<IMessagingSkeleton>()} {},
     *      Names.named(MessagingSkeletonFactory.MIDDLEWARE_MESSAGING_SKELETONS));
     *  messagingSkeletonFactory.addBinding(InProcessAddress.class).to(InProcessMessagingSkeleton.class);
     * </pre>
     *
     * @param messagingSkeletons a map of all skeletons (message receivers) that are to be started
     * @param scheduler ExecutorService that schedules all messaging communication
     */
    @Inject
    public MessagingSkeletonFactory(@Named(MIDDLEWARE_MESSAGING_SKELETONS) Map<Class<? extends Address>, IMessagingSkeleton> messagingSkeletons,
                                    @Named(MessageRouter.SCHEDULEDTHREADPOOL) ScheduledExecutorService scheduler,
                                    ShutdownNotifier shutdownNotifier) {
        this.messagingSkeletons = messagingSkeletons;
        this.scheduler = scheduler;
        shutdownNotifier.registerForShutdown(this);
    }

    public void start() {
        for (final IMessagingSkeleton messagingSkeleton : messagingSkeletons.values()) {
            scheduler.schedule(new Runnable() {

                @Override
                public void run() {
                    try {
                        messagingSkeleton.init();
                    } catch (Exception e) {
                        logger.error("unable to start skeleton: {}. Reason: {}",
                                     messagingSkeleton.getClass().getSimpleName(),
                                     e.getMessage());
                    }
                }
            }, 0, TimeUnit.MILLISECONDS);
        }
    }

    @Override
    public void shutdown() {
        for (IMessagingSkeleton messagingSkeleton : messagingSkeletons.values()) {
            messagingSkeleton.shutdown();
        }
    }

    @CheckForNull
    public IMessagingSkeleton getSkeleton(Address address) {
        if (address != null) {
            return messagingSkeletons.get(address.getClass());
        }
        return null;
    }
}
