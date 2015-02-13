package io.joynr.accesscontrol;

import io.joynr.accesscontrol.broadcastlistener.LdacDomainRoleEntryChangedBroadcastListener;
import io.joynr.accesscontrol.broadcastlistener.LdacMasterAccessControlEntryChangedBroadcastListener;
import io.joynr.accesscontrol.broadcastlistener.LdacMediatorAccessControlEntryChangedBroadcastListener;
import io.joynr.accesscontrol.broadcastlistener.LdacOwnerAccessControlEntryChangedBroadcastListener;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.capabilities.LocalCapabilitiesDirectory;
import io.joynr.dispatcher.rpc.Callback;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilderDefaultImpl;
import io.joynr.proxy.ProxyInvocationHandlerFactory;

import java.util.List;

import joynr.OnChangeSubscriptionQos;
import joynr.infrastructure.DomainRoleEntry;
import joynr.infrastructure.GlobalDomainAccessControllerBroadcastInterface.DomainRoleEntryChangedBroadcastFilterParameters;
import joynr.infrastructure.GlobalDomainAccessControllerBroadcastInterface.MasterAccessControlEntryChangedBroadcastFilterParameters;
import joynr.infrastructure.GlobalDomainAccessControllerBroadcastInterface.MediatorAccessControlEntryChangedBroadcastFilterParameters;
import joynr.infrastructure.GlobalDomainAccessControllerBroadcastInterface.OwnerAccessControlEntryChangedBroadcastFilterParameters;
import joynr.infrastructure.GlobalDomainAccessControllerProxy;
import joynr.infrastructure.MasterAccessControlEntry;
import joynr.infrastructure.MasterRegistrationControlEntry;
import joynr.infrastructure.OwnerAccessControlEntry;
import joynr.infrastructure.OwnerRegistrationControlEntry;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

public class GlobalDomainAccessControllerClient {

    // TODO: define a proper max messaging ttl
    private static final long TTL_30_DAYS_IN_MS = 30L * 24L * 60L * 60L * 1000L;
    private String domain;
    private LocalCapabilitiesDirectory capabilitiesDirectory;
    private ProxyInvocationHandlerFactory proxyInvocationHandlerFactory;

    public GlobalDomainAccessControllerClient(String domain,
                                              LocalCapabilitiesDirectory capabilitiesDirectory,
                                              ProxyInvocationHandlerFactory proxyInvocationHandlerFactory) {
        this.domain = domain;
        this.capabilitiesDirectory = capabilitiesDirectory;
        this.proxyInvocationHandlerFactory = proxyInvocationHandlerFactory;
    }

    private GlobalDomainAccessControllerProxy getProxy(long ttl) {
        ProxyBuilder<GlobalDomainAccessControllerProxy> accessControlProxyBuilder = new ProxyBuilderDefaultImpl<GlobalDomainAccessControllerProxy>(capabilitiesDirectory,
                                                                                                                                                   domain,
                                                                                                                                                   GlobalDomainAccessControllerProxy.class,
                                                                                                                                                   proxyInvocationHandlerFactory);
        DiscoveryQos discoveryQos = new DiscoveryQos(DiscoveryScope.GLOBAL_ONLY, DiscoveryQos.NO_MAX_AGE);
        MessagingQos messagingQos = new MessagingQos(ttl);
        return accessControlProxyBuilder.setDiscoveryQos(discoveryQos).setMessagingQos(messagingQos).build();
    }

    public List<MasterAccessControlEntry> getMasterAccessControlEntries(String userId) {
        return getProxy(TTL_30_DAYS_IN_MS).getMasterAccessControlEntries(userId);
    }

    public Future<List<MasterAccessControlEntry>> getMasterAccessControlEntries(Callback<List<MasterAccessControlEntry>> callback,
                                                                                String userId) {
        return getProxy(TTL_30_DAYS_IN_MS).getMasterAccessControlEntries(callback, userId);
    }

    public boolean updateMasterAccessControlEntry(MasterAccessControlEntry updatedMasterAce) {
        return getProxy(TTL_30_DAYS_IN_MS).updateMasterAccessControlEntry(updatedMasterAce);
    }

    public Future<Boolean> updateMasterAccessControlEntry(Callback<Boolean> callback,
                                                          MasterAccessControlEntry updatedMasterAce) {
        return getProxy(TTL_30_DAYS_IN_MS).updateMasterAccessControlEntry(callback, updatedMasterAce);
    }

    public boolean removeMasterAccessControlEntry(String userId, String domain, String interfaceName, String operation) {
        return getProxy(TTL_30_DAYS_IN_MS).removeMasterAccessControlEntry(userId, domain, interfaceName, operation);
    }

    public List<MasterAccessControlEntry> getMediatorAccessControlEntries(String userId) {
        return getProxy(TTL_30_DAYS_IN_MS).getMediatorAccessControlEntries(userId);
    }

