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
package io.joynr.accesscontrol;

import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

import io.joynr.accesscontrol.primarykey.UserDomainInterfaceOperationKey;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.exceptions.JoynrWaitExpiredException;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.proxy.Callback;
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyBuilderFactory;
import io.joynr.runtime.SystemServicesSettings;
import joynr.exceptions.ApplicationException;
import joynr.infrastructure.GlobalCapabilitiesDirectory;
import joynr.infrastructure.DacTypes.DomainRoleEntry;
import joynr.infrastructure.DacTypes.MasterAccessControlEntry;
import joynr.infrastructure.DacTypes.MasterRegistrationControlEntry;
import joynr.infrastructure.DacTypes.OwnerAccessControlEntry;
import joynr.infrastructure.DacTypes.OwnerRegistrationControlEntry;
import joynr.infrastructure.DacTypes.Permission;
import joynr.infrastructure.DacTypes.Role;
import joynr.infrastructure.DacTypes.TrustLevel;
import joynr.system.Discovery;
import joynr.system.Routing;
import joynr.types.GlobalDiscoveryEntry;

@Singleton
public class LocalDomainAccessControllerImpl implements LocalDomainAccessController {

    private static final Logger logger = LoggerFactory.getLogger(LocalDomainAccessControllerImpl.class);

    private final String discoveryDirectoriesDomain;
    private AccessControlAlgorithm accessControlAlgorithm = new AccessControlAlgorithm();
    private static final String WILDCARD = "*";
    private Map<UserDomainInterfaceOperationKey, AceSubscription> subscriptionsMap = new HashMap<UserDomainInterfaceOperationKey, AceSubscription>();

    private DomainAccessControlStore localDomainAccessStore;
    private String systemServicesDomain;

    // Class that holds subscription ids.
    static class AceSubscription {
        private final Future<String> masterSubscriptionFuture;
        private final Future<String> mediatorSubscriptionFuture;
        private final Future<String> ownerSubscriptionFuture;

        public AceSubscription(Future<String> masterSubscriptionFuture,
                               Future<String> mediatorSubscriptionFuture,
                               Future<String> ownerSubscriptionFuture) {
            this.masterSubscriptionFuture = masterSubscriptionFuture;
            this.mediatorSubscriptionFuture = mediatorSubscriptionFuture;
            this.ownerSubscriptionFuture = ownerSubscriptionFuture;
        }

        public String getMasterSubscriptionId() throws JoynrWaitExpiredException, JoynrRuntimeException,
                                                InterruptedException, ApplicationException {
            return masterSubscriptionFuture.get(1337);
        }

        public String getMediatorSubscriptionId() throws JoynrWaitExpiredException, JoynrRuntimeException,
                                                  InterruptedException, ApplicationException {
            return mediatorSubscriptionFuture.get(1337);
        }

        public String getOwnerSubscriptionId() throws JoynrWaitExpiredException, JoynrRuntimeException,
                                               InterruptedException, ApplicationException {
            return ownerSubscriptionFuture.get(1337);
        }
    }

    @Inject
    public LocalDomainAccessControllerImpl(@Named(MessagingPropertyKeys.CAPABILITIES_DIRECTORY_DISCOVERY_ENTRY) GlobalDiscoveryEntry capabilitiesDirectoryEntry,
                                           DomainAccessControlStore localDomainAccessStore,
                                           ProxyBuilderFactory proxyBuilderFactory,
                                           @Named(SystemServicesSettings.PROPERTY_SYSTEM_SERVICES_DOMAIN) String systemServicesDomain) {
        this.discoveryDirectoriesDomain = capabilitiesDirectoryEntry.getDomain();
        this.localDomainAccessStore = localDomainAccessStore;
        this.systemServicesDomain = systemServicesDomain;
    }

    @Override
    public boolean hasRole(String userId, String domain, Role role) {
        boolean hasRole = false;
        DomainRoleEntry dre;
        synchronized (localDomainAccessStore) {
            dre = localDomainAccessStore.getDomainRole(userId, role);
        }
        if (dre != null) {
            List<String> domains = Arrays.asList(dre.getDomains());
            if (domains.contains(domain)) {
                hasRole = true;
            }
        }

        return hasRole;
    }

    @Override
    public void getConsumerPermission(final String userId,
                                      final String domain,
                                      final String interfaceName,
                                      final TrustLevel trustLevel,
                                      final GetConsumerPermissionCallback callback) {
        final UserDomainInterfaceOperationKey subscriptionKey = new UserDomainInterfaceOperationKey(null,
                                                                                                    domain,
                                                                                                    interfaceName,
                                                                                                    null);
        logger.debug("getConsumerPermission on domain {}, interface {}", domain, interfaceName);

        // Handle special cases which should not require a lookup or a subscription
        Optional<Permission> specialPermission = handleSpecialCases(domain, interfaceName);
        if (specialPermission.isPresent()) {
            callback.getConsumerPermission(specialPermission.get());
            return;
        }

        if (subscriptionsMap.get(subscriptionKey) == null) {

        } else {
            getConsumerPermissionWithCachedEntries(userId, domain, interfaceName, trustLevel, callback);
        }
    }

