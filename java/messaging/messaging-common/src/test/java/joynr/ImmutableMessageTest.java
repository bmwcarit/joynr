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
package joynr;

import static org.hamcrest.core.StringContains.containsString;
import static org.junit.Assert.assertThat;
import static org.junit.Assert.assertTrue;

import java.util.HashMap;
import java.util.Map;

import org.junit.Test;

public class ImmutableMessageTest {
    @Test
    public void testToLogMessageWithNullValues() throws Exception {
        MutableMessage testMessage = new MutableMessage();

        // Set all values to null which may be null
        testMessage.setCompressed(false);
        testMessage.setEffort(null);
        testMessage.setLocalMessage(false);
        testMessage.setPayload(new byte[]{ 0, 1, 2 });
        testMessage.setRecipient("");
        testMessage.setSender("");
        testMessage.setReplyTo(null);
        testMessage.setTtlAbsolute(false);
        testMessage.setTtlMs(0);
        testMessage.setType(null);

        ImmutableMessage immutableMessage = testMessage.getImmutableMessage();

        // Expect no exception is thrown
        immutableMessage.toLogMessage();
    }

    @Test
    public void testPayloadIsWrittenAsString() throws Exception {
        final String payload = "$=payload=$";
        MutableMessage testMessage = new MutableMessage();

        testMessage.setPayload(payload.getBytes());
        testMessage.setRecipient("test");
        testMessage.setSender("test");

        String logMessage = testMessage.getImmutableMessage().toLogMessage();

        assertThat(logMessage, containsString(payload));
    }

    @Test
    public void testAllCustomHeadersGetter() throws Exception {
        final String payload = "$=payload=$";
        MutableMessage testMessage = new MutableMessage();
        final String TEST_CUSTOM_HEADER_KEY_1 = "testCustHeaderKey1";
        final String TEST_CUSTOM_HEADER_VALUE_1 = "testCustHeaderValue1";
        final String TEST_CUSTOM_HEADER_KEY_2 = "testCustHeaderKey2";
        final String TEST_CUSTOM_HEADER_VALUE_2 = "testCustHeaderValue2";

        testMessage.setPayload(payload.getBytes());
        testMessage.setRecipient("test");
        testMessage.setSender("test");

        Map<String, String> expectedCustomHeaders = new HashMap<>();
        expectedCustomHeaders.put(TEST_CUSTOM_HEADER_KEY_1, TEST_CUSTOM_HEADER_VALUE_1);
        expectedCustomHeaders.put(TEST_CUSTOM_HEADER_KEY_2, TEST_CUSTOM_HEADER_VALUE_2);
        testMessage.setCustomHeaders(expectedCustomHeaders);

        Map<String, String> expectedPrefixedCustomHeaders = new HashMap<>();
        expectedPrefixedCustomHeaders.put(Message.CUSTOM_HEADER_PREFIX + TEST_CUSTOM_HEADER_KEY_1,
                                          TEST_CUSTOM_HEADER_VALUE_1);
        expectedPrefixedCustomHeaders.put(Message.CUSTOM_HEADER_PREFIX + TEST_CUSTOM_HEADER_KEY_2,
                                          TEST_CUSTOM_HEADER_VALUE_2);

        ImmutableMessage immutableMessage = testMessage.getImmutableMessage();

        // check getter without prefix
        Map<String, String> actualPrefixedCustomHeaders = immutableMessage.getPrefixedCustomHeaders();
        assertTrue(actualPrefixedCustomHeaders.equals(expectedPrefixedCustomHeaders));

        // check getter with prefix
        Map<String, String> actualCustomHeaders = immutableMessage.getCustomHeaders();
        assertTrue(actualCustomHeaders.equals(expectedCustomHeaders));
    }
}
