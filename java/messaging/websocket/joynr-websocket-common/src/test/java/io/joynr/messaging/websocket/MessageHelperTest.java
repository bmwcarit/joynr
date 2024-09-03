/*
 * #%L
 * %%
 * Copyright (C) 2024 BMW Car IT GmbH
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
package io.joynr.messaging.websocket;

import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertArrayEquals;

import io.joynr.exceptions.JoynrRuntimeException;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.junit.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
public class MessageHelperTest {

    private static final byte[] PAYLOAD = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    @Test(expected = JoynrRuntimeException.class)
    public void testShouldThrowExceptionIfPayloadIsNull() {
        MessageHelper.extractMessage(null, 0, 10);
    }

    @Test(expected = JoynrRuntimeException.class)
    public void testShouldThrowExceptionIfOffsetIsLessThanZero() {
        MessageHelper.extractMessage(PAYLOAD, -1, 10);
    }

    @Test(expected = JoynrRuntimeException.class)
    public void testShouldThrowExceptionIfOffsetIsGreaterThanPayloadSize() {
        MessageHelper.extractMessage(PAYLOAD, 20, 10);
    }

    @Test
    public void testShouldExtractMessageIfParametersAreCorrect() {
        final byte[] message = MessageHelper.extractMessage(PAYLOAD, 0, 2);
        assertNotNull(message);
        assertArrayEquals(message, new byte[]{ 0, 1 });
    }

}
