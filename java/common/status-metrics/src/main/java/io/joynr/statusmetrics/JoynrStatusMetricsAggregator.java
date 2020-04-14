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
import java.util.concurrent.atomic.AtomicLong;

import javax.ejb.Singleton;

@Singleton
public class JoynrStatusMetricsAggregator implements JoynrStatusMetrics {
    private Map<String, List<ConnectionStatusMetrics>> gbidToConnectionStatusMetricsListMap = new HashMap<String, List<ConnectionStatusMetrics>>();

    private volatile AtomicLong droppedMessages = new AtomicLong();

    @Override
    public List<ConnectionStatusMetrics> getAllConnectionStatusMetrics() {
        List<ConnectionStatusMetrics> returnList = new ArrayList<ConnectionStatusMetrics>();
        for (Entry<String, List<ConnectionStatusMetrics>> entry : gbidToConnectionStatusMetricsListMap.entrySet()) {
            returnList.addAll(entry.getValue());
        }
        return returnList;
    }

    @Override
    public List<ConnectionStatusMetrics> getConnectionStatusMetrics(String gbid) {
        if (gbidToConnectionStatusMetricsListMap.containsKey(gbid)) {
            return gbidToConnectionStatusMetricsListMap.get(gbid);
        }
        return new ArrayList<>();
    }

    public void addConnectionStatusMetrics(ConnectionStatusMetrics metrics) {
        if (gbidToConnectionStatusMetricsListMap.containsKey(metrics.getGbid().get())) {
            gbidToConnectionStatusMetricsListMap.get(metrics.getGbid().get()).add(metrics);
        } else {
            List<ConnectionStatusMetrics> newList = new ArrayList<>();
            newList.add(metrics);
            gbidToConnectionStatusMetricsListMap.put(metrics.getGbid().get(), newList);
        }
    }

    public void notifyMessageDropped() {
        droppedMessages.incrementAndGet();
    }

    @Override
    public long getNumDroppedMessages() {
        return droppedMessages.get();
    }
}
