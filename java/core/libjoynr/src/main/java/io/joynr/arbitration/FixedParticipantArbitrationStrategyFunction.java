package io.joynr.arbitration;

import java.util.Arrays;

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

import java.util.Collection;
import java.util.Map;

import joynr.types.DiscoveryEntry;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class FixedParticipantArbitrationStrategyFunction extends ArbitrationStrategyFunction {
    private static final Logger logger = LoggerFactory.getLogger(FixedParticipantArbitrationStrategyFunction.class);

    @Override
    public Collection<DiscoveryEntry> select(Map<String, String> parameters, Collection<DiscoveryEntry> capabilities) {
        String participantId = parameters.get(ArbitrationConstants.FIXEDPARTICIPANT_KEYWORD);
        logger.trace("starting select Provider by participant Id: {}", participantId);
        DiscoveryEntry capabilityWithParticipantId = null;
        for (DiscoveryEntry discoveryEntry : capabilities) {
            if (discoveryEntry.getParticipantId().equals(participantId)) {
                capabilityWithParticipantId = discoveryEntry;
                break;
            }
        }
        logger.trace("capability with participantId: {}: {}" + participantId, capabilityWithParticipantId);

        return capabilityWithParticipantId == null ? null : Arrays.asList(capabilityWithParticipantId);
    }
}
