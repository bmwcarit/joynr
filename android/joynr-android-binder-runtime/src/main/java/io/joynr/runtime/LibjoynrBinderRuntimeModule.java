/*-
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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

import android.content.Context;
import android.os.Process;

import com.google.inject.Provides;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

import io.joynr.android.AndroidBinderRuntime;
import io.joynr.android.messaging.binder.BinderClientMessagingStubFactory;
import io.joynr.android.messaging.binder.BinderMessagingSkeletonFactory;
import io.joynr.android.messaging.binder.BinderMulticastAddressCalculator;
import io.joynr.messaging.GbidArrayFactory;
import io.joynr.messaging.MulticastReceiverRegistrar;
import io.joynr.messaging.routing.DummyRoutingTable;
import io.joynr.messaging.routing.LibJoynrMessageRouter;
import io.joynr.messaging.routing.LibjoynrBinderRoutingTableAddressValidator;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.routing.RoutingTable;
import io.joynr.messaging.routing.RoutingTableAddressValidator;
import io.joynr.messaging.sender.LibJoynrMessageSender;
import io.joynr.messaging.sender.MessageSender;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.BinderAddress;

import static io.joynr.messaging.MessagingPropertyKeys.GBID_ARRAY;

/**
 * Use this module if you want to start a lib joynr instance which connects to a cluster controller by binder
 */
public class LibjoynrBinderRuntimeModule extends AbstractRuntimeModule {

    private Context context;
    private BinderAddress ccBinderAddress;

    public LibjoynrBinderRuntimeModule(Context context, BinderAddress ccBinderAddress) {
        this.context = context;
        this.ccBinderAddress = ccBinderAddress;
    }

    @Override
    protected void configure() {
        super.configure();

        bind(JoynrRuntime.class).to(LibjoynrRuntime.class).in(Singleton.class);
        bind(LibJoynrMessageRouter.class).in(Singleton.class);
        bind(MessageRouter.class).to(LibJoynrMessageRouter.class);
        bind(MulticastReceiverRegistrar.class).to(LibJoynrMessageRouter.class);
        bind(MessageSender.class).to(LibJoynrMessageSender.class);
        bind(RoutingTable.class).to(DummyRoutingTable.class).asEagerSingleton();
        bind(RoutingTableAddressValidator.class).to(LibjoynrBinderRoutingTableAddressValidator.class);

        messagingSkeletonFactory.addBinding(BinderAddress.class).to(BinderMessagingSkeletonFactory.class);
        messagingStubFactory.addBinding(BinderAddress.class).to(BinderClientMessagingStubFactory.class);
        multicastAddressCalculators.addBinding().to(BinderMulticastAddressCalculator.class);
    }

    @Provides
    @Singleton
    @Named(SystemServicesSettings.PROPERTY_CC_MESSAGING_ADDRESS)
    public Address provideCCMessagingAddress() {
        return ccBinderAddress;
    }

    @Provides
    @Named(SystemServicesSettings.LIBJOYNR_MESSAGING_ADDRESS)
    Address getLibjoynrMessagingAddress() {
        return new BinderAddress(context.getPackageName(), Process.myUid());
    }

    @Provides
    @Singleton
    @Named(AndroidBinderRuntime.PROPERTY_CONTEXT_ANDROID)
    Context provideContext() {
        return context;
    }

    @Provides
    @Singleton
    @Named(GBID_ARRAY)
    public String[] provideGbidArray(GbidArrayFactory gbidArrayFactory) {
        return gbidArrayFactory.create();
    }
}
