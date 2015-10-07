package io.joynr.integration;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

import com.google.inject.Injector;
import com.google.inject.Module;
import io.joynr.integration.util.ServersUtil;
import io.joynr.messaging.ConfigurableMessagingSettings;

import io.joynr.runtime.JoynrInjectorFactory;
import org.eclipse.jetty.server.Server;
import org.junit.AfterClass;
import org.junit.BeforeClass;

import java.util.Properties;

//import io.joynr.util.PreconfiguredEndpointDirectoryModule;

/**
 * Tests the interaction of the dispatcher and communication manager.
 */
public class MessagingIntegrationTestIgnored extends AbstractMessagingIntegrationTest {

    private static Server jettyServer;

    @BeforeClass
    public static void startServer() throws Exception {
        System.setProperty(ConfigurableMessagingSettings.PROPERTY_SEND_MSG_RETRY_INTERVAL_MS, "10");
        System.setProperty(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_REQUEST_TIMEOUT, "200");
        System.setProperty(ConfigurableMessagingSettings.PROPERTY_ARBITRATION_MINIMUMRETRYDELAY, "200");

        jettyServer = ServersUtil.startBounceproxy();

    }

    @AfterClass
    public static void stopServer() throws Exception {
        jettyServer.stop();
    }

    @Override
    public Injector createInjector(Properties joynrConfig, Module... modules) {
        return new JoynrInjectorFactory(joynrConfig, modules).getInjector();
    }
}
