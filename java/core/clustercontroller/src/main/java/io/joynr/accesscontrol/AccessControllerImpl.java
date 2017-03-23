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
import io.joynr.capabilities.CapabilitiesProvisioning;
import io.joynr.capabilities.CapabilityCallback;
import io.joynr.capabilities.CapabilityListener;
import io.joynr.capabilities.LocalCapabilitiesDirectory;
import io.joynr.runtime.SystemServicesSettings;

import java.io.IOException;
import java.util.HashSet;
import java.util.Set;

import joynr.JoynrMessage;
import joynr.Request;
import joynr.infrastructure.DacTypes.Permission;
import joynr.infrastructure.DacTypes.TrustLevel;

import joynr.types.DiscoveryEntry;
import joynr.types.DiscoveryEntryWithMetaInfo;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.Inject;
import com.google.inject.name.Named;

public class AccessControllerImpl implements AccessController {
    private static final Logger logger = LoggerFactory.getLogger(AccessControllerImpl.class);

    private final LocalCapabilitiesDirectory localCapabilitiesDirectory;
    private final LocalDomainAccessController localDomainAccessController;
    private final ObjectMapper objectMapper;

    private Set<String> whitelistedParticipantIds = new HashSet<String>();

    @Inject
    AccessControllerImpl(LocalCapabilitiesDirectory localCapabilitiesDirectory,
                         LocalDomainAccessController localDomainAccessController,
                         ObjectMapper objectMapper,
                         CapabilitiesProvisioning capabilitiesProvisioning,
                         @Named(SystemServicesSettings.PROPERTY_CC_DISCOVERY_PROVIDER_PARTICIPANT_ID) String discoveryProviderParticipantId,
                         @Named(SystemServicesSettings.PROPERTY_CC_ROUTING_PROVIDER_PARTICIPANT_ID) String routingProviderParticipantId) {
        this.localCapabilitiesDirectory = localCapabilitiesDirectory;
        this.localDomainAccessController = localDomainAccessController;
        this.objectMapper = objectMapper;

        defineAndRegisterCapabilityListener();
        whitelistProvisionedEntries(capabilitiesProvisioning);
        whitelistedParticipantIds.add(discoveryProviderParticipantId);
        whitelistedParticipantIds.add(routingProviderParticipantId);
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

    private void whitelistProvisionedEntries(CapabilitiesProvisioning capabilitiesProvisioning) {
        for (DiscoveryEntry entry : capabilitiesProvisioning.getDiscoveryEntries()) {
            whitelistedParticipantIds.add(entry.getParticipantId());
        }
    }

    private boolean needsPermissionCheck(final JoynrMessage message) {
        if (whitelistedParticipantIds.contains(message.getTo())) {
            return false;
        }

        String messageType = message.getType();

        if (messageType.equals(JoynrMessage.MESSAGE_TYPE_REPLY)
                || messageType.equals(JoynrMessage.MESSAGE_TYPE_PUBLICATION)
                || messageType.equals(JoynrMessage.MESSAGE_TYPE_MULTICAST)
                || messageType.equals(JoynrMessage.MESSAGE_TYPE_SUBSCRIPTION_REPLY)) {
            return false;
        }

        return true;
    }

    @Override
    public void hasConsumerPermission(final JoynrMessage message,
                                      final HasConsumerPermissionCallback hasConsumerPermissionCallback) {
        if (!needsPermissionCheck(message)) {
            hasConsumerPermissionCallback.hasConsumerPermission(true);
            return;
        }

        // Check permission at the interface level
        // First get the domain and interface that is being called from appropriate capability entry
        getCapabilityEntry(message, new CapabilityCallback() {
            @Override
            public void processCapabilityReceived(DiscoveryEntryWithMetaInfo discoveryEntry) {
                if (discoveryEntry == null) {
                    logger.error("Failed to get capability for participant id {} for acl check", message.getTo());
                    hasConsumerPermissionCallback.hasConsumerPermission(false);
                    return;
                }

                final String domain = discoveryEntry.getDomain();
                final String interfaceName = discoveryEntry.getInterfaceName();
                final String msgCreatorUid = message.getHeaderValue(JoynrMessage.HEADER_NAME_CREATOR_USER_ID);

                GetConsumerPermissionCallback consumerPermissionCallback = new GetConsumerPermissionCallback() {
                    @Override
                    public void getConsumerPermission(Permission permission) {
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

                        boolean hasPermissionResult = false;

                        switch (permission) {
                        case ASK:
                            assert false : "Permission.ASK user dialog not yet implemnted.";
                            hasPermissionResult = false;
                            break;
                        case YES:
                            hasPermissionResult = true;
                            break;
                        default:
                            logger.warn("Message {} to domain {}, interface {} failed AccessControl check",
                                        new Object[]{ message.getId(), domain, interfaceName });
                            hasPermissionResult = false;
                            break;
                        }

                        hasConsumerPermissionCallback.hasConsumerPermission(hasPermissionResult);
                    }

                    @Override
                    public void getConsumerPermissionFailed() {
                        logger.error("Failed to query permission for message {}", message.getId());
                        hasConsumerPermissionCallback.hasConsumerPermission(false);
                    }
                };

                // try determine permission without expensive message deserialization
                // since obtaining trust level from message header is still not supported use TrustLevel.HIGH
                localDomainAccessController.getConsumerPermission(msgCreatorUid,
                                                                  domain,
                                                                  interfaceName,
                                                                  TrustLevel.HIGH,
                                                                  consumerPermissionCallback);
            }

            @Override
            public void onError(Throwable e) {
                logger.error("Failed to get capability for participant id {} for acl check", message.getTo());
                hasConsumerPermissionCallback.hasConsumerPermission(false);
            }
        });
    }

    @Override
    public boolean hasProviderPermission(String userId, TrustLevel trustLevel, String domain, String interfaceName) {
        assert false : "Not yet implemented";
        return true;
    }

    // Get the capability entry for the given message
    private void getCapabilityEntry(JoynrMessage message, CapabilityCallback callback) {

        long cacheMaxAge = Long.MAX_VALUE;
        DiscoveryQos discoveryQos = new DiscoveryQos(DiscoveryQos.NO_MAX_AGE,
                                                     ArbitrationStrategy.NotSet,
                                                     cacheMaxAge,
                                                     DiscoveryScope.LOCAL_THEN_GLOBAL);

        String participantId = message.getTo();
        localCapabilitiesDirectory.lookup(participantId, discoveryQos, callback);
    }
}