    public Future<Boolean> removeMasterAccessControlEntry(Callback<Boolean> callback,
                                                          String userId,
                                                          String domain,
                                                          String interfaceName,
                                                          String operation) {
        return getProxy(TTL_30_DAYS_IN_MS).removeMasterAccessControlEntry(callback,
                                                                          userId,
                                                                          domain,
                                                                          interfaceName,
                                                                          operation);
    }

    public boolean updateMediatorAccessControlEntry(MasterAccessControlEntry updatedMediatorAce) {
        return getProxy(TTL_30_DAYS_IN_MS).updateMediatorAccessControlEntry(updatedMediatorAce);
    }

    public Future<Boolean> updateMediatorAccessControlEntry(Callback<Boolean> callback,
                                                            MasterAccessControlEntry updatedMediatorAce) {
        return getProxy(TTL_30_DAYS_IN_MS).updateMediatorAccessControlEntry(callback, updatedMediatorAce);
    }

    public boolean removeMediatorAccessControlEntry(String userId, String domain, String interfaceName, String operation) {
        return getProxy(TTL_30_DAYS_IN_MS).removeMediatorAccessControlEntry(userId, domain, interfaceName, operation);
    }

    public Future<Boolean> removeMediatorAccessControlEntry(Callback<Boolean> callback,
                                                            String userId,
                                                            String domain,
                                                            String interfaceName,
                                                            String operation) {
        return getProxy(TTL_30_DAYS_IN_MS).removeMediatorAccessControlEntry(callback,
                                                                            userId,
                                                                            domain,
                                                                            interfaceName,
                                                                            operation);
    }

    public List<OwnerAccessControlEntry> getOwnerAccessControlEntries(String userId) {
        return getProxy(TTL_30_DAYS_IN_MS).getOwnerAccessControlEntries(userId);
    }

    public boolean updateOwnerAccessControlEntry(OwnerAccessControlEntry updatedOwnerAce) {
        return getProxy(TTL_30_DAYS_IN_MS).updateOwnerAccessControlEntry(updatedOwnerAce);
    }

    public Future<Boolean> updateOwnerAccessControlEntry(Callback<Boolean> callback,
                                                         OwnerAccessControlEntry updatedOwnerAce) {
        return getProxy(TTL_30_DAYS_IN_MS).updateOwnerAccessControlEntry(callback, updatedOwnerAce);
    }

    public boolean removeOwnerAccessControlEntry(String userId, String domain, String interfaceName, String operation) {
        return getProxy(TTL_30_DAYS_IN_MS).removeOwnerAccessControlEntry(userId, domain, interfaceName, operation);
    }

    public Future<Boolean> removeOwnerAccessControlEntry(Callback<Boolean> callback,
                                                         String userId,
                                                         String domain,
                                                         String interfaceName,
                                                         String operation) {
        return getProxy(TTL_30_DAYS_IN_MS).removeOwnerAccessControlEntry(callback,
                                                                         userId,
                                                                         domain,
                                                                         interfaceName,
                                                                         operation);
    }

    public List<MasterRegistrationControlEntry> getMasterRegistrationControlEntries(String userId) {
        return getProxy(TTL_30_DAYS_IN_MS).getMasterRegistrationControlEntries(userId);
    }

    public Future<List<MasterRegistrationControlEntry>> getMasterRegistrationControlEntries(Callback<List<MasterRegistrationControlEntry>> callback,
                                                                                            String userId) {
        return getProxy(TTL_30_DAYS_IN_MS).getMasterRegistrationControlEntries(callback, userId);
    }

    public List<MasterRegistrationControlEntry> getEditableMasterRegistrationControlEntries(String userId) {
        return getProxy(TTL_30_DAYS_IN_MS).getEditableMasterRegistrationControlEntries(userId);
    }

    public Future<List<MasterRegistrationControlEntry>> getEditableMasterRegistrationControlEntries(Callback<List<MasterRegistrationControlEntry>> callback,
                                                                                                    String userId) {
        return getProxy(TTL_30_DAYS_IN_MS).getEditableMasterRegistrationControlEntries(callback, userId);
    }

    public List<MasterRegistrationControlEntry> getMediatorRegistrationControlEntries(String userId) {
        return getProxy(TTL_30_DAYS_IN_MS).getMediatorRegistrationControlEntries(userId);
    }

    public Future<List<MasterRegistrationControlEntry>> getMediatorRegistrationControlEntries(Callback<List<MasterRegistrationControlEntry>> callback,
                                                                                              String userId) {
        return getProxy(TTL_30_DAYS_IN_MS).getMediatorRegistrationControlEntries(callback, userId);
    }

    public List<MasterRegistrationControlEntry> getEditableMediatorRegistrationControlEntries(String userId) {
        return getProxy(TTL_30_DAYS_IN_MS).getEditableMediatorRegistrationControlEntries(userId);
    }

