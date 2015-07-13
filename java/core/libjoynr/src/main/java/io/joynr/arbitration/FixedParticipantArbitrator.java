package io.joynr.arbitration;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2014 BMW Car IT GmbH
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

import io.joynr.capabilities.CapabilityEntry;

import java.util.Collection;
import java.util.Map;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class FixedParticipantArbitrator extends ArbitrationStrategyFunction {
    private static final Logger logger = LoggerFactory.getLogger(FixedParticipantArbitrator.class);

    @Override
    public CapabilityEntry select(Map<String, String> parameters, Collection<CapabilityEntry> capabilities) {
        String participantId = parameters.get(ArbitrationConstants.FIXEDPARTICIPANT_KEYWORD);
        logger.trace("starting select Provider by participant Id: {}", participantId);
        CapabilityEntry capabilityWithParticipantId = null;
        for (CapabilityEntry capEntry : capabilities) {
            if (capEntry.getParticipantId().equals(participantId)) {
                capabilityWithParticipantId = capEntry;
                break;
            }
        }
        logger.trace("capability with participantId: {}: {}" + participantId, capabilityWithParticipantId);

        return capabilityWithParticipantId;
    }
}
