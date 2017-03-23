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

import org.hamcrest.BaseMatcher;
import org.hamcrest.Description;
import org.hamcrest.Matcher;

import com.jayway.restassured.path.json.JsonPath;

public class ChannelServiceResponseMatchers {

    public static Matcher<JsonPath> containsChannel(final String channelId) {

        return new BaseMatcher<JsonPath>() {

            @Override
            public boolean matches(Object item) {

                JsonPath jsonPath = (JsonPath) item;

                List<String> channelIds = jsonPath.getList("name");

                for (String c : channelIds) {
                    if (c.equals(channelId)) {
                        return true;
                    }
                }

                return false;
            }

            @Override
            public void describeMismatch(Object item, Description description) {
                JsonPath jsonPath = (JsonPath) item;
                description.appendText(jsonPath.prettyPrint());
            }

            @Override
            public void describeTo(Description description) {
                description.appendText("contains channel ID '" + channelId + "'");
            }

        };
    }

    public static Matcher<JsonPath> numberOfChannels(final int size) {

        return new BaseMatcher<JsonPath>() {

            @Override
            public boolean matches(Object item) {

                JsonPath jsonPath = (JsonPath) item;

                List<String> channelIds = jsonPath.getList("name");
                return channelIds.size() == size;
            }

            @Override
            public void describeTo(Description description) {
                description.appendText("contains " + size + " channels");
            }

            @Override
            public void describeMismatch(Object item, Description description) {
                JsonPath jsonPath = (JsonPath) item;
                List<String> channelIds = jsonPath.getList("name");
                description.appendText("contains " + channelIds.size() + " channels");
            }

        };
    }

    public static Matcher<String> isChannelUrlwithJsessionId(final String baseUrl,
                                                             final String channelId,
                                                             final String sessionIdName) {

        return new BaseMatcher<String>() {

            @Override
            public boolean matches(Object item) {

                String url = (String) item;

                if (!Utilities.isSessionEncodedInUrl(url, sessionIdName)) {
                    return false;
                }

                String urlWithoutJsessionid = Utilities.getUrlWithoutSessionId(url, sessionIdName);
                if (!urlWithoutJsessionid.equals(baseUrl + "channels/" + channelId + "/")) {
                    return false;
                }

                return true;
            }

            @Override
            public void describeTo(Description description) {
                description.appendText("starts with '" + baseUrl + "channels/" + channelId + "/;jsessionid=<sessionid>"
                        + "'");
            }

        };
    }

}
