/*-
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
package io.joynr.android.messaging.binder;

import androidx.test.ext.junit.runners.AndroidJUnit4;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import java.util.Set;

import org.junit.Test;
import org.mockito.Mock;

import joynr.ImmutableMessage;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.BinderAddress;
import joynr.system.RoutingTypes.MqttAddress;
import org.junit.runner.RunWith;
import org.mockito.junit.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
//@RunWith(AndroidJUnit4.class)
public class BinderMulticastAddressCalculatorTest {
    @Mock
    private BinderAddress binderAddress;

    @Mock
    private MqttAddress mqttAddress;

    @Mock
    private ImmutableMessage immutableMessage;

    @Test
    public void testAcceptBinderAddress() {
        BinderMulticastAddressCalculator subject = new BinderMulticastAddressCalculator(binderAddress);
        Set<Address> resultSet = subject.calculate(immutableMessage);
        assertEquals(1, resultSet.size());
        assertTrue(resultSet.contains(binderAddress));
    }

    @Test
    public void testDenyNonBinderAddress() {
        BinderMulticastAddressCalculator subject = new BinderMulticastAddressCalculator(mqttAddress);
        Set<Address> resultSet = subject.calculate(immutableMessage);
        assertEquals(0, resultSet.size());
    }

    @Test
    public void testSupportsBinderTransport() {
        BinderMulticastAddressCalculator subject = new BinderMulticastAddressCalculator(binderAddress);
        assertTrue(subject.supports("binder"));
    }
}
