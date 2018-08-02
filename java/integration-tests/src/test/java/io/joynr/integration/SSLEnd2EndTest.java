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

import com.google.inject.Module;
import io.joynr.integration.util.DummyJoynrApplication;
import io.joynr.integration.util.SSLSettings;
import io.joynr.integration.util.ServersUtil;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.JoynrRuntime;
import org.eclipse.jetty.server.Server;
import org.junit.After;
import org.junit.AfterClass;
import org.junit.BeforeClass;

import java.io.File;
import java.net.URISyntaxException;
import java.net.URL;
import java.util.ArrayList;
import java.util.List;
import java.util.Properties;

public class SSLEnd2EndTest extends AbstractSSLEnd2EndTest {

    private static Server jettyServer;
    private static String resourcePath;

    private List<DummyJoynrApplication> dummyApplications = new ArrayList<>();

    @Override
    protected JoynrRuntime getRuntime(Properties joynrConfig, Module... modules) {
        DummyJoynrApplication application = (DummyJoynrApplication) new JoynrInjectorFactory(joynrConfig,
                                                                                             modules).createApplication(DummyJoynrApplication.class);

        dummyApplications.add(application);
        return application.getRuntime();
    }

    @BeforeClass
    public static void startServer() throws Exception {

        resourcePath = getResourcePath();

        // Set global SSL properties for all Joynr SSL clients
        System.setProperty("javax.net.ssl.keyStore", resourcePath + "/javaclient.jks");
        System.setProperty("javax.net.ssl.trustStore", resourcePath + "/truststore.jks");
        System.setProperty("javax.net.ssl.keyStorePassword", "changeit");
        System.setProperty("javax.net.ssl.trustStorePassword", "changeit");

        // Uncomment the line below to enable SSL debug
        // System.setProperty("javax.net.debug", "ssl");

        // Set Jetty SSL properties for bounce proxy and discovery directory servlet listeners
        SSLSettings settings = new SSLSettings(resourcePath + "/localhost.jks", // KeyStore
                                               resourcePath + "/truststore.jks", // TrustStore
                                               "changeit", // KeyStore password
                                               "changeit" // TrustStore password
        );

        // keep delays and timeout low for tests
        System.setProperty(ConfigurableMessagingSettings.PROPERTY_SEND_MSG_RETRY_INTERVAL_MS, "10");
        System.setProperty(ConfigurableMessagingSettings.PROPERTY_ARBITRATION_MINIMUMRETRYDELAY, "200");

        provisionDiscoveryDirectoryAccessControlEntries();
        jettyServer = ServersUtil.startSSLServers(settings);

    }

    @After
    public void tearDown() {
        for (DummyJoynrApplication application : dummyApplications) {
            application.shutdown();
        }
        dummyApplications.clear();
    }

    @AfterClass
    public static void stopServer() throws Exception {
        jettyServer.stop();
    }

    // Get the path of the test resources
    private static String getResourcePath() throws URISyntaxException {
        URL resource = ClassLoader.getSystemClassLoader().getResource("truststore.jks");
        File fullPath = new File(resource.toURI());
        return fullPath.getParent();
    }
}
