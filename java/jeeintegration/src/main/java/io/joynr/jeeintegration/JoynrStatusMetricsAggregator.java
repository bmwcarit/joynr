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
package io.joynr.jeeintegration;

import javax.ejb.Singleton;

import io.joynr.messaging.mqtt.statusmetrics.MqttStatusReceiver;

@Singleton
public class JoynrStatusMetricsAggregator implements JoynrStatusReceiver, MqttStatusReceiver {
    private int numDiscardedMqttRequest = 0;
    private boolean isConnectedToMqttBroker = false;
    private long disconnectedFromMqttBrokerSinceTimestamp;

    @Override
    public synchronized void notifyMessageDropped() {
        numDiscardedMqttRequest++;
    }

    @Override
    public synchronized void notifyConnectionStatusChanged(ConnectionStatus connectionStatus) {
        switch (connectionStatus) {
        case CONNECTED:
            isConnectedToMqttBroker = true;
            disconnectedFromMqttBrokerSinceTimestamp = -1;
            break;
        case NOT_CONNECTED:
            if (isConnectedToMqttBroker) {
                isConnectedToMqttBroker = false;
                disconnectedFromMqttBrokerSinceTimestamp = System.currentTimeMillis();
            }
            break;
        }
    }

    @Override
    public synchronized int getNumDiscardedMqttRequests() {
        return numDiscardedMqttRequest;
    }

    @Override
    public synchronized boolean isConnectedToMqttBroker() {
        return isConnectedToMqttBroker;
    }

    @Override
    public synchronized long getDisconnectedFromMqttBrokerSinceTimestamp() {
        return disconnectedFromMqttBrokerSinceTimestamp;
    }
}