    public List<OwnerRegistrationControlEntry> getOwnerRegistrationControlEntries(String userId) {
        return getProxy(TTL_30_DAYS_IN_MS).getOwnerRegistrationControlEntries(userId);
    }

    public Future<List<OwnerRegistrationControlEntry>> getOwnerRegistrationControlEntries(Callback<List<OwnerRegistrationControlEntry>> callback,
                                                                                          String userId) {
        return getProxy(TTL_30_DAYS_IN_MS).getOwnerRegistrationControlEntries(callback, userId);
    }

    public List<OwnerRegistrationControlEntry> getEditableOwnerRegistrationControlEntries(String userId) {
        return getProxy(TTL_30_DAYS_IN_MS).getEditableOwnerRegistrationControlEntries(userId);
    }

    public Future<List<OwnerRegistrationControlEntry>> getEditableOwnerRegistrationControlEntries(Callback<List<OwnerRegistrationControlEntry>> callback,
                                                                                                  String userId) {
        return getProxy(TTL_30_DAYS_IN_MS).getEditableOwnerRegistrationControlEntries(callback, userId);
    }

    public void subscribeToDomainRoleEntryChangedBroadcast(LdacDomainRoleEntryChangedBroadcastListener ldacDomainRoleEntryChangedBroadcastListener,
                                                           OnChangeSubscriptionQos broadcastSubscriptionQos,
                                                           DomainRoleEntryChangedBroadcastFilterParameters domainRoleFilterParameters) {
        getProxy(TTL_30_DAYS_IN_MS).subscribeToDomainRoleEntryChangedBroadcast(ldacDomainRoleEntryChangedBroadcastListener,
                                                                               broadcastSubscriptionQos,
                                                                               domainRoleFilterParameters);

    }

    public String subscribeToMasterAccessControlEntryChangedBroadcast(LdacMasterAccessControlEntryChangedBroadcastListener ldacMasterAccessControlEntryChangedBroadcastListener,
                                                                      OnChangeSubscriptionQos broadcastSubscriptionQos,
                                                                      MasterAccessControlEntryChangedBroadcastFilterParameters masterAcefilterParameters) {
        return getProxy(TTL_30_DAYS_IN_MS).subscribeToMasterAccessControlEntryChangedBroadcast(ldacMasterAccessControlEntryChangedBroadcastListener,
                                                                                               broadcastSubscriptionQos,
                                                                                               masterAcefilterParameters);
    }

    public void subscribeToMediatorAccessControlEntryChangedBroadcast(LdacMediatorAccessControlEntryChangedBroadcastListener ldacMediatorAccessControlEntryChangedBroadcastListener,
                                                                      OnChangeSubscriptionQos broadcastSubscriptionQos,
                                                                      MediatorAccessControlEntryChangedBroadcastFilterParameters mediatorAceFilterParameters,
                                                                      String subscriptionId) {
        getProxy(TTL_30_DAYS_IN_MS).subscribeToMediatorAccessControlEntryChangedBroadcast(ldacMediatorAccessControlEntryChangedBroadcastListener,
                                                                                          broadcastSubscriptionQos,
                                                                                          mediatorAceFilterParameters,
                                                                                          subscriptionId);
    }

    public void subscribeToOwnerAccessControlEntryChangedBroadcast(LdacOwnerAccessControlEntryChangedBroadcastListener ldacOwnerAccessControlEntryChangedBroadcastListener,
                                                                   OnChangeSubscriptionQos broadcastSubscriptionQos,
                                                                   OwnerAccessControlEntryChangedBroadcastFilterParameters ownerAceFilterParameters,
                                                                   String subscriptionId) {
        getProxy(TTL_30_DAYS_IN_MS).subscribeToOwnerAccessControlEntryChangedBroadcast(ldacOwnerAccessControlEntryChangedBroadcastListener,
                                                                                       broadcastSubscriptionQos,
                                                                                       ownerAceFilterParameters,
                                                                                       subscriptionId);

    }

    public List<DomainRoleEntry> getDomainRoles(String userId) {
        return getProxy(TTL_30_DAYS_IN_MS).getDomainRoles(userId);
    }

    public List<MasterAccessControlEntry> getMasterAccessControlEntries(String domain, String interfaceName) {
        return getProxy(TTL_30_DAYS_IN_MS).getMasterAccessControlEntries(domain, interfaceName);
    }

    public List<MasterAccessControlEntry> getMediatorAccessControlEntries(String domain, String interfaceName) {
        return getProxy(TTL_30_DAYS_IN_MS).getMediatorAccessControlEntries(domain, interfaceName);
    }

    public List<OwnerAccessControlEntry> getOwnerAccessControlEntries(String domain, String interfaceName) {
        return getProxy(TTL_30_DAYS_IN_MS).getOwnerAccessControlEntries(domain, interfaceName);
    }
}
