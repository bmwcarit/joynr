package io.joynr.runtime;

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

import io.joynr.dispatcher.MessagingEndpointDirectory;
import io.joynr.dispatcher.RequestReplyDispatcher;
import io.joynr.dispatcher.RequestReplyDispatcherImpl;
import io.joynr.dispatcher.RequestReplySender;
import io.joynr.dispatcher.RequestReplySenderImpl;
import io.joynr.dispatcher.rpc.JoynrMessagingConnectorFactory;
import io.joynr.dispatcher.rpc.RpcUtils;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.IMessageReceivers;
import io.joynr.messaging.MessageReceivers;
import io.joynr.messaging.MessageSender;
import io.joynr.messaging.MessageSenderImpl;
import io.joynr.messaging.MessagingSettings;
import io.joynr.messaging.httpoperation.HttpClientProvider;
import io.joynr.messaging.httpoperation.HttpDefaultRequestConfigProvider;
import joynr.Request;

import org.apache.http.client.config.RequestConfig;
import org.apache.http.impl.client.CloseableHttpClient;

import com.google.inject.AbstractModule;
import com.google.inject.Singleton;

public class DefaultRuntimeModule extends AbstractModule {
    // private static final Logger logger = LoggerFactory.getLogger(DefaultRuntimeModule.class);

    @Override
    protected void configure() {
        bind(MessagingSettings.class).to(ConfigurableMessagingSettings.class);

        bind(JoynrRuntime.class).to(JoynrRuntimeImpl.class).in(Singleton.class);
        bind(CloseableHttpClient.class).toProvider(HttpClientProvider.class).in(Singleton.class);
        bind(RequestConfig.class).toProvider(HttpDefaultRequestConfigProvider.class).in(Singleton.class);
        bind(RequestReplySender.class).to(RequestReplySenderImpl.class);
        bind(RequestReplyDispatcher.class).to(RequestReplyDispatcherImpl.class);
        bind(MessageSender.class).to(MessageSenderImpl.class);
        // bind(MessageReceiver.class).to(LongPollingMessageReceiver.class);
        bind(MessagingEndpointDirectory.class).in(Singleton.class);
        requestStaticInjection(JoynrMessagingConnectorFactory.class, RpcUtils.class, Request.class);

        bind(IMessageReceivers.class).to(MessageReceivers.class).asEagerSingleton();
    }

}
