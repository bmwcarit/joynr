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
package io.joynr.arbitration;

import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.DelayQueue;
import java.util.concurrent.Semaphore;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.exceptions.DiscoveryException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.exceptions.JoynrShutdownException;
import io.joynr.exceptions.MultiDomainNoCompatibleProviderFoundException;
import io.joynr.exceptions.NoCompatibleProviderFoundException;
import io.joynr.proxy.CallbackWithModeledError;
import joynr.exceptions.ApplicationException;
import joynr.system.DiscoveryAsync;
import joynr.types.DiscoveryEntry;
import joynr.types.DiscoveryEntryWithMetaInfo;
import joynr.types.DiscoveryError;
import joynr.types.ProviderQos;
import joynr.types.Version;

/**
 *
 * The Arbitrator controls the discovery process:
 * <ul>
 * <li> search for matching {@link DiscoveryEntry} elements locally and/or globally depending on the DiscoveryQos
 * <li> call the {@link ArbitrationStrategyFunction} to select a discoveryEntry to be used for the proxy being created
 * </ul>
 */
public class Arbitrator {
    private static final Logger logger = LoggerFactory.getLogger(Arbitrator.class);
    static final DelayQueue<DelayableArbitration> arbitrationQueue = new DelayQueue<>();

    protected DiscoveryQos discoveryQos;
    protected DiscoveryAsync localDiscoveryAggregator;
    protected ArbitrationResult arbitrationResult = new ArbitrationResult();
    protected ArbitrationStatus arbitrationStatus = ArbitrationStatus.ArbitrationNotStarted;
    protected ArbitrationCallback arbitrationListener;
    // Initialized with 0 to block until the listener is registered
    private Semaphore arbitrationListenerSemaphore = new Semaphore(0);
    private long retryDelay = 0;
    private long arbitrationDeadline;
    private Set<String> domains;
    private String interfaceName;
    private Version interfaceVersion;
    private ArbitrationStrategyFunction arbitrationStrategyFunction;
    private DiscoveryEntryVersionFilter discoveryEntryVersionFilter;
    private final Map<String, Set<Version>> discoveredVersionsByDomainMap = new HashMap<>();
    private String[] gbids;
    private int arbitrationCnt = 0;

    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public Arbitrator(final Set<String> domains,
                      final String interfaceName,
                      final Version interfaceVersion,
                      final DiscoveryQos discoveryQos,
                      DiscoveryAsync localDiscoveryAggregator,
                      ArbitrationStrategyFunction arbitrationStrategyFunction,
                      DiscoveryEntryVersionFilter discoveryEntryVersionFilter,
                      String[] gbids) {
        this.domains = domains;
        this.interfaceName = interfaceName;
        this.interfaceVersion = interfaceVersion;
        this.discoveryQos = discoveryQos;
        this.localDiscoveryAggregator = localDiscoveryAggregator;
        this.arbitrationStrategyFunction = arbitrationStrategyFunction;
        arbitrationDeadline = System.currentTimeMillis() + discoveryQos.getDiscoveryTimeoutMs();
        this.discoveryEntryVersionFilter = discoveryEntryVersionFilter;
        if (gbids == null) {
            this.gbids = new String[0];
        } else {
            this.gbids = gbids.clone();
        }
    }

    protected void onError(Throwable exception) {
        if (exception instanceof JoynrShutdownException) {
            arbitrationFailed(exception);
        } else if (exception instanceof JoynrRuntimeException) {
            if (isArbitrationInTime()) {
                restartArbitration(exception);
            } else {
                arbitrationFailed(exception);
            }
        } else if (exception instanceof ApplicationException) {
            arbitrationFailed(exception);
        } else {
            arbitrationFailed(new JoynrRuntimeException(exception));
        }
    }

    /**
     * Called by the proxy builder to start the arbitration process.
     */
    public void scheduleArbitration() {
        DelayableArbitration arbitration = new DelayableArbitration(this, retryDelay);
        arbitrationQueue.put(arbitration);
    }

    void attemptArbitration() {
        arbitrationCnt++;
        logger.debug("DISCOVERY lookup #{} for domains: {}, interface: {}, {}, gbids: {}",
                     arbitrationCnt,
                     domains,
                     interfaceName,
                     interfaceVersion,
                     Arrays.toString(gbids));
        localDiscoveryAggregator.lookup(new DiscoveryCallback(),
                                        domains.toArray(new String[domains.size()]),
                                        interfaceName,
                                        new joynr.types.DiscoveryQos(discoveryQos.getCacheMaxAgeMs(),
                                                                     arbitrationDeadline - System.currentTimeMillis(),
                                                                     joynr.types.DiscoveryScope.valueOf(discoveryQos.getDiscoveryScope()
                                                                                                                    .name()),
                                                                     discoveryQos.getProviderMustSupportOnChange()),
                                        gbids);
    }

