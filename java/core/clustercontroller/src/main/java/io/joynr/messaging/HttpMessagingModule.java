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

import org.apache.http.client.config.RequestConfig;
import org.apache.http.impl.client.CloseableHttpClient;

import com.google.inject.AbstractModule;
import com.google.inject.Singleton;
import com.google.inject.TypeLiteral;
import com.google.inject.multibindings.MapBinder;
import com.google.inject.name.Names;

import io.joynr.messaging.channel.ChannelMessagingStubFactory;
import io.joynr.messaging.http.operation.ApacheHttpRequestFactory;
import io.joynr.messaging.http.operation.HttpClientProvider;
import io.joynr.messaging.http.operation.HttpDefaultRequestConfigProvider;
import io.joynr.messaging.http.operation.HttpRequestFactory;
import io.joynr.messaging.routing.MessagingStubFactory;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.ChannelAddress;

public class HttpMessagingModule extends AbstractModule {

    @Override
    protected void configure() {
        bind(RequestConfig.class).toProvider(HttpDefaultRequestConfigProvider.class).in(Singleton.class);
        bind(CloseableHttpClient.class).toProvider(HttpClientProvider.class).in(Singleton.class);
        bind(HttpRequestFactory.class).to(ApacheHttpRequestFactory.class);

        MapBinder<Class<? extends Address>, AbstractMiddlewareMessagingStubFactory<? extends IMessagingStub, ? extends Address>> messagingStubFactory;
        messagingStubFactory = MapBinder.newMapBinder(binder(), new TypeLiteral<Class<? extends Address>>() {
        }, new TypeLiteral<AbstractMiddlewareMessagingStubFactory<? extends IMessagingStub, ? extends Address>>() {
        }, Names.named(MessagingStubFactory.MIDDLEWARE_MESSAGING_STUB_FACTORIES));
        messagingStubFactory.addBinding(ChannelAddress.class).to(ChannelMessagingStubFactory.class);
    }
}
