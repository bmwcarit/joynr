/*
 * #%L
 * %%
 * Copyright (C) 2021 BMW Car IT GmbH
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
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertThat;

import java.util.HashMap;
import java.util.Map;

import org.junit.Before;
import org.junit.Test;

import io.joynr.exceptions.JoynrIllegalStateException;

public class ImmutableMessageTest {
    private MutableMessage testMessage = new MutableMessage();
    private Map<String, String> expectedPrefixedCustomHeadersWithExtraOnes = new HashMap<String, String>();

    private Map<String, String> expectedCustomHeaders = new HashMap<String, String>();

    @Before
    public void setup() throws Exception {
        final String prefixedCustomHeaderKey1 = "c-header1";
        final String prefixedCustomHeaderKey2 = "c-header2";
        final String prefixedExtraCustomHeaderKey = "c-vin";

        final String customHeaderValue1 = "value1";
        final String customHeaderValue2 = "value2";
        final String customHeaderValue3 = "1N6SD16Y8PC417372";

        expectedPrefixedCustomHeadersWithExtraOnes.put(prefixedCustomHeaderKey1, customHeaderValue1);
        expectedPrefixedCustomHeadersWithExtraOnes.put(prefixedCustomHeaderKey2, customHeaderValue2);
        expectedPrefixedCustomHeadersWithExtraOnes.put(prefixedExtraCustomHeaderKey, customHeaderValue3);

        expectedCustomHeaders.put("header1", customHeaderValue1);
        expectedCustomHeaders.put("header2", customHeaderValue2);
        expectedCustomHeaders.put("vin", customHeaderValue3);

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
    }

    @Test
    public void testToLogMessageWithNullValues() throws Exception {
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
        assertEquals(expectedPrefixedCustomHeaders, actualPrefixedCustomHeaders);

        // check getter with prefix
        Map<String, String> actualCustomHeaders = immutableMessage.getCustomHeaders();
        assertEquals(expectedCustomHeaders, actualCustomHeaders);
    }

    @Test
    public void testExtraCustomHeadersGetter() throws Exception {
        ImmutableMessage immutableMessage1 = testMessage.getImmutableMessage();
        // we set prefixed custom headers along with extra ones
        immutableMessage1.setPrefixedExtraCustomHeaders(expectedPrefixedCustomHeadersWithExtraOnes);

        // we set striped prefix of custom headers along with extra ones 
        assertEquals(expectedCustomHeaders, immutableMessage1.getExtraCustomHeaders());

        ImmutableMessage immutableMessage2 = testMessage.getImmutableMessage();
        // set custom headers which their prefix is already stripped out 
        immutableMessage2.setExtraCustomHeaders(expectedCustomHeaders);

        // get the custom headers
        assertEquals(expectedCustomHeaders, immutableMessage2.getExtraCustomHeaders());
    }

    @Test
    public void testExtraCustomHeadersSetter_succeeded_on_non_prefixed_headers() throws Exception {
        ImmutableMessage immutableMessage = testMessage.getImmutableMessage();
        immutableMessage.setExtraCustomHeaders(expectedCustomHeaders);
        assertEquals(expectedCustomHeaders, immutableMessage.getExtraCustomHeaders());
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void testExtraCustomHeadersSetter_throws_on_non_prefixed_headers() throws Exception {
        ImmutableMessage immutableMessage = testMessage.getImmutableMessage();
        expectedCustomHeaders.put("c-anykey", "anyvalue");
        immutableMessage.setExtraCustomHeaders(expectedCustomHeaders);
    }

}
