/**
 *
 */
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

import org.apache.http.client.config.RequestConfig;
import org.apache.http.impl.client.CloseableHttpClient;

import com.google.inject.AbstractModule;
import com.google.inject.Singleton;
import com.google.inject.TypeLiteral;
import com.google.inject.multibindings.MapBinder;
import com.google.inject.multibindings.Multibinder;
import com.google.inject.name.Names;

import io.joynr.accesscontrol.AccessControlClientModule;
import io.joynr.dispatcher.ServletMessageReceiver;
import io.joynr.messaging.AbstractMiddlewareMessagingStubFactory;
import io.joynr.messaging.IMessagingSkeleton;
import io.joynr.messaging.IMessagingStub;
import io.joynr.messaging.MessageReceiver;
import io.joynr.messaging.channel.ChannelMessagingSkeleton;
import io.joynr.messaging.channel.ChannelMessagingStubFactory;
import io.joynr.messaging.http.ServletHttpGlobalAddressFactory;
import io.joynr.messaging.http.operation.ApacheHttpRequestFactory;
import io.joynr.messaging.http.operation.HttpClientProvider;
import io.joynr.messaging.http.operation.HttpDefaultRequestConfigProvider;
import io.joynr.messaging.http.operation.HttpRequestFactory;
import io.joynr.messaging.routing.GlobalAddressFactory;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.sender.CcMessageSender;
import io.joynr.messaging.sender.MessageSender;
import io.joynr.runtime.GlobalAddressProvider;
import io.joynr.runtime.ReplyToAddressProvider;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.ChannelAddress;

/**
 * Configures bindings so that joynr can send and receive messages via HTTP. In particular, the messages are received
 * via a custom {@link JeeMessagingEndpoint} and sent to the custom {@link JeeServletMessageReceiver}.
 */
public class JeeHttpMessagingModule extends AbstractModule {

    private MapBinder<Class<? extends Address>, IMessagingSkeleton> messagingSkeletonFactory;
    private MapBinder<Class<? extends Address>, AbstractMiddlewareMessagingStubFactory<? extends IMessagingStub, ? extends Address>> messagingStubFactory;

    public JeeHttpMessagingModule(MapBinder<Class<? extends Address>, IMessagingSkeleton> messagingSkeletonFactory,
                                  MapBinder<Class<? extends Address>, AbstractMiddlewareMessagingStubFactory<? extends IMessagingStub, ? extends Address>> messagingStubFactory) {
        this.messagingSkeletonFactory = messagingSkeletonFactory;
        this.messagingStubFactory = messagingStubFactory;
    }

    @Override
    protected void configure() {
        messagingSkeletonFactory.addBinding(ChannelAddress.class).to(ChannelMessagingSkeleton.class);
        messagingStubFactory.addBinding(ChannelAddress.class).to(ChannelMessagingStubFactory.class);

        Multibinder<GlobalAddressFactory<? extends Address>> globalAddresses;
        globalAddresses = Multibinder.newSetBinder(binder(),
                                                   new TypeLiteral<GlobalAddressFactory<? extends Address>>() {
                                                   }, Names.named(GlobalAddressProvider.GLOBAL_ADDRESS_PROVIDER));
        globalAddresses.addBinding().to(ServletHttpGlobalAddressFactory.class);

        Multibinder<GlobalAddressFactory<? extends Address>> replyToAddresses;
        replyToAddresses = Multibinder.newSetBinder(binder(),
                                                   new TypeLiteral<GlobalAddressFactory<? extends Address>>() {
                                                   }, Names.named(ReplyToAddressProvider.REPLY_TO_ADDRESS_PROVIDER));
        replyToAddresses.addBinding().to(ServletHttpGlobalAddressFactory.class);

        install(new AccessControlClientModule());

        bind(RequestConfig.class).toProvider(HttpDefaultRequestConfigProvider.class).in(Singleton.class);
        bind(CloseableHttpClient.class).toProvider(HttpClientProvider.class).in(Singleton.class);
        bind(HttpRequestFactory.class).to(ApacheHttpRequestFactory.class);

        bind(MessageRouter.class).to(JeeMessageRouter.class).in(Singleton.class);
        bind(MessageSender.class).to(CcMessageSender.class);
        bind(MessageReceiver.class).to(JeeServletMessageReceiver.class);
        bind(ServletMessageReceiver.class).to(JeeServletMessageReceiver.class);
    }
}
