/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2024 BMW Car IT GmbH
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

import static io.joynr.messaging.routing.MessageRouter.SCHEDULEDTHREADPOOL;

import java.util.Set;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.exceptions.DiscoveryException;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.runtime.ShutdownListener;
import io.joynr.runtime.ShutdownNotifier;
import joynr.system.DiscoveryAsync;
import joynr.types.Version;

public final class ArbitratorFactory {

    @Inject
    private static DiscoveryEntryVersionFilter discoveryEntryVersionFilter;

    @Inject
    private static ShutdownNotifier shutdownNotifier;

    @Inject
    private static MessageRouter messageRouter;

    @Inject
    @Named(SCHEDULEDTHREADPOOL)
    private static ScheduledExecutorService scheduler;

    private static ArbitratorRunnable arbitratorRunnable;

    private ArbitratorFactory() {

    }

    /**
     * Creates an arbitrator defined by the arbitrationStrategy set in the discoveryQos.
     *
     * @param domains
     *            Set of domains of the provider.
     * @param interfaceName
     *            Provided interface.
     * @param interfaceVersion
     *            the Version of the interface being looked for.
     * @param discoveryQos
     *            Arbitration settings like arbitration strategy, timeout and strategy specific parameters.
     * @param localDiscoveryAggregator
     *            Source for capabilities lookup.
     * @param gbids
     *            Array of GBID strings
     * @return the created Arbitrator object
     * @throws DiscoveryException
     *             if arbitration strategy is unknown
     */
    public static Arbitrator create(final Set<String> domains,
                                    final String interfaceName,
                                    final Version interfaceVersion,
                                    final DiscoveryQos discoveryQos,
                                    DiscoveryAsync localDiscoveryAggregator,
                                    String[] gbids) throws DiscoveryException {

        ArbitrationStrategyFunction arbitrationStrategyFunction;
        switch (discoveryQos.getArbitrationStrategy()) {
        case FixedChannel:
            arbitrationStrategyFunction = new FixedParticipantArbitrationStrategyFunction();
            break;
        case Keyword:
            arbitrationStrategyFunction = new KeywordArbitrationStrategyFunction();
            break;
        case HighestPriority:
            arbitrationStrategyFunction = new HighestPriorityArbitrationStrategyFunction();
            break;
        case LastSeen:
            arbitrationStrategyFunction = new LastSeenArbitrationStrategyFunction();
            break;
        case Custom:
            arbitrationStrategyFunction = discoveryQos.getArbitrationStrategyFunction();
            break;
        default:
            throw new DiscoveryException("Arbitration failed: domain: " + domains + " interface: " + interfaceName
                    + " qos: " + discoveryQos + ": unknown arbitration strategy or strategy not set!");
        }
        return new Arbitrator(domains,
                              interfaceName,
                              interfaceVersion,
                              discoveryQos,
                              localDiscoveryAggregator,
                              arbitrationStrategyFunction,
                              discoveryEntryVersionFilter,
                              gbids,
                              messageRouter);
    }

    public static synchronized void start() {
        if (arbitratorRunnable == null) {
            arbitratorRunnable = new ArbitratorRunnable();
            scheduler.execute(arbitratorRunnable);
            shutdownNotifier.registerForShutdown(new ShutdownListener() {
                @Override
                public void shutdown() {
                    ArbitratorFactory.shutdown();
                }
            });
        }
    }

    public static synchronized void shutdown() {
        if (arbitratorRunnable != null) {
            arbitratorRunnable.stop();
            arbitratorRunnable = null;
        }
    }

    static class ArbitratorRunnable implements Runnable {
        private Logger logger = LoggerFactory.getLogger(ArbitratorRunnable.class);
        private volatile boolean stopped;

        ArbitratorRunnable() {
            stopped = false;
        }

        void stop() {
            stopped = true;
        }

        @Override
        public void run() {
            DelayableArbitration delayableArbitration = null;

            Thread.currentThread().setName("ArbitratorRunnable");
            logger.trace("Start ArbitratorRunnable");

            while (!stopped) {
                try {
                    delayableArbitration = Arbitrator.arbitrationQueue.poll(1000, TimeUnit.MILLISECONDS);
                    if (delayableArbitration != null) {
                        delayableArbitration.getArbitration().attemptArbitration();
                    }
                } catch (InterruptedException e) {
                    if (stopped) {
                        logger.info("ArbitratorRunnable interrupted during shutdown. Terminating...");
                        Thread.currentThread().interrupt();
                        return;
                    }

                    logger.trace("ArbitratorRunnable interrupted. Continuing in a new cycle.");
                } catch (Exception e) {
                    logger.error("Unexpected exception in ArbitratorRunnable: ", e);
                    if (delayableArbitration != null) {
                        delayableArbitration.getArbitration().arbitrationFailed(new DiscoveryException(e));
                    }
                }
            }
            logger.trace("Stop ArbitratorRunnable");
        }

    }
}
