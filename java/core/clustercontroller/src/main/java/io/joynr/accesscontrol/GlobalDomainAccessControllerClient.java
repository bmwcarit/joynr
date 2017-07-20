package io.joynr.accesscontrol;

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

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import io.joynr.accesscontrol.broadcastlistener.LdacDomainRoleEntryChangedBroadcastListener;
import io.joynr.accesscontrol.broadcastlistener.LdacMasterAccessControlEntryChangedBroadcastListener;
import io.joynr.accesscontrol.broadcastlistener.LdacMediatorAccessControlEntryChangedBroadcastListener;
import io.joynr.accesscontrol.broadcastlistener.LdacOwnerAccessControlEntryChangedBroadcastListener;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.Callback;
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilderFactory;
import joynr.MulticastSubscriptionQos;
import joynr.infrastructure.DacTypes.DomainRoleEntry;
import joynr.infrastructure.DacTypes.MasterAccessControlEntry;
import joynr.infrastructure.DacTypes.MasterRegistrationControlEntry;
import joynr.infrastructure.DacTypes.OwnerAccessControlEntry;
import joynr.infrastructure.DacTypes.OwnerRegistrationControlEntry;
import joynr.infrastructure.GlobalDomainAccessControllerProxy;
import joynr.infrastructure.GlobalDomainRoleControllerProxy;
import joynr.infrastructure.GlobalDomainAccessControlListEditorProxy;

public class GlobalDomainAccessControllerClient {

    // TODO: define a proper max messaging ttl
    private static final long TTL_30_DAYS_IN_MS = 30L * 24L * 60L * 60L * 1000L;
    private String domain;
    private final ProxyBuilderFactory proxyBuilderFactory;

    public GlobalDomainAccessControllerClient(String domain, ProxyBuilderFactory proxyBuilderFactory) {
        this.domain = domain;
        this.proxyBuilderFactory = proxyBuilderFactory;
    }

