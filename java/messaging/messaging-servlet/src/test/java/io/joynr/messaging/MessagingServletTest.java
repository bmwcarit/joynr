package io.joynr.messaging;

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

import static com.jayway.restassured.RestAssured.given;
import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.runtime.AbstractJoynrApplication;
import io.joynr.runtime.AcceptsMessageReceiver;
import io.joynr.runtime.JoynrApplication;
import io.joynr.runtime.JoynrApplicationModule;
import io.joynr.runtime.JoynrBaseModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.MessageReceiverType;
import io.joynr.runtime.MessagingServletConfig;
import io.joynr.runtime.PropertyLoader;
import io.joynr.servlet.ServletUtil;

import java.net.InetAddress;
import java.util.Properties;
import java.util.Random;
import java.util.UUID;

import joynr.chat.MessengerSync;
import joynr.chat.messagetypecollection.Message;

import org.eclipse.jetty.server.Server;
import org.eclipse.jetty.servlet.DefaultServlet;
import org.eclipse.jetty.servlet.ServletContextHandler;
import org.eclipse.jetty.servlet.ServletHolder;
import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Ignore;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.servlet.GuiceFilter;
import com.jayway.restassured.config.HttpClientConfig;
import com.jayway.restassured.config.RestAssuredConfig;
import com.jayway.restassured.http.ContentType;
import com.jayway.restassured.specification.RequestSpecification;

/**
 * This test sets up a servlet message receiver in a jetty container. The messaging servlet config will cause all
 * "de.bmw" packages to be seached for instances of AbstractJoynApplication which are then started. In this test,
 * ServletJoynChatApplication should be found and run. The ServletJoynChatApplication is a provider for the
 * chat.Messager interface.
 *
 * NOTE: other classes implementing AbstractJoynApplication may also be found and started if they are on the classpath.
 * Classes that should not be loaded and started should TODO
 *
 * This provider implementation prepends the senderId to the message. The test does a getMessage after the set, and
 * checks that the prepend indeed happened.
 *
 * @author david.katz
 *
 */
public class MessagingServletTest {

    private static final int LOAD_ON_USE = -1;
    private static final String PROPERTIES = "WEB-INF/test.properties";
    protected static final String ROOT = "/*";
    private static final Logger logger = LoggerFactory.getLogger(MessagingServletTest.class);
    String contextId = UUID.randomUUID().toString();

    Random generator = new Random();
    private String providerChannelId;
    // private String consumerChannelId;
    private static Properties testproperties;
    private int port;
    private Server server;
    private static String serverDomain;

    @Before
    @edu.umd.cs.findbugs.annotations.SuppressWarnings(value = "ST_WRITE_TO_STATIC_FROM_INSTANCE_METHOD", justification = "Only a Test. Only one instance will ever exist.")
    public void setUp() throws Exception {
        testproperties = PropertyLoader.loadProperties(PROPERTIES);
        port = ServletUtil.findFreePort();
        logger.info("starting server on port: {}", port);

        // NOTE: MessagingJettyLanucher usually constructs the hostPath from the system properties host and port

        // InetAddress[] allMyIps = InetAddress.getAllByName(InetAddress.getLocalHost().getCanonicalHostName());
        //
        // String hostName = allMyIps[4].getHostAddress();
        String hostName = InetAddress.getLocalHost().getHostAddress();

        System.getProperties().setProperty(MessagingPropertyKeys.PROPERTY_SERVLET_HOST_PATH,
                                           "http://" + hostName + ":" + port);
        logger.debug("setting hostPath to: http://" + hostName + ":" + port);
        serverDomain = "domain_" + UUID.randomUUID().toString();
        System.getProperties().setProperty(AbstractJoynrApplication.PROPERTY_JOYNR_DOMAIN_LOCAL, serverDomain);
        System.getProperties().setProperty(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_REQUEST_TIMEOUT, "" + 5000);

        // Create the server.
        server = new Server(port);
        providerChannelId = "provider-channel-" + UUID.randomUUID().toString();

        ServletContextHandler servletContextHandler = new ServletContextHandler(server, "/");
        servletContextHandler.setInitParameter("channelId", providerChannelId);
        servletContextHandler.setInitParameter("properties", PROPERTIES);

        servletContextHandler.setInitParameter(MessagingServletConfig.INIT_PARAM_SERVLET_MODULE_CLASSNAME,
                                               "io.joynr.runtime.EmptyModule");

        servletContextHandler.addEventListener(new MessagingServletConfig());

        // Then add GuiceFilter and configure the server to
        // reroute all requests through this filter.
        servletContextHandler.addFilter(GuiceFilter.class, "/*", null);

        // Must add DefaultServlet for embedded Jetty.
        // Failing to do this will cause 404 errors.
        // This is not needed if web.xml is used instead.
        ServletHolder servletHolder = servletContextHandler.addServlet(DefaultServlet.class, "/");
        servletHolder.setInitOrder(LOAD_ON_USE);

        // Start the server
        server.start();

        while (!server.isStarted()) {
            Thread.sleep(1000);
        }
        // join causes a wait here until the server shuts down, good if you just want to fire up the server but not run
        // the test
        // server.join();

    }