    public void lookup() {
        arbitrationCnt++;
        logger.debug("DISCOVERY lookup #{} for domains: {}, interface: {}, {}, gbids: {}",
                     arbitrationCnt,
                     domains,
                     interfaceName,
                     interfaceVersion,
                     Arrays.toString(gbids));
        localDiscoveryAggregator.lookup(new DiscoveryCallback(false),
                                        domains.toArray(new String[domains.size()]),
                                        interfaceName,
                                        new joynr.types.DiscoveryQos(discoveryQos.getCacheMaxAgeMs(),
                                                                     arbitrationDeadline - System.currentTimeMillis(),
                                                                     joynr.types.DiscoveryScope.valueOf(discoveryQos.getDiscoveryScope()
                                                                                                                    .name()),
                                                                     discoveryQos.getProviderMustSupportOnChange()),
                                        gbids);
    }

    /**
     * Called by the proxy builder to register the listener for arbitration results (DiscoveryAgent Instance).
     * If the arbitration is already running or even finished the current state and in case of a successful arbitration
     * the result is set at the newly registered listener.
     *
     * @param arbitrationListener the arbitration listener
     */
    public void setArbitrationListener(ArbitrationCallback arbitrationListener) {

        this.arbitrationListener = arbitrationListener;
        // listener is now ready to receive arbitration result/status
        arbitrationListenerSemaphore.release();
        if (arbitrationStatus == ArbitrationStatus.ArbitrationSuccessful) {
            arbitrationFinished(arbitrationStatus, arbitrationResult);
        }
    }

    /**
     * Sets the arbitration result at the arbitrationListener if the listener is already registered
     * @param arbitrationStatus ArbitrationStatus
     * @param arbitrationResult ArbitrationResult
     */
    protected void arbitrationFinished(ArbitrationStatus arbitrationStatus, ArbitrationResult arbitrationResult) {
        this.arbitrationStatus = arbitrationStatus;
        this.arbitrationResult = arbitrationResult;

        // wait for arbitration listener to be registered
        if (arbitrationListenerSemaphore.tryAcquire()) {
            arbitrationListener.onSuccess(arbitrationResult);
            arbitrationListenerSemaphore.release();
        }
    }

    public ArbitrationStatus getArbitrationStatus() {
        return arbitrationStatus;
    }

    protected boolean isArbitrationInTime() {
        return System.currentTimeMillis() < arbitrationDeadline;
    }

    protected void restartArbitration() {
        restartArbitration(null);
    }

    protected void restartArbitration(Throwable exception) {
        retryDelay = discoveryQos.getRetryIntervalMs();
        if (System.currentTimeMillis() + retryDelay > arbitrationDeadline) {
            arbitrationFailed(exception);
            return;
        }
        logger.trace("Rescheduling arbitration with delay {}ms", retryDelay);
        scheduleArbitration();
    }

    protected void arbitrationFailed() {
        arbitrationFailed(null);
    }

    protected void arbitrationFailed(Throwable exception) {
        arbitrationStatus = ArbitrationStatus.ArbitrationCanceledForever;
        Throwable reason;
        if (arbitrationListenerSemaphore.tryAcquire()) {
            if (exception != null) {
                if (exception instanceof ApplicationException) {
                    DiscoveryError discoveryError = ((ApplicationException) exception).getError();
                    reason = new DiscoveryException("Unable to find provider due to DiscoveryError: " + discoveryError
                            + " interface: " + interfaceName + " domains: " + domains + " gbids: "
                            + Arrays.toString(gbids) + " " + interfaceVersion);
                } else {
                    reason = exception;
                }
            } else if (discoveredVersionsByDomainMap == null || discoveredVersionsByDomainMap.isEmpty()) {
                reason = new DiscoveryException("Unable to find provider in time: interface: " + interfaceName
                        + " domains: " + domains + " " + interfaceVersion);
            } else {
                reason = noCompatibleProviderFound();
            }
            arbitrationListener.onError(reason);
        }
    }

