/*
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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
package io.joynr.messaging.mqtt.statusmetrics;

public interface MqttStatusMetrics {
    /**
     * @return Returns the number of messages which were dropped since the start of the
     * instance because an overload situation was detected.
     */
    long getDroppedIncomingRequestsCount();

    /**
     *  @return Returns whether there is a MQTT connection or not.
     */
    boolean isConnected();

    /**
     * @return Returns a timestamp at which the MQTT connection became unavailable. If the connection has never been
     * established because the instance just started, the method returns the timestamp of the first connection attempt.
     */
    long getOfflineSinceTimestamp();
}