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
package io.joynr.capabilities.directory;

import java.util.Arrays;
import java.util.Collection;
import java.util.HashSet;
import java.util.Optional;
import java.util.Set;
import java.util.stream.Collectors;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

import io.joynr.capabilities.CapabilityUtils;
import io.joynr.capabilities.GlobalDiscoveryEntryPersisted;
import io.joynr.capabilities.GlobalDiscoveryEntryStore;
import io.joynr.capabilities.directory.util.GcdUtilities;
import io.joynr.exceptions.JoynrException;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;
import io.joynr.provider.PromiseListener;
import joynr.exceptions.ApplicationException;
import joynr.exceptions.ProviderRuntimeException;
import joynr.infrastructure.GlobalCapabilitiesDirectoryAbstractProvider;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.ChannelAddress;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.types.DiscoveryError;
import joynr.types.GlobalDiscoveryEntry;

/**
 * The capabilities directory implementation for server-side capabilities querying.
 */
@Singleton
public class CapabilitiesDirectoryImpl extends GlobalCapabilitiesDirectoryAbstractProvider {
    private static final Logger logger = LoggerFactory.getLogger(CapabilitiesDirectoryImpl.class);
    public static final String PROPERTY_PREFIX = "joynr.gcd.";
    public static final String GCD_GBID = PROPERTY_PREFIX + "gbid";
    public static final String VALID_GBIDS = PROPERTY_PREFIX + "valid.gbids";

    private GlobalDiscoveryEntryStore<GlobalDiscoveryEntryPersisted> discoveryEntryStore;
    private String gcdGbid;
    private Set<String> validGbids;

    @Inject
    public CapabilitiesDirectoryImpl(GlobalDiscoveryEntryStore<GlobalDiscoveryEntryPersisted> discoveryEntryStore,
                                     @Named(GCD_GBID) String gcdGbid,
                                     @Named(VALID_GBIDS) String validGbidsString) {
        this.discoveryEntryStore = discoveryEntryStore;
        this.gcdGbid = gcdGbid;
        this.validGbids = GcdUtilities.convertArrayStringToSet(validGbidsString, gcdGbid);
    }

