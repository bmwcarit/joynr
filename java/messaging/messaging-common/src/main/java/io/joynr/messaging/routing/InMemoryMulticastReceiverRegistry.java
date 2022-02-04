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
package io.joynr.messaging.routing;

import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.regex.Pattern;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;

import io.joynr.messaging.util.MulticastWildcardRegexFactory;

@Singleton
public class InMemoryMulticastReceiverRegistry implements MulticastReceiverRegistry {

    private static final Logger logger = LoggerFactory.getLogger(InMemoryMulticastReceiverRegistry.class);

    private final MulticastWildcardRegexFactory multicastWildcardRegexFactory;

    private static class PatternEntry {
        private Pattern pattern;
        private Set<String> participantIds = new HashSet();

        public PatternEntry(Pattern pattern) {
            this.pattern = pattern;
        }

        public Pattern getPattern() {
            return pattern;
        }

        public Set<String> getParticipantIds() {
            return participantIds;
        }
    }

    private HashMap<String, PatternEntry> multicastReceivers = new HashMap<>();

    @Inject
    public InMemoryMulticastReceiverRegistry(MulticastWildcardRegexFactory multicastWildcardRegexFactory) {
        this.multicastWildcardRegexFactory = multicastWildcardRegexFactory;
    }

    @Override
    public synchronized void registerMulticastReceiver(String multicastId, String participantId) {
        PatternEntry patternEntry;
        if (!multicastReceivers.containsKey(multicastId)) {
            Pattern idPattern = multicastWildcardRegexFactory.createIdPattern(multicastId);
            logger.trace("Compiled pattern {} for multicast ID {}", idPattern, multicastId);

            patternEntry = new PatternEntry(idPattern);
            multicastReceivers.put(multicastId, patternEntry);
        } else {
            patternEntry = multicastReceivers.get(multicastId);
        }
        patternEntry.getParticipantIds().add(participantId);
    }

    @Override
    public synchronized void unregisterMulticastReceiver(String multicastId, String participantId) {
        PatternEntry patternEntry = multicastReceivers.get(multicastId);
        if (patternEntry != null) {
            patternEntry.getParticipantIds().remove(participantId);
            if (patternEntry.getParticipantIds().size() == 0) {
                multicastReceivers.remove(multicastId);
            }
        }
    }

    @Override
    public synchronized Set<String> getReceivers(String multicastId) {
        Set<String> result = new HashSet<>();
        for (Map.Entry<String, PatternEntry> entry : multicastReceivers.entrySet()) {
            if (entry.getValue().getPattern().matcher(multicastId).matches()) {
                result.addAll(entry.getValue().getParticipantIds());
            }
        }
        return result;
    }

    @Override
    public synchronized Map<String, Set<String>> getReceivers() {
        Map<String, Set<String>> result = new HashMap<>();
        for (Map.Entry<String, PatternEntry> entry : multicastReceivers.entrySet()) {
            result.put(entry.getValue().getPattern().pattern(),
                       Collections.unmodifiableSet(entry.getValue().getParticipantIds()));
        }
        return result;
    }
}
