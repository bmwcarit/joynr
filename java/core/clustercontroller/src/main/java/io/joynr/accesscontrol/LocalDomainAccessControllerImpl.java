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
import java.util.List;
import java.util.Optional;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.proxy.ProxyBuilderFactory;
import io.joynr.runtime.SystemServicesSettings;
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

    private final DomainAccessControlStore localDomainAccessStore;
    private String systemServicesDomain;

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
        dre = localDomainAccessStore.getDomainRole(userId, role);
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
        logger.debug("getConsumerPermission on domain {}, interface {}", domain, interfaceName);

        // Handle special cases which should not require a lookup or a subscription
        Optional<Permission> specialPermission = handleSpecialCases(domain, interfaceName);
        if (specialPermission.isPresent()) {
            callback.getConsumerPermission(specialPermission.get());
            return;
        }

        getConsumerPermissionWithCachedEntries(userId, domain, interfaceName, trustLevel, callback);

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
            logger.error("getConsumerPermission on domain {}, interface {} : ACL entries for non wildcard operations are not supported yet",
                         domain,
                         interfaceName);
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
        Optional<MasterAccessControlEntry> masterAce;
        Optional<MasterAccessControlEntry> mediatorAce;
        Optional<OwnerAccessControlEntry> ownerAce;

        synchronized (localDomainAccessStore) {
            masterAce = Optional.ofNullable(localDomainAccessStore.getMasterAccessControlEntry(userId,
                                                                                               domain,
                                                                                               interfaceName,
                                                                                               operation));
            mediatorAce = Optional.ofNullable(localDomainAccessStore.getMediatorAccessControlEntry(userId,
                                                                                                   domain,
                                                                                                   interfaceName,
                                                                                                   operation));
            ownerAce = Optional.ofNullable(localDomainAccessStore.getOwnerAccessControlEntry(userId,
                                                                                             domain,
                                                                                             interfaceName,
                                                                                             operation));
        }

        Permission permission = accessControlAlgorithm.getConsumerPermission(masterAce,
                                                                             mediatorAce,
                                                                             ownerAce,
                                                                             trustLevel);

        logger.debug("getConsumerPermission on domain {}, interface {} result: {}",
                     domain,
                     interfaceName,
                     getPermissionLogStatement(permission));

        return permission;
    }

    private String getPermissionLogStatement(Permission permission) {
        switch (permission) {
        case YES:
            return "permission granted";
        case NO:
            return "permission denied";
        case ASK:
            return "ask for permission";
        default:
            return "permission unknown";
        }
    }

    @Override
    public Permission getProviderPermission(String uid, String domain, String interfaceName, TrustLevel trustLevel) {
        logger.debug("getProviderPermission on domain {}, interface {}", domain, interfaceName);
        Optional<MasterRegistrationControlEntry> masterRce;
        Optional<MasterRegistrationControlEntry> mediatorRce;
        Optional<OwnerRegistrationControlEntry> ownerRce;

        synchronized (localDomainAccessStore) {
            masterRce = Optional.ofNullable(localDomainAccessStore.getMasterRegistrationControlEntry(uid,
                                                                                                     domain,
                                                                                                     interfaceName));
            mediatorRce = Optional.ofNullable(localDomainAccessStore.getMediatorRegistrationControlEntry(uid,
                                                                                                         domain,
                                                                                                         interfaceName));
            ownerRce = Optional.ofNullable(localDomainAccessStore.getOwnerRegistrationControlEntry(uid,
                                                                                                   domain,
                                                                                                   interfaceName));
        }

        return accessControlAlgorithm.getProviderPermission(masterRce, mediatorRce, ownerRce, trustLevel);
    }

}
