/*
 * #%L
 * %%
 * Copyright (C) 2025 BMW Car IT GmbH
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
package io.joynr.discovery.pt.jee;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import jakarta.ejb.Stateless;
import jakarta.inject.Inject;
import jakarta.transaction.Transactional;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.capabilities.CapabilityUtils;
import io.joynr.jeeintegration.api.ServiceProvider;
import io.joynr.jeeintegration.api.SubscriptionPublisher;
import joynr.exceptions.ApplicationException;
import joynr.exceptions.ProviderRuntimeException;
import joynr.infrastructure.GlobalCapabilitiesDirectorySubscriptionPublisher;
import joynr.infrastructure.GlobalCapabilitiesDirectorySync;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.Version;

@Stateless
@ServiceProvider(serviceInterface = GlobalCapabilitiesDirectorySync.class)
@Transactional(rollbackOn = Exception.class)
public class GlobalCapabilitiesDirectoryEjb implements GlobalCapabilitiesDirectoryService {
    private static final Logger logger = LoggerFactory.getLogger(GlobalCapabilitiesDirectoryEjb.class);
    @SuppressWarnings("unused")
    private GlobalCapabilitiesDirectorySubscriptionPublisher gcdSubPublisher;

    final private String fakedParticipantId1 = "lrc.erqw.sde.vbgf.as.nmc.tt.yte.bgtrfvcd.OtmvqaZwertvbgfdwqBTG";
    final private String fakedParticipantId2 = "anotherFakedParticipantId";
    final private String[] fakedDomains = new String[]{ "gfz.abcd" };
    final private String interfaceName = "go/wef/yg/sdf/ghtrqazs/OtmvqaZwertvbgfdwqBTG";
    private static final String TOPIC_NAME = "h/f/gfz.abcds";

    @Inject
    public GlobalCapabilitiesDirectoryEjb(@SubscriptionPublisher GlobalCapabilitiesDirectorySubscriptionPublisher gcdSubPublisher) {
        logger.trace("### CONSTRUCTOR called");
        this.gcdSubPublisher = gcdSubPublisher;
    }

    @Override
    public void add(GlobalDiscoveryEntry[] globalDiscoveryEntries) {
        throw new ProviderRuntimeException("Not implemented yet");
    }

    @Override
    public void add(GlobalDiscoveryEntry globalDiscoveryEntry) {
        throw new ProviderRuntimeException("Not implemented yet");
    }

    @Override
    public void add(GlobalDiscoveryEntry globalDiscoveryEntry, String[] gbids) throws ApplicationException {
        throw new ProviderRuntimeException("Not implemented yet");
    }

    @Override
    public void fireGlobalDiscoveryEntryChanged() {
        throw new UnsupportedOperationException("Not implemented yet");
    }

    @Override
    public GlobalDiscoveryEntry[] lookup(String[] domains, String interfaceName) {
        throw new ProviderRuntimeException("Not implemented yet");
    }

    @Override
    public GlobalDiscoveryEntry[] lookup(String[] domains,
                                         String interfaceName,
                                         String[] gbids) throws ApplicationException {
        logger.debug("Looking up global discovery entries for domains {} and interfaceName {} and Gbids {}",
                     Arrays.toString(domains),
                     interfaceName,
                     Arrays.toString(gbids));

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
                                                                                                                  TOPIC_NAME));

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
                                                                                                                  TOPIC_NAME));

        List<GlobalDiscoveryEntry> globalDiscoveryEntries = new ArrayList<GlobalDiscoveryEntry>();
        globalDiscoveryEntries.add(fakedGlobalDiscoveryEntry1);
        globalDiscoveryEntries.add(fakedGlobalDiscoveryEntry2);

        GlobalDiscoveryEntry[] globalDiscoveryEntriesArray = new GlobalDiscoveryEntry[2];
        globalDiscoveryEntriesArray = globalDiscoveryEntries.toArray(globalDiscoveryEntriesArray);
        return globalDiscoveryEntriesArray;
    }

    @Override
    public GlobalDiscoveryEntry lookup(String participantId, String[] gbids) throws ApplicationException {
        logger.debug("Looking up global discovery entry for participantId {} and Gbids {}",
                     participantId,
                     Arrays.toString(gbids));

        GlobalDiscoveryEntry fakedGlobalDiscoveryEntry1 = CapabilityUtils.newGlobalDiscoveryEntry(new Version(0, 5),
                                                                                                  fakedDomains[0],
                                                                                                  interfaceName,
                                                                                                  participantId,
                                                                                                  new ProviderQos(),
                                                                                                  System.currentTimeMillis(),
                                                                                                  System.currentTimeMillis()
                                                                                                          + 1000L,
                                                                                                  "fakedPublicKeyId",
                                                                                                  new MqttAddress("tcp://mqttbroker-1:1883",
                                                                                                                  TOPIC_NAME));

        return fakedGlobalDiscoveryEntry1;
    }

    @Override
    public GlobalDiscoveryEntry lookup(String participantId) {
        throw new ProviderRuntimeException("Not implemented yet");
    }

    @Override
    public void remove(String[] participantIds) {
        throw new ProviderRuntimeException("Not implemented yet");
    }

    @Override
    public void remove(String participantId) {
        throw new ProviderRuntimeException("Not implemented yet");
    }

    @Override
    public void remove(String participantId, String[] gbids) throws ApplicationException {
        logger.debug("Calling remove entries for participantId {} and gbids )", participantId, Arrays.toString(gbids));
    }

    @Override
    public void touch(String clusterControllerId) {
        throw new ProviderRuntimeException("Not implemented yet");
    }

    @Override
    public void touch(String clusterControllerId, String[] participantIds) {
        throw new ProviderRuntimeException("Not implemented yet");
    }

    @Override
    public void removeStale(String clusterControllerId, Long maxLastSeenDateMs) {
        throw new ProviderRuntimeException("Not implemented yet");
    }

}
