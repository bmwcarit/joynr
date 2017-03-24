package io.joynr.arbitration;

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
import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_ARBITRATION_MINIMUMRETRYDELAY;

import java.util.Set;

import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.exceptions.DiscoveryException;
import joynr.system.DiscoveryAsync;
import joynr.types.Version;

public final class ArbitratorFactory {

    @Inject
    @Named(PROPERTY_ARBITRATION_MINIMUMRETRYDELAY)
    private static long minimumArbitrationRetryDelay;

    @Inject
    private static DiscoveryEntryVersionFilter discoveryEntryVersionFilter;

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
     * @return the created Arbitrator object
     * @throws DiscoveryException
     *             if arbitration strategy is unknown
     */
    public static Arbitrator create(final Set<String> domains,
                                    final String interfaceName,
                                    final Version interfaceVersion,
                                    final DiscoveryQos discoveryQos,
                                    DiscoveryAsync localDiscoveryAggregator) throws DiscoveryException {

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
                              minimumArbitrationRetryDelay,
                              arbitrationStrategyFunction,
                              discoveryEntryVersionFilter);
    }

}
