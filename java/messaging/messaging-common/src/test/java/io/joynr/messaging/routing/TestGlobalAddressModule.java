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
package io.joynr.messaging.routing;

import com.google.inject.AbstractModule;
import com.google.inject.Provides;
import com.google.inject.name.Named;

import io.joynr.messaging.MessagingPropertyKeys;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.MqttAddress;

public class TestGlobalAddressModule extends AbstractModule {

    @Provides
    @Named(MessagingPropertyKeys.GLOBAL_ADDRESS)
    public Address provideMqttOwnAddress() {
        return new MqttAddress("brokerUri", "topic");
    }

    @Provides
    @Named(MessagingPropertyKeys.REPLY_TO_ADDRESS)
    public Address provideMqttOwnReplyToAddress() {
        return new MqttAddress("brokerUri", "topic");
    }

}
