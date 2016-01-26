package io.joynr.messaging;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

import joynr.system.RoutingTypes.Address;

@Singleton
public class MessagingSkeletonFactory {

    public static final String MIDDLEWARE_MESSAGING_SKELETONS = "MIDDLEWARE_MESSAGING_SKELETONS";
    private Map<Class<? extends Address>, IMessagingSkeleton> messagingSkeletons;

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
     * @param messagingSkeletons
     */
    @Inject
    public MessagingSkeletonFactory(@Named(MIDDLEWARE_MESSAGING_SKELETONS) Map<Class<? extends Address>, IMessagingSkeleton> messagingSkeletons) {
        this.messagingSkeletons = messagingSkeletons;
    }

    public void start() {
        for (IMessagingSkeleton messagingSkeleton : messagingSkeletons.values()) {
            messagingSkeleton.init();
        }
    }

    public void shutdown() {
        for (IMessagingSkeleton messagingSkeleton : messagingSkeletons.values()) {
            messagingSkeleton.shutdown();
        }
    }
}
