package io.joynr.messaging;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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

import joynr.ImmutableMessage;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
public class LongPollingHttpMulticastAddressCalculatorTest {

    @Mock
    private LongPollingHttpGlobalAddressFactory globalAddressFactory;

    @Mock
    private ImmutableMessage joynrMessage;

    private LongPollingHttpMulticastAddressCalculator subject;

    @Before
    public void setup() {
        subject = new LongPollingHttpMulticastAddressCalculator(globalAddressFactory);
    }

    @Test
    public void testSupportsLongPolling() {
        assertTrue(subject.supports(LongPollingHttpGlobalAddressFactory.SUPPORTED_TRANSPORT_LONGPOLLING));
    }

    @Test(expected = UnsupportedOperationException.class)
    public void testCalculate() {
        subject.calculate(joynrMessage);
    }
}
