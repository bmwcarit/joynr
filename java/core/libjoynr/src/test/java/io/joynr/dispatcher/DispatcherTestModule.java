package io.joynr.dispatcher;

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

import io.joynr.dispatcher.rpc.JoynrMessagingConnectorFactory;
import io.joynr.dispatcher.rpc.RpcUtils;
import io.joynr.messaging.IMessageReceivers;
import io.joynr.messaging.MessageReceiver;
import io.joynr.messaging.MessageReceivers;
import io.joynr.messaging.MessageSender;
import io.joynr.pubsub.publication.PublicationManager;
import io.joynr.pubsub.publication.PublicationManagerImpl;
import io.joynr.pubsub.subscription.SubscriptionManager;
import io.joynr.pubsub.subscription.SubscriptionManagerImpl;
import joynr.Request;

import com.google.inject.AbstractModule;

public class DispatcherTestModule extends AbstractModule {

    @Override
    protected void configure() {

        bind(MessageSender.class).to(MessageSenderReceiverMock.class);
        bind(IMessageReceivers.class).to(MessageReceivers.class).asEagerSingleton();
        bind(MessageReceiver.class).to(MessageSenderReceiverMock.class);
        bind(RequestReplySender.class).to(RequestReplySenderImpl.class);
        bind(RequestReplyDispatcher.class).to(RequestReplyDispatcherImpl.class);
        bind(SubscriptionManager.class).to(SubscriptionManagerImpl.class);
        bind(PublicationManager.class).to(PublicationManagerImpl.class);

        requestStaticInjection(RpcUtils.class, Request.class, JoynrMessagingConnectorFactory.class);
    }

}
