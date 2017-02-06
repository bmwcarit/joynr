package io.joynr.integration;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

import com.google.inject.Module;
import com.google.inject.util.Modules;

import io.joynr.integration.util.DummyJoynrApplication;
import io.joynr.integration.util.ServersUtil;
import io.joynr.messaging.AtmosphereMessagingModule;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.JoynrRuntime;
import org.eclipse.jetty.server.Server;
import org.junit.After;
import org.junit.AfterClass;
import org.junit.BeforeClass;

import java.util.ArrayList;
import java.util.List;
import java.util.Properties;

public class SubscriptionEnd2EndTest extends AbstractSubscriptionEnd2EndTest {

    private static Server jettyServer;
    private static Properties originalProperties;

    private List<DummyJoynrApplication> dummyApplications = new ArrayList<>();

    @BeforeClass
    public static void startServer() throws Exception {
        originalProperties = System.getProperties();
        System.setProperty(MessagingPropertyKeys.PROPERTY_SERVLET_SKIP_LONGPOLL_DEREGISTRATION, "true");
        // keep delays and timeout low for tests
        System.setProperty(ConfigurableMessagingSettings.PROPERTY_SEND_MSG_RETRY_INTERVAL_MS, "10");
        System.setProperty(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_REQUEST_TIMEOUT, "200");
        System.setProperty(ConfigurableMessagingSettings.PROPERTY_ARBITRATION_MINIMUMRETRYDELAY, "200");

        provisionDiscoveryDirectoryAccessControlEntries();
        jettyServer = ServersUtil.startServers();
    }

    @Override
    protected JoynrRuntime getRuntime(Properties joynrConfig, Module... modules) {
        Module runtimeModule = Modules.override(new CCInProcessRuntimeModule()).with(new AtmosphereMessagingModule());
        Module modulesWithRuntime = Modules.override(modules).with(runtimeModule);
        DummyJoynrApplication application = (DummyJoynrApplication) new JoynrInjectorFactory(joynrConfig,
                                                                                             modulesWithRuntime).createApplication(DummyJoynrApplication.class);

        dummyApplications.add(application);
        return application.getRuntime();
    }

    @Override
    @After
    public void tearDown() throws InterruptedException {
        super.tearDown();
        for (DummyJoynrApplication application : dummyApplications) {
            application.shutdown();
        }
        dummyApplications.clear();
    }

    @AfterClass
    public static void stopServer() throws Exception {
        System.setProperties(originalProperties);

        jettyServer.stop();
    }
}
