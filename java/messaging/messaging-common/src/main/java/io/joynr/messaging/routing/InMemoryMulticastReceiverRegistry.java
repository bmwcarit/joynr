package io.joynr.messaging.routing;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentMap;
import java.util.regex.Pattern;

import com.google.common.collect.Maps;
import com.google.inject.Inject;
import com.google.inject.Singleton;
import io.joynr.messaging.util.MulticastWildcardRegexFactory;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

@Singleton
public class InMemoryMulticastReceiverRegistry implements MulticastReceiverRegistry {

    private static final Logger logger = LoggerFactory.getLogger(InMemoryMulticastReceiverRegistry.class);

    private final MulticastWildcardRegexFactory multicastWildcardRegexFactory;

    private ConcurrentMap<Pattern, Set<String>> multicastReceivers = Maps.newConcurrentMap();

    @Inject
    public InMemoryMulticastReceiverRegistry(MulticastWildcardRegexFactory multicastWildcardRegexFactory) {
        this.multicastWildcardRegexFactory = multicastWildcardRegexFactory;
    }

    @Override
    public void registerMulticastReceiver(String multicastId, String participantId) {
        Pattern idPattern = multicastWildcardRegexFactory.createIdPattern(multicastId);
        logger.trace("Compiled pattern {} for multicast ID {}", idPattern, multicastId);
        if (!multicastReceivers.containsKey(idPattern)) {
            multicastReceivers.putIfAbsent(idPattern, new HashSet<String>());
        }
        multicastReceivers.get(idPattern).add(participantId);
    }

    @Override
    public void unregisterMulticastReceiver(String multicastId, String participantId) {
        Set<String> participants = multicastReceivers.get(multicastWildcardRegexFactory.createIdPattern(multicastId));
        if (participants != null) {
            participants.remove(participantId);
        }
    }

    @Override
    public Set<String> getReceivers(String multicastId) {
        Set<String> result = new HashSet<>();
        for (Map.Entry<Pattern, Set<String>> entry : multicastReceivers.entrySet()) {
            if (entry.getKey().matcher(multicastId).matches()) {
                result.addAll(entry.getValue());
            }
        }
        return result;
    }

    @Override
    public Map<String, Set<String>> getReceivers() {
        Map<String, Set<String>> result = new HashMap<>();
        for (Map.Entry<Pattern, Set<String>> entry : multicastReceivers.entrySet()) {
            result.put(entry.getKey().pattern(), Collections.unmodifiableSet(entry.getValue()));
        }
        return result;
    }
}
