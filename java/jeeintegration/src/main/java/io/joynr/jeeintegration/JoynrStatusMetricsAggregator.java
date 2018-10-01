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

import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;

import javax.ejb.ConcurrencyManagement;
import javax.ejb.ConcurrencyManagementType;
import javax.ejb.Singleton;

import io.joynr.messaging.mqtt.statusmetrics.MqttStatusReceiver;
import io.joynr.statusmetrics.MessageWorkerStatus;
import io.joynr.statusmetrics.StatusReceiver;

@Singleton
@ConcurrencyManagement(ConcurrencyManagementType.BEAN)
public class JoynrStatusMetricsAggregator implements JoynrStatusMetrics, MqttStatusReceiver, StatusReceiver {
    private AtomicInteger numDiscardedMqttRequest = new AtomicInteger();
    private Object connectionStatusChangedLock = new Object();
    private AtomicBoolean isConnectedToMqttBroker = new AtomicBoolean();
    private AtomicLong disconnectedFromMqttBrokerSinceTimestamp = new AtomicLong();
    private ConcurrentHashMap<Integer, MessageWorkerStatus> messageWorkersStatus = new ConcurrentHashMap<Integer, MessageWorkerStatus>();

    @Override
    public void notifyMessageDropped() {
        numDiscardedMqttRequest.incrementAndGet();
    }

    @Override
    public void notifyConnectionStatusChanged(ConnectionStatus connectionStatus) {
        switch (connectionStatus) {
        case CONNECTED:
            synchronized (connectionStatusChangedLock) {
                isConnectedToMqttBroker.set(true);
                disconnectedFromMqttBrokerSinceTimestamp.set(-1);
            }
            break;
        case NOT_CONNECTED:
            synchronized (connectionStatusChangedLock) {
                if (isConnectedToMqttBroker.get()) {
                    isConnectedToMqttBroker.set(false);
                    disconnectedFromMqttBrokerSinceTimestamp.set(System.currentTimeMillis());
                }
            }
            break;
        }
    }

    @Override
    public int getNumDiscardedMqttRequests() {
        return numDiscardedMqttRequest.get();
    }

    @Override
    public boolean isConnectedToMqttBroker() {
        return isConnectedToMqttBroker.get();
    }

    @Override
    public long getDisconnectedFromMqttBrokerSinceTimestamp() {
        return disconnectedFromMqttBrokerSinceTimestamp.get();
    }

    @Override
    public void updateMessageWorkerStatus(int messageWorkerId, MessageWorkerStatus messageWorkerStatus) {
        this.messageWorkersStatus.put(messageWorkerId, messageWorkerStatus);
    }
}
