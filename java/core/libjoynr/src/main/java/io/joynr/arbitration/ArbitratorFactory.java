package io.joynr.arbitration;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
import io.joynr.capabilities.LocalCapabilitiesDirectory;
import io.joynr.exceptions.JoynrArbitrationException;

import com.google.inject.Inject;
import com.google.inject.name.Named;

public final class ArbitratorFactory {

    @Inject
    @Named(PROPERTY_ARBITRATION_MINIMUMRETRYDELAY)
    private static long minimumArbitrationRetryDelay;

    private ArbitratorFactory() {

    }

    /**
     * Creates an arbitrator defined by the arbitrationStrategy set in the discoveryQos.
     *
     * @param domain
     *            Domain of the provider.
     * @param interfaceName
     *            Provided interface.
     * @param discoveryQos
     *            Arbitration settings like arbitration strategy, timeout and strategy specific parameters.
     * @param capabilitiesSource
     *            Source for capabilities lookup.
     * @return the created Arbitrator object
     * @throws JoynrArbitrationException if arbitration strategy is unknown
     */
    public static Arbitrator create(final String domain,
                                    final String interfaceName,
                                    final DiscoveryQos discoveryQos,
                                    LocalCapabilitiesDirectory capabilitiesSource) throws JoynrArbitrationException {

        switch (discoveryQos.getArbitrationStrategy()) {
        case FixedChannel:
            return new Arbitrator(domain,
                                  interfaceName,
                                  discoveryQos,
                                  capabilitiesSource,
                                  minimumArbitrationRetryDelay,
                                  new FixedParticipantArbitrator());
        case Keyword:
            return new Arbitrator(domain,
                                  interfaceName,
                                  discoveryQos,
                                  capabilitiesSource,
                                  minimumArbitrationRetryDelay,
                                  new KeywordArbitrator());
        case HighestPriority:
            return new Arbitrator(domain,
                                  interfaceName,
                                  discoveryQos,
                                  capabilitiesSource,
                                  minimumArbitrationRetryDelay,
                                  new HighestPriorityArbitrator());
        case Custom:
            return new Arbitrator(domain,
                                  interfaceName,
                                  discoveryQos,
                                  capabilitiesSource,
                                  minimumArbitrationRetryDelay,
                                  discoveryQos.getArbitrationStrategyFunction());

        default:
            throw new JoynrArbitrationException("Arbitration failed: domain: " + domain + " interface: "
                    + interfaceName + " qos: " + discoveryQos + ": unknown arbitration strategy or strategy not set!");
        }

    }
}
