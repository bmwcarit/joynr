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
package io.joynr.capabilities;

import static io.joynr.runtime.SystemServicesSettings.PROPERTY_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

import javax.annotation.CheckForNull;
import javax.annotation.Nonnull;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.exceptions.DiscoveryException;
import io.joynr.exceptions.JoynrException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.routing.TransportReadyListener;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;
import io.joynr.provider.PromiseListener;
import io.joynr.proxy.Callback;
import io.joynr.proxy.CallbackWithModeledError;
import io.joynr.proxy.Future;
import io.joynr.runtime.GlobalAddressProvider;
import io.joynr.runtime.ShutdownNotifier;
import joynr.exceptions.ApplicationException;
import joynr.exceptions.ProviderRuntimeException;
import joynr.infrastructure.GlobalCapabilitiesDirectory;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.types.DiscoveryEntry;
import joynr.types.DiscoveryEntryWithMetaInfo;
import joynr.types.DiscoveryError;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderScope;

@Singleton
public class LocalCapabilitiesDirectoryImpl extends AbstractLocalCapabilitiesDirectory
        implements TransportReadyListener {

    private static final Logger logger = LoggerFactory.getLogger(LocalCapabilitiesDirectoryImpl.class);

    private static final Set<DiscoveryScope> INCLUDE_LOCAL_SCOPES = new HashSet<>();
    static {
        INCLUDE_LOCAL_SCOPES.add(DiscoveryScope.LOCAL_ONLY);
        INCLUDE_LOCAL_SCOPES.add(DiscoveryScope.LOCAL_AND_GLOBAL);
        INCLUDE_LOCAL_SCOPES.add(DiscoveryScope.LOCAL_THEN_GLOBAL);
    }

    private static final Set<DiscoveryScope> INCLUDE_GLOBAL_SCOPES = new HashSet<>();
    static {
        INCLUDE_GLOBAL_SCOPES.add(DiscoveryScope.GLOBAL_ONLY);
        INCLUDE_GLOBAL_SCOPES.add(DiscoveryScope.LOCAL_AND_GLOBAL);
        INCLUDE_GLOBAL_SCOPES.add(DiscoveryScope.LOCAL_THEN_GLOBAL);
    }

    private ScheduledExecutorService freshnessUpdateScheduler;

    private DiscoveryEntryStore<DiscoveryEntry> localDiscoveryEntryStore;
    private GlobalCapabilitiesDirectoryClient globalCapabilitiesDirectoryClient;
    private DiscoveryEntryStore<GlobalDiscoveryEntry> globalDiscoveryEntryCache;
    private final Map<String, Set<String>> globalProviderParticipantIdToGbidSetMap;
    private final long defaultDiscoveryRetryInterval;

    private MessageRouter messageRouter;

    private GlobalAddressProvider globalAddressProvider;

    private Address globalAddress;
    private Object globalAddressLock = new Object();

    private List<QueuedDiscoveryEntry> queuedDiscoveryEntries = new ArrayList<QueuedDiscoveryEntry>();

    private final String[] knownGbids;

    static class QueuedDiscoveryEntry {
        private DiscoveryEntry discoveryEntry;
        private String[] gbids;
        private Add1Deferred deferred;
        private boolean awaitGlobalRegistration;

        public QueuedDiscoveryEntry(DiscoveryEntry discoveryEntry,
                                    String[] gbids,
                                    Add1Deferred deferred,
                                    boolean awaitGlobalRegistration) {
            this.discoveryEntry = discoveryEntry;
            this.gbids = gbids;
            this.deferred = deferred;
            this.awaitGlobalRegistration = awaitGlobalRegistration;
        }

        public DiscoveryEntry getDiscoveryEntry() {
            return discoveryEntry;
        }

        public String[] getGbids() {
            return gbids;
        }

        public Add1Deferred getDeferred() {
            return deferred;
        }

        public boolean getAwaitGlobalRegistration() {
            return awaitGlobalRegistration;
        }
    }

    @Inject
    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public LocalCapabilitiesDirectoryImpl(CapabilitiesProvisioning capabilitiesProvisioning,
                                          GlobalAddressProvider globalAddressProvider,
                                          DiscoveryEntryStore<DiscoveryEntry> localDiscoveryEntryStore,
                                          DiscoveryEntryStore<GlobalDiscoveryEntry> globalDiscoveryEntryCache,
                                          MessageRouter messageRouter,
                                          GlobalCapabilitiesDirectoryClient globalCapabilitiesDirectoryClient,
                                          ExpiredDiscoveryEntryCacheCleaner expiredDiscoveryEntryCacheCleaner,
                                          @Named(PROPERTY_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS) long freshnessUpdateIntervalMs,
                                          @Named(JOYNR_SCHEDULER_CAPABILITIES_FRESHNESS) ScheduledExecutorService freshnessUpdateScheduler,
                                          @Named(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_DEFAULT_RETRY_INTERVAL_MS) long defaultDiscoveryRetryInterval,
                                          ShutdownNotifier shutdownNotifier,
                                          @Named(MessagingPropertyKeys.GBID_ARRAY) String[] knownGbids) {
        globalProviderParticipantIdToGbidSetMap = new HashMap<>();
        this.globalAddressProvider = globalAddressProvider;
        // CHECKSTYLE:ON
        this.defaultDiscoveryRetryInterval = defaultDiscoveryRetryInterval;
        this.messageRouter = messageRouter;
        this.localDiscoveryEntryStore = localDiscoveryEntryStore;
        this.globalDiscoveryEntryCache = globalDiscoveryEntryCache;
        this.globalCapabilitiesDirectoryClient = globalCapabilitiesDirectoryClient;
        this.knownGbids = knownGbids.clone();
        Collection<GlobalDiscoveryEntry> provisionedDiscoveryEntries = capabilitiesProvisioning.getDiscoveryEntries();
        this.globalDiscoveryEntryCache.add(provisionedDiscoveryEntries);
        for (GlobalDiscoveryEntry provisionedEntry : provisionedDiscoveryEntries) {
            String participantId = provisionedEntry.getParticipantId();
            if (GlobalCapabilitiesDirectory.INTERFACE_NAME.equals(provisionedEntry.getInterfaceName())) {
                mapGbidsToGlobalProviderParticipantId(participantId, knownGbids);
            } else {
                Address address = CapabilityUtils.getAddressFromGlobalDiscoveryEntry(provisionedEntry);
                mapGbidsToGlobalProviderParticipantId(participantId, getGbids(address));
            }
        }
        expiredDiscoveryEntryCacheCleaner.scheduleCleanUpForCaches(new ExpiredDiscoveryEntryCacheCleaner.CleanupAction() {
            @Override
            public void cleanup(Set<DiscoveryEntry> expiredDiscoveryEntries) {
                for (DiscoveryEntry discoveryEntry : expiredDiscoveryEntries) {
                    remove(discoveryEntry);
                }
            }
        }, globalDiscoveryEntryCache, localDiscoveryEntryStore);
        this.freshnessUpdateScheduler = freshnessUpdateScheduler;
        setUpPeriodicFreshnessUpdate(freshnessUpdateIntervalMs);
        shutdownNotifier.registerForShutdown(this);
    }

    private String[] getGbids(Address address) {
        String[] gbids;
        if (address instanceof MqttAddress) {
            // For backwards compatibility, the GBID is stored in the brokerUri field of MqttAddress which was not evaluated in older joynr versions
            gbids = new String[]{ ((MqttAddress) address).getBrokerUri() };
        } else {
            // use default GBID for all other address types
            gbids = new String[]{ knownGbids[0] };
        }
        return gbids;
    }

    private void mapGbidsToGlobalProviderParticipantId(String participantId, String[] gbids) {
        if (globalProviderParticipantIdToGbidSetMap.containsKey(participantId)) {
            for (String gbid : gbids) {
                if (!globalProviderParticipantIdToGbidSetMap.get(participantId).contains(gbid)) {
                    globalProviderParticipantIdToGbidSetMap.get(participantId).add(gbid);
                }
            }
        } else {
            Set<String> gbidSetForParticipantId = new HashSet<String>();
            for (String gbid : gbids) {
                gbidSetForParticipantId.add(gbid);
            }
            globalProviderParticipantIdToGbidSetMap.put(participantId, gbidSetForParticipantId);
        }
    }

    private void setUpPeriodicFreshnessUpdate(final long freshnessUpdateIntervalMs) {
        logger.trace("Setting up periodic freshness update with interval {}", freshnessUpdateIntervalMs);
        Runnable command = new Runnable() {
            @Override
            public void run() {
                try {
                    logger.debug("Updating last seen date ms.");
                    globalCapabilitiesDirectoryClient.touch();
                } catch (JoynrRuntimeException e) {
                    logger.error("error sending freshness update", e);
                }
            }
        };
        freshnessUpdateScheduler.scheduleAtFixedRate(command,
                                                     freshnessUpdateIntervalMs,
                                                     freshnessUpdateIntervalMs,
                                                     TimeUnit.MILLISECONDS);
    }

    /**
     * Adds local capability to local and (depending on SCOPE) the global directory
     */
    @Override
    public Promise<DeferredVoid> add(final DiscoveryEntry discoveryEntry) {
        boolean awaitGlobalRegistration = false;
        return add(discoveryEntry, awaitGlobalRegistration);
    }

    @Override
    public Promise<DeferredVoid> add(final DiscoveryEntry discoveryEntry, final Boolean awaitGlobalRegistration) {
        Promise<Add1Deferred> addPromise = add(discoveryEntry, awaitGlobalRegistration, new String[]{ knownGbids[0] });
        DeferredVoid deferredVoid = new DeferredVoid();
        addPromise.then(new PromiseListener() {
            @Override
            public void onRejection(JoynrException exception) {
                if (exception instanceof ApplicationException) {
                    DiscoveryError error = ((ApplicationException) exception).getError();
                    deferredVoid.reject(new ProviderRuntimeException("Error registering provider "
                            + discoveryEntry.getParticipantId() + " in default backend: " + error));
                } else if (exception instanceof ProviderRuntimeException) {
                    deferredVoid.reject((ProviderRuntimeException) exception);
                } else {
                    deferredVoid.reject(new ProviderRuntimeException("Unknown error registering provider "
                            + discoveryEntry.getParticipantId() + " in default backend: " + exception));
                }
            }

            @Override
            public void onFulfillment(Object... values) {
                deferredVoid.resolve();
            }
        });
        return new Promise<>(deferredVoid);
    }

    private DiscoveryError validateGbids(final String[] gbids) {
        if (gbids == null) {
            return DiscoveryError.INVALID_GBID;
        }

        HashSet<String> gbidSet = new HashSet<String>();
        for (String gbid : gbids) {
            if (gbid == null || gbid.isEmpty() || gbidSet.contains(gbid)) {
                return DiscoveryError.INVALID_GBID;
            }
            gbidSet.add(gbid);

            boolean found = false;
            for (String validGbid : knownGbids) {
                if (gbid.equals(validGbid)) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                return DiscoveryError.UNKNOWN_GBID;
            }
        }
        return null;
    }

    @Override
    public Promise<Add1Deferred> add(DiscoveryEntry discoveryEntry, Boolean awaitGlobalRegistration, String[] gbids) {
        final Add1Deferred deferred = new Add1Deferred();

        DiscoveryError validationResult = validateGbids(gbids);
        if (validationResult != null) {
            deferred.reject(validationResult);
            return new Promise<>(deferred);
        }
        if (gbids.length == 0) {
            // register provider in default backend
            gbids = new String[]{ knownGbids[0] };
        }

        if (localDiscoveryEntryStore.hasDiscoveryEntry(discoveryEntry)) {
            if (discoveryEntry.getQos().getScope().equals(ProviderScope.LOCAL)) {
                // in this case, no further need for global registration is required. Registration completed.
                // TODO provider might still be registered globally if it was registered globally before
                deferred.resolve();
                return new Promise<>(deferred);
            }
            synchronized (globalDiscoveryEntryCache) {
                if (globalDiscoveryEntryCache.lookup(discoveryEntry.getParticipantId(),
                                                     DiscoveryQos.NO_MAX_AGE) != null) {
                    mapGbidsToGlobalProviderParticipantId(discoveryEntry.getParticipantId(), gbids);
                    deferred.resolve();
                    return new Promise<>(deferred);
                }
            }
            // in the other case, the global registration needs to be done
        } else {
            localDiscoveryEntryStore.add(discoveryEntry);
            notifyCapabilityAdded(discoveryEntry);
        }

        /*
         * In case awaitGlobalRegistration is true, a result for this 'add' call will not be returned before the call to
         * the globalDiscovery has either succeeded, failed or timed out. In case of failure or timeout the already
         * created discoveryEntry will also be removed again from localDiscoveryStore.
         *
         * If awaitGlobalRegistration is false, the call to the globalDiscovery will just be triggered, but it is not
         * being waited for results or timeout. Also, in case it does not succeed, the entry remains in
         * localDiscoveryStore.
         */
        if (discoveryEntry.getQos().getScope().equals(ProviderScope.GLOBAL)) {
            Add1Deferred deferredForRegisterGlobal;
            if (awaitGlobalRegistration == true) {
                deferredForRegisterGlobal = deferred;
            } else {
                // use an independent DeferredVoid not used for waiting
                deferredForRegisterGlobal = new Add1Deferred();
                deferred.resolve();
            }
            registerGlobal(discoveryEntry, gbids, deferredForRegisterGlobal, awaitGlobalRegistration);
        } else {
            deferred.resolve();
        }
        return new Promise<>(deferred);
    }

    @Override
    public Promise<AddToAllDeferred> addToAll(DiscoveryEntry discoveryEntry, Boolean awaitGlobalRegistration) {
        Promise<Add1Deferred> addPromise = add(discoveryEntry, awaitGlobalRegistration, knownGbids);
        AddToAllDeferred addToAllDeferred = new AddToAllDeferred();
        addPromise.then(new PromiseListener() {
            @Override
            public void onRejection(JoynrException error) {
                if (error instanceof ApplicationException) {
                    addToAllDeferred.reject(((ApplicationException) error).getError());
                } else if (error instanceof ProviderRuntimeException) {
                    addToAllDeferred.reject((ProviderRuntimeException) error);
                } else {
                    addToAllDeferred.reject(new ProviderRuntimeException("Unknown error registering provider "
                            + discoveryEntry.getParticipantId() + " in all known backends: " + error));
                }
            }

            @Override
            public void onFulfillment(Object... values) {
                addToAllDeferred.resolve();
            }
        });
        return new Promise<>(addToAllDeferred);
    }

    private void registerGlobal(final DiscoveryEntry discoveryEntry,
                                final String[] gbids,
                                final Add1Deferred deferred,
                                final boolean awaitGlobalRegistration) {
        synchronized (globalAddressLock) {
            try {
                globalAddress = globalAddressProvider.get();
            } catch (Exception e) {
                logger.debug("error getting global address", e);
                globalAddress = null;
            }

            if (globalAddress == null) {
                Add1Deferred deferredForQueueDiscoveryEntry;
                if (awaitGlobalRegistration == true) {
                    deferredForQueueDiscoveryEntry = deferred;
                } else {
                    // use an independent DeferredVoid we do not wait for
                    deferredForQueueDiscoveryEntry = new Add1Deferred();
                }
                queuedDiscoveryEntries.add(new QueuedDiscoveryEntry(discoveryEntry,
                                                                    gbids,
                                                                    deferredForQueueDiscoveryEntry,
                                                                    awaitGlobalRegistration));
                globalAddressProvider.registerGlobalAddressesReadyListener(this);
                return;
            }
        }

        final GlobalDiscoveryEntry globalDiscoveryEntry = CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry(discoveryEntry,
                                                                                                              globalAddress);
        if (globalDiscoveryEntry != null) {

            logger.info("starting global registration for " + globalDiscoveryEntry.getDomain() + " : "
                    + globalDiscoveryEntry.getInterfaceName());

            globalCapabilitiesDirectoryClient.add(new CallbackWithModeledError<Void, DiscoveryError>() {

                @Override
                public void onSuccess(Void nothing) {
                    logger.info("global registration for " + globalDiscoveryEntry.getParticipantId() + ", "
                            + globalDiscoveryEntry.getDomain() + " : " + globalDiscoveryEntry.getInterfaceName()
                            + " completed");
                    synchronized (globalDiscoveryEntryCache) {
                        mapGbidsToGlobalProviderParticipantId(discoveryEntry.getParticipantId(), gbids);
                        globalDiscoveryEntryCache.add(globalDiscoveryEntry);
                    }
                    deferred.resolve();
                }

                @Override
                public void onFailure(JoynrRuntimeException exception) {
                    logger.info("global registration for " + globalDiscoveryEntry.getParticipantId() + ", "
                            + globalDiscoveryEntry.getDomain() + " : " + globalDiscoveryEntry.getInterfaceName()
                            + " failed");
                    if (awaitGlobalRegistration == true) {
                        localDiscoveryEntryStore.remove(globalDiscoveryEntry.getParticipantId());
                    }
                    deferred.reject(new ProviderRuntimeException(exception.toString()));
                }

                @Override
                public void onFailure(DiscoveryError errorEnum) {
                    logger.info("global registration for " + globalDiscoveryEntry.getParticipantId() + ", "
                            + globalDiscoveryEntry.getDomain() + " : " + globalDiscoveryEntry.getInterfaceName()
                            + " failed");
                    if (awaitGlobalRegistration == true) {
                        localDiscoveryEntryStore.remove(globalDiscoveryEntry.getParticipantId());
                    }
                    deferred.reject(errorEnum);
                }
            }, globalDiscoveryEntry, gbids);
        }
    }

    @Override
    public io.joynr.provider.Promise<io.joynr.provider.DeferredVoid> remove(String participantId) {
        DeferredVoid deferred = new DeferredVoid();
        DiscoveryEntry entryToRemove = localDiscoveryEntryStore.lookup(participantId, Long.MAX_VALUE);
        if (entryToRemove != null) {
            remove(entryToRemove);
            deferred.resolve();
        } else {
            deferred.reject(new ProviderRuntimeException("Failed to remove participantId: " + participantId));
        }
        return new Promise<>(deferred);
    }

    @Override
    public void remove(final DiscoveryEntry discoveryEntry) {
        localDiscoveryEntryStore.remove(discoveryEntry.getParticipantId());
        notifyCapabilityRemoved(discoveryEntry);
        // Remove from the global capabilities directory if needed
        if (discoveryEntry.getQos().getScope() != ProviderScope.LOCAL) {

            CallbackWithModeledError<Void, DiscoveryError> callback = new CallbackWithModeledError<Void, DiscoveryError>() {

                @Override
                public void onSuccess(Void result) {
                    synchronized (globalDiscoveryEntryCache) {
                        globalDiscoveryEntryCache.remove(discoveryEntry.getParticipantId());
                        globalProviderParticipantIdToGbidSetMap.remove(discoveryEntry.getParticipantId());
                    }
                }

                @Override
                public void onFailure(JoynrRuntimeException error) {
                    // do nothing
                }

                @Override
                public void onFailure(DiscoveryError errorEnum) {

                }
            };
            String participantId = discoveryEntry.getParticipantId();
            if (globalProviderParticipantIdToGbidSetMap.containsKey(participantId)) {
                String[] gbidsToRemove = globalProviderParticipantIdToGbidSetMap.get(participantId)
                                                                                .toArray(new String[0]);
                globalCapabilitiesDirectoryClient.remove(callback, participantId, gbidsToRemove);
            } else {
                logger.warn("Participant " + participantId + " is not registered and cannot be removed!");
            }
        }

        // Remove endpoint addresses
        messageRouter.removeNextHop(discoveryEntry.getParticipantId());
    }

    @Override
    public Promise<Lookup1Deferred> lookup(String[] domains,
                                           String interfaceName,
                                           joynr.types.DiscoveryQos discoveryQos) {
        final Lookup1Deferred deferred = new Lookup1Deferred();
        CapabilitiesCallback callback = new CapabilitiesCallback() {
            @Override
            public void processCapabilitiesReceived(@CheckForNull Collection<DiscoveryEntryWithMetaInfo> capabilities) {
                if (capabilities == null) {
                    deferred.reject(new ProviderRuntimeException("Received capablities collection was null"));
                } else {
                    deferred.resolve(capabilities.toArray(new DiscoveryEntryWithMetaInfo[capabilities.size()]));
                }
            }

            @Override
            public void onError(Throwable e) {
                deferred.reject(new ProviderRuntimeException(e.toString()));
            }

            @Override
            public void onError(DiscoveryError error) {
                // TODO
                deferred.reject(new ProviderRuntimeException(error.toString()));
            }
        };
        DiscoveryScope discoveryScope = DiscoveryScope.valueOf(discoveryQos.getDiscoveryScope().name());
        lookup(domains,
               interfaceName,
               new DiscoveryQos(discoveryQos.getDiscoveryTimeout(),
                                defaultDiscoveryRetryInterval,
                                ArbitrationStrategy.NotSet,
                                discoveryQos.getCacheMaxAge(),
                                discoveryScope),
               knownGbids,
               callback);

        return new Promise<>(deferred);
    }

    @Override
    public Promise<Lookup2Deferred> lookup(String[] domains,
                                           String interfaceName,
                                           joynr.types.DiscoveryQos discoveryQos,
                                           String[] gbids) {
        // TODO
        throw new ProviderRuntimeException("NOT IMPLEMENTED");
    }

    private boolean isEntryForGbids(GlobalDiscoveryEntry entry, Set<String> gbidSet) {
        Address entryAddress;
        try {
            entryAddress = CapabilityUtils.getAddressFromGlobalDiscoveryEntry(entry);
        } catch (Exception e) {
            logger.error("Error reading address from GlobalDiscoveryEntry: " + entry);
            return false;
        }
        if (entryAddress instanceof MqttAddress) {
            if (!gbidSet.contains(((MqttAddress) entryAddress).getBrokerUri())) {
                return false;
            }
        } else if (entry == null) {
            return false;
        }
        // return true for all other address types
        return true;
    }

    private Set<DiscoveryEntryWithMetaInfo> filterGloballyCachedEntriesByGbids(Collection<GlobalDiscoveryEntry> globalEntries,
                                                                               String[] gbids) {
        Set<DiscoveryEntryWithMetaInfo> result = new HashSet<>();
        Set<String> gbidSet = new HashSet<>(Arrays.asList(gbids));
        for (GlobalDiscoveryEntry entry : globalEntries) {
            Set<String> entryBackends = globalProviderParticipantIdToGbidSetMap.get(entry.getParticipantId());
            if (entryBackends != null) {
                // local provider which is globally registered
                if (gbidSet.stream().noneMatch(gbid -> entryBackends.contains(gbid))) {
                    continue;
                }
            } else if (!isEntryForGbids(entry, gbidSet)) {
                // globally looked up provider in wrong backend
                continue;
            }
            result.add(CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(false, entry));
        }
        return result;
    }

    @Override
    public void lookup(final String[] domains,
                       final String interfaceName,
                       final DiscoveryQos discoveryQos,
                       String[] gbids,
                       final CapabilitiesCallback capabilitiesCallback) {
        DiscoveryScope discoveryScope = discoveryQos.getDiscoveryScope();
        Set<DiscoveryEntry> localDiscoveryEntries = getLocalEntriesIfRequired(discoveryScope, domains, interfaceName);
        Set<DiscoveryEntryWithMetaInfo> globalDiscoveryEntries = getGloballyCachedEntriesIfRequired(discoveryScope,
                                                                                                    gbids,
                                                                                                    domains,
                                                                                                    interfaceName,
                                                                                                    discoveryQos.getCacheMaxAgeMs());
        switch (discoveryScope) {
        case LOCAL_ONLY:
            capabilitiesCallback.processCapabilitiesReceived(CapabilityUtils.convertToDiscoveryEntryWithMetaInfoSet(true,
                                                                                                                    localDiscoveryEntries));
            break;
        case LOCAL_THEN_GLOBAL:
            handleLocalThenGlobal(domains,
                                  interfaceName,
                                  discoveryQos,
                                  gbids,
                                  capabilitiesCallback,
                                  CapabilityUtils.convertToDiscoveryEntryWithMetaInfoSet(true, localDiscoveryEntries),
                                  globalDiscoveryEntries);
            break;
        case GLOBAL_ONLY:
            handleGlobalOnly(domains, interfaceName, discoveryQos, gbids, capabilitiesCallback, globalDiscoveryEntries);
            break;
        case LOCAL_AND_GLOBAL:
            handleLocalAndGlobal(domains,
                                 interfaceName,
                                 discoveryQos,
                                 gbids,
                                 capabilitiesCallback,
                                 localDiscoveryEntries,
                                 globalDiscoveryEntries);
            break;
        default:
            throw new IllegalStateException("Unknown or illegal DiscoveryScope value: " + discoveryScope);
        }
    }

    private void handleLocalThenGlobal(String[] domains,
                                       String interfaceName,
                                       DiscoveryQos discoveryQos,
                                       String[] gbids,
                                       CapabilitiesCallback capabilitiesCallback,
                                       Set<DiscoveryEntryWithMetaInfo> localDiscoveryEntries,
                                       Set<DiscoveryEntryWithMetaInfo> globalDiscoveryEntries) {
        Set<String> domainsForGlobalLookup = new HashSet<>();
        Set<DiscoveryEntryWithMetaInfo> matchedDiscoveryEntries = new HashSet<>();
        for (String domainToMatch : domains) {
            boolean domainMatched = addEntriesForDomain(localDiscoveryEntries, matchedDiscoveryEntries, domainToMatch);
            domainMatched = domainMatched
                    || addEntriesForDomain(globalDiscoveryEntries, matchedDiscoveryEntries, domainToMatch);
            if (!domainMatched) {
                domainsForGlobalLookup.add(domainToMatch);
            }
        }
        handleMissingGlobalEntries(interfaceName,
                                   discoveryQos,
                                   gbids,
                                   capabilitiesCallback,
                                   domainsForGlobalLookup,
                                   matchedDiscoveryEntries);
    }

    private void handleLocalAndGlobal(String[] domains,
                                      String interfaceName,
                                      DiscoveryQos discoveryQos,
                                      String[] gbids,
                                      CapabilitiesCallback capabilitiesCallback,
                                      Set<DiscoveryEntry> localDiscoveryEntries,
                                      Set<DiscoveryEntryWithMetaInfo> globalDiscoveryEntries) {
        Set<DiscoveryEntryWithMetaInfo> localDiscoveryEntriesWithMetaInfo = CapabilityUtils.convertToDiscoveryEntryWithMetaInfoSet(true,
                                                                                                                                   localDiscoveryEntries);

        Set<String> domainsForGlobalLookup = new HashSet<>();
        Set<DiscoveryEntryWithMetaInfo> matchedDiscoveryEntries = new HashSet<>();
        for (String domainToMatch : domains) {
            addEntriesForDomain(localDiscoveryEntriesWithMetaInfo, matchedDiscoveryEntries, domainToMatch);
            if (!addNonDuplicatedEntriesForDomain(globalDiscoveryEntries,
                                                  matchedDiscoveryEntries,
                                                  domainToMatch,
                                                  localDiscoveryEntries)) {
                domainsForGlobalLookup.add(domainToMatch);
            }
        }
        handleMissingGlobalEntries(interfaceName,
                                   discoveryQos,
                                   gbids,
                                   capabilitiesCallback,
                                   domainsForGlobalLookup,
                                   matchedDiscoveryEntries);
    }

    private void handleGlobalOnly(String[] domains,
                                  String interfaceName,
                                  DiscoveryQos discoveryQos,
                                  String[] gbids,
                                  CapabilitiesCallback capabilitiesCallback,
                                  Set<DiscoveryEntryWithMetaInfo> globalDiscoveryEntries) {
        Set<String> domainsForGlobalLookup = new HashSet<>(Arrays.asList(domains));
        for (DiscoveryEntry discoveryEntry : globalDiscoveryEntries) {
            domainsForGlobalLookup.remove(discoveryEntry.getDomain());
        }
        handleMissingGlobalEntries(interfaceName,
                                   discoveryQos,
                                   gbids,
                                   capabilitiesCallback,
                                   domainsForGlobalLookup,
                                   globalDiscoveryEntries);
    }

    private void handleMissingGlobalEntries(String interfaceName,
                                            DiscoveryQos discoveryQos,
                                            String[] gbids,
                                            CapabilitiesCallback capabilitiesCallback,
                                            Set<String> domainsForGlobalLookup,
                                            Set<DiscoveryEntryWithMetaInfo> matchedDiscoveryEntries) {
        if (domainsForGlobalLookup.isEmpty()) {
            capabilitiesCallback.processCapabilitiesReceived(matchedDiscoveryEntries);
        } else {
            asyncGetGlobalCapabilitities(gbids,
                                         domainsForGlobalLookup.toArray(new String[domainsForGlobalLookup.size()]),
                                         interfaceName,
                                         matchedDiscoveryEntries,
                                         discoveryQos.getDiscoveryTimeoutMs(),
                                         capabilitiesCallback);
        }
    }

    private boolean addEntriesForDomain(Collection<DiscoveryEntryWithMetaInfo> discoveryEntries,
                                        Collection<DiscoveryEntryWithMetaInfo> addTo,
                                        String domain) {
        boolean domainMatched = false;
        for (DiscoveryEntryWithMetaInfo discoveryEntry : discoveryEntries) {
            if (discoveryEntry.getDomain().equals(domain)) {
                addTo.add(discoveryEntry);
                domainMatched = true;
            }
        }
        return domainMatched;
    }

    private boolean addNonDuplicatedEntriesForDomain(Collection<DiscoveryEntryWithMetaInfo> discoveryEntries,
                                                     Collection<DiscoveryEntryWithMetaInfo> addTo,
                                                     String domain,
                                                     Collection<DiscoveryEntry> possibleDuplicateEntries) {

        boolean domainMatched = false;
        for (DiscoveryEntryWithMetaInfo discoveryEntry : discoveryEntries) {
            if (discoveryEntry.getDomain().equals(domain)) {
                DiscoveryEntry foundDiscoveryEntry = new DiscoveryEntry(discoveryEntry);
                if (!possibleDuplicateEntries.contains(foundDiscoveryEntry)) {
                    addTo.add(discoveryEntry);
                }
                domainMatched = true;
            }
        }
        return domainMatched;
    }

    private Set<DiscoveryEntryWithMetaInfo> getGloballyCachedEntriesIfRequired(DiscoveryScope discoveryScope,
                                                                               String[] gbids,
                                                                               String[] domains,
                                                                               String interfaceName,
                                                                               long cacheMaxAge) {
        if (INCLUDE_GLOBAL_SCOPES.contains(discoveryScope)) {
            Collection<GlobalDiscoveryEntry> globallyCachedEntries = globalDiscoveryEntryCache.lookup(domains,
                                                                                                      interfaceName,
                                                                                                      cacheMaxAge);
            return filterGloballyCachedEntriesByGbids(globallyCachedEntries, gbids);
        }
        return null;
    }

    private Set<DiscoveryEntry> getLocalEntriesIfRequired(DiscoveryScope discoveryScope,
                                                          String[] domains,
                                                          String interfaceName) {
        if (INCLUDE_LOCAL_SCOPES.contains(discoveryScope)) {
            return new HashSet<DiscoveryEntry>(localDiscoveryEntryStore.lookup(domains, interfaceName));
        }
        return null;
    }

    @Override
    public Promise<Lookup3Deferred> lookup(String participantId) {
        Lookup3Deferred deferred = new Lookup3Deferred();
        DiscoveryEntryWithMetaInfo discoveryEntry = lookup(participantId, DiscoveryQos.NO_FILTER);
        deferred.resolve(discoveryEntry);
        return new Promise<>(deferred);
    }

    @Override
    public Promise<Lookup4Deferred> lookup(String participantId, String[] gbids) {
        // TODO
        throw new ProviderRuntimeException("NOT IMPLEMENTED");
    }

    @Override
    @CheckForNull
    public DiscoveryEntryWithMetaInfo lookup(String participantId, DiscoveryQos discoveryQos) {
        final Future<DiscoveryEntryWithMetaInfo> lookupFuture = new Future<>();
        lookup(participantId, discoveryQos, knownGbids, new CapabilityCallback() {

            @Override
            public void processCapabilityReceived(DiscoveryEntryWithMetaInfo capability) {
                lookupFuture.onSuccess(capability);
            }

            @Override
            public void onError(Throwable e) {
                lookupFuture.onFailure(new JoynrRuntimeException(e));
            }

            @Override
            public void onError(DiscoveryError error) {
                // TODO
                lookupFuture.onFailure(new ProviderRuntimeException(error.toString()));
            }
        });
        DiscoveryEntryWithMetaInfo retrievedCapabilitiyEntry = null;

        try {
            retrievedCapabilitiyEntry = lookupFuture.get();
        } catch (InterruptedException e1) {
            logger.error("interrupted while retrieving capability entry by participant ID", e1);
        } catch (ApplicationException e1) {
            // should not be reachable since ApplicationExceptions are not used internally
            logger.error("ApplicationException while retrieving capability entry by participant ID", e1);
        }
        return retrievedCapabilitiyEntry;
    }

    @Override
    @CheckForNull
    public void lookup(final String participantId,
                       final DiscoveryQos discoveryQos,
                       final String[] gbids,
                       final CapabilityCallback capabilityCallback) {

        final DiscoveryEntry localDiscoveryEntry = localDiscoveryEntryStore.lookup(participantId,
                                                                                   discoveryQos.getCacheMaxAgeMs());

        DiscoveryScope discoveryScope = discoveryQos.getDiscoveryScope();
        switch (discoveryScope) {
        case LOCAL_ONLY:
            if (localDiscoveryEntry != null) {
                capabilityCallback.processCapabilityReceived(CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(true,
                                                                                                                 localDiscoveryEntry));
            } else {
                capabilityCallback.processCapabilityReceived(null);
            }
            break;
        case LOCAL_THEN_GLOBAL:
        case LOCAL_AND_GLOBAL:
            if (localDiscoveryEntry != null) {
                capabilityCallback.processCapabilityReceived(CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(true,
                                                                                                                 localDiscoveryEntry));
            } else {
                asyncGetGlobalCapabilitity(gbids, participantId, discoveryQos, capabilityCallback);
            }
            break;
        case GLOBAL_ONLY:
            asyncGetGlobalCapabilitity(gbids, participantId, discoveryQos, capabilityCallback);
            break;
        default:
            break;
        }
    }

    private void registerIncomingEndpoints(Collection<GlobalDiscoveryEntry> caps) {
        for (GlobalDiscoveryEntry ce : caps) {
            // TODO when are entries purged from the messagingEndpointDirectory?
            if (ce.getParticipantId() != null && ce.getAddress() != null) {
                Address address = CapabilityUtils.getAddressFromGlobalDiscoveryEntry(ce);
                final boolean isGloballyVisible = (ce.getQos().getScope() == ProviderScope.GLOBAL);
                final long expiryDateMs = Long.MAX_VALUE;

                messageRouter.addToRoutingTable(ce.getParticipantId(), address, isGloballyVisible, expiryDateMs);
            }
        }
    }

    private void asyncGetGlobalCapabilitity(final String[] gbids,
                                            final String participantId,
                                            DiscoveryQos discoveryQos,
                                            final CapabilityCallback capabilitiesCallback) {

        GlobalDiscoveryEntry cachedGlobalCapability = globalDiscoveryEntryCache.lookup(participantId,
                                                                                       discoveryQos.getCacheMaxAgeMs());

        if (cachedGlobalCapability != null
                && isEntryForGbids(cachedGlobalCapability, new HashSet<>(Arrays.asList(gbids)))) {
            capabilitiesCallback.processCapabilityReceived(CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(false,
                                                                                                               cachedGlobalCapability));
        } else {
            globalCapabilitiesDirectoryClient.lookup(new Callback<GlobalDiscoveryEntry>() {

                @Override
                public void onSuccess(@CheckForNull GlobalDiscoveryEntry newGlobalDiscoveryEntry) {
                    if (newGlobalDiscoveryEntry != null) {
                        registerIncomingEndpoints(Arrays.asList(newGlobalDiscoveryEntry));
                        globalDiscoveryEntryCache.add(newGlobalDiscoveryEntry);
                        if (isEntryForGbids(newGlobalDiscoveryEntry, new HashSet<>(Arrays.asList(gbids)))) {
                            capabilitiesCallback.processCapabilityReceived(CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(false,
                                                                                                                               newGlobalDiscoveryEntry));
                        } else {
                            capabilitiesCallback.onError(DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS);
                        }
                    } else {
                        capabilitiesCallback.onError(new NullPointerException("Received capabilities are null"));
                    }
                }

                @Override
                public void onFailure(JoynrRuntimeException exception) {
                    capabilitiesCallback.onError(exception);

                }
            }, participantId, discoveryQos.getDiscoveryTimeoutMs());
        }

    }

    /**
     * mixes in the localDiscoveryEntries to global capabilities found by participantId
     */
    private void asyncGetGlobalCapabilitities(final String[] gbids,
                                              final String[] domains,
                                              final String interfaceName,
                                              Collection<DiscoveryEntryWithMetaInfo> localDiscoveryEntries2,
                                              long discoveryTimeout,
                                              final CapabilitiesCallback capabilitiesCallback) {

        final Collection<DiscoveryEntryWithMetaInfo> localDiscoveryEntries = localDiscoveryEntries2 == null
                ? new LinkedList<DiscoveryEntryWithMetaInfo>()
                : localDiscoveryEntries2;

        globalCapabilitiesDirectoryClient.lookup(new Callback<List<GlobalDiscoveryEntry>>() {

            @Override
            public void onSuccess(List<GlobalDiscoveryEntry> globalDiscoverEntries) {
                if (globalDiscoverEntries != null) {
                    registerIncomingEndpoints(globalDiscoverEntries);
                    globalDiscoveryEntryCache.add(globalDiscoverEntries);
                    Collection<DiscoveryEntryWithMetaInfo> allDisoveryEntries = new ArrayList<DiscoveryEntryWithMetaInfo>(globalDiscoverEntries.size()
                            + localDiscoveryEntries.size());
                    allDisoveryEntries.addAll(CapabilityUtils.convertToDiscoveryEntryWithMetaInfoList(false,
                                                                                                      globalDiscoverEntries));
                    allDisoveryEntries.addAll(localDiscoveryEntries);
                    capabilitiesCallback.processCapabilitiesReceived(allDisoveryEntries);
                } else {
                    capabilitiesCallback.onError(new NullPointerException("Received capabilities are null"));
                }
            }

            @Override
            public void onFailure(JoynrRuntimeException exception) {
                capabilitiesCallback.onError(exception);
            }
        }, domains, interfaceName, discoveryTimeout);
    }

    @Override
    public void shutdown(boolean unregisterAllRegisteredCapabilities) {
        if (unregisterAllRegisteredCapabilities) {
            Set<DiscoveryEntry> allDiscoveryEntries = localDiscoveryEntryStore.getAllDiscoveryEntries();

            List<DiscoveryEntry> discoveryEntries = new ArrayList<>(allDiscoveryEntries.size());

            for (DiscoveryEntry capabilityEntry : allDiscoveryEntries) {
                if (capabilityEntry.getQos().getScope() == ProviderScope.GLOBAL) {
                    discoveryEntries.add(capabilityEntry);
                }
            }

            if (discoveryEntries.size() > 0) {
                try {
                    CallbackWithModeledError<Void, DiscoveryError> callback = new CallbackWithModeledError<Void, DiscoveryError>() {

                        @Override
                        public void onFailure(JoynrRuntimeException error) {
                        }

                        @Override
                        public void onSuccess(Void result) {
                        }

                        @Override
                        public void onFailure(DiscoveryError errorEnum) {

                        }

                    };
                    List<String> participantIds = discoveryEntries.stream()
                                                                  .filter(Objects::nonNull)
                                                                  .map(dEntry -> dEntry.getParticipantId())
                                                                  .collect(Collectors.toList());
                    for (String participantId : participantIds) {
                        globalCapabilitiesDirectoryClient.remove(callback,
                                                                 participantId,
                                                                 globalProviderParticipantIdToGbidSetMap.get(participantId)
                                                                                                        .toArray(new String[0]));
                    }
                } catch (DiscoveryException e) {
                    logger.debug("error removing discovery entries", e);
                }
            }
        }
    }

    @Override
    public Set<DiscoveryEntry> listLocalCapabilities() {
        return localDiscoveryEntryStore.getAllDiscoveryEntries();
    }

    @Override
    public void transportReady(@Nonnull Address address) {
        synchronized (globalAddressLock) {
            globalAddress = address;
        }
        for (QueuedDiscoveryEntry queuedDiscoveryEntry : queuedDiscoveryEntries) {
            registerGlobal(queuedDiscoveryEntry.getDiscoveryEntry(),
                           queuedDiscoveryEntry.getGbids(),
                           queuedDiscoveryEntry.getDeferred(),
                           queuedDiscoveryEntry.getAwaitGlobalRegistration());
        }
    }

}
