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

import org.hamcrest.BaseMatcher;
import org.hamcrest.Description;
import org.hamcrest.Matcher;

import com.jayway.restassured.path.json.JsonPath;

public class MonitoringServiceResponseMatchers {

    public static Matcher<JsonPath> containsBounceProxy(final String id, final String status) {

        return new BaseMatcher<JsonPath>() {

            @Override
            public boolean matches(Object item) {

                JsonPath jsonPath = (JsonPath) item;

                for (int i = 0; i < jsonPath.getList("").size(); i++) {

                    if (jsonPath.get("[" + i + "].status").equals(status)
                            && jsonPath.get("[" + i + "].bounceProxyId").equals(id)) {
                        return true;
                    }
                }

                return false;
            }

            @Override
            public void describeTo(Description description) {
                description.appendText("contains entry with status=" + status + " and bounceProxyId=" + id);
            }

            @Override
            public void describeMismatch(final Object item, final Description description) {
                description.appendText("was").appendValue(((JsonPath) item).get(""));
            }
        };
    }
}
