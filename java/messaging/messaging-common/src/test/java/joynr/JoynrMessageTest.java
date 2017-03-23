package joynr;

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

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import java.util.HashMap;
import java.util.Map;

import org.junit.Test;

public class JoynrMessageTest {

    @Test
    public void testCustomHeaders() {
        JoynrMessage joynrMessage = new JoynrMessage();
        String headerName = "header";
        String headerValue = "value";
        String customHeaderName = JoynrMessage.MESSAGE_CUSTOM_HEADER_PREFIX + headerName;
        joynrMessage.setHeaderValue(customHeaderName, headerValue);
        joynrMessage.setHeaderValue(headerName, headerValue);
        Map<String, String> customHeaders = joynrMessage.getCustomHeaders();
        assertTrue(customHeaders.containsKey(headerName));
        assertFalse(customHeaders.containsKey(customHeaderName));
    }

    @Test
    public void testSetCustomHeaders() {
        JoynrMessage joynrMessage = new JoynrMessage();
        String headerName = "header";
        String headerValue = "value";
        String customHeaderName = JoynrMessage.MESSAGE_CUSTOM_HEADER_PREFIX + headerName;
        Map<String, String> expectedCustomHeaders = new HashMap<>();
        expectedCustomHeaders.put(headerName, headerValue);
        joynrMessage.setCustomHeaders(expectedCustomHeaders);
        Map<String, String> customHeaders = joynrMessage.getCustomHeaders();
        assertTrue(customHeaders.containsKey(headerName));
        assertFalse(customHeaders.containsKey(customHeaderName));
        assertTrue(joynrMessage.getHeader().containsKey(customHeaderName));
    }

    @Test
    public void testSetCustomHeadersWithCustomInKeyName() {
        JoynrMessage joynrMessage = new JoynrMessage();
        String headerName = "header-custom";
        String headerValue = "value";
        String customHeaderName = JoynrMessage.MESSAGE_CUSTOM_HEADER_PREFIX + headerName;
        Map<String, String> expectedCustomHeaders = new HashMap<>();
        expectedCustomHeaders.put(headerName, headerValue);
        joynrMessage.setCustomHeaders(expectedCustomHeaders);
        Map<String, String> customHeaders = joynrMessage.getCustomHeaders();
        assertTrue(customHeaders.containsKey(headerName));
        assertFalse(customHeaders.containsKey(customHeaderName));
        assertTrue(joynrMessage.getHeader().containsKey(customHeaderName));
    }
}
