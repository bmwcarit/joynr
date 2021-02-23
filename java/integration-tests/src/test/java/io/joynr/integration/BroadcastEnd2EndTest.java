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
package io.joynr.integration;

import java.util.ArrayList;
import java.util.List;
import java.util.Properties;

import org.junit.After;
import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Ignore;

import com.google.inject.Module;
import com.google.inject.util.Modules;

import io.joynr.integration.util.DummyJoynrApplication;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.JoynrRuntime;

@Ignore("HTTP does not support binary messages (SMRF)")
public class BroadcastEnd2EndTest extends AbstractBroadcastEnd2EndTest {

    private List<DummyJoynrApplication> dummyApplications = new ArrayList<>();

    private static Properties originalProperties;

    @BeforeClass
    public static void startServer() throws Exception {
        originalProperties = System.getProperties();
        // keep delays and timeout low for tests
        System.setProperty(ConfigurableMessagingSettings.PROPERTY_SEND_MSG_RETRY_INTERVAL_MS, "10");
        System.setProperty(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_MINIMUM_RETRY_INTERVAL_MS, "200");

        provisionDiscoveryDirectoryAccessControlEntries();

    }

    @Override
    protected JoynrRuntime getRuntime(Properties joynrConfig, Module... modules) {
        Module runtimeModule = new CCInProcessRuntimeModule();
        Module modulesWithRuntime = Modules.override(modules).with(runtimeModule);
        DummyJoynrApplication application = (DummyJoynrApplication) new JoynrInjectorFactory(joynrConfig,
                                                                                             modulesWithRuntime).createApplication(DummyJoynrApplication.class);

        dummyApplications.add(application);
        return application.getRuntime();
    }

    @Override
    @After
    public void tearDown() throws Exception {
        super.tearDown();
        for (DummyJoynrApplication application : dummyApplications) {
            application.shutdown();
        }
        dummyApplications.clear();
    }

    @AfterClass
    public static void stopServer() throws Exception {
        System.setProperties(originalProperties);
    }
}
