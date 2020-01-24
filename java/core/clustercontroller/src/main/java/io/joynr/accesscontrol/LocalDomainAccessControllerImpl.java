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
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

import io.joynr.accesscontrol.broadcastlistener.LdacDomainRoleEntryChangedBroadcastListener;
import io.joynr.accesscontrol.broadcastlistener.LdacMasterAccessControlEntryChangedBroadcastListener;
import io.joynr.accesscontrol.broadcastlistener.LdacMediatorAccessControlEntryChangedBroadcastListener;
import io.joynr.accesscontrol.broadcastlistener.LdacOwnerAccessControlEntryChangedBroadcastListener;
import io.joynr.accesscontrol.primarykey.UserDomainInterfaceOperationKey;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.exceptions.JoynrWaitExpiredException;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.proxy.Callback;
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyBuilderFactory;
import io.joynr.runtime.SystemServicesSettings;
import joynr.MulticastSubscriptionQos;
import joynr.exceptions.ApplicationException;
import joynr.infrastructure.GlobalCapabilitiesDirectory;
import joynr.infrastructure.GlobalDomainAccessController;
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

    // The LDAC subscribes to broadcasts of ACL changes, below are the subscriptionQos parameters
    private static final long QOS_DURATION_MS = 10 * 365 * 24 * 3600 * 1000L; // 10 years

    private static final Logger LOG = LoggerFactory.getLogger(LocalDomainAccessControllerImpl.class);
    private static final String SINGLE_LEVEL_WILDCARD = "+";

    private final String discoveryDirectoriesDomain;
    private AccessControlAlgorithm accessControlAlgorithm = new AccessControlAlgorithm();
    private static final String WILDCARD = "*";
    private Map<UserDomainInterfaceOperationKey, AceSubscription> subscriptionsMap = new HashMap<UserDomainInterfaceOperationKey, AceSubscription>();
    private Set<UserDomainInterfaceOperationKey> ongoingSubscriptions = new HashSet<UserDomainInterfaceOperationKey>();
    private GlobalDomainAccessControllerClient globalDomainAccessControllerClient;

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
        globalDomainAccessControllerClient = new GlobalDomainAccessControllerClient(discoveryDirectoriesDomain,
                                                                                    proxyBuilderFactory);
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

        if (!hasRole) {
            subscribeForDreChange(userId);
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
        LOG.debug("getConsumerPermission on domain {}, interface {}", domain, interfaceName);

        // Handle special cases which should not require a lookup or a subscription
        Optional<Permission> specialPermission = handleSpecialCases(domain, interfaceName);
        if (specialPermission.isPresent()) {
            callback.getConsumerPermission(specialPermission.get());
            return;
        }

        if (subscriptionsMap.get(subscriptionKey) == null) {
            if (!ongoingSubscriptions.contains(subscriptionKey)) {
                queryDomainRoles(userId);
                queryAccessControlEntries(domain, interfaceName, new QueryAccessControlEntriesCallback() {
                    @Override
                    public void queryAccessControlEntriesSucceeded() {
                        subscriptionsMap.put(subscriptionKey, subscribeForAceChange(domain, interfaceName));
                        ongoingSubscriptions.remove(subscriptionKey);
                        getConsumerPermissionWithCachedEntries(userId, domain, interfaceName, trustLevel, callback);
                    }

                    @Override
                    public void queryAccessControlEntriesFailed() {
                        callback.getConsumerPermissionFailed();
                    }
                });
            }
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
                    || interfaceName.equals(GlobalDomainAccessController.INTERFACE_NAME)
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
        LOG.debug("getConsumerPermission on domain {}, interface {}", domain, interfaceName);
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
    public List<MasterAccessControlEntry> getMasterAccessControlEntries(String uid) {
        return globalDomainAccessControllerClient.getMasterAccessControlEntries(uid);
    }

    @Override
    public Future<List<MasterAccessControlEntry>> getMasterAccessControlEntries(Callback<List<MasterAccessControlEntry>> callback,
                                                                                String uid) {
        return globalDomainAccessControllerClient.getMasterAccessControlEntries(callback, uid);
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
    public boolean updateMasterAccessControlEntry(final MasterAccessControlEntry updatedMasterAce) {
        return globalDomainAccessControllerClient.updateMasterAccessControlEntry(updatedMasterAce);
    }

    @Override
    public Future<Boolean> updateMasterAccessControlEntry(Callback<Boolean> callback,
                                                          final MasterAccessControlEntry updatedMasterAce) {
        return globalDomainAccessControllerClient.updateMasterAccessControlEntry(callback, updatedMasterAce);
    }

    @Override
    public boolean removeMasterAccessControlEntry(String uid, String domain, String interfaceName, String operation) {
        return globalDomainAccessControllerClient.removeMasterAccessControlEntry(uid, domain, interfaceName, operation);
    }

    @Override
    public Future<Boolean> removeMasterAccessControlEntry(Callback<Boolean> callback,
                                                          String uid,
                                                          String domain,
                                                          String interfaceName,
                                                          String operation) {
        return globalDomainAccessControllerClient.removeMasterAccessControlEntry(callback,
                                                                                 uid,
                                                                                 domain,
                                                                                 interfaceName,
                                                                                 operation);
    }

    @Override
    public List<MasterAccessControlEntry> getMediatorAccessControlEntries(String uid) {
        return globalDomainAccessControllerClient.getMediatorAccessControlEntries(uid);
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
    public boolean updateMediatorAccessControlEntry(MasterAccessControlEntry updatedMediatorAce) {
        return globalDomainAccessControllerClient.updateMediatorAccessControlEntry(updatedMediatorAce);
    }

    @Override
    public Future<Boolean> updateMediatorAccessControlEntry(Callback<Boolean> callback,
                                                            MasterAccessControlEntry updatedMediatorAce) {
        return globalDomainAccessControllerClient.updateMediatorAccessControlEntry(callback, updatedMediatorAce);
    }

    @Override
    public boolean removeMediatorAccessControlEntry(String uid, String domain, String interfaceName, String operation) {
        return globalDomainAccessControllerClient.removeMediatorAccessControlEntry(uid,
                                                                                   domain,
                                                                                   interfaceName,
                                                                                   operation);
    }

    @Override
    public Future<Boolean> removeMediatorAccessControlEntry(Callback<Boolean> callback,
                                                            String uid,
                                                            String domain,
                                                            String interfaceName,
                                                            String operation) {
        return globalDomainAccessControllerClient.removeMediatorAccessControlEntry(callback,
                                                                                   uid,
                                                                                   domain,
                                                                                   interfaceName,
                                                                                   operation);
    }

    @Override
    public List<OwnerAccessControlEntry> getOwnerAccessControlEntries(String uid) {
        return globalDomainAccessControllerClient.getOwnerAccessControlEntries(uid);
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
    public boolean updateOwnerAccessControlEntry(OwnerAccessControlEntry updatedOwnerAce) {
        return globalDomainAccessControllerClient.updateOwnerAccessControlEntry(updatedOwnerAce);
    }

    @Override
    public Future<Boolean> updateOwnerAccessControlEntry(Callback<Boolean> callback,
                                                         OwnerAccessControlEntry updatedOwnerAce) {
        return globalDomainAccessControllerClient.updateOwnerAccessControlEntry(callback, updatedOwnerAce);
    }

    @Override
    public boolean removeOwnerAccessControlEntry(String uid, String domain, String interfaceName, String operation) {
        return globalDomainAccessControllerClient.removeOwnerAccessControlEntry(uid, domain, interfaceName, operation);
    }

    @Override
    public Future<Boolean> removeOwnerAccessControlEntry(Callback<Boolean> callback,
                                                         String uid,
                                                         String domain,
                                                         String interfaceName,
                                                         String operation) {
        return globalDomainAccessControllerClient.removeOwnerAccessControlEntry(callback,
                                                                                uid,
                                                                                domain,
                                                                                interfaceName,
                                                                                operation);
    }

    @Override
    public Permission getProviderPermission(String uid, String domain, String interfaceName, TrustLevel trustLevel) {
        throw new UnsupportedOperationException("Provider registration permission check is not implemented yet.");
        //        return accessControlAlgorithm.getProviderPermission(null, null, null, trustLevel);
    }

    @Override
    public List<MasterRegistrationControlEntry> getMasterRegistrationControlEntries(String uid) {
        return globalDomainAccessControllerClient.getMasterRegistrationControlEntries(uid);
    }

    @Override
    public Future<List<MasterRegistrationControlEntry>> getMasterRegistrationControlEntries(Callback<List<MasterRegistrationControlEntry>> callback,
                                                                                            String uid) {
        return globalDomainAccessControllerClient.getMasterRegistrationControlEntries(callback, uid);
    }

    @Override
    public List<MasterRegistrationControlEntry> getEditableMasterRegistrationControlEntries(String uid) {
        return globalDomainAccessControllerClient.getEditableMasterRegistrationControlEntries(uid);
    }

    @Override
    public Future<List<MasterRegistrationControlEntry>> getEditableMasterRegistrationControlEntries(Callback<List<MasterRegistrationControlEntry>> callback,
                                                                                                    String uid) {
        return globalDomainAccessControllerClient.getEditableMasterRegistrationControlEntries(callback, uid);
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
    public List<MasterRegistrationControlEntry> getMediatorRegistrationControlEntries(String uid) {
        return globalDomainAccessControllerClient.getMediatorRegistrationControlEntries(uid);
    }

    @Override
    public Future<List<MasterRegistrationControlEntry>> getMediatorRegistrationControlEntries(Callback<List<MasterRegistrationControlEntry>> callback,
                                                                                              String uid) {
        return globalDomainAccessControllerClient.getMediatorRegistrationControlEntries(callback, uid);
    }

    @Override
    public List<MasterRegistrationControlEntry> getEditableMediatorRegistrationControlEntries(String uid) {
        return globalDomainAccessControllerClient.getEditableMediatorRegistrationControlEntries(uid);
    }

    @Override
    public Future<List<MasterRegistrationControlEntry>> getEditableMediatorRegistrationControlEntries(Callback<List<MasterRegistrationControlEntry>> callback,
                                                                                                      String uid) {
        return globalDomainAccessControllerClient.getEditableMasterRegistrationControlEntries(callback, uid);
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
    public List<OwnerRegistrationControlEntry> getOwnerRegistrationControlEntries(String uid) {
        return globalDomainAccessControllerClient.getOwnerRegistrationControlEntries(uid);
    }

    @Override
    public Future<List<OwnerRegistrationControlEntry>> getOwnerRegistrationControlEntries(Callback<List<OwnerRegistrationControlEntry>> callback,
                                                                                          String uid) {
        return globalDomainAccessControllerClient.getOwnerRegistrationControlEntries(callback, uid);
    }

    @Override
    public List<OwnerRegistrationControlEntry> getEditableOwnerRegistrationControlEntries(String uid) {
        return globalDomainAccessControllerClient.getEditableOwnerRegistrationControlEntries(uid);
    }

    @Override
    public Future<List<OwnerRegistrationControlEntry>> getEditableOwnerRegistrationControlEntries(Callback<List<OwnerRegistrationControlEntry>> callback,
                                                                                                  String uid) {
        return globalDomainAccessControllerClient.getEditableOwnerRegistrationControlEntries(callback, uid);
    }

    @Override
    public boolean updateOwnerRegistrationControlEntry(OwnerRegistrationControlEntry updatedOwnerRce) {
        throw new UnsupportedOperationException("Provider registration permission check is not implemented yet.");
    }

    @Override
    public boolean removeOwnerRegistrationControlEntry(String uid, String domain, String interfaceName) {
        throw new UnsupportedOperationException("Provider registration permission check is not implemented yet.");
    }

    @Override
    public void unsubscribeFromAceChanges(String domain, String interfaceName) {
        UserDomainInterfaceOperationKey subscriptionKey = new UserDomainInterfaceOperationKey(null,
                                                                                              domain,
                                                                                              interfaceName,
                                                                                              null);
        AceSubscription subscriptions = subscriptionsMap.get(subscriptionKey);
        if (subscriptions != null) {
            try {
                globalDomainAccessControllerClient.unsubscribeFromMasterAccessControlEntryChangedBroadcast(subscriptions.getMasterSubscriptionId());
                globalDomainAccessControllerClient.unsubscribeFromMediatorAccessControlEntryChangedBroadcast(subscriptions.getMediatorSubscriptionId());
                globalDomainAccessControllerClient.unsubscribeFromOwnerAccessControlEntryChangedBroadcast(subscriptions.getOwnerSubscriptionId());
            } catch (JoynrRuntimeException | InterruptedException | ApplicationException e) {
                LOG.warn("unsubscribe from AceChanges failed due to the following error: {}", e.getMessage());
                return;
            }
        } else {
            /*
             * This can be the case, when no consumer request has been performed during the lifetime of the provider
             */
            LOG.debug("Subscription for ace subscription for interface '{}' domain '{}' not found",
                      interfaceName,
                      domain);
        }
    }

    private String sanitiseForPartition(String value) {
        return value.replaceAll("[^a-zA-Z0-9]", "");
    }

    private void subscribeForDreChange(String userId) {
        long wsbExpiryDate = System.currentTimeMillis() + QOS_DURATION_MS;
        MulticastSubscriptionQos broadcastSubscriptionQos = new MulticastSubscriptionQos();
        broadcastSubscriptionQos.setExpiryDateMs(wsbExpiryDate);
        globalDomainAccessControllerClient.subscribeToDomainRoleEntryChangedBroadcast(new LdacDomainRoleEntryChangedBroadcastListener(localDomainAccessStore),
                                                                                      broadcastSubscriptionQos,
                                                                                      sanitiseForPartition(userId));
    }

    private AceSubscription subscribeForAceChange(String domain, String interfaceName) {
        long wsbExpiryDate = System.currentTimeMillis() + QOS_DURATION_MS;
        MulticastSubscriptionQos broadcastSubscriptionQos = new MulticastSubscriptionQos();
        broadcastSubscriptionQos.setExpiryDateMs(wsbExpiryDate);
        Future<String> mastersubscriptionId = globalDomainAccessControllerClient.subscribeToMasterAccessControlEntryChangedBroadcast(new LdacMasterAccessControlEntryChangedBroadcastListener(localDomainAccessStore),
                                                                                                                                     broadcastSubscriptionQos,
                                                                                                                                     SINGLE_LEVEL_WILDCARD,
                                                                                                                                     sanitiseForPartition(domain),
                                                                                                                                     sanitiseForPartition(interfaceName));

        Future<String> mediatorsubscriptionId = globalDomainAccessControllerClient.subscribeToMediatorAccessControlEntryChangedBroadcast(new LdacMediatorAccessControlEntryChangedBroadcastListener(localDomainAccessStore),
                                                                                                                                         broadcastSubscriptionQos,
                                                                                                                                         SINGLE_LEVEL_WILDCARD,
                                                                                                                                         sanitiseForPartition(domain),
                                                                                                                                         sanitiseForPartition(interfaceName));

        Future<String> ownersubscriptionId = globalDomainAccessControllerClient.subscribeToOwnerAccessControlEntryChangedBroadcast(new LdacOwnerAccessControlEntryChangedBroadcastListener(localDomainAccessStore),
                                                                                                                                   broadcastSubscriptionQos,
                                                                                                                                   SINGLE_LEVEL_WILDCARD,
                                                                                                                                   sanitiseForPartition(domain),
                                                                                                                                   sanitiseForPartition(interfaceName));

        return new AceSubscription(mastersubscriptionId, mediatorsubscriptionId, ownersubscriptionId);
    }

    private void queryDomainRoles(String userId) {
        LOG.debug("queryDomainRoles on userId {}", userId);

        globalDomainAccessControllerClient.getDomainRoles(new Callback<DomainRoleEntry[]>() {
            @Override
            public void onFailure(JoynrRuntimeException runtimeException) {
            }

            @Override
            public void onSuccess(DomainRoleEntry[] domainRoleEntries) {
                if (domainRoleEntries != null) {
                    synchronized (localDomainAccessStore) {
                        for (DomainRoleEntry entry : domainRoleEntries) {
                            localDomainAccessStore.updateDomainRole(entry);
                        }
                    }
                }
            }
        }, userId);
    }

    private void queryAccessControlEntries(String domain,
                                           String interfaceName,
                                           QueryAccessControlEntriesCallback callback) {
        LOG.debug("queryAccessControlEntries on domain {}, interface {}", domain, interfaceName);

        final AceQuerySync querySync = new AceQuerySync(callback);

        globalDomainAccessControllerClient.getMasterAccessControlEntries(new Callback<MasterAccessControlEntry[]>() {
            @Override
            public void onFailure(JoynrRuntimeException runtimeException) {
                LOG.error("Failed to query master access control entries: {}", runtimeException.toString());
                querySync.registerException();
            }

            @Override
            public void onSuccess(MasterAccessControlEntry[] masterAccessControlEntries) {
                synchronized (localDomainAccessStore) {
                    if (masterAccessControlEntries != null) {
                        for (MasterAccessControlEntry entry : masterAccessControlEntries) {
                            localDomainAccessStore.updateMasterAccessControlEntry(entry);
                        }
                    }
                }
                querySync.registerSuccess();
            }
        }, domain, interfaceName);

        globalDomainAccessControllerClient.getMediatorAccessControlEntries(new Callback<MasterAccessControlEntry[]>() {
            @Override
            public void onFailure(JoynrRuntimeException runtimeException) {
                LOG.error("Failed to query mediator access control entries: {}", runtimeException.toString());
                querySync.registerException();
            }

            @Override
            public void onSuccess(MasterAccessControlEntry[] mediatorAccessControlEntries) {
                synchronized (localDomainAccessStore) {
                    if (mediatorAccessControlEntries != null) {
                        for (MasterAccessControlEntry entry : mediatorAccessControlEntries) {
                            localDomainAccessStore.updateMediatorAccessControlEntry(entry);
                        }
                    }
                }
                querySync.registerSuccess();
            }
        }, domain, interfaceName);

        globalDomainAccessControllerClient.getOwnerAccessControlEntries(new Callback<OwnerAccessControlEntry[]>() {
            @Override
            public void onFailure(JoynrRuntimeException runtimeException) {
                LOG.error("Failed to query owner access control entries: {}", runtimeException.toString());
                querySync.registerException();
            }

            @Override
            public void onSuccess(OwnerAccessControlEntry[] ownerAccessControlEntries) {
                synchronized (localDomainAccessStore) {
                    if (ownerAccessControlEntries != null) {
                        for (OwnerAccessControlEntry entry : ownerAccessControlEntries) {
                            localDomainAccessStore.updateOwnerAccessControlEntry(entry);
                        }
                    }
                }
                querySync.registerSuccess();
            }
        }, domain, interfaceName);

        LOG.debug("Finished initializeLocalDomainAccessStore on domain {}, interface {}", domain, interfaceName);
    }

    static class AceQuerySync {
        private int numPendingResponses = 3;
        private int numErrors = 0;
        private QueryAccessControlEntriesCallback finishedCallback;

        public AceQuerySync(QueryAccessControlEntriesCallback finishedCallback) {
            this.finishedCallback = finishedCallback;
        }

        public synchronized void registerSuccess() {
            registerReponse();
        }

        public synchronized void registerException() {
            numErrors++;
            registerReponse();
        }

        private void registerReponse() {
            numPendingResponses--;

            if (numPendingResponses <= 0) {
                if (numErrors == 0) {
                    finishedCallback.queryAccessControlEntriesSucceeded();
                } else {
                    finishedCallback.queryAccessControlEntriesFailed();
                }
            }
        }
    }
}
