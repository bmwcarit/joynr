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

import java.time.Instant;
import java.util.Optional;

/**
 * Status metrics for a single connection.
 * Currently, only HivemqMqttClient provides ConnectionStatusMetrics.
 */
public interface ConnectionStatusMetrics {

    /**
     * @return Returns an optional containing the gbid the connection connects to.
     */
    Optional<String> getGbid();

    /**
     * @return Returns the url of the connection.
     */
    String getUrl();

    /**
     * @return Returns whether the connection is configured for sending messages.
     */
    boolean isSender();

    /**
     * @return Returns whether the connection is configured for receiving messages.
     */
    boolean isReceiver();

    /**
     * @return Returns true if the connection is currently connected.
     */
    boolean isConnected();

    /**
     * @return Returns the date of the last connection state change.
     */
    Instant getLastConnectionStateChangeDate();

    /**
     * @return Returns the number of messages received via this connection.
     */
    long getReceivedMessages();

    /**
     * @return Returns the number of messages sent via this connection.
     */
    long getSentMessages();

    /**
     * @return Returns the number of times the connection was lost.
     */
    long getConnectionDrops();

    /**
     * @return Returns the number of connection attempts made with this connection.<br/>
     *     <b>NOTE:</b> HivemqMqttClient currently only reports initial connection attempts. (Automatic) Reconnect attempts
     *     after a connection loss are not available.
     */
    long getConnectionAttempts();
}
