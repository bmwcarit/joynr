package io.joynr.arbitration;

/*
 * #%L
 * joynr::java::core::libjoynr
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

import io.joynr.capabilities.LocalCapabilitiesDirectory;
import io.joynr.exceptions.JoynrArbitrationException;

import com.google.inject.Inject;
import com.google.inject.name.Named;

public final class ArbitratorFactory {

    @Inject
    @Named("joynr.arbitration.minimumretrydelay")
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
     * @param minimumArbitrationRetryDelay
     * @return
     * @throws JoynrArbitrationException
     */
    public static Arbitrator create(final String domain,
                                    final String interfaceName,
                                    final DiscoveryQos discoveryQos,
                                    LocalCapabilitiesDirectory capabilitiesSource) throws JoynrArbitrationException {

        switch (discoveryQos.getArbitrationStrategy()) {
        case FixedChannel:
            return new FixedParticipantArbitrator(discoveryQos, capabilitiesSource, minimumArbitrationRetryDelay);
        case Keyword:
            return new KeywordArbitrator(domain,
                                         interfaceName,
                                         discoveryQos,
                                         capabilitiesSource,
                                         minimumArbitrationRetryDelay);
        case HighestPriority:
            return new HighestPriorityArbitrator(domain,
                                                 interfaceName,
                                                 discoveryQos,
                                                 capabilitiesSource,
                                                 minimumArbitrationRetryDelay);
        default:
            throw new JoynrArbitrationException("Arbitration failed: domain: " + domain + " interface: "
                    + interfaceName + " qos: " + discoveryQos + ": unknown arbitration strategy or strategy not set!");
        }

    }
}
