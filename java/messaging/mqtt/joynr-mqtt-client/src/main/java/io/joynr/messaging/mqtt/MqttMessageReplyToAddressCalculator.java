package io.joynr.messaging.mqtt;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

import joynr.JoynrMessage;

/**
 * Used to calcuate the reply-to address to set on a message in the case that a message is a request, subscription
 * request or broadcast subscriptions request. Useful if you want to override the default strategy for which topic will
 * receive the reply message.
 */
public interface MqttMessageReplyToAddressCalculator {

    /**
     * Called in order to set or alter the message's reply-to address if necessary.
     *
     * @param message
     *            the message to check the reply-to address on and add or change it as necessary.
     */
    void setReplyTo(JoynrMessage message);

}
