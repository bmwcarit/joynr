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
import static org.hamcrest.CoreMatchers.hasItem;
import static org.junit.Assert.assertThat;

import java.util.Collection;
import java.util.HashSet;
import java.util.Properties;
import java.util.Set;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.ObjectMapper.DefaultTyping;
import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;

import io.joynr.capabilities.CapabilitiesProvisioning;
import io.joynr.capabilities.CapabilityUtils;
import io.joynr.capabilities.StaticCapabilitiesProvisioning;
import io.joynr.capabilities.StaticCapabilitiesProvisioningModule;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.types.DiscoveryEntry;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderQos;

public class StaticCapabilitiesProvisioningTest {

    private ObjectMapper objectMapper;

    @Before
    public void setUp() throws Exception {
        Guice.createInjector(new AbstractModule() {
            @Override
            protected void configure() {
                objectMapper = new ObjectMapper();
                objectMapper.enableDefaultTypingAsProperty(DefaultTyping.JAVA_LANG_OBJECT, "_typeName");
                bind(ObjectMapper.class).toInstance(objectMapper);
                requestStaticInjection(CapabilityUtils.class);
            }
        });
    }

    @After
    public void tearDown() throws Exception {
    }

    @Test
    public void testLoadingSerailizedDiscoveryEntries() throws Exception {
        Set<DiscoveryEntry> discoveryEntries = new HashSet<DiscoveryEntry>();
        String participantId = "particpantId";
        ProviderQos qos = new ProviderQos();
        Long lastSeenDateMs = 0L;
        Address address = new MqttAddress("brokerUri", "topic");
        GlobalDiscoveryEntry entry1 = CapabilityUtils.newGlobalDiscoveryEntry("domain1",
                                                                              "interfaceName1",
                                                                              participantId,
                                                                              qos,
                                                                              lastSeenDateMs,
                                                                              address);

        GlobalDiscoveryEntry entry2 = CapabilityUtils.newGlobalDiscoveryEntry("domain2",
                                                                              "interfaceName2",
                                                                              participantId,
                                                                              qos,
                                                                              lastSeenDateMs,
                                                                              address);
        discoveryEntries.add(entry1);
        discoveryEntries.add(entry2);

        String serializedDiscoveryEntries = objectMapper.writeValueAsString(discoveryEntries);
        final Properties properties = new Properties();
        properties.put(StaticCapabilitiesProvisioning.PROPERTY_PROVISIONED_CAPABILITIES, serializedDiscoveryEntries);
        Injector injector = Guice.createInjector(new AbstractModule() {

            @Override
            protected void configure() {
                bind(ObjectMapper.class).toInstance(objectMapper);

            }
        }, new StaticCapabilitiesProvisioningModule(properties));

        CapabilitiesProvisioning capabilitiesProvisioning = injector.getInstance(CapabilitiesProvisioning.class);
        Collection<DiscoveryEntry> provisionedDiscoveryEntries = capabilitiesProvisioning.getDiscoveryEntries();
        assertThat(provisionedDiscoveryEntries, hasItem(entry1));
        assertThat(provisionedDiscoveryEntries, hasItem(entry2));

    }

}
