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

import io.joynr.capabilities.CapabilityCallback;
import io.joynr.capabilities.CapabilityEntry;
import io.joynr.capabilities.LocalCapabilitiesDirectory;
import io.joynr.endpoints.EndpointAddressBase;

import java.util.List;

import javax.annotation.CheckForNull;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class FixedParticipantArbitrator extends Arbitrator {
    private static final Logger logger = LoggerFactory.getLogger(FixedParticipantArbitrator.class);

    private String participantId;

    public FixedParticipantArbitrator(final DiscoveryQos discoveryQos,
                                      LocalCapabilitiesDirectory capabilitiesSource,
                                      long minimumArbitrationRetryDelay) {
        super(discoveryQos, capabilitiesSource, minimumArbitrationRetryDelay);
    }

    @Override
    public void startArbitration() {
        participantId = discoveryQos.getCustomParameter(ArbitrationConstants.FIXEDPARTICIPANT_KEYWORD).toString();
        arbitrationStatus = ArbitrationStatus.ArbitrationRunning;
        localCapabilitiesDirectory.lookup(participantId, discoveryQos, new CapabilityCallback() {

            @Override
            public void processCapabilityReceived(@CheckForNull CapabilityEntry capability) {
                selectProvider(capability);
            }

            @Override
            public void onError(Throwable exception) {
                FixedParticipantArbitrator.this.onError(exception);
            }
        });
    }

    protected void selectProvider(@CheckForNull final CapabilityEntry capability) {

        if (capability != null) {
            List<EndpointAddressBase> endpointAddress = capability.getEndpointAddresses();
            ;
            arbitrationResult.setEndpointAddress(endpointAddress);
            arbitrationResult.setParticipantId(participantId);
            arbitrationStatus = ArbitrationStatus.ArbitrationSuccesful;
            updateArbitrationResultAtListener();
        } else {
            logger.debug("FixedParticipantArbitrator received empty list of capabilities or multiple entries for fixed participantId. Arbitration canceled.");
            arbitrationStatus = ArbitrationStatus.ArbitrationCanceledForever;
            notifyArbitrationStatusChanged();

        }
    }

}