    @Override
    public Promise<DeferredVoid> add(GlobalDiscoveryEntry[] globalDiscoveryEntries) {
        DeferredVoid deferred = new DeferredVoid();
        if (globalDiscoveryEntries == null) {
            logger.trace("Error adding GlobalDiscoveryEntries. List of entries is null");
            deferred.reject(new ProviderRuntimeException("Error adding GlobalDiscoveryEntries. List of entries is null"));
            return new Promise<DeferredVoid>(deferred);
        }
        for (GlobalDiscoveryEntry globalDiscoveryEntry : globalDiscoveryEntries) {
            if (globalDiscoveryEntry == null) {
                logger.trace("Error adding GlobalDiscoveryEntry. Entry is null");
                deferred.reject(new ProviderRuntimeException("Error adding GlobalDiscoveryEntry. Entry is null"));
                return new Promise<DeferredVoid>(deferred);
            }
        }
        for (GlobalDiscoveryEntry globalDiscoveryEntry : globalDiscoveryEntries) {
            add(globalDiscoveryEntry);
        }
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public Promise<DeferredVoid> add(GlobalDiscoveryEntry globalDiscoveryEntry) {
        logger.debug("Adding global discovery entry to own gbid {}: {}", gcdGbid, globalDiscoveryEntry);
        DeferredVoid deferred = new DeferredVoid();
        Promise<DeferredVoid> promise = new Promise<DeferredVoid>(deferred);
        try {
            addInternal(globalDiscoveryEntry, gcdGbid);
            deferred.resolve();
        } catch (ProviderRuntimeException e) {
            deferred.reject(e);
        } catch (ApplicationException e) {
            deferred.reject(new ProviderRuntimeException(e.getError().name() + ": Unable to add DiscoveryEntry for: "
                    + globalDiscoveryEntry.getParticipantId()));
        }
        return promise;
    }

    private void addInternal(GlobalDiscoveryEntry globalDiscoveryEntry, String... gbids) throws ApplicationException {
        gbids = Arrays.asList(gbids).stream().map(gbid -> {
            if (gbid.isEmpty()) {
                logger.warn("Received add with empty gbid for participantId: {}, treating as ownGbid.",
                            globalDiscoveryEntry.getParticipantId());
                return gcdGbid;
            } else {
                return gbid;
            }
        }).toArray(String[]::new);
        if (globalDiscoveryEntry.getDomain() == null || globalDiscoveryEntry.getInterfaceName() == null
                || globalDiscoveryEntry.getParticipantId() == null || globalDiscoveryEntry.getAddress() == null) {
            String message = "DiscoveryEntry being registered is not complete: " + globalDiscoveryEntry;
            logger.error(message);
            throw new ProviderRuntimeException(message);
        }

        Address address = CapabilityUtils.getAddressFromGlobalDiscoveryEntry(globalDiscoveryEntry);
        String clusterControllerId = "";
        if (address instanceof MqttAddress) {
            // not always the clusterControllerId. If a unicast topic prefix is set, this clusterControllerId is a part of the topic
            clusterControllerId = ((MqttAddress) address).getTopic();
        } else if (address instanceof ChannelAddress) {
            clusterControllerId = ((ChannelAddress) address).getChannelId();
        } else {
            logger.error("Error adding DiscoveryEntry for " + globalDiscoveryEntry.getParticipantId()
                    + ". Unknown address type: " + globalDiscoveryEntry.getAddress());
            throw new ProviderRuntimeException("Unable to add DiscoveryEntry for "
                    + globalDiscoveryEntry.getParticipantId() + ". Unknown address type: "
                    + globalDiscoveryEntry.getAddress());
        }

        try {
            // gbid will be set correctly in discoveryEntryStore
            GlobalDiscoveryEntryPersisted gdep = new GlobalDiscoveryEntryPersisted(globalDiscoveryEntry,
                                                                                   clusterControllerId,
                                                                                   gcdGbid);
            discoveryEntryStore.add(gdep, gbids);
        } catch (Exception e) {
            logger.error("Error adding discoveryEntry for {} and gbids {}: {}",
                         globalDiscoveryEntry.getParticipantId(),
                         Arrays.toString(gbids),
                         e);
            throw new ApplicationException(DiscoveryError.INTERNAL_ERROR);
        }
    }

    @Override
    public Promise<Add1Deferred> add(GlobalDiscoveryEntry globalDiscoveryEntry, String[] gbids) {
        logger.debug("Adding global discovery entry to {}: {}", Arrays.toString(gbids), globalDiscoveryEntry);
        Add1Deferred deferred = new Add1Deferred();
        Promise<Add1Deferred> promise = new Promise<Add1Deferred>(deferred);
        switch (GcdUtilities.validateGbids(gbids, gcdGbid, validGbids)) {
        case INVALID:
            deferred.reject(DiscoveryError.INVALID_GBID);
            break;
        case UNKNOWN:
            deferred.reject(DiscoveryError.UNKNOWN_GBID);
            break;
        case OK:
            try {
                addInternal(globalDiscoveryEntry, gbids);
                deferred.resolve();
            } catch (ProviderRuntimeException e) {
                deferred.reject(e);
            } catch (ApplicationException e) {
                deferred.reject(e.getError());
            }
            break;
        default:
            deferred.reject(DiscoveryError.INTERNAL_ERROR);
            break;
        }
        return promise;
    }

    @Override
    public Promise<DeferredVoid> remove(String[] participantIds) {
        DeferredVoid deferred = new DeferredVoid();
        int deletedCount = 0;
        for (String participantId : participantIds) {
            removeInternal(participantId);
            deletedCount += removeInternal(participantId);
        }
        logger.debug("Deleted {} entries (number of IDs passed in {})", deletedCount, participantIds.length);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    private int removeInternal(String participantId, String... gbids) {
        if (gbids.length > 0) {
            logger.debug("Removing global discovery entries with participantId {} from gbids {}",
                         participantId,
                         Arrays.toString(gbids));
        } else {
            logger.debug("Removing global discovery entries with participantId {} from own Gbid {}",
                         participantId,
                         gcdGbid);
            gbids = new String[]{ gcdGbid };
        }
        return discoveryEntryStore.remove(participantId, gbids);
    }

    @Override
    public Promise<DeferredVoid> remove(String participantId) {
        DeferredVoid deferred = new DeferredVoid();
        int deletedCount = removeInternal(participantId);
        logger.debug("Deleted {} entries for participantId {})", deletedCount, participantId);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public Promise<Remove1Deferred> remove(String participantId, String[] gbids) {
        Remove1Deferred deferred = new Remove1Deferred();
        Promise<Remove1Deferred> promise = new Promise<Remove1Deferred>(deferred);
        switch (GcdUtilities.validateGbids(gbids, gcdGbid, validGbids)) {
        case INVALID:
            logger.error("Error removing participantId {}: INVALID GBIDs: {}", participantId, Arrays.toString(gbids));
            deferred.reject(DiscoveryError.INVALID_GBID);
            break;
        case UNKNOWN:
            logger.error("Error removing participantId {}: UNKNOWN_GBID: {}", participantId, Arrays.toString(gbids));
            deferred.reject(DiscoveryError.UNKNOWN_GBID);
            break;
        case OK:
            gbids = Arrays.asList(gbids).stream().map(gbid -> {
                if (gbid.isEmpty()) {
                    logger.warn("Received remove with empty gbid for participantId {}, defaulting to ownGbid.",
                                participantId);
                    return gcdGbid;
                } else {
                    return gbid;
                }
            }).toArray(String[]::new);
            try {
                int deletedCount = removeInternal(participantId, gbids);
                switch (deletedCount) {
                case 0:
                    logger.warn("Error removing participantId {}. Participant is not registered (NO_ENTRY_FOR_PARTICIPANT).",
                                participantId);
                    deferred.reject(DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
                    break;
                case -1:
                    logger.warn("Error removing participantId {}. Participant is not registered in GBIDs {} (NO_ENTRY_FOR_SELECTED_BACKENDS).",
                                participantId,
                                Arrays.toString(gbids));
                    deferred.reject(DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS);
                    break;
                default:
                    logger.debug("Deleted {} entries for participantId {})", deletedCount, participantId);
                    deferred.resolve();
                }
            } catch (Exception e) {
                logger.error("Error removing discoveryEntry for {} and gbids {}: {}",
                             participantId,
                             Arrays.toString(gbids),
                             e);
                deferred.reject(DiscoveryError.INTERNAL_ERROR);
            }
            break;
        default:
            deferred.reject(DiscoveryError.INTERNAL_ERROR);
            break;
        }
        return promise;
    }

    @Override
    public Promise<Lookup1Deferred> lookup(final String[] domains, final String interfaceName) {
        logger.debug("Looking up global discovery entries for domains {} and interfaceName {} and own Gbid {}",
                     Arrays.toString(domains),
                     interfaceName,
                     gcdGbid);
        Lookup1Deferred deferred = new Lookup1Deferred();
        String[] gcdGbidArray = { gcdGbid };

        lookup(domains, interfaceName, gcdGbidArray).then(new PromiseListener() {
            @Override
            public void onRejection(JoynrException exception) {
                if (exception instanceof ApplicationException) {
                    DiscoveryError error = ((ApplicationException) exception).getError();
                    logger.error("Error looking up global discovery entries for domains {} and interfaceName {} and own Gbid {}: {}",
                                 Arrays.toString(domains),
                                 interfaceName,
                                 gcdGbid,
                                 exception);
                    deferred.reject(new ProviderRuntimeException("Error on lookup: " + error));
                } else if (exception instanceof ProviderRuntimeException) {
                    deferred.reject((ProviderRuntimeException) exception);
                } else {
                    deferred.reject(new ProviderRuntimeException("Unknown error on lookup: " + exception));
                }
            }

            @Override
            public void onFulfillment(Object... values) {
                deferred.resolve((GlobalDiscoveryEntry[]) values[0]);
            }
        });

        return new Promise<Lookup1Deferred>(deferred);
    }

    @Override
    public Promise<Lookup2Deferred> lookup(String[] domains, String interfaceName, String[] gbids) {
        logger.debug("Looking up global discovery entries for domains {} and interfaceName {} and Gbids {}",
                     Arrays.toString(domains),
                     interfaceName,
                     Arrays.toString(gbids));
        Lookup2Deferred deferred = new Lookup2Deferred();
        Promise<Lookup2Deferred> promise = new Promise<Lookup2Deferred>(deferred);
        switch (GcdUtilities.validateGbids(gbids, gcdGbid, validGbids)) {
        case INVALID:
            deferred.reject(DiscoveryError.INVALID_GBID);
            break;
        case UNKNOWN:
            deferred.reject(DiscoveryError.UNKNOWN_GBID);
            break;
        case OK:
            gbids = Arrays.asList(gbids).stream().map(gbid -> {
                if (gbid.isEmpty()) {
                    logger.warn("Received lookup with empty gbid for domains {} and interfaceName {}, treating as ownGbid.",
                                Arrays.toString(domains),
                                interfaceName);
                    return gcdGbid;
                } else {
                    return gbid;
                }
            }).toArray(String[]::new);
            try {
                Collection<GlobalDiscoveryEntryPersisted> lookupResult = discoveryEntryStore.lookup(domains,
                                                                                                    interfaceName);
                if (lookupResult.isEmpty()) {
                    deferred.resolve(new GlobalDiscoveryEntryPersisted[0]);
                    return promise;
                }

                Collection<GlobalDiscoveryEntryPersisted> filteredResult = filterByGbids(lookupResult, gbids);
                if (filteredResult.isEmpty()) {
                    deferred.reject(DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS);
                    return promise;
                }

                GlobalDiscoveryEntry[] globalDiscoveryEntriesArray = GcdUtilities.chooseOneGlobalDiscoveryEntryPerParticipantId(filteredResult,
                                                                                                                                gcdGbid);
                deferred.resolve(globalDiscoveryEntriesArray);
            } catch (Exception e) {
                logger.error("Error looking up global discovery entries for domains {} and interfaceName {} and Gbids {}: {}",
                             Arrays.toString(domains),
                             interfaceName,
                             Arrays.toString(gbids),
                             e);
                deferred.reject(DiscoveryError.INTERNAL_ERROR);
            }
        }
        return promise;
    }

    @Override
    public Promise<Lookup3Deferred> lookup(String participantId) {
        logger.debug("Looking up global discovery entry for participantId {} and own Gbid {}", participantId, gcdGbid);
        Lookup3Deferred deferred = new Lookup3Deferred();
        String[] gcdGbidArray = { gcdGbid };

        lookup(participantId, gcdGbidArray).then(new PromiseListener() {
            @Override
            public void onRejection(JoynrException exception) {
                if (exception instanceof ApplicationException) {
                    DiscoveryError error = ((ApplicationException) exception).getError();
                    logger.error("Error looking up global discovery entry for participantId {} and own Gbid {}: {}",
                                 participantId,
                                 gcdGbid,
                                 exception);
                    deferred.reject(new ProviderRuntimeException("Error on lookup: " + error));
                } else if (exception instanceof ProviderRuntimeException) {
                    deferred.reject((ProviderRuntimeException) exception);
                } else {
                    deferred.reject(new ProviderRuntimeException("Unknown error on lookup: " + exception));
                }
            }

            @Override
            public void onFulfillment(Object... values) {
                deferred.resolve((GlobalDiscoveryEntry) values[0]);
            }
        });
        return new Promise<Lookup3Deferred>(deferred);
    }

    private Collection<GlobalDiscoveryEntryPersisted> filterByGbids(Collection<GlobalDiscoveryEntryPersisted> queryResult,
                                                                    String[] gbids) {
        Set<String> gbidSet = new HashSet<>(Arrays.asList(gbids));
        Collection<GlobalDiscoveryEntryPersisted> filteredResult = queryResult.stream()
                                                                              .filter(gdep -> gbidSet.contains(gdep.getGbid()))
                                                                              .collect(Collectors.toList());
        return filteredResult;
    }

    @Override
    public Promise<Lookup4Deferred> lookup(String participantId, String[] gbids) {
        logger.debug("Looking up global discovery entry for participantId {} and Gbids {}",
                     participantId,
                     Arrays.toString(gbids));
        Lookup4Deferred deferred = new Lookup4Deferred();
        Promise<Lookup4Deferred> promise = new Promise<Lookup4Deferred>(deferred);
        switch (GcdUtilities.validateGbids(gbids, gcdGbid, validGbids)) {
        case INVALID:
            deferred.reject(DiscoveryError.INVALID_GBID);
            break;
        case UNKNOWN:
            deferred.reject(DiscoveryError.UNKNOWN_GBID);
            break;
        case OK:
            gbids = Arrays.asList(gbids).stream().map(gbid -> {
                if (gbid.isEmpty()) {
                    logger.warn("Received lookup with empty gbid for participantId {}, treating as ownGbid.",
                                participantId);
                    return gcdGbid;
                } else {
                    return gbid;
                }
            }).toArray(String[]::new);
            try {
                Optional<Collection<GlobalDiscoveryEntryPersisted>> optionalResult = discoveryEntryStore.lookup(participantId);
                Collection<GlobalDiscoveryEntryPersisted> lookupResult = optionalResult.isPresent()
                        ? optionalResult.get()
                        : null;
                if (lookupResult == null || lookupResult.isEmpty()) {
                    deferred.reject(DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
                    return promise;
                }
                Collection<GlobalDiscoveryEntryPersisted> filteredResult = filterByGbids(lookupResult, gbids);
                if (filteredResult.isEmpty()) {
                    deferred.reject(DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS);
                    return promise;
                }
                deferred.resolve(GcdUtilities.chooseOneGlobalDiscoveryEntry(filteredResult, gcdGbid));
            } catch (Exception e) {
                logger.error("Error looking up global discovery entry for participantId {} and Gbids {}: {}",
                             participantId,
                             Arrays.toString(gbids),
                             e);
                deferred.reject(DiscoveryError.INTERNAL_ERROR);
            }
        }
        return promise;
    }

    @Override
    public Promise<DeferredVoid> touch(String clusterControllerId) {
        DeferredVoid deferred = new DeferredVoid();
        discoveryEntryStore.touch(clusterControllerId);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public Promise<DeferredVoid> touch(String clusterControllerId, String[] participantIds) {
        DeferredVoid deferred = new DeferredVoid();
        final String message = "Error: touch method for clusterControllerId: " + clusterControllerId
                + " and participantIds: " + Arrays.toString(participantIds) + " is not yet implemented";
        logger.error(message);
        deferred.reject(new ProviderRuntimeException(message));
        return new Promise<DeferredVoid>(deferred);
    }

}
