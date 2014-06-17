package io.joynr.joynrandroidruntime.messaging;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2014 BMW Car IT GmbH
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

import io.joynr.messaging.MessageReceiver;
import io.joynr.messaging.MessageSender;
import io.joynr.messaging.MessageSenderImpl;
import io.joynr.messaging.MessagingModule;
import io.joynr.messaging.httpoperation.HttpRequestFactory;
import io.joynr.messaging.httpoperation.LongPollingMessageReceiver;

public class AndroidLongPollingMessagingModule extends MessagingModule {

    @Override
    protected void configure() {
        super.configure();
        bind(MessageSender.class).to(MessageSenderImpl.class);
        bind(MessageReceiver.class).to(LongPollingMessageReceiver.class).asEagerSingleton();
        bind(HttpRequestFactory.class).to(ApacheAndroidHttpRequestFactory.class);
    }
}
