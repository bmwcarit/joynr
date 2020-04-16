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
package io.joynr.arbitration;

import java.util.Arrays;
import java.util.Collection;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import joynr.types.DiscoveryEntryWithMetaInfo;

/**
 * Arbitrator using a custom parameter in the QoS map called "priority". The provider with the highest priority value is
 * chosen.
 *
 */
public class HighestPriorityArbitrationStrategyFunction extends ArbitrationStrategyFunction {
    private static final Logger logger = LoggerFactory.getLogger(HighestPriorityArbitrationStrategyFunction.class);

    @Override
    public final Set<DiscoveryEntryWithMetaInfo> select(Map<String, String> parameters,
                                                        final Collection<DiscoveryEntryWithMetaInfo> capabilities) {
        logger.trace("Starting select Provider by priority");
        DiscoveryEntryWithMetaInfo highestPriorityCapability = null;
        long highestPriority = -1L;
        for (DiscoveryEntryWithMetaInfo discoveryEntry : capabilities) {
            // Search for the provider with the highest priority
            Long priority = discoveryEntry.getQos().getPriority();
            logger.trace("Looking at capability with priority {}", priority.toString());
            if (highestPriority < priority) {
                highestPriority = priority;
                highestPriorityCapability = discoveryEntry;
            }
        }
        logger.trace("Capability with highest priority ({}): {}", highestPriority, highestPriorityCapability);

        return highestPriorityCapability == null ? null
                : new HashSet<DiscoveryEntryWithMetaInfo>(Arrays.asList(highestPriorityCapability));
    }
}
