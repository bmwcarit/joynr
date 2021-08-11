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
package io.joynr.messaging.mqtt;

import java.util.Map;

import io.joynr.messaging.FailureAction;
import io.joynr.messaging.SuccessAction;

public interface JoynrMqttClient {

    public void start();

    public void setMessageListener(IMqttMessagingSkeleton rawMessaging);

    public void shutdown();

    public void publishMessage(String topic,
                               byte[] serializedMessage,
                               Map<String, String> prefixedCustomHeaders,
                               int qosLevel,
                               long messageExpiryDateMs,
                               SuccessAction successAction,
                               FailureAction failureAction);

    public void subscribe(String topic);

    public void unsubscribe(String topic);

    /**
     * Can be used to determine whether {@link #shutdown()} has already been called on this client.
     *
     * @return <code>true</code> if {@link #shutdown()} has already been called, otherwise <code>false</code>.
     */
    boolean isShutdown();

}
