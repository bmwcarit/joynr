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

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;

import com.google.inject.Singleton;

@Singleton
public class JoynrStatusMetricsAggregator implements JoynrStatusMetrics, MqttStatusReceiver {
    private AtomicInteger numDiscardedMqttRequest = new AtomicInteger();
    private Object connectionStatusChangedLock = new Object();
    private AtomicBoolean isConnectedToMqttBroker = new AtomicBoolean();
    private AtomicLong disconnectedFromMqttBrokerSinceTimestamp = new AtomicLong();
    private Map<String, ConnectionStatusMetrics> connectionStatusMetricsSendersMap = new HashMap<String, ConnectionStatusMetrics>();
    private Map<String, ConnectionStatusMetrics> connectionStatusMetricsReceiversMap = new HashMap<String, ConnectionStatusMetrics>();
    private Map<String, ConnectionStatusMetrics> connectionStatusMetricsCombinedMap = new HashMap<String, ConnectionStatusMetrics>();

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
    public List<ConnectionStatusMetrics> getAllConnectionStatusMetrics() {
        List<ConnectionStatusMetrics> returnList = new ArrayList<ConnectionStatusMetrics>();
        for (Entry<String, ConnectionStatusMetrics> entry : connectionStatusMetricsCombinedMap.entrySet()) {
            returnList.add(entry.getValue());
        }
        for (Entry<String, ConnectionStatusMetrics> entry : connectionStatusMetricsSendersMap.entrySet()) {
            returnList.add(entry.getValue());
        }
        for (Entry<String, ConnectionStatusMetrics> entry : connectionStatusMetricsReceiversMap.entrySet()) {
            returnList.add(entry.getValue());
        }
        return returnList;
    }

    @Override
    public List<ConnectionStatusMetrics> getConnectionStatusMetrics(String gbid) {
        List<ConnectionStatusMetrics> returnList = new ArrayList<ConnectionStatusMetrics>();
        if (connectionStatusMetricsCombinedMap.containsKey(gbid)) {
            returnList.add(connectionStatusMetricsCombinedMap.get(gbid));
        }
        if (connectionStatusMetricsSendersMap.containsKey(gbid)) {
            returnList.add(connectionStatusMetricsSendersMap.get(gbid));
        }
        if (connectionStatusMetricsReceiversMap.containsKey(gbid)) {
            returnList.add(connectionStatusMetricsReceiversMap.get(gbid));
        }
        return returnList;
    }

    @Override
    public boolean addConnectionStatusMetrics(ConnectionStatusMetrics metrics) {
        if (metrics.isReceiver() && metrics.isSender()) {
            if (connectionStatusMetricsCombinedMap.containsKey(metrics.getGbid().get())) {
                return false;
            }
            connectionStatusMetricsCombinedMap.put(metrics.getGbid().get(), metrics);
        } else if (metrics.isSender()) {
            if (connectionStatusMetricsSendersMap.containsKey(metrics.getGbid().get())) {
                return false;
            }
            connectionStatusMetricsSendersMap.put(metrics.getGbid().get(), metrics);
        } else if (metrics.isReceiver()) {
            if (connectionStatusMetricsReceiversMap.containsKey(metrics.getGbid().get())) {
                return false;
            }
            connectionStatusMetricsReceiversMap.put(metrics.getGbid().get(), metrics);
        }
        return false;
    }
}
