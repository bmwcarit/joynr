package io.joynr.test;

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

import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.runtime.AbstractJoynrApplication;

import com.google.inject.Inject;
import com.google.inject.name.Named;

public class TestJoynrApplication extends AbstractJoynrApplication {
    public static final String PROPERTY_TEST_CONFIG_ENTRY = "joynrapp.test.config.entry";

    @Inject
    @Named(PROPERTY_TEST_CONFIG_ENTRY)
    public String testConfigEntry;

    @Inject
    @Named(MessagingPropertyKeys.BOUNCE_PROXY_URL)
    public String bounceProxyUrl;

    @Override
    public void run() {

    }

}
