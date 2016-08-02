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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import java.lang.reflect.Field;

import org.junit.Before;
import org.junit.Test;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.ObjectMapper.DefaultTyping;

import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.Version;

/**
 * Unit tests for {@link CapabilitiesUtils}.
 */
public class CapabilitiesUtilsTest {

    @Before
    public void setUp() throws Exception {
        ObjectMapper objectMapper = new ObjectMapper();
        objectMapper.enableDefaultTypingAsProperty(DefaultTyping.JAVA_LANG_OBJECT, "_typeName");
        Field objectMapperField = CapabilityUtils.class.getDeclaredField("objectMapper");
        objectMapperField.setAccessible(true);
        objectMapperField.set(CapabilityUtils.class, objectMapper);
    }

    @Test
    public void testCreateNewGlobalDiscoveryEntry() {
        Version version = new Version(0, 0);
        String domain = "domain";
        String interfaceName = "interfaceName";
        String participantId = "participantId";
        String publicKeyId = "publicKeyId";
        Address mqttAddress = new MqttAddress("tcp://broker:1883", "topic");
        ProviderQos providerQos = new ProviderQos();

        GlobalDiscoveryEntry result = CapabilityUtils.newGlobalDiscoveryEntry(version,
                                                                              domain,
                                                                              interfaceName,
                                                                              participantId,
                                                                              providerQos,
                                                                              0L,
                                                                              0L,
                                                                              publicKeyId,
                                                                              mqttAddress);

        assertNotNull(result);
        assertEquals(version, result.getProviderVersion());
        assertEquals(domain, result.getDomain());
        assertEquals(interfaceName, result.getInterfaceName());
        assertEquals(participantId, result.getParticipantId());
        assertEquals(providerQos, result.getQos());
        assertEquals((Long) 0L, result.getLastSeenDateMs());
        assertEquals((Long) 0L, result.getExpiryDateMs());
        assertEquals(publicKeyId, result.getPublicKeyId());
        assertEquals("{\"_typeName\":\"joynr.system.RoutingTypes.MqttAddress\",\"brokerUri\":\"tcp://broker:1883\",\"topic\":\"topic\"}",
                     result.getAddress());
    }

    @Test
    public void testGetMqttAddressFromGlobalDiscoveryEntry() {
        GlobalDiscoveryEntry globalDiscoveryEntry = new GlobalDiscoveryEntry(new Version(0, 0),
                                                                             "domain",
                                                                             "interfaceName",
                                                                             "participantId",
                                                                             new ProviderQos(),
                                                                             0L,
                                                                             0L,
                                                                             "publicKeyId",
                                                                             "{\"_typeName\":\"joynr.system.RoutingTypes.MqttAddress\",\"brokerUri\":\"tcp://broker:1883\",\"topic\":\"topic\"}");
        Address result = CapabilityUtils.getAddressFromGlobalDiscoveryEntry(globalDiscoveryEntry);

        assertTrue(result instanceof MqttAddress);
        assertEquals("tcp://broker:1883", ((MqttAddress) result).getBrokerUri());
        assertEquals("topic", ((MqttAddress) result).getTopic());
    }

}
