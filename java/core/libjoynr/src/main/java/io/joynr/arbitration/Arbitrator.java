package io.joynr.arbitration;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

import static java.lang.String.format;

import java.util.Arrays;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.Semaphore;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.common.collect.Sets;

import io.joynr.exceptions.DiscoveryException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.exceptions.JoynrShutdownException;
import io.joynr.proxy.Callback;
import joynr.system.DiscoveryAsync;
import joynr.types.DiscoveryEntry;
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
    private final long MINIMUM_ARBITRATION_RETRY_DELAY;
    protected DiscoveryQos discoveryQos;
    protected DiscoveryAsync localDiscoveryAggregator;
    protected ArbitrationResult arbitrationResult = new ArbitrationResult();
    protected ArbitrationStatus arbitrationStatus = ArbitrationStatus.ArbitrationNotStarted;
    protected ArbitrationCallback arbitrationListener;
    // Initialized with 0 to block until the listener is registered
    private Semaphore arbitrationListenerSemaphore = new Semaphore(0);
    private long arbitrationDeadline;
    private Set<String> domains;
    private String interfaceName;
    private Version interfaceVersion;
    private ArbitrationStrategyFunction arbitrationStrategyFunction;
    private DiscoveryEntryVersionFilter discoveryEntryVersionFilter;

    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public Arbitrator(final Set<String> domains,
                      final String interfaceName,
                      final Version interfaceVersion,
                      final DiscoveryQos discoveryQos,
                      DiscoveryAsync localDiscoveryAggregator,
                      long minimumArbitrationRetryDelay,
                      ArbitrationStrategyFunction arbitrationStrategyFunction,
                      DiscoveryEntryVersionFilter discoveryEntryVersionFilter) {
        // CHECKSTYLE:ON
        this.domains = domains;
        this.interfaceName = interfaceName;
        this.interfaceVersion = interfaceVersion;
        MINIMUM_ARBITRATION_RETRY_DELAY = minimumArbitrationRetryDelay;
        this.discoveryQos = discoveryQos;
        this.localDiscoveryAggregator = localDiscoveryAggregator;
        this.arbitrationStrategyFunction = arbitrationStrategyFunction;
        arbitrationDeadline = System.currentTimeMillis() + discoveryQos.getDiscoveryTimeoutMs();
        this.discoveryEntryVersionFilter = discoveryEntryVersionFilter;
    }

    // TODO JOYN-911 make sure we are shutting down correctly onError
    protected void onError(Throwable exception) {
        if (exception instanceof IllegalStateException) {
            logger.error("CapabilitiesCallback: " + exception.getMessage(), exception);
            return;
        } else if (exception instanceof JoynrShutdownException) {
            logger.warn("CapabilitiesCallback onError: " + exception.getMessage(), exception);
        } else if (exception instanceof JoynrRuntimeException) {
            restartArbitrationIfNotExpired();
        } else {
            logger.error("CapabilitiesCallback onError thowable: " + exception.getMessage(), exception);
        }
    }

    /**
     * Called by the proxy builder to start the arbitration process.
     */
    public void startArbitration() {
        logger.debug("start arbitration for domain: {}, interface: {}", domains, interfaceName);
        // TODO qos map is not used. Implement qos filter in
        // capabilitiesDirectory or remove qos argument.
        arbitrationStatus = ArbitrationStatus.ArbitrationRunning;
        notifyArbitrationStatusChanged();

        localDiscoveryAggregator.lookup(new Callback<DiscoveryEntry[]>() {

            private Map<String, Set<Version>> discoveredVersions = new HashMap<>();

            @Override
            public void onFailure(JoynrRuntimeException error) {
                Arbitrator.this.onError(error);
            }

            @Override
            public void onSuccess(DiscoveryEntry[] discoveryEntries) {
                assert discoveryEntries != null : "Discovery entries may not be null.";
                Set<String> discoveredDomains = new HashSet<String>();
                for (DiscoveryEntry foundDiscoveryEntry: discoveryEntries) {
                    discoveredDomains.add(foundDiscoveryEntry.getDomain());
                }
                if (!discoveredDomains.equals(domains)) {
                    Set<String> missingDomains = new HashSet<String>(domains);
                    missingDomains.removeAll(discoveredDomains);
                    String discoveryErrorMessage = "All domains must be found. The following domain(s) was/were not found: " + missingDomains;
                    Arbitrator.this.onError(new DiscoveryException(discoveryErrorMessage));
                } else {
                    logger.debug("Lookup succeeded. Got {}", Arrays.toString(discoveryEntries));
                    Set<DiscoveryEntry> discoveryEntriesSet = filterDiscoveryEntries(discoveryEntries);

                    Collection<DiscoveryEntry> selectedCapabilities = arbitrationStrategyFunction
                            .select(discoveryQos.getCustomParameters(), discoveryEntriesSet);

                    logger.debug("Selected capabilities: {}", selectedCapabilities);
                    if (selectedCapabilities != null && !selectedCapabilities.isEmpty()) {
                        Set<String> participantIds = getParticipantIds(selectedCapabilities);
                        arbitrationResult.setParticipantIds(participantIds);
                        arbitrationStatus = ArbitrationStatus.ArbitrationSuccesful;
                        updateArbitrationResultAtListener();
                    } else {
                        restartArbitrationIfNotExpired(discoveredVersions);
                    }
                }
            }

            private Set<String> getParticipantIds(Collection<DiscoveryEntry> selectedCapabilities) {
                Set<String> participantIds = new HashSet<>();
                for (DiscoveryEntry selectedCapability : selectedCapabilities) {
                    if (selectedCapability != null) {
                        participantIds.add(selectedCapability.getParticipantId());
                    }
                }
                logger.debug("Resulting participant IDs: {}", participantIds);
                return participantIds;
            }

            private Set<DiscoveryEntry> filterDiscoveryEntries(DiscoveryEntry[] discoveryEntries) {
                Set<DiscoveryEntry> discoveryEntriesSet;
                // If onChange subscriptions are required ignore
                // providers that do not support them
                if (discoveryQos.getProviderMustSupportOnChange()) {
                    discoveryEntriesSet = new HashSet<DiscoveryEntry>(discoveryEntries.length);
                    for (DiscoveryEntry discoveryEntry : discoveryEntries) {
                        ProviderQos providerQos = discoveryEntry.getQos();
                        if (providerQos.getSupportsOnChangeSubscriptions()) {
                            discoveryEntriesSet.add(discoveryEntry);
                        }
                    }
                } else {
                    discoveryEntriesSet = Sets.newHashSet(discoveryEntries);
                }
                discoveryEntriesSet = discoveryEntryVersionFilter.filter(interfaceVersion, discoveryEntriesSet, discoveredVersions);
                if (discoveryEntriesSet.isEmpty()) {
                    logger.debug(
                              format("No discovery entries left after filtering while looking for %s in version %s.%nEntries found: %s",
                                                           interfaceName, interfaceVersion, Arrays.toString(discoveryEntries)));
                }
                return discoveryEntriesSet;
            }
        }, domains.toArray(new String[domains.size()]), interfaceName,
                                        new joynr.types.DiscoveryQos(discoveryQos.getCacheMaxAgeMs(),
                                                                     discoveryQos.getDiscoveryTimeoutMs(),
                                                                     joynr.types.DiscoveryScope.valueOf(discoveryQos.getDiscoveryScope()
                                                                                                        .name()),
                                                                     discoveryQos.getProviderMustSupportOnChange()));
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
        if (arbitrationStatus == ArbitrationStatus.ArbitrationSuccesful) {
            updateArbitrationResultAtListener();
        } else if (arbitrationStatus != ArbitrationStatus.ArbitrationNotStarted) {
            notifyArbitrationStatusChanged();
        }

    }

    /**
     * Sets the arbitration result at the arbitrationListener if the listener is already registered
     */
    protected void updateArbitrationResultAtListener() {
        // wait for arbitration listener to be registered
        if (arbitrationListenerSemaphore.tryAcquire()) {
            arbitrationListener.setArbitrationResult(arbitrationStatus, arbitrationResult);
            arbitrationListenerSemaphore.release();
        }
    }

    /**
     * Notify the arbitrationListener about a changed status without setting an arbitrationResult (e.g. when arbitration
     * failed)
     */
    protected void notifyArbitrationStatusChanged() {
        // wait for arbitration listener to be registered
        if (arbitrationListenerSemaphore.tryAcquire()) {
            arbitrationListener.notifyArbitrationStatusChanged(arbitrationStatus);
            arbitrationListenerSemaphore.release();
        }
    }

    public ArbitrationStatus getArbitrationStatus() {
        return arbitrationStatus;
    }

    protected boolean isArbitrationInTime() {
        return System.currentTimeMillis() < arbitrationDeadline;
    }

    protected void restartArbitrationIfNotExpired() {
        restartArbitrationIfNotExpired(null);
    }

    protected void restartArbitrationIfNotExpired(Map<String, Set<Version>> discoveredVersions) {
        if (isArbitrationInTime()) {
            logger.info("Restarting Arbitration");
            long backoff = Math.max(discoveryQos.getRetryIntervalMs(), MINIMUM_ARBITRATION_RETRY_DELAY);
            try {
                if (backoff > 0) {
                    Thread.sleep(backoff);
                }
                startArbitration();
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
        } else {
            arbitrationStatus = ArbitrationStatus.ArbitrationCanceledForever;
            if (arbitrationListenerSemaphore.tryAcquire()) {
                if (discoveredVersions != null && !discoveredVersions.isEmpty()) {
                    arbitrationListener.setDiscoveredVersions(discoveredVersions);
                }
                arbitrationListener.notifyArbitrationStatusChanged(arbitrationStatus);
            }
        }
    }

}
