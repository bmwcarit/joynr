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

public interface MqttStatusReceiver {
    public enum ConnectionStatus {
        CONNECTED, NOT_CONNECTED,
    }

    /**
     * Will be called whenever a message is dropped because the upper message queue limit was reached.
     */
    void notifyMessageDropped();

    /**
     * Will be called whenever the connection status changed.
     * @param connectionStatus A new connection status
     */
    void notifyConnectionStatusChanged(ConnectionStatus connectionStatus);
}