    private Throwable noCompatibleProviderFound() {
        Throwable reason = null;
        if (domains.size() == 1) {
            if (discoveredVersionsByDomainMap.size() != 1) {
                reason = new IllegalStateException("Only looking for one domain, but got multi-domain result with discovered but incompatible versions.");
            } else {
                reason = new NoCompatibleProviderFoundException(interfaceName,
                                                                interfaceVersion,
                                                                discoveredVersionsByDomainMap.keySet()
                                                                                             .iterator()
                                                                                             .next(),
                                                                discoveredVersionsByDomainMap.values()
                                                                                             .iterator()
                                                                                             .next());
            }
        } else if (domains.size() > 1) {
            Map<String, NoCompatibleProviderFoundException> exceptionsByDomain = new HashMap<>();
            for (Map.Entry<String, Set<Version>> versionsByDomainEntry : discoveredVersionsByDomainMap.entrySet()) {
                exceptionsByDomain.put(versionsByDomainEntry.getKey(),
                                       new NoCompatibleProviderFoundException(interfaceName,
                                                                              interfaceVersion,
                                                                              versionsByDomainEntry.getKey(),
                                                                              versionsByDomainEntry.getValue()));
            }
            reason = new MultiDomainNoCompatibleProviderFoundException(exceptionsByDomain);
        }
        return reason;
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + (int) (arbitrationDeadline ^ (arbitrationDeadline >>> 32));
        result = prime * result + ((arbitrationResult == null) ? 0 : arbitrationResult.hashCode());
        result = prime * result + ((arbitrationStatus == null) ? 0 : arbitrationStatus.hashCode());
        result = prime * result
                + ((discoveredVersionsByDomainMap == null) ? 0 : discoveredVersionsByDomainMap.hashCode());
        result = prime * result + ((domains == null) ? 0 : domains.hashCode());
        result = prime * result + ((interfaceName == null) ? 0 : interfaceName.hashCode());
        result = prime * result + ((interfaceVersion == null) ? 0 : interfaceVersion.hashCode());
        return result;
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj) {
            return true;
        }
        if (obj == null) {
            return false;
        }
        if (getClass() != obj.getClass()) {
            return false;
        }
        Arbitrator other = (Arbitrator) obj;
        if (arbitrationDeadline != other.arbitrationDeadline) {
            return false;
        }
        if (arbitrationResult == null) {
            if (other.arbitrationResult != null) {
                return false;
            }
        } else if (!arbitrationResult.equals(other.arbitrationResult)) {
            return false;
        }
        if (arbitrationStatus != other.arbitrationStatus) {
            return false;
        }
        if (discoveredVersionsByDomainMap == null) {
            if (other.discoveredVersionsByDomainMap != null) {
                return false;
            }
        } else if (!discoveredVersionsByDomainMap.equals(other.discoveredVersionsByDomainMap)) {
            return false;
        }
        if (domains == null) {
            if (other.domains != null) {
                return false;
            }
        } else if (!domains.equals(other.domains)) {
            return false;
        }
        if (interfaceName == null) {
            if (other.interfaceName != null) {
                return false;
            }
        } else if (!interfaceName.equals(other.interfaceName)) {
            return false;
        }
        if (interfaceVersion == null) {
            if (other.interfaceVersion != null) {
                return false;
            }
        } else if (!interfaceVersion.equals(other.interfaceVersion)) {
            return false;
        }
        return true;
    }

    private class DiscoveryCallback extends CallbackWithModeledError<DiscoveryEntryWithMetaInfo[], DiscoveryError> {

        private boolean filterByVersionAndArbitrationStrategy = true;

        public DiscoveryCallback() {
        };

        public DiscoveryCallback(boolean filterByVersionAndArbitrationStrategy) {
            this.filterByVersionAndArbitrationStrategy = filterByVersionAndArbitrationStrategy;
        }

        @Override
        public void onFailure(JoynrRuntimeException error) {
            Arbitrator.this.onError(error);
        }

        @Override
        public void onFailure(DiscoveryError error) {
            switch (error) {
            case NO_ENTRY_FOR_PARTICIPANT:
            case NO_ENTRY_FOR_SELECTED_BACKENDS:
                logger.trace("DISCOVERY lookup #{} for domains: {}, interface: {}, {}, gbids: {} returned DiscoveryError {}, continuing",
                             arbitrationCnt,
                             domains,
                             interfaceName,
                             interfaceVersion,
                             Arrays.toString(gbids),
                             error);
                if (isArbitrationInTime()) {
                    restartArbitration(new ApplicationException(error));
                } else {
                    arbitrationFailed(new ApplicationException(error));
                }
                break;
            case UNKNOWN_GBID:
            case INVALID_GBID:
            case INTERNAL_ERROR:
            default:
                logger.trace("DISCOVERY lookup #{} for domains: {}, interface: {}, {}, gbids: {} returned DiscoveryError {}, giving up",
                             arbitrationCnt,
                             domains,
                             interfaceName,
                             interfaceVersion,
                             Arrays.toString(gbids),
                             error);
                arbitrationFailed(new ApplicationException(error));
                break;
            }
        }

        @Override
        public void onSuccess(DiscoveryEntryWithMetaInfo[] discoveryEntries) {
            assert discoveryEntries != null : "Discovery entries may not be null.";
            if (allDomainsDiscovered(discoveryEntries)) {
                if (logger.isTraceEnabled()) {
                    logger.trace("DISCOVERY lookup #{} for domains: {}, interface: {}, {}, gbids: {} succeeded. Got {}",
                                 arbitrationCnt,
                                 domains,
                                 interfaceName,
                                 interfaceVersion,
                                 Arrays.toString(gbids),
                                 Arrays.toString(discoveryEntries));
                }
                Set<DiscoveryEntryWithMetaInfo> discoveryEntriesSet = filterDiscoveryEntries(discoveryEntries);

                Set<DiscoveryEntryWithMetaInfo> selectedCapabilities;
                if (filterByVersionAndArbitrationStrategy) {
                    selectedCapabilities = arbitrationStrategyFunction.select(discoveryQos.getCustomParameters(),
                                                                              discoveryEntriesSet);
                } else {
                    selectedCapabilities = discoveryEntriesSet;
                }

                logger.trace("Selected capabilities: {}", selectedCapabilities);
                if (selectedCapabilities != null && !selectedCapabilities.isEmpty()) {
                    arbitrationResult.setDiscoveryEntries(selectedCapabilities);
                    arbitrationFinished(ArbitrationStatus.ArbitrationSuccessful, arbitrationResult);
                } else {
                    arbitrationFailed();
                }
            } else {
                if (isArbitrationInTime()) {
                    restartArbitration();
                } else {
                    arbitrationFailed();
                }
            }
        }

        private boolean allDomainsDiscovered(DiscoveryEntry[] discoveryEntries) {
            Set<String> discoveredDomains = new HashSet<>();
            for (DiscoveryEntry foundDiscoveryEntry : discoveryEntries) {
                discoveredDomains.add(foundDiscoveryEntry.getDomain());
            }
            boolean allDomainsDiscovered = discoveredDomains.equals(domains);

            if (!allDomainsDiscovered) {
                Set<String> missingDomains = new HashSet<>(domains);
                missingDomains.removeAll(discoveredDomains);
                logger.trace("All domains must be found. Domains not found: {}", missingDomains);
            }

            return allDomainsDiscovered;
        }

        private Set<DiscoveryEntryWithMetaInfo> filterDiscoveryEntries(DiscoveryEntryWithMetaInfo[] discoveryEntries) {
            Set<DiscoveryEntryWithMetaInfo> discoveryEntriesSet;
            // If onChange subscriptions are required ignore
            // providers that do not support them
            if (discoveryQos.getProviderMustSupportOnChange()) {
                discoveryEntriesSet = new HashSet<>(discoveryEntries.length);
                for (DiscoveryEntryWithMetaInfo discoveryEntry : discoveryEntries) {
                    ProviderQos providerQos = discoveryEntry.getQos();
                    if (providerQos.getSupportsOnChangeSubscriptions()) {
                        discoveryEntriesSet.add(discoveryEntry);
                    }
                }
            } else {
                discoveryEntriesSet = new HashSet<DiscoveryEntryWithMetaInfo>(Arrays.asList(discoveryEntries));
            }
            if (filterByVersionAndArbitrationStrategy) {
                discoveryEntriesSet = discoveryEntryVersionFilter.filter(interfaceVersion,
                                                                         discoveryEntriesSet,
                                                                         discoveredVersionsByDomainMap);
                if (discoveryEntriesSet.isEmpty()) {
                    logger.debug("No discovery entries left after filtering while looking for interface {} in {}. Entries found: {}",
                                 interfaceName,
                                 interfaceVersion,
                                 Arrays.toString(discoveryEntries));
                }
            }
            return discoveryEntriesSet;
        }
    }
}
