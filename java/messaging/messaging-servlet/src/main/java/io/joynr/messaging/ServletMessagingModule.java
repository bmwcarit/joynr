package io.joynr.messaging;

import com.google.inject.AbstractModule;
import com.google.inject.TypeLiteral;
import com.google.inject.multibindings.MapBinder;
import com.google.inject.multibindings.Multibinder;
import com.google.inject.name.Names;

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

import io.joynr.dispatcher.ServletMessageReceiver;
import io.joynr.dispatcher.ServletMessageReceiverImpl;
import io.joynr.messaging.channel.ChannelMessagingSkeleton;
import io.joynr.messaging.http.ServletHttpGlobalAddressFactory;
import io.joynr.messaging.routing.GlobalAddressFactory;
import io.joynr.runtime.GlobalAddressProvider;
import io.joynr.runtime.ReplyToAddressProvider;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.ChannelAddress;

/**
 * Used in conjunction with DefaultDispatcherModule to inject the application side
 *
 */
public class ServletMessagingModule extends AbstractModule {

    @Override
    protected void configure() {
        install(new HttpMessagingModule());
        bind(MessageListeners.class).to(MessageListenersImpl.class).asEagerSingleton();
        bind(ServletMessageReceiver.class).to(ServletMessageReceiverImpl.class);
        bind(MessageReceiver.class).to(ServletMessageReceiverImpl.class);
        MapBinder<Class<? extends Address>, IMessagingSkeleton> messagingSkeletonFactory;
        messagingSkeletonFactory = MapBinder.newMapBinder(binder(), new TypeLiteral<Class<? extends Address>>() {
        }, new TypeLiteral<IMessagingSkeleton>() {
        }, Names.named(MessagingSkeletonFactory.MIDDLEWARE_MESSAGING_SKELETONS));
        messagingSkeletonFactory.addBinding(ChannelAddress.class).to(ChannelMessagingSkeleton.class);

        Multibinder<GlobalAddressFactory<? extends Address>> globalAddresses;
        globalAddresses = Multibinder.newSetBinder(binder(),
                                                   new TypeLiteral<GlobalAddressFactory<? extends Address>>() {
                                                   },
                                                   Names.named(GlobalAddressProvider.GLOBAL_ADDRESS_PROVIDER));
        globalAddresses.addBinding().to(ServletHttpGlobalAddressFactory.class);
        Multibinder<GlobalAddressFactory<? extends Address>> replyToAddresses;
        replyToAddresses = Multibinder.newSetBinder(binder(),
                                                    new TypeLiteral<GlobalAddressFactory<? extends Address>>() {
                                                    },
                                                    Names.named(ReplyToAddressProvider.REPLY_TO_ADDRESS_PROVIDER));
        replyToAddresses.addBinding().to(ServletHttpGlobalAddressFactory.class);
    }
}
