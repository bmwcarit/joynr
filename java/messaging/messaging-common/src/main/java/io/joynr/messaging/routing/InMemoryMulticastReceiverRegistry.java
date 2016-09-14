package io.joynr.messaging.routing;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

import java.util.Collections;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentMap;

import com.google.common.collect.Maps;
import com.google.inject.Singleton;

@Singleton
public class InMemoryMulticastReceiverRegistry implements MulticastReceiverRegistry {

    private ConcurrentMap<String, Set<String>> multicastReceivers = Maps.newConcurrentMap();

    @Override
    public void registerMulticastReceiver(String multicastId, String participantId) {
        if (!multicastReceivers.containsKey(multicastId)) {
            multicastReceivers.putIfAbsent(multicastId, new HashSet<String>());
        }
        multicastReceivers.get(multicastId).add(participantId);
    }

    @Override
    public void unregisterMulticastReceiver(String multicastId, String participantId) {
        Set<String> participants = multicastReceivers.get(multicastId);
        if (participants != null) {
            participants.remove(participantId);
        }
    }

    @Override
    public Set<String> getReceivers(String multicastId) {
        Set<String> receiversForMulticastId = multicastReceivers.get(multicastId);
        return Collections.unmodifiableSet(receiversForMulticastId);
    }

    @Override
    public Map<String, Set<String>> getReceivers() {
        return Collections.unmodifiableMap(multicastReceivers);
    }
}