    private GlobalDomainAccessControllerProxy getGlobalDomainAccessControllerProxy(long ttl) {
        ProxyBuilder<GlobalDomainAccessControllerProxy> accessControlProxyBuilder = proxyBuilderFactory.get(domain,
                                                                                                            GlobalDomainAccessControllerProxy.class);
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);
        discoveryQos.setCacheMaxAgeMs(DiscoveryQos.NO_MAX_AGE);
        MessagingQos messagingQos = new MessagingQos(ttl);
        return accessControlProxyBuilder.setDiscoveryQos(discoveryQos).setMessagingQos(messagingQos).build();
    }

    private GlobalDomainRoleControllerProxy getGlobalDomainRoleControllerProxy(long ttl) {
        ProxyBuilder<GlobalDomainRoleControllerProxy> roleControlProxyBuilder = proxyBuilderFactory.get(domain,
                                                                                                        GlobalDomainRoleControllerProxy.class);
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);
        discoveryQos.setCacheMaxAgeMs(DiscoveryQos.NO_MAX_AGE);
        MessagingQos messagingQos = new MessagingQos(ttl);
        return roleControlProxyBuilder.setDiscoveryQos(discoveryQos).setMessagingQos(messagingQos).build();
    }

    private GlobalDomainAccessControlListEditorProxy getGlobalDomainAccessControlListEditorProxy(long ttl) {
        ProxyBuilder<GlobalDomainAccessControlListEditorProxy> accessControlListEditorProxyBuilder = proxyBuilderFactory.get(domain,
                                                                                                                             GlobalDomainAccessControlListEditorProxy.class);
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);
        discoveryQos.setCacheMaxAgeMs(DiscoveryQos.NO_MAX_AGE);
        MessagingQos messagingQos = new MessagingQos(ttl);
        return accessControlListEditorProxyBuilder.setDiscoveryQos(discoveryQos).setMessagingQos(messagingQos).build();
    }

    public List<MasterAccessControlEntry> getMasterAccessControlEntries(String userId) {
        return Arrays.asList(getGlobalDomainAccessControllerProxy(TTL_30_DAYS_IN_MS).getMasterAccessControlEntries(userId));
    }

    public Future<List<MasterAccessControlEntry>> getMasterAccessControlEntries(final Callback<List<MasterAccessControlEntry>> callback,
                                                                                String userId) {

        final Future<List<MasterAccessControlEntry>> future = new Future<List<MasterAccessControlEntry>>();

        getGlobalDomainAccessControllerProxy(TTL_30_DAYS_IN_MS).getMasterAccessControlEntries(new Callback<MasterAccessControlEntry[]>() {
                                                                                                  @Override
                                                                                                  public void onFailure(JoynrRuntimeException error) {
                                                                                                      callback.onFailure(error);
                                                                                                      future.onFailure(error);
                                                                                                  }

                                                                                                  @Override
                                                                                                  public void onSuccess(MasterAccessControlEntry[] result) {
                                                                                                      List<MasterAccessControlEntry> masterAccessContolEntry;
                                                                                                      if (result == null) {
                                                                                                          masterAccessContolEntry = new ArrayList<MasterAccessControlEntry>();
                                                                                                      } else {
                                                                                                          masterAccessContolEntry = Arrays.asList(result);
                                                                                                      }
                                                                                                      callback.onSuccess(masterAccessContolEntry);
                                                                                                      future.onSuccess(masterAccessContolEntry);
                                                                                                  }
                                                                                              },
                                                                                              userId);
        return future;
    }

    public boolean updateMasterAccessControlEntry(MasterAccessControlEntry updatedMasterAce) {
        return getGlobalDomainAccessControlListEditorProxy(TTL_30_DAYS_IN_MS).updateMasterAccessControlEntry(updatedMasterAce);
    }

    public Future<Boolean> updateMasterAccessControlEntry(Callback<Boolean> callback,
                                                          MasterAccessControlEntry updatedMasterAce) {
        return getGlobalDomainAccessControlListEditorProxy(TTL_30_DAYS_IN_MS).updateMasterAccessControlEntry(callback,
                                                                                                             updatedMasterAce);
    }

    public boolean removeMasterAccessControlEntry(String userId, String domain, String interfaceName, String operation) {
        return getGlobalDomainAccessControlListEditorProxy(TTL_30_DAYS_IN_MS).removeMasterAccessControlEntry(userId,
                                                                                                             domain,
                                                                                                             interfaceName,
                                                                                                             operation);
    }

    public List<MasterAccessControlEntry> getMediatorAccessControlEntries(String userId) {
        return Arrays.asList(getGlobalDomainAccessControllerProxy(TTL_30_DAYS_IN_MS).getMediatorAccessControlEntries(userId));
    }

    public Future<Boolean> removeMasterAccessControlEntry(Callback<Boolean> callback,
                                                          String userId,
                                                          String domain,
                                                          String interfaceName,
                                                          String operation) {
        return getGlobalDomainAccessControlListEditorProxy(TTL_30_DAYS_IN_MS).removeMasterAccessControlEntry(callback,
                                                                                                             userId,
                                                                                                             domain,
                                                                                                             interfaceName,
                                                                                                             operation);
    }

    public boolean updateMediatorAccessControlEntry(MasterAccessControlEntry updatedMediatorAce) {
        return getGlobalDomainAccessControlListEditorProxy(TTL_30_DAYS_IN_MS).updateMediatorAccessControlEntry(updatedMediatorAce);
    }

    public Future<Boolean> updateMediatorAccessControlEntry(Callback<Boolean> callback,
                                                            MasterAccessControlEntry updatedMediatorAce) {
        return getGlobalDomainAccessControlListEditorProxy(TTL_30_DAYS_IN_MS).updateMediatorAccessControlEntry(callback,
                                                                                                               updatedMediatorAce);
    }

    public boolean removeMediatorAccessControlEntry(String userId, String domain, String interfaceName, String operation) {
        return getGlobalDomainAccessControlListEditorProxy(TTL_30_DAYS_IN_MS).removeMediatorAccessControlEntry(userId,
                                                                                                               domain,
                                                                                                               interfaceName,
                                                                                                               operation);
    }

    public Future<Boolean> removeMediatorAccessControlEntry(Callback<Boolean> callback,
                                                            String userId,
                                                            String domain,
                                                            String interfaceName,
                                                            String operation) {
        return getGlobalDomainAccessControlListEditorProxy(TTL_30_DAYS_IN_MS).removeMediatorAccessControlEntry(callback,
                                                                                                               userId,
                                                                                                               domain,
                                                                                                               interfaceName,
                                                                                                               operation);
    }

    public List<OwnerAccessControlEntry> getOwnerAccessControlEntries(String userId) {
        return Arrays.asList(getGlobalDomainAccessControllerProxy(TTL_30_DAYS_IN_MS).getOwnerAccessControlEntries(userId));
    }

    public boolean updateOwnerAccessControlEntry(OwnerAccessControlEntry updatedOwnerAce) {
        return getGlobalDomainAccessControlListEditorProxy(TTL_30_DAYS_IN_MS).updateOwnerAccessControlEntry(updatedOwnerAce);
    }

    public Future<Boolean> updateOwnerAccessControlEntry(Callback<Boolean> callback,
                                                         OwnerAccessControlEntry updatedOwnerAce) {
        return getGlobalDomainAccessControlListEditorProxy(TTL_30_DAYS_IN_MS).updateOwnerAccessControlEntry(callback,
                                                                                                            updatedOwnerAce);
    }

    public boolean removeOwnerAccessControlEntry(String userId, String domain, String interfaceName, String operation) {
        return getGlobalDomainAccessControlListEditorProxy(TTL_30_DAYS_IN_MS).removeOwnerAccessControlEntry(userId,
                                                                                                            domain,
                                                                                                            interfaceName,
                                                                                                            operation);
    }

    public Future<Boolean> removeOwnerAccessControlEntry(Callback<Boolean> callback,
                                                         String userId,
                                                         String domain,
                                                         String interfaceName,
                                                         String operation) {
        return getGlobalDomainAccessControlListEditorProxy(TTL_30_DAYS_IN_MS).removeOwnerAccessControlEntry(callback,
                                                                                                            userId,
                                                                                                            domain,
                                                                                                            interfaceName,
                                                                                                            operation);
    }

    public List<MasterRegistrationControlEntry> getMasterRegistrationControlEntries(String userId) {
        return Arrays.asList(getGlobalDomainAccessControllerProxy(TTL_30_DAYS_IN_MS).getMasterRegistrationControlEntries(userId));
    }

    public Future<List<MasterRegistrationControlEntry>> getMasterRegistrationControlEntries(final Callback<List<MasterRegistrationControlEntry>> callback,
                                                                                            String userId) {
        final Future<List<MasterRegistrationControlEntry>> future = new Future<List<MasterRegistrationControlEntry>>();

        getGlobalDomainAccessControllerProxy(TTL_30_DAYS_IN_MS).getMasterRegistrationControlEntries(new Callback<MasterRegistrationControlEntry[]>() {
                                                                                                        @Override
                                                                                                        public void onFailure(JoynrRuntimeException error) {
                                                                                                            callback.onFailure(error);
                                                                                                            future.onFailure(error);
                                                                                                        }

                                                                                                        @Override
                                                                                                        public void onSuccess(MasterRegistrationControlEntry[] result) {
                                                                                                            List<MasterRegistrationControlEntry> masterRegistrationControlEntryList;
                                                                                                            if (result == null) {
                                                                                                                masterRegistrationControlEntryList = new ArrayList<MasterRegistrationControlEntry>();
                                                                                                            } else {
                                                                                                                masterRegistrationControlEntryList = Arrays.asList(result);
                                                                                                            }
                                                                                                            callback.onSuccess(masterRegistrationControlEntryList);
                                                                                                            future.onSuccess(masterRegistrationControlEntryList);
                                                                                                        }
                                                                                                    },
                                                                                                    userId);
        return future;

    }

    public List<MasterRegistrationControlEntry> getEditableMasterRegistrationControlEntries(String userId) {
        return Arrays.asList(getGlobalDomainAccessControlListEditorProxy(TTL_30_DAYS_IN_MS).getEditableMasterRegistrationControlEntries(userId));
    }

    public Future<List<MasterRegistrationControlEntry>> getEditableMasterRegistrationControlEntries(final Callback<List<MasterRegistrationControlEntry>> callback,
                                                                                                    String userId) {
        final Future<List<MasterRegistrationControlEntry>> future = new Future<List<MasterRegistrationControlEntry>>();

        getGlobalDomainAccessControlListEditorProxy(TTL_30_DAYS_IN_MS).getEditableMasterRegistrationControlEntries(new Callback<MasterRegistrationControlEntry[]>() {
                                                                                                                       @Override
                                                                                                                       public void onFailure(JoynrRuntimeException error) {
                                                                                                                           callback.onFailure(error);
                                                                                                                           future.onFailure(error);
                                                                                                                       }

                                                                                                                       @Override
                                                                                                                       public void onSuccess(MasterRegistrationControlEntry[] result) {
                                                                                                                           List<MasterRegistrationControlEntry> masterRegistrationControlEntryList;
                                                                                                                           if (result == null) {
                                                                                                                               masterRegistrationControlEntryList = new ArrayList<MasterRegistrationControlEntry>();
                                                                                                                           } else {
                                                                                                                               masterRegistrationControlEntryList = Arrays.asList(result);
                                                                                                                           }
                                                                                                                           callback.onSuccess(masterRegistrationControlEntryList);
                                                                                                                           future.onSuccess(masterRegistrationControlEntryList);
                                                                                                                       }
                                                                                                                   },
                                                                                                                   userId);
        return future;

    }

    public List<MasterRegistrationControlEntry> getMediatorRegistrationControlEntries(String userId) {
        return Arrays.asList(getGlobalDomainAccessControllerProxy(TTL_30_DAYS_IN_MS).getMediatorRegistrationControlEntries(userId));
    }

    public Future<List<MasterRegistrationControlEntry>> getMediatorRegistrationControlEntries(final Callback<List<MasterRegistrationControlEntry>> callback,
                                                                                              String userId) {
        final Future<List<MasterRegistrationControlEntry>> future = new Future<List<MasterRegistrationControlEntry>>();

        getGlobalDomainAccessControllerProxy(TTL_30_DAYS_IN_MS).getMediatorRegistrationControlEntries(new Callback<MasterRegistrationControlEntry[]>() {
                                                                                                          @Override
                                                                                                          public void onFailure(JoynrRuntimeException error) {
                                                                                                              callback.onFailure(error);
                                                                                                              future.onFailure(error);
                                                                                                          }

                                                                                                          @Override
                                                                                                          public void onSuccess(MasterRegistrationControlEntry[] result) {
                                                                                                              List<MasterRegistrationControlEntry> masterRegistrationControlEntryList;
                                                                                                              if (result == null) {
                                                                                                                  masterRegistrationControlEntryList = new ArrayList<MasterRegistrationControlEntry>();
                                                                                                              } else {
                                                                                                                  masterRegistrationControlEntryList = Arrays.asList(result);
                                                                                                              }
                                                                                                              callback.onSuccess(masterRegistrationControlEntryList);
                                                                                                              future.onSuccess(masterRegistrationControlEntryList);
                                                                                                          }
                                                                                                      },
                                                                                                      userId);
        return future;
    }

    public List<MasterRegistrationControlEntry> getEditableMediatorRegistrationControlEntries(String userId) {
        return Arrays.asList(getGlobalDomainAccessControlListEditorProxy(TTL_30_DAYS_IN_MS).getEditableMediatorRegistrationControlEntries(userId));
    }

    public List<OwnerRegistrationControlEntry> getOwnerRegistrationControlEntries(String userId) {
        return Arrays.asList(getGlobalDomainAccessControllerProxy(TTL_30_DAYS_IN_MS).getOwnerRegistrationControlEntries(userId));
    }

    public Future<List<OwnerRegistrationControlEntry>> getOwnerRegistrationControlEntries(final Callback<List<OwnerRegistrationControlEntry>> callback,
                                                                                          String userId) {
        final Future<List<OwnerRegistrationControlEntry>> future = new Future<List<OwnerRegistrationControlEntry>>();

        getGlobalDomainAccessControllerProxy(TTL_30_DAYS_IN_MS).getOwnerRegistrationControlEntries(new Callback<OwnerRegistrationControlEntry[]>() {
                                                                                                       @Override
                                                                                                       public void onFailure(JoynrRuntimeException error) {
                                                                                                           callback.onFailure(error);
                                                                                                           future.onFailure(error);
                                                                                                       }

                                                                                                       @Override
                                                                                                       public void onSuccess(OwnerRegistrationControlEntry[] result) {
                                                                                                           List<OwnerRegistrationControlEntry> ownerRegistrationControlEntryList;
                                                                                                           if (result == null) {
                                                                                                               ownerRegistrationControlEntryList = new ArrayList<OwnerRegistrationControlEntry>();
                                                                                                           } else {
                                                                                                               ownerRegistrationControlEntryList = Arrays.asList(result);
                                                                                                           }
                                                                                                           callback.onSuccess(ownerRegistrationControlEntryList);
                                                                                                           future.onSuccess(ownerRegistrationControlEntryList);
                                                                                                       }
                                                                                                   },
                                                                                                   userId);
        return future;

    }

    public List<OwnerRegistrationControlEntry> getEditableOwnerRegistrationControlEntries(String userId) {
        return Arrays.asList(getGlobalDomainAccessControlListEditorProxy(TTL_30_DAYS_IN_MS).getEditableOwnerRegistrationControlEntries(userId));
    }

    public Future<List<OwnerRegistrationControlEntry>> getEditableOwnerRegistrationControlEntries(final Callback<List<OwnerRegistrationControlEntry>> callback,
                                                                                                  String userId) {
        final Future<List<OwnerRegistrationControlEntry>> future = new Future<List<OwnerRegistrationControlEntry>>();

        getGlobalDomainAccessControlListEditorProxy(TTL_30_DAYS_IN_MS).getEditableOwnerRegistrationControlEntries(new Callback<OwnerRegistrationControlEntry[]>() {
                                                                                                                      @Override
                                                                                                                      public void onFailure(JoynrRuntimeException error) {
                                                                                                                          callback.onFailure(error);
                                                                                                                          future.onFailure(error);
                                                                                                                      }

                                                                                                                      @Override
                                                                                                                      public void onSuccess(OwnerRegistrationControlEntry[] result) {
                                                                                                                          List<OwnerRegistrationControlEntry> ownerRegistrationControlEntryList;
                                                                                                                          if (result == null) {
                                                                                                                              ownerRegistrationControlEntryList = new ArrayList<OwnerRegistrationControlEntry>();
                                                                                                                          } else {
                                                                                                                              ownerRegistrationControlEntryList = Arrays.asList(result);
                                                                                                                          }
                                                                                                                          callback.onSuccess(ownerRegistrationControlEntryList);
                                                                                                                          future.onSuccess(ownerRegistrationControlEntryList);
                                                                                                                      }
                                                                                                                  },
                                                                                                                  userId);
        return future;
    }

    public void subscribeToDomainRoleEntryChangedBroadcast(LdacDomainRoleEntryChangedBroadcastListener ldacDomainRoleEntryChangedBroadcastListener,
                                                           MulticastSubscriptionQos broadcastSubscriptionQos,
                                                           String... partitions) {
        getGlobalDomainRoleControllerProxy(TTL_30_DAYS_IN_MS).subscribeToDomainRoleEntryChangedBroadcast(ldacDomainRoleEntryChangedBroadcastListener,
                                                                                                         broadcastSubscriptionQos,
                                                                                                         partitions);

    }

    public Future<String> subscribeToMasterAccessControlEntryChangedBroadcast(LdacMasterAccessControlEntryChangedBroadcastListener ldacMasterAccessControlEntryChangedBroadcastListener,
                                                                              MulticastSubscriptionQos broadcastSubscriptionQos,
                                                                              String... partitions) {
        return getGlobalDomainAccessControllerProxy(TTL_30_DAYS_IN_MS).subscribeToMasterAccessControlEntryChangedBroadcast(ldacMasterAccessControlEntryChangedBroadcastListener,
                                                                                                                           broadcastSubscriptionQos,
                                                                                                                           partitions);
    }

    public Future<String> subscribeToMediatorAccessControlEntryChangedBroadcast(LdacMediatorAccessControlEntryChangedBroadcastListener ldacMediatorAccessControlEntryChangedBroadcastListener,
                                                                                MulticastSubscriptionQos broadcastSubscriptionQos,
                                                                                String... partitions) {
        return getGlobalDomainAccessControllerProxy(TTL_30_DAYS_IN_MS).subscribeToMediatorAccessControlEntryChangedBroadcast(ldacMediatorAccessControlEntryChangedBroadcastListener,
                                                                                                                             broadcastSubscriptionQos,
                                                                                                                             partitions);
    }

    public Future<String> subscribeToOwnerAccessControlEntryChangedBroadcast(LdacOwnerAccessControlEntryChangedBroadcastListener ldacOwnerAccessControlEntryChangedBroadcastListener,
                                                                             MulticastSubscriptionQos broadcastSubscriptionQos,
                                                                             String... partitions) {
        return getGlobalDomainAccessControllerProxy(TTL_30_DAYS_IN_MS).subscribeToOwnerAccessControlEntryChangedBroadcast(ldacOwnerAccessControlEntryChangedBroadcastListener,
                                                                                                                          broadcastSubscriptionQos,
                                                                                                                          partitions);

    }

    public List<DomainRoleEntry> getDomainRoles(String userId) {
        return Arrays.asList(getGlobalDomainRoleControllerProxy(TTL_30_DAYS_IN_MS).getDomainRoles(userId));
    }

    public void getDomainRoles(Callback<DomainRoleEntry[]> callback, String userId) {
        getGlobalDomainRoleControllerProxy(TTL_30_DAYS_IN_MS).getDomainRoles(callback, userId);
    }

    public List<MasterAccessControlEntry> getMasterAccessControlEntries(String domain, String interfaceName) {
        return Arrays.asList(getGlobalDomainAccessControllerProxy(TTL_30_DAYS_IN_MS).getMasterAccessControlEntries(domain,
                                                                                                                   interfaceName));
    }

    public void getMasterAccessControlEntries(Callback<MasterAccessControlEntry[]> callback,
                                              String domain,
                                              String interfaceName) {
        getGlobalDomainAccessControllerProxy(TTL_30_DAYS_IN_MS).getMasterAccessControlEntries(callback,
                                                                                              domain,
                                                                                              interfaceName);
    }

    public List<MasterAccessControlEntry> getMediatorAccessControlEntries(String domain, String interfaceName) {
        return Arrays.asList(getGlobalDomainAccessControllerProxy(TTL_30_DAYS_IN_MS).getMediatorAccessControlEntries(domain,
                                                                                                                     interfaceName));
    }

    public void getMediatorAccessControlEntries(Callback<MasterAccessControlEntry[]> callback,
                                                String domain,
                                                String interfaceName) {
        getGlobalDomainAccessControllerProxy(TTL_30_DAYS_IN_MS).getMediatorAccessControlEntries(callback,
                                                                                                domain,
                                                                                                interfaceName);
    }

    public List<OwnerAccessControlEntry> getOwnerAccessControlEntries(String domain, String interfaceName) {
        return Arrays.asList(getGlobalDomainAccessControllerProxy(TTL_30_DAYS_IN_MS).getOwnerAccessControlEntries(domain,
                                                                                                                  interfaceName));
    }

    public void getOwnerAccessControlEntries(Callback<OwnerAccessControlEntry[]> callback,
                                             String domain,
                                             String interfaceName) {
        getGlobalDomainAccessControllerProxy(TTL_30_DAYS_IN_MS).getOwnerAccessControlEntries(callback,
                                                                                             domain,
                                                                                             interfaceName);
    }

    public void unsubscribeFromMasterAccessControlEntryChangedBroadcast(String subscriptionId) {
        getGlobalDomainAccessControllerProxy(TTL_30_DAYS_IN_MS).unsubscribeFromMasterAccessControlEntryChangedBroadcast(subscriptionId);
    }

    public void unsubscribeFromMediatorAccessControlEntryChangedBroadcast(String subscriptionId) {
        getGlobalDomainAccessControllerProxy(TTL_30_DAYS_IN_MS).unsubscribeFromMediatorAccessControlEntryChangedBroadcast(subscriptionId);
    }

    public void unsubscribeFromOwnerAccessControlEntryChangedBroadcast(String subscriptionId) {
        getGlobalDomainAccessControllerProxy(TTL_30_DAYS_IN_MS).unsubscribeFromOwnerAccessControlEntryChangedBroadcast(subscriptionId);
    }
}
