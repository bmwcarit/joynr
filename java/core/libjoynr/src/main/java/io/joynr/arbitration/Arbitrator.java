package io.joynr.arbitration;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2014 BMW Car IT GmbH
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

import io.joynr.capabilities.CapabilitiesCallback;
import io.joynr.capabilities.CapabilityEntry;
import io.joynr.capabilities.LocalCapabilitiesDirectory;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.exceptions.JoynrShutdownException;

import java.util.Collection;
import java.util.Iterator;
import java.util.concurrent.Semaphore;

import javax.annotation.CheckForNull;

import joynr.types.ProviderQos;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Base class for provider arbitrators. Concrete arbitrators have to provide implementations for startArbitration() and
 * {@literal selectProvider(ArrayList<CapabilityEntry> capabilities)} which set the ArbitrationStatus and call
 * notifyArbitrationStatusChanged() or set the ArbitrationResult and call updateArbitrationResultAtListener(). The base
 * class offers a CapabilitiesCallback which is used for async requests.
 *
 */
public class Arbitrator {
    private static final Logger logger = LoggerFactory.getLogger(Arbitrator.class);
    private final long MINIMUM_ARBITRATION_RETRY_DELAY;
    protected DiscoveryQos discoveryQos;
    protected LocalCapabilitiesDirectory localCapabilitiesDirectory;
    protected ArbitrationResult arbitrationResult = new ArbitrationResult();
    protected ArbitrationStatus arbitrationStatus = ArbitrationStatus.ArbitrationNotStarted;
    protected ArbitrationCallback arbitrationListener;
    // Initialized with 0 to block until the listener is registered
    private Semaphore arbitrationListenerSemaphore = new Semaphore(0);
    private long arbitrationDeadline;
    private String domain;
    private String interfaceName;
    private ArbitrationStrategyFunction arbitrationStrategyFunction;

    public Arbitrator(final String domain,
                      final String interfaceName,
                      final DiscoveryQos discoveryQos,
                      LocalCapabilitiesDirectory localCapabilitiesDirectory,
                      long minimumArbitrationRetryDelay,
                      ArbitrationStrategyFunction arbitrationStrategyFunction) {
        this.domain = domain;
        this.interfaceName = interfaceName;
        MINIMUM_ARBITRATION_RETRY_DELAY = minimumArbitrationRetryDelay;
        this.discoveryQos = discoveryQos;
        this.localCapabilitiesDirectory = localCapabilitiesDirectory;
        this.arbitrationStrategyFunction = arbitrationStrategyFunction;
        arbitrationDeadline = System.currentTimeMillis() + discoveryQos.getDiscoveryTimeout();
    }

    // TODO JOYN-911 make sure we are shutting down correctly onError
    protected void onError(Throwable exception) {
        try {
            throw exception;
        } catch (IllegalStateException e) {
            logger.error("CapabilitiesCallback: " + e.getMessage(), e);
            return;

        } catch (JoynrShutdownException e) {
            logger.warn("CapabilitiesCallback onError: " + e.getMessage(), e);

        } catch (JoynrRuntimeException e) {
            restartArbitrationIfNotExpired();
        } catch (Throwable e) {
            logger.error("CapabilitiesCallback onError thowable: " + e.getMessage(), e);
        }
    }

    /**
     * Called by the proxy builder to start the arbitration process.
     */
    public void startArbitration() {
        logger.debug("start arbitration for domain: {}, interface: {}", domain, interfaceName);
        // TODO qos map is not used. Implement qos filter in
        // capabilitiesDirectory or remove qos argument.
        arbitrationStatus = ArbitrationStatus.ArbitrationRunning;
        notifyArbitrationStatusChanged();

        localCapabilitiesDirectory.lookup(domain, interfaceName, discoveryQos, new CapabilitiesCallback() {

            @Override
            public void processCapabilitiesReceived(@CheckForNull Collection<CapabilityEntry> capabilities) {
                assert (capabilities != null);

                // If onChange subscriptions are required ignore providers that do not support them
                if (discoveryQos.getProviderMustSupportOnChange()) {
                    for (Iterator<CapabilityEntry> iterator = capabilities.iterator(); iterator.hasNext();) {
                        CapabilityEntry capabilityEntry = (CapabilityEntry) iterator.next();
                        ProviderQos providerQos = capabilityEntry.getProviderQos();
                        if (!providerQos.getSupportsOnChangeSubscriptions()) {
                            iterator.remove();
                        }
                    }
                }

                CapabilityEntry selectedCapability = arbitrationStrategyFunction.select(discoveryQos.getCustomParametes(),
                                                                                        capabilities);

                if (selectedCapability != null) {
                    arbitrationResult.setEndpointAddress(selectedCapability.getEndpointAddresses());
                    arbitrationResult.setParticipantId(selectedCapability.getParticipantId());
                    arbitrationStatus = ArbitrationStatus.ArbitrationSuccesful;
                    updateArbitrationResultAtListener();
                } else {
                    restartArbitrationIfNotExpired();
                }
            }

            @Override
            public void onError(Throwable exception) {
                Arbitrator.this.onError(exception);
            }
        });

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
        synchronized (arbitrationStatus) {
            return arbitrationStatus;
        }
    }

    protected boolean isArbitrationInTime() {
        return System.currentTimeMillis() < arbitrationDeadline;
    }

    protected void restartArbitrationIfNotExpired() {
        if (isArbitrationInTime()) {
            logger.info("Restarting Arbitration");
            long backoff = Math.max(discoveryQos.getRetryInterval(), MINIMUM_ARBITRATION_RETRY_DELAY);
            try {
                if (backoff > 0) {
                    Thread.sleep(backoff);
                }
                startArbitration();
            } catch (InterruptedException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
        } else {
            cancelArbitration();
        }
    }

    protected void cancelArbitration() {
        arbitrationStatus = ArbitrationStatus.ArbitrationCanceledForever;
        notifyArbitrationStatusChanged();
    }

}
