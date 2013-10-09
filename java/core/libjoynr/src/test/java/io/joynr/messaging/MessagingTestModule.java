package io.joynr.messaging;

/*
 * #%L
 * joynr::java::core::libjoynr
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

import joynr.infrastructure.ChannelUrlDirectoryProxy;

import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import com.google.inject.AbstractModule;

public class MessagingTestModule extends AbstractModule {

    @Mock
    private ChannelUrlDirectoryProxy mockChannelUrlClient;

    public MessagingTestModule() {
        MockitoAnnotations.initMocks(this);

    }

    @Override
    protected void configure() {
        bind(MessagingSettings.class).to(ConfigurableMessagingSettings.class);

        // don't override like this. Override via properties passed to createJoynInjector
        // bind(String.class).annotatedWith(Names.named(ConfigurableMessagingSettings.PROPERTY_BOUNCE_PROXY_URL)).toInstance(bounceProxyUrl);

        // binding global channelurl interface to local as well, to prevent channelurl comm.
        // this leads to endless loops
        // bind(ChannelUrlDirectoryClient.class).to(LocalChannelUrlDirectoryClientImpl.class);
        bind(ChannelUrlDirectoryProxy.class).toInstance(mockChannelUrlClient);

    }
}
