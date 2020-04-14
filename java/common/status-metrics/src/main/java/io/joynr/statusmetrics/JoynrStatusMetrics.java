/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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
package io.joynr.statusmetrics;

import java.util.List;

/**
 * Can be injected to receive status metrics from joynr. These metrics are useful to monitor the state of a service.
 */
public interface JoynrStatusMetrics {
    /**
     * @return Returns the number of request messages which were discarded because the message queue reached its upper limit.
     */
    int getNumDiscardedMqttRequests();

    /**
     * @return Returns whether or not there is a valid connection to the MQTT broker.
     */
    boolean isConnectedToMqttBroker();

    /**
     * @return Returns a timestamp at which the MQTT connection became unavailable. If the connection has never been
     * established because the instance just started, the method returns the timestamp of the first connection attempt.
     * When there is an active connection, the return value is undefined.
     * The timestamp is provided as a Unix time (milliseconds since January 1, 1970 UTC (midnight)).
     */
    long getDisconnectedFromMqttBrokerSinceTimestamp();

    List<ConnectionStatusMetrics> getAllConnectionStatusMetrics();

    List<ConnectionStatusMetrics> getConnectionStatusMetrics(String gbid);

    boolean addConnectionStatusMetrics(ConnectionStatusMetrics metrics);
}
