package io.joynr.capabilities;

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

import static org.junit.Assert.assertTrue;
import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_CHANNEL_ID;
import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_PARTICIPANT_ID;
import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_DISCOVERY_DIRECTORIES_DOMAIN;
import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_DOMAIN_ACCESS_CONTROLLER_CHANNEL_ID;
import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_DOMAIN_ACCESS_CONTROLLER_PARTICIPANT_ID;
import static io.joynr.messaging.MessagingPropertyKeys.CAPABILITYDIRECTORYURL;
import static io.joynr.messaging.MessagingPropertyKeys.CHANNELID;
import static io.joynr.messaging.MessagingPropertyKeys.DISCOVERYDIRECTORYURL;

import java.util.Collection;
import java.util.HashSet;
import java.util.Properties;
import java.util.Set;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.ObjectMapper.DefaultTyping;
import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.ProvisionException;
import com.google.inject.name.Names;

import io.joynr.messaging.routing.RoutingTable;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.types.DiscoveryEntry;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.Version;

@RunWith(MockitoJUnitRunner.class)
public class StaticCapabilitiesProvisioningTest {

    private ObjectMapper objectMapper;

    @Mock
    private RoutingTable routingTable;

    @Before
    public void setUp() throws Exception {
        objectMapper = new ObjectMapper();
        objectMapper.enableDefaultTypingAsProperty(DefaultTyping.JAVA_LANG_OBJECT, "_typeName");
    }

    @Test
    public void testLoadingSerailizedDiscoveryEntries() throws Exception {
        Set<DiscoveryEntry> discoveryEntries = new HashSet<DiscoveryEntry>();
        String participantId = "particpantId";
        ProviderQos qos = new ProviderQos();
        Long lastSeenDateMs = 0L;
        Long expiryDateMs = 0L;
        String publicKeyId = "publicKeyId";
        Address address = new MqttAddress("brokerUri", "topic");
        GlobalDiscoveryEntry entry1 = CapabilityUtils.newGlobalDiscoveryEntry(new Version(),
                                                                              "domain1",
                                                                              "interfaceName1",
                                                                              participantId,
                                                                              qos,
                                                                              lastSeenDateMs,
                                                                              expiryDateMs,
                                                                              publicKeyId,
                                                                              address);

        GlobalDiscoveryEntry entry2 = CapabilityUtils.newGlobalDiscoveryEntry(new Version(),
                                                                              "domain2",
                                                                              "interfaceName2",
                                                                              participantId,
                                                                              qos,
                                                                              lastSeenDateMs,
                                                                              expiryDateMs,
                                                                              publicKeyId,
                                                                              address);
        discoveryEntries.add(entry1);
        discoveryEntries.add(entry2);

        final String serializedDiscoveryEntries = objectMapper.writeValueAsString(discoveryEntries);
        Injector injector = createInjectorForJsonValue(serializedDiscoveryEntries);

        CapabilitiesProvisioning subject = injector.getInstance(CapabilitiesProvisioning.class);
        Collection<DiscoveryEntry> provisionedDiscoveryEntries = subject.getDiscoveryEntries();

        assertTrue(provisionedDiscoveryEntries.contains(entry1));
        assertTrue(provisionedDiscoveryEntries.contains(entry2));
    }

    @Test(expected = ProvisionException.class)
    public void testInvalidJson() {
        Properties properties = new Properties();
        properties.put(StaticCapabilitiesProvisioning.PROPERTY_PROVISIONED_CAPABILITIES, "this is not json");
        Injector injector = createInjectorForJsonValue("this is not json");
        injector.getInstance(CapabilitiesProvisioning.class);
    }

    private Injector createInjectorForJsonValue(final String jsonValue) {
        Injector injector = Guice.createInjector(new AbstractModule() {
            @Override
            protected void configure() {
                bind(String.class).annotatedWith(Names.named(PROPERTY_CAPABILITIES_DIRECTORY_CHANNEL_ID))
                                  .toInstance("");
                bind(String.class).annotatedWith(Names.named(PROPERTY_CAPABILITIES_DIRECTORY_PARTICIPANT_ID))
                                  .toInstance("");
                bind(String.class).annotatedWith(Names.named(PROPERTY_DISCOVERY_DIRECTORIES_DOMAIN)).toInstance("");
                bind(String.class).annotatedWith(Names.named(PROPERTY_DOMAIN_ACCESS_CONTROLLER_CHANNEL_ID))
                                  .toInstance("");
                bind(String.class).annotatedWith(Names.named(PROPERTY_DOMAIN_ACCESS_CONTROLLER_PARTICIPANT_ID))
                                  .toInstance("");
                bind(String.class).annotatedWith(Names.named(CAPABILITYDIRECTORYURL)).toInstance("");
                bind(String.class).annotatedWith(Names.named(CHANNELID)).toInstance("");
                bind(String.class).annotatedWith(Names.named(DISCOVERYDIRECTORYURL)).toInstance("");

                bind(ObjectMapper.class).toInstance(objectMapper);
                bind(RoutingTable.class).toInstance(routingTable);
                bind(String.class).annotatedWith(Names.named(StaticCapabilitiesProvisioning.PROPERTY_PROVISIONED_CAPABILITIES))
                                  .toInstance(jsonValue);
                requestStaticInjection(CapabilityUtils.class);
            }
        },
                                                 new StaticCapabilitiesProvisioningModule());
        return injector;
    }
}
