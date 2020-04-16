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

public class FixedParticipantArbitrationStrategyFunction extends ArbitrationStrategyFunction {
    private static final Logger logger = LoggerFactory.getLogger(FixedParticipantArbitrationStrategyFunction.class);

    @Override
    public Set<DiscoveryEntryWithMetaInfo> select(Map<String, String> parameters,
                                                  Collection<DiscoveryEntryWithMetaInfo> capabilities) {
        String participantId = parameters.get(ArbitrationConstants.FIXEDPARTICIPANT_KEYWORD);
        logger.trace("Starting select Provider by participant Id: {}", participantId);
        DiscoveryEntryWithMetaInfo capabilityWithParticipantId = null;
        for (DiscoveryEntryWithMetaInfo discoveryEntry : capabilities) {
            if (discoveryEntry.getParticipantId().equals(participantId)) {
                capabilityWithParticipantId = discoveryEntry;
                break;
            }
        }
        logger.trace("Capability with participantId: {}: {}", participantId, capabilityWithParticipantId);

        return capabilityWithParticipantId == null ? null
                : new HashSet<DiscoveryEntryWithMetaInfo>(Arrays.asList(capabilityWithParticipantId));
    }
}
