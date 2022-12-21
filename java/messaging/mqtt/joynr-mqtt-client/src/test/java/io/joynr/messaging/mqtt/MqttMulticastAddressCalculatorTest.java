/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
package io.joynr.messaging.mqtt;

import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.when;

import java.util.Set;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

import joynr.ImmutableMessage;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.MqttAddress;

@RunWith(MockitoJUnitRunner.class)
public class MqttMulticastAddressCalculatorTest {

    @Mock
    private ImmutableMessage immutableMessage;

    @Mock
    private MqttTopicPrefixProvider mqttTopicPrefixProvider;

    private String[] gbidArray = { "gbid1", "gbid2" };

    @Test
    public void testCalculateAddressesWithGbids() {
        when(mqttTopicPrefixProvider.getMulticastTopicPrefix()).thenReturn("multicastTopic");
        when(immutableMessage.getRecipient()).thenReturn("Recipient");
        MqttMulticastAddressCalculator subject = new MqttMulticastAddressCalculator(gbidArray, mqttTopicPrefixProvider);
        Set<Address> resultSet = subject.calculate(immutableMessage);
        assertTrue(gbidArray.length == resultSet.size());
        for (String gbid : gbidArray) {
            boolean found = false;
            for (Address address : resultSet) {
                if (gbid.equals(((MqttAddress) address).getBrokerUri())) {
                    found = true;
                }
            }
            assertTrue(found);
        }
    }

    @Test
    public void testSupportsMqttTransport() {
        MqttMulticastAddressCalculator subject = new MqttMulticastAddressCalculator(gbidArray, mqttTopicPrefixProvider);
        assertTrue(subject.supports("mqtt"));
    }

}
