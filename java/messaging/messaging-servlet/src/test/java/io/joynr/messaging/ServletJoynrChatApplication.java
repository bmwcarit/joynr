package io.joynr.messaging;

/*
 * #%L
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

import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;
import io.joynr.runtime.AbstractJoynrApplication;
import joynr.chat.DefaultMessengerProvider;
import joynr.chat.messagetypecollection.Message;

public class ServletJoynrChatApplication extends AbstractJoynrApplication {

    // @Inject
    // @Named("DummyJoynApplication.participantId")
    // String participantId;

    private DefaultMessengerProvider provider;

    @Override
    public void run() {
        provider = new DefaultMessengerProvider() {
            @Override
            public Promise<DeferredVoid> setMessage(Message message) {
                DeferredVoid deferred = new DeferredVoid();
                // manipulate the message so that the consumer can verify that the set worked
                message.setMessage(message.getSenderId() + message.getMessage());
                this.message = message;
                messageChanged(message);
                return new Promise<DeferredVoid>(deferred);
            }

        };
        runtime.registerProvider(localDomain, provider);
    }

    @Override
    public void shutdown() {
        runtime.unregisterProvider(localDomain, provider);

    }
}
