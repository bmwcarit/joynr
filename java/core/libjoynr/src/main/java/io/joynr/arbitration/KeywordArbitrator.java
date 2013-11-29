package io.joynr.arbitration;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
import io.joynr.capabilities.LocalCapabilitiesDirectory;

import java.util.Collection;

import javax.annotation.CheckForNull;

import joynr.types.CustomParameter;
import joynr.types.ProviderQos;
import joynr.types.ProviderQosRequirements;

public class KeywordArbitrator extends Arbitrator {

    private String domain;
    private String interfaceName;
    private String requestedKeyword;

    public KeywordArbitrator(final String domain,
                             String interfaceName,
                             final DiscoveryQos discoveryQos,
                             LocalCapabilitiesDirectory capabilitiesSource,
                             long minimumArbitrationRetryDelay) {
        super(discoveryQos, capabilitiesSource, minimumArbitrationRetryDelay);
        this.domain = domain;
        this.interfaceName = interfaceName;
    }

    @Override
    public void startArbitration() {
        arbitrationStatus = ArbitrationStatus.ArbitrationRunning;
        notifyArbitrationStatusChanged();
        requestedKeyword = discoveryQos.getCustomParameter(ArbitrationConstants.KEYWORD_PARAMETER).toString();
        ProviderQosRequirements providerQosRequirements = new ProviderQosRequirements();

        localCapabilitiesDirectory.getCapabilities(domain,
                                                   interfaceName,
                                                   providerQosRequirements,
                                                   discoveryQos,
                                                   callback);
    }

    @Override
    protected void selectProvider(@CheckForNull final Collection<CapabilityEntry> capabilities) {
        if (capabilities == null || capabilities.size() < 1) {
            arbitrationStatus = ArbitrationStatus.ArbitrationCanceledForever;
            notifyArbitrationStatusChanged();
        } else {
            CapabilityEntry result;
            for (CapabilityEntry capEntry : capabilities) {
                ProviderQos providerQos = capEntry.getProviderQos();

                // If onChange subscriptions are required ignore providers that do not support them
                if (discoveryQos.getProviderMustSupportOnChange() && !providerQos.getSupportsOnChangeSubscriptions()) {
                    continue;
                }

                // Search for a matching keyword parameter
                CustomParameter keywordParameter = findQosParameter(capEntry, ArbitrationConstants.KEYWORD_PARAMETER);
                if (keywordParameter != null) {
                    String currentKeyword = keywordParameter.getValue();
                    if (currentKeyword.equals(requestedKeyword)) {
                        result = capEntry;
                        arbitrationResult.setEndpointAddress(result.getEndpointAddresses());
                        arbitrationResult.setParticipantId(result.getParticipantId());
                        arbitrationStatus = ArbitrationStatus.ArbitrationSuccesful;
                        updateArbitrationResultAtListener();
                        return;
                    }
                }
            }
            arbitrationStatus = ArbitrationStatus.ArbitrationCanceledForever;
            notifyArbitrationStatusChanged();
        }
    }
}
