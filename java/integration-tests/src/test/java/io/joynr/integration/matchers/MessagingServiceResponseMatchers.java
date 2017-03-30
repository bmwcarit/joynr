package io.joynr.integration.matchers;

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

import io.joynr.messaging.util.Utilities;

import java.util.List;

import joynr.JoynrMessage;

import org.hamcrest.BaseMatcher;
import org.hamcrest.Description;
import org.hamcrest.Matcher;

public class MessagingServiceResponseMatchers {

    public static Matcher<List<JoynrMessage>> containsMessage(final String msgId) {

        return new BaseMatcher<List<JoynrMessage>>() {

            @Override
            public boolean matches(Object item) {

                @SuppressWarnings("unchecked")
                List<JoynrMessage> messages = (List<JoynrMessage>) item;

                for (JoynrMessage message : messages) {
                    String msgIdInJson = message.getId();

                    if (msgIdInJson != null && msgIdInJson.equals(msgId)) {
                        return true;
                    }
                    ;
                }

                return false;
            }

            @Override
            public void describeTo(Description description) {
                description.appendText("contains message ID '" + msgId + "'");
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
