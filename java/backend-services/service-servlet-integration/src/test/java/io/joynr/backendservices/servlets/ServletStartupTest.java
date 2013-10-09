package io.joynr.backendservices.servlets;

/*
 * #%L
 * joynr::java::backend-services::service-servlet-integration
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

import io.joynr.bounceproxy.LocalGrizzlyBounceProxy;
import io.joynr.capabilities.RegistrationFuture;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.runtime.AbstractJoynrApplication;
import io.joynr.runtime.JoynrApplicationModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.MessagingServletConfig;
import io.joynr.servlet.ServletUtil;

import java.util.Properties;

import joynr.tests.DefaultTestProvider;
import joynr.tests.TestSync;

import org.eclipse.jetty.server.Server;
import org.eclipse.jetty.servlet.DefaultServlet;
import org.eclipse.jetty.servlet.ServletContextHandler;
import org.eclipse.jetty.servlet.ServletHolder;
import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.servlet.GuiceFilter;

public class ServletStartupTest {

    private static final Logger logger = LoggerFactory.getLogger(ServletStartupTest.class);

    private int servletsPort;

    protected String root = "/*";

    private Server server;

    private LocalGrizzlyBounceProxy localGrizzlyBounceProxy;

    private int bpPort;

    private TestApp testApp;

    @Before
    public void startUp() throws Exception {

        localGrizzlyBounceProxy = new LocalGrizzlyBounceProxy();
        localGrizzlyBounceProxy.start();

        bpPort = localGrizzlyBounceProxy.getPort();

        System.setProperty(AbstractJoynrApplication.PROPERTY_JOYNR_DOMAIN_LOCAL, "ServletStartupTest_localTestDomain");

        servletsPort = ServletUtil.findFreePort();
        String hostAddress = "127.0.0.1";
        System.setProperty("hostPath", "http://" + hostAddress + ":" + servletsPort);
        System.setProperty("joynr.messaging.capabilitiesDirectoryUrl", "http://" + hostAddress + ":" + servletsPort
                + "/channels/directories/");
        System.setProperty("joynr.messaging.channelUrlDirectoryUrl", "http://" + hostAddress + ":" + servletsPort
                + "/channels/directories/");
        System.setProperty("joynr.messaging.bounceProxyUrl", "http://" + hostAddress + ":" + bpPort + "/bounceproxy/");
        // Create the server.
        server = new Server(servletsPort);

        // providerChannelId = "provider-channel-" + UUID.randomUUID().toString();

        ServletContextHandler servletContextHandler = new ServletContextHandler(server, "/");

        servletContextHandler.setInitParameter("channelId", "directories");

        servletContextHandler.setInitParameter(MessagingServletConfig.SERVLET_MODULE_CLASSNAME,
                                               "io.joynr.servlet.ServletModule");

        server.setHandler(servletContextHandler);

        // setting properties is optional
        String properties = System.getProperty("props");
        if (properties != null) {
            servletContextHandler.setInitParameter("properties", properties);
        }

        servletContextHandler.addEventListener(new MessagingServletConfig());

        // Then add GuiceFilter and configure the server to
        // reroute all requests through this filter.
        servletContextHandler.addFilter(GuiceFilter.class, "/*", null);

        // Must add DefaultServlet for embedded Jetty.
        // Failing to do this will cause 404 errors.
        // This is not needed if web.xml is used instead.
        ServletHolder servletHolder = servletContextHandler.addServlet(DefaultServlet.class, "/");
        servletHolder.setInitOrder(-1);

        // Start the server
        server.start();
        logger.debug("************************************");
        logger.debug("BP started on port: " + bpPort);
        logger.debug("Servlet server started on port: " + servletsPort);
        logger.debug("************************************");
        while (!server.isStarted()) {
            logger.debug("waiting on server initialization...");
            Thread.sleep(1000);
        }
        // wait for localGrizzlyBounceProxy
        Thread.sleep(5000);
    }

    @After
    public void tearDown() throws Exception {
        if (server != null) {
            server.stop();
        }
        logger.debug("server stopped");
    }

    @Test
    public void testRegisterAndUnregister() throws InterruptedException {
        Properties appProperties = new Properties();
        appProperties.setProperty(MessagingPropertyKeys.CHANNELID, "ServletStartupTest_channelId");
        testApp = (TestApp) new JoynrInjectorFactory(appProperties).createApplication(new JoynrApplicationModule(TestApp.class));

        logger.debug("registerAndUnregister started");
        DefaultTestProvider dummy = new DefaultTestProvider();
        RegistrationFuture registerCapabilityFuture = testApp.getRuntime().registerCapability("testDomain",
                                                                                              dummy,
                                                                                              TestSync.class,
                                                                                              "testToken");

        Assert.assertTrue(registerCapabilityFuture.waitForFullRegistration(30000));
        testApp.getRuntime().unregisterCapability("testDomain", dummy, TestSync.class, "testToken");
    }

}
