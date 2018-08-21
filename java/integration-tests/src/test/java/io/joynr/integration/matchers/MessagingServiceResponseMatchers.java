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
package io.joynr.integration.matchers;

import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.List;

import org.hamcrest.BaseMatcher;
import org.hamcrest.Description;
import org.hamcrest.Matcher;

import io.joynr.messaging.util.Utilities;
import io.joynr.smrf.EncodingException;
import joynr.ImmutableMessage;

public class MessagingServiceResponseMatchers {

    public static Matcher<List<ImmutableMessage>> containsPayload(final String payload) {
        return new BaseMatcher<List<ImmutableMessage>>() {
            @Override
            public boolean matches(Object item) {
                @SuppressWarnings("unchecked")
                List<ImmutableMessage> messages = (List<ImmutableMessage>) item;
                byte[] binaryPayload = payload.getBytes(StandardCharsets.UTF_8);

                for (ImmutableMessage message : messages) {
                    try {
                        if (Arrays.equals(message.getUnencryptedBody(), binaryPayload)) {
                            return true;
                        }
                    } catch (EncodingException e) {
                        e.printStackTrace();
                        return false;
                    }
                }
                return false;
            }

            @Override
            public void describeTo(Description arg0) {
            }

        };
    }

    public static Matcher<String> isMessageUrlwithJsessionId(final String baseUrl,
                                                             final String msgId,
                                                             final String jsessionId,
                                                             final String sessionIdName) {

        return new BaseMatcher<String>() {

            @Override
            public boolean matches(Object item) {
                String url = (String) item;
                return url.equals(getExpectedUrl());
            }

            @Override
            public void describeTo(Description description) {
                description.appendText("equals '" + getExpectedUrl() + "'");
            }

            private String getExpectedUrl() {
                return Utilities.getSessionEncodedUrl(baseUrl + "messages/" + msgId, sessionIdName, jsessionId);
            }

        };
    }
}
