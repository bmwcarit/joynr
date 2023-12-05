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
package io.joynr.messaging;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import java.util.Arrays;
import java.util.Collection;
import java.util.HashMap;
import java.util.Map;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized.Parameters;
import org.junit.runners.Parameterized;

@RunWith(Parameterized.class)
public class MessagingQosCustomHeadersTest {

    private Boolean conforms;
    private String key;
    private String value;

    public MessagingQosCustomHeadersTest(String key, String value, Boolean conforms) {
        this.key = key;
        this.value = value;
        this.conforms = conforms;
    }

    @Parameters(name = "{index}: {0}:{1}")
    public static Collection<Object[]> data() {
        return Arrays.asList(new Object[][]{ { new String("key"), new String("value"), Boolean.valueOf(true) },
                { new String("1key"), new String("1value"), Boolean.valueOf(true) },
                { new String("key1"), new String("value1"), Boolean.valueOf(true) },
                { new String("key-1"), new String("value1"), Boolean.valueOf(true) },
                { new String("123"), new String("123"), Boolean.valueOf(true) },
                { new String("key"), new String("one two"), Boolean.valueOf(true) },
                { new String("key"), new String("one;two"), Boolean.valueOf(true) },
                { new String("key"), new String("one:two"), Boolean.valueOf(true) },
                { new String("key"), new String("one,two"), Boolean.valueOf(true) },
                { new String("key"), new String("one+two"), Boolean.valueOf(true) },
                { new String("key"), new String("one&two"), Boolean.valueOf(true) },
                { new String("key"), new String("one?two"), Boolean.valueOf(true) },
                { new String("key"), new String("one-two"), Boolean.valueOf(true) },
                { new String("key"), new String("one.two"), Boolean.valueOf(true) },
                { new String("key"), new String("one*two"), Boolean.valueOf(true) },
                { new String("key"), new String("one/two"), Boolean.valueOf(true) },
                { new String("key"), new String("one\\two"), Boolean.valueOf(true) },
                { new String("key"), new String("one_two"), Boolean.valueOf(true) },
                { new String("key"), new String("wrongvalue§"), Boolean.valueOf(false) },
                { new String("key"), new String("wrongvalue$"), Boolean.valueOf(false) },
                { new String("key"), new String("wrongvalue%"), Boolean.valueOf(false) },
                { new String("key"), new String("wrongvalü"), Boolean.valueOf(false) },
                { new String("wrongkey "), new String("value"), Boolean.valueOf(false) },
                { new String("wrongkey;"), new String("value"), Boolean.valueOf(false) },
                { new String("wrongkey:"), new String("value"), Boolean.valueOf(false) },
                { new String("wrongkey,"), new String("value"), Boolean.valueOf(false) },
                { new String("wrongkey+"), new String("value"), Boolean.valueOf(false) },
                { new String("wrongkey&"), new String("value"), Boolean.valueOf(false) },
                { new String("wrongkey?"), new String("value"), Boolean.valueOf(false) },
                { new String("wrongkey."), new String("value"), Boolean.valueOf(false) },
                { new String("wrongkey*"), new String("value"), Boolean.valueOf(false) },
                { new String("wrongkey/"), new String("value"), Boolean.valueOf(false) },
                { new String("wrongkey\\"), new String("value"), Boolean.valueOf(false) }, });
    }

    @Test
    public void putCustomMessageHeaderTest() {
        MessagingQos messagingQos = new MessagingQos();
        try {
            messagingQos.putCustomMessageHeader(key, value);
            assertTrue("key: " + key + " value: " + value + " should have caused an exception", conforms);
            Map<String, String> customMessageHeaders = messagingQos.getCustomMessageHeaders();
            assertTrue(customMessageHeaders.containsKey(key));
            assertEquals(value, customMessageHeaders.get(key));
        } catch (Exception e) {
            assertFalse("key: " + key + " value: " + value + " should not have caused an exception", conforms);
        }
    }

    @Test
    public void putCustomMessageHeadersTest() {
        MessagingQos messagingQos = new MessagingQos();
        Map<String, String> customHeaders = new HashMap<>();
        customHeaders.put(key, value);
        try {
            messagingQos.putAllCustomMessageHeaders(customHeaders);
            assertTrue("key: " + key + " value: " + value + " should have caused an exception", conforms);
            Map<String, String> customMessageHeaders = messagingQos.getCustomMessageHeaders();
            assertTrue(customMessageHeaders.containsKey(key));
            assertEquals(value, customMessageHeaders.get(key));
        } catch (Exception e) {
            assertFalse("key: " + key + " value: " + value + " should not have caused an exception", conforms);
        }
    }
}