    private void getConsumerPermissionWithCachedEntries(String userId,
                                                        String domain,
                                                        String interfaceName,
                                                        TrustLevel trustLevel,
                                                        GetConsumerPermissionCallback callback) {
        List<MasterAccessControlEntry> masterAces;
        List<MasterAccessControlEntry> mediatorAces;
        List<OwnerAccessControlEntry> ownerAces;

        synchronized (localDomainAccessStore) {
            masterAces = localDomainAccessStore.getMasterAccessControlEntries(userId, domain, interfaceName);
            mediatorAces = localDomainAccessStore.getMediatorAccessControlEntries(userId, domain, interfaceName);
            ownerAces = localDomainAccessStore.getOwnerAccessControlEntries(userId, domain, interfaceName);
        }

        if ((masterAces != null && masterAces.size() > 1) || (mediatorAces != null && mediatorAces.size() > 1)
                || (ownerAces != null && ownerAces.size() > 1)) {
            callback.getConsumerPermission(null);
        } else {
            callback.getConsumerPermission(getConsumerPermission(userId, domain, interfaceName, WILDCARD, trustLevel));
        }
    }

    private Optional<Permission> handleSpecialCases(String domain, String interfaceName) {

        // Allow access to the global directories
        if (domain.equals(discoveryDirectoriesDomain) || domain.equals(systemServicesDomain)) {
            if (interfaceName.equals(GlobalCapabilitiesDirectory.INTERFACE_NAME)
                    || interfaceName.equals(Discovery.INTERFACE_NAME) || interfaceName.equals(Routing.INTERFACE_NAME)) {
                return Optional.of(Permission.YES);
            }
        }

        return Optional.empty();
    }

    @Override
    public Permission getConsumerPermission(String userId,
                                            String domain,
                                            String interfaceName,
                                            String operation,
                                            TrustLevel trustLevel) {
        logger.debug("getConsumerPermission on domain {}, interface {}", domain, interfaceName);
        MasterAccessControlEntry masterAce;
        MasterAccessControlEntry mediatorAce;
        OwnerAccessControlEntry ownerAce;

        synchronized (localDomainAccessStore) {
            masterAce = localDomainAccessStore.getMasterAccessControlEntry(userId, domain, interfaceName, operation);
            mediatorAce = localDomainAccessStore.getMediatorAccessControlEntry(userId,
                                                                               domain,
                                                                               interfaceName,
                                                                               operation);
            ownerAce = localDomainAccessStore.getOwnerAccessControlEntry(userId, domain, interfaceName, operation);
        }

        return accessControlAlgorithm.getConsumerPermission(Optional.ofNullable(masterAce),
                                                            Optional.ofNullable(mediatorAce),
                                                            Optional.ofNullable(ownerAce),
                                                            trustLevel);
    }

    @Override
    public List<MasterAccessControlEntry> getEditableMasterAccessControlEntries(String uid) {
        throw new UnsupportedOperationException("Editing of access control entries is not implemented yet.");
    }

    @Override
    public Future<List<MasterAccessControlEntry>> getEditableMasterAccessControlEntries(Callback<List<MasterAccessControlEntry>> callback,
                                                                                        String uid) {
        throw new UnsupportedOperationException("Editing of access control entries is not implemented yet.");
    }

    @Override
    public Future<List<MasterAccessControlEntry>> getMediatorAccessControlEntries(Callback<List<MasterAccessControlEntry>> callback,
                                                                                  String uid) {
        return null;
    }

    @Override
    public List<MasterAccessControlEntry> getEditableMediatorAccessControlEntries(String uid) {
        return null;
    }

    @Override
    public Future<List<MasterAccessControlEntry>> getEditableMediatorAccessControlEntries(Callback<List<MasterAccessControlEntry>> callback,
                                                                                          String uid) {
        return null;
    }

    @Override
    public Future<List<OwnerAccessControlEntry>> getOwnerAccessControlEntries(Callback<List<OwnerAccessControlEntry>> callback,
                                                                              String uid) {
        throw new UnsupportedOperationException("Editing of access control entries is not implemented yet.");
    }

    @Override
    public List<OwnerAccessControlEntry> getEditableOwnerAccessControlEntries(String uid) {
        throw new UnsupportedOperationException("Editing of access control entries is not implemented yet.");
    }

    @Override
    public Future<List<OwnerAccessControlEntry>> getEditableOwnerAccessControlEntries(Callback<List<OwnerAccessControlEntry>> callback,
                                                                                      String uid) {
        throw new UnsupportedOperationException("Editing of access control entries is not implemented yet.");
    }

    @Override
    public Permission getProviderPermission(String uid, String domain, String interfaceName, TrustLevel trustLevel) {
        throw new UnsupportedOperationException("Provider registration permission check is not implemented yet.");
        //        return accessControlAlgorithm.getProviderPermission(null, null, null, trustLevel);
    }

    @Override
    public boolean updateMasterRegistrationControlEntry(MasterRegistrationControlEntry updatedMasterRce) {
        throw new UnsupportedOperationException("Provider registration permission check is not implemented yet.");
    }

    @Override
    public boolean removeMasterRegistrationControlEntry(String uid, String domain, String interfaceName) {
        throw new UnsupportedOperationException("Provider registration permission check is not implemented yet.");
    }

    @Override
    public boolean updateMediatorRegistrationControlEntry(MasterRegistrationControlEntry updatedMediatorRce) {
        throw new UnsupportedOperationException("Provider registration permission check is not implemented yet.");
    }

    @Override
    public boolean removeMediatorRegistrationControlEntry(String uid, String domain, String interfaceName) {
        throw new UnsupportedOperationException("Provider registration permission check is not implemented yet.");
    }

    @Override
    public boolean updateOwnerRegistrationControlEntry(OwnerRegistrationControlEntry updatedOwnerRce) {
        throw new UnsupportedOperationException("Provider registration permission check is not implemented yet.");
    }

    @Override
    public boolean removeOwnerRegistrationControlEntry(String uid, String domain, String interfaceName) {
        throw new UnsupportedOperationException("Provider registration permission check is not implemented yet.");
    }
}
