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
import java.util.Collection;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.atomic.AtomicLong;
import java.util.Optional;

import com.google.inject.Singleton;

@Singleton
public class JoynrStatusMetricsAggregator implements JoynrStatusMetricsReceiver {
    private List<ConnectionStatusMetrics> connectionStatusMetricsList = new CopyOnWriteArrayList<ConnectionStatusMetrics>();

    private volatile AtomicLong droppedMessages = new AtomicLong();

    @Override
    public Collection<ConnectionStatusMetrics> getAllConnectionStatusMetrics() {
        List<ConnectionStatusMetrics> returnList = new ArrayList<ConnectionStatusMetrics>(connectionStatusMetricsList);
        return returnList;
    }

    @Override
    public Collection<ConnectionStatusMetrics> getConnectionStatusMetrics(String gbid) {
        List<ConnectionStatusMetrics> returnList = new ArrayList<ConnectionStatusMetrics>();
        for (ConnectionStatusMetrics metrics : connectionStatusMetricsList) {
            Optional<String> metricsGbid = metrics.getGbid();
            if (metricsGbid.isPresent() && metricsGbid.get().equals(gbid)) {
                returnList.add(metrics);
            }
        }
        return returnList;
    }

    public void addConnectionStatusMetrics(ConnectionStatusMetrics metrics) {
        connectionStatusMetricsList.add(metrics);
    }

    public void notifyMessageDropped() {
        droppedMessages.incrementAndGet();
    }

    @Override
    public long getNumDroppedMessages() {
        return droppedMessages.get();
    }
}
