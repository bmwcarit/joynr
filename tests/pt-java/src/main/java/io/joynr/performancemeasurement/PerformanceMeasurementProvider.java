/*
 * #%L
 * %%
 * Copyright (C) 2021 BMW Car IT GmbH
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
package io.joynr.performancemeasurement;

import java.util.ArrayList;
import java.util.List;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.capabilities.CapabilityUtils;
import io.joynr.provider.Promise;
import joynr.infrastructure.DefaultGlobalCapabilitiesDirectoryProvider;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.Version;

public class PerformanceMeasurementProvider extends DefaultGlobalCapabilitiesDirectoryProvider {
    private static final Logger logger = LoggerFactory.getLogger(PerformanceMeasurementProvider.class);

    final private String fakedParticipantId1 = "GlobalDiscoveryEntry_fakedParticipantId1";
    final private String fakedParticipantId2 = "GlobalDiscoveryEntry_fakedParticipantId1";

    public PerformanceMeasurementProvider() {
    }

    /*
    * lookup
    */
    @Override
    public Promise<Lookup1Deferred> lookup(String[] domains, String interfaceName) {
        logger.debug("* PerformanceMeasurementProvider.lookup(String[] domains, String interfaceName) called");
        Lookup1Deferred deferred = new Lookup1Deferred();

        GlobalDiscoveryEntry fakedGlobalDiscoveryEntry1 = CapabilityUtils.newGlobalDiscoveryEntry(new Version(0, 5),
                                                                                                  domains[0],
                                                                                                  interfaceName,
                                                                                                  fakedParticipantId1,
                                                                                                  new ProviderQos(),
                                                                                                  System.currentTimeMillis(),
                                                                                                  System.currentTimeMillis()
                                                                                                          + 1000L,
                                                                                                  "fakedPublicKeyId",
                                                                                                  new MqttAddress("tcp://mqttbroker-1:1883",
                                                                                                                  "TOPIC"));

        GlobalDiscoveryEntry fakedGlobalDiscoveryEntry2 = CapabilityUtils.newGlobalDiscoveryEntry(new Version(0, 5),
                                                                                                  domains[0],
                                                                                                  interfaceName,
                                                                                                  fakedParticipantId2,
                                                                                                  new ProviderQos(),
                                                                                                  System.currentTimeMillis(),
                                                                                                  System.currentTimeMillis()
                                                                                                          + 1000L,
                                                                                                  "fakedPublicKeyId",
                                                                                                  new MqttAddress("tcp://mqttbroker-1:1883",
                                                                                                                  "TOPIC"));

        List<GlobalDiscoveryEntry> globalDiscoveryEntries = new ArrayList<GlobalDiscoveryEntry>();
        globalDiscoveryEntries.add(fakedGlobalDiscoveryEntry1);
        globalDiscoveryEntries.add(fakedGlobalDiscoveryEntry2);

        GlobalDiscoveryEntry[] globalDiscoveryEntriesArray = new GlobalDiscoveryEntry[2];
        globalDiscoveryEntriesArray = globalDiscoveryEntries.toArray(globalDiscoveryEntriesArray);

        deferred.resolve(globalDiscoveryEntriesArray);
        return new Promise<>(deferred);
    }
}
