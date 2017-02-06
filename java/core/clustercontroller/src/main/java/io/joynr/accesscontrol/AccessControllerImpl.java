package io.joynr.accesscontrol;

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

import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.capabilities.CapabilityListener;
import io.joynr.capabilities.LocalCapabilitiesDirectory;

import java.io.IOException;

import joynr.JoynrMessage;
import joynr.Request;
import joynr.infrastructure.DacTypes.Permission;
import joynr.infrastructure.DacTypes.TrustLevel;

import joynr.types.DiscoveryEntry;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.Inject;

public class AccessControllerImpl implements AccessController {
    private static final Logger logger = LoggerFactory.getLogger(AccessControllerImpl.class);

    private final LocalCapabilitiesDirectory localCapabilitiesDirectory;
    private final LocalDomainAccessController localDomainAccessController;
    private final ObjectMapper objectMapper;

    @Inject
    AccessControllerImpl(LocalCapabilitiesDirectory localCapabilitiesDirectory,
                         LocalDomainAccessController localDomainAccessController,
                         ObjectMapper objectMapper) {
        this.localCapabilitiesDirectory = localCapabilitiesDirectory;
        this.localDomainAccessController = localDomainAccessController;
        this.objectMapper = objectMapper;

        defineAndRegisterCapabilityListener();
    }

    private void defineAndRegisterCapabilityListener() {
        localCapabilitiesDirectory.addCapabilityListener(new CapabilityListener() {

            @Override
            public void capabilityRemoved(DiscoveryEntry removedCapability) {
                localDomainAccessController.unsubscribeFromAceChanges(removedCapability.getDomain(),
                                                                      removedCapability.getInterfaceName());
            }

            @Override
            public void capabilityAdded(DiscoveryEntry addedCapability) {
                // NOOP
            }
        });
    }

    @Override
    public boolean hasConsumerPermission(final JoynrMessage message) {
        // Check permission at the interface level
        // First get the domain and interface that is being called from appropriate capability entry
        DiscoveryEntry discoveryEntry = getCapabilityEntry(message);
        if (discoveryEntry == null) {
            logger.error("Failed to get capability for participant id {} for acl check", message.getTo());
            return false;
        }

        String domain = discoveryEntry.getDomain();
        String interfaceName = discoveryEntry.getInterfaceName();

        // try determine permission without expensive message deserialization
        // since obtaining trust level from message header is still not supported use TrustLevel.HIGH
        String msgCreatorUid = message.getHeaderValue(JoynrMessage.HEADER_NAME_CREATOR_USER_ID);
        Permission permission = localDomainAccessController.getConsumerPermission(msgCreatorUid,
                                                                                  domain,
                                                                                  interfaceName,
                                                                                  TrustLevel.HIGH);

        // if permission still not defined, have to deserialize message and try again
        if (permission == null) {
            try {
                // Deserialize the request from message and take operation value
                Request request = objectMapper.readValue(message.getPayload(), Request.class);
                String operation = request.getMethodName();

                // Get the permission for the requested operation
                permission = localDomainAccessController.getConsumerPermission(msgCreatorUid,
                                                                               domain,
                                                                               interfaceName,
                                                                               operation,
                                                                               TrustLevel.HIGH);
            } catch (IOException e) {
                logger.error("Cannot deserialize message", e);
                permission = Permission.NO;
            }
        }

        switch (permission) {
        case ASK:
            assert false : "Permission.ASK user dialog not yet implemnted.";
            return false;
        case YES:
            return true;
        default:
            logger.warn("Message {} to domain {}, interface {} failed AccessControl check",
                        new Object[]{ message.getId(), discoveryEntry.getDomain(), discoveryEntry.getInterfaceName() });
            return false;
        }
    }

    @Override
    public boolean hasProviderPermission(String userId, TrustLevel trustLevel, String domain, String interfaceName) {
        assert false : "Not yet implemented";
        return true;
    }

    // Get the capability entry for the given message
    private DiscoveryEntry getCapabilityEntry(JoynrMessage message) {

        long cacheMaxAge = Long.MAX_VALUE;
        DiscoveryQos discoveryQos = new DiscoveryQos(DiscoveryQos.NO_MAX_AGE,
                                                     ArbitrationStrategy.NotSet,
                                                     cacheMaxAge,
                                                     DiscoveryScope.LOCAL_THEN_GLOBAL);

        String participantId = message.getTo();
        return localCapabilitiesDirectory.lookup(participantId, discoveryQos);
    }
}
