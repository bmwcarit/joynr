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
package io.joynr.test;

import java.util.Properties;

import io.joynr.runtime.JoynrApplicationModule;

public class TestApplicationModule extends JoynrApplicationModule {

    public TestApplicationModule(Properties properties) {
        this(properties, false);
    }

    public TestApplicationModule(Properties properties, boolean overrideJoynDefaultProps) {
        super("testapplication", TestJoynrApplication.class, getProperties(properties, overrideJoynDefaultProps));
    }

    private static Properties getProperties(Properties properties, boolean overrideJoynDefaultProps) {
        Properties fProperties = new Properties();
        if (overrideJoynDefaultProps) {
            fProperties.setProperty(TestJoynrApplication.PROPERTY_TEST_CONFIG_ENTRY, "test-value");
        } else {
            fProperties.setProperty(TestJoynrApplication.PROPERTY_TEST_CONFIG_ENTRY, "default-value");
        }
        fProperties.putAll(properties);
        return fProperties;
    }
}