    @After
    public void tearDown() {
        try {
            server.stop();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    // TODO this test would be nice but is tricky to implement.
    // the directories need to be able to connect directly to the machine running the servlets
    // could work on jenkins but probably won't work on developer machines
    @Ignore
    @Test
    public void testEnd2endServlet() throws InterruptedException {

        // need to ping the server because dont know how to start servlet in jersey test framework with loadonstartup
        // ping();
        String host = System.getProperty("host");
        logger.info(host);

        Properties properties = new Properties();
        properties.put(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_REQUEST_TIMEOUT, "" + 5000);
        JoynrApplication consumer = new JoynrInjectorFactory(new JoynrBaseModule(properties)).createApplication(new JoynrApplicationModule(MessagingServletTestApplication.class));

        Thread.sleep(10000);
        consumer.run();

    }

    /**
     * initialize a RequestSpecification with the given timeout
     *
     * @param timeout_ms
     *            : a SocketTimeoutException will be thrown if no response is received in this many milliseconds
     * @return
     */
    @SuppressWarnings("unused")
    private RequestSpecification onrequest(int timeout_ms) {
        return given().port(port)
                      .contentType(ContentType.JSON)
                      .log()
                      .everything()
                      .config(RestAssuredConfig.config().httpClient(HttpClientConfig.httpClientConfig()
                                                                                    .setParam("http.socket.timeout",
                                                                                              timeout_ms)));
    }

    @AcceptsMessageReceiver(MessageReceiverType.LONGPOLLING)
    public static class MessagingServletTestApplication extends AbstractJoynrApplication {

        @Override
        public void shutdown() {
            // TODO Auto-generated method stub

        }

        @Override
        public void run() {
            ProxyBuilder<MessengerSync> proxyBuilder = runtime.getProxyBuilder(serverDomain, MessengerSync.class);

            MessagingQos messagingQos = new MessagingQos(10000);
            DiscoveryQos discoveryQos = new DiscoveryQos(10000, ArbitrationStrategy.HighestPriority, Long.MAX_VALUE);

            MessengerSync proxy;
            String prefix = "david";
            String randomString = System.currentTimeMillis() + "";
            Message message = null;
            try {
                proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();
                proxy.setMessage(new Message(randomString, System.currentTimeMillis(), prefix));
                message = proxy.getMessage();
            } catch (Exception e) {
                Assert.fail(e.getMessage());
            }

            if (!message.getMessage().startsWith(prefix)) {

                System.err.println(" ERROR ");
                // server.join();
            }

            Assert.assertEquals(prefix, message.getMessage().substring(0, 5));
        }

        public static Properties getApplicationProperties() {
            return testproperties;
        }

    }
}
