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

import io.joynr.capabilities.CapabilitiesCallback;
import io.joynr.capabilities.CapabilityEntry;
import io.joynr.capabilities.LocalCapabilitiesDirectory;

import java.util.Collection;

import javax.annotation.CheckForNull;

import joynr.types.ProviderQos;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Arbitrator using a custom parameter in the QoS map called "priority". The provider with the highest priority value is
 * chosen.
 * 
 */
public class HighestPriorityArbitrator extends Arbitrator {
    private static final Logger logger = LoggerFactory.getLogger(HighestPriorityArbitrator.class);

    private String domain;
    private String interfaceName;

    public HighestPriorityArbitrator(final String domain,
                                     final String interfaceName,
                                     final DiscoveryQos discoveryQos,
                                     LocalCapabilitiesDirectory capabilities,
                                     long minimumArbitrationRetryDelay) {
        super(discoveryQos, capabilities, minimumArbitrationRetryDelay);
        this.domain = domain;
        this.interfaceName = interfaceName;

    }

    @Override
    public void startArbitration() {
        logger.debug("start arbitration for domain: {}, interface: {}", domain, interfaceName);
        // TODO qos map is not used. Implement qos filter in
        // capabilitiesDirectory or remove qos argument.
        arbitrationStatus = ArbitrationStatus.ArbitrationRunning;
        notifyArbitrationStatusChanged();

        localCapabilitiesDirectory.lookup(domain, interfaceName, discoveryQos, new CapabilitiesCallback() {

            @Override
            public void processCapabilitiesReceived(@CheckForNull Collection<CapabilityEntry> capabilities) {
                // when using javax.annotations.NonNull annotation on capablities parameter it will
                // cause a NoSuchMethodError
                assert (capabilities != null);
                selectProvider(capabilities);
            }

            @Override
            public void onError(Throwable exception) {
                HighestPriorityArbitrator.this.onError(exception);
            }
        });

    }

    protected void selectProvider(final Collection<CapabilityEntry> capabilities) {
        logger.trace("starting select Provider");
        if (capabilities == null || capabilities.isEmpty()) {
            logger.trace("received Emtpy capabilitiesList");
            restartArbitrationIfNotExpired();
        } else {
            CapabilityEntry result = null;
            logger.trace("received Capabilities List of Size > 1");
            long highestPriority = Long.MIN_VALUE;
            for (CapabilityEntry capEntry : capabilities) {
                if (capEntry.getEndpointAddresses() == null || capEntry.getEndpointAddresses().size() == 0) {
                    continue;
                }
                // If onChange subscriptions are required ignore providers that do not support them
                ProviderQos providerQos = capEntry.getProviderQos();
                if (discoveryQos.getProviderMustSupportOnChange() && !providerQos.getSupportsOnChangeSubscriptions()) {
                    continue;
                }

                // Search for the provider with the highest priority
                Long priority = providerQos.getPriority();
                logger.trace("Looking at capability with priority " + priority.toString());
                if (highestPriority < priority) {
                    highestPriority = priority;
                    result = capEntry;
                }
            }
            logger.trace("capability with highest priority: " + highestPriority + "\r\n" + result);
            if (result != null) {
                arbitrationResult.setEndpointAddress(result.getEndpointAddresses());
                arbitrationResult.setParticipantId(result.getParticipantId());
                arbitrationStatus = ArbitrationStatus.ArbitrationSuccesful;
                updateArbitrationResultAtListener();
            } else {
                restartArbitrationIfNotExpired();
            }
        }
    }

}
