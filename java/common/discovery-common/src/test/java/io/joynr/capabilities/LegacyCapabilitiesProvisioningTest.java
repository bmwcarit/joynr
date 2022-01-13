/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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
package io.joynr.capabilities;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import java.lang.reflect.Field;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.junit.MockitoJUnitRunner;

import io.joynr.util.ObjectMapper;
import joynr.infrastructure.GlobalCapabilitiesDirectory;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.MqttAddress;

@RunWith(MockitoJUnitRunner.class)
public class LegacyCapabilitiesProvisioningTest {

    @Before
    public void setup() throws Exception {
        Field objectMapperField = CapabilityUtils.class.getDeclaredField("objectMapper");
        objectMapperField.setAccessible(true);
        objectMapperField.set(CapabilityUtils.class, new ObjectMapper());
    }

    @Test
    public void testMqttAddressGeneratedCorrectly() {
        LegacyCapabilitiesProvisioning.LegacyProvisioningPropertiesHolder propertiesHolder = new LegacyCapabilitiesProvisioning.LegacyProvisioningPropertiesHolder("tcp://localhost:1883",
                                                                                                                                                                   "channel_id",
                                                                                                                                                                   "discovery_domain",
                                                                                                                                                                   "capabilities_domain",
                                                                                                                                                                   "capdir_channel_id");
        LegacyCapabilitiesProvisioning subject = new LegacyCapabilitiesProvisioning(propertiesHolder);
        Address address = subject.getAddressForInterface(GlobalCapabilitiesDirectory.class);
        assertTrue(address instanceof MqttAddress);
        MqttAddress globalCapabilitiesAddress = (MqttAddress) address;
        assertEquals("tcp://localhost:1883", globalCapabilitiesAddress.getBrokerUri());
        assertEquals("capdir_channel_id", globalCapabilitiesAddress.getTopic());
    }

}
