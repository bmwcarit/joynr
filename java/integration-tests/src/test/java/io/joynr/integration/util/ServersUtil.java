/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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
package io.joynr.integration.util;

import static io.joynr.integration.matchers.MonitoringServiceResponseMatchers.containsBounceProxy;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;
import java.util.Properties;

import org.eclipse.jetty.server.Connector;
import org.eclipse.jetty.server.Handler;
import org.eclipse.jetty.server.HttpConfiguration;
import org.eclipse.jetty.server.HttpConnectionFactory;
import org.eclipse.jetty.server.SecureRequestCustomizer;
import org.eclipse.jetty.server.Server;
import org.eclipse.jetty.server.ServerConnector;
import org.eclipse.jetty.server.SslConnectionFactory;
import org.eclipse.jetty.server.handler.ContextHandlerCollection;
import org.eclipse.jetty.util.ssl.SslContextFactory;
import org.eclipse.jetty.webapp.Configuration;
import org.eclipse.jetty.webapp.WebAppContext;
import org.eclipse.jetty.webapp.WebInfConfiguration;
import org.eclipse.jetty.webapp.WebXmlConfiguration;
import org.hamcrest.CoreMatchers;
import org.hamcrest.Matcher;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper.DefaultTyping;
import com.jayway.restassured.RestAssured;
import com.jayway.restassured.path.json.JsonPath;
import com.jayway.restassured.response.Response;

import io.joynr.capabilities.CapabilityUtils;
import io.joynr.capabilities.StaticCapabilitiesProvisioning;
import io.joynr.integration.setup.SystemPropertyServletConfiguration;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.inprocess.InProcessAddress;
import io.joynr.servlet.ServletUtil;
import io.joynr.util.ObjectMapper;
import joynr.infrastructure.GlobalCapabilitiesDirectory;
import joynr.infrastructure.GlobalDomainAccessController;
import joynr.system.RoutingTypes.ChannelAddress;
import joynr.types.DiscoveryEntry;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.Version;

public class ServersUtil {
    public static final String BOUNCEPROXY_CONTEXT = "/bounceproxy";

    public static final String DISCOVERY_CONTEXT = "/discovery";
    public static final String ACCESSCONTROL_CONTEXT = "/accesscontrol";

    public static final String BOUNCEPROXYCONTROLLER_CONTEXT = "/controller";

    private static final Logger logger = LoggerFactory.getLogger(ServersUtil.class);

    private static void setBounceProxyUrl() {
        if (System.getProperty(MessagingPropertyKeys.BOUNCE_PROXY_URL) != null) {
            // use existing bounceproxy
            return;
        }
        String serverUrl = System.getProperty(MessagingPropertyKeys.PROPERTY_SERVLET_HOST_PATH);
        if (serverUrl.endsWith("/")) {
            serverUrl = serverUrl.substring(0, serverUrl.length() - 1);
        }
        String bounceProxyUrl = serverUrl + BOUNCEPROXY_CONTEXT + "/";
        System.setProperty(MessagingPropertyKeys.BOUNCE_PROXY_URL, bounceProxyUrl);
    }

    private static void setDirectoriesUrl() {
        String serverUrl = System.getProperty(MessagingPropertyKeys.PROPERTY_SERVLET_HOST_PATH);
        if (serverUrl.endsWith("/")) {
            serverUrl = serverUrl.substring(0, serverUrl.length() - 1);
        }
        String directoriesUrl = serverUrl + DISCOVERY_CONTEXT + "/channels/discoverydirectory_channelid/";
        try {
            File tmpFile = File.createTempFile("provisioned_capabilities", ".json");
            tmpFile.deleteOnExit();
            try (FileWriter writer = new FileWriter(tmpFile)) {
                String provisionedCapabilitiesJson = createJsonFor(directoriesUrl);
                logger.debug("Writing capabilities JSON: {} to file: {}",
                             provisionedCapabilitiesJson,
                             tmpFile.getAbsolutePath());
                writer.write(provisionedCapabilitiesJson);
                writer.flush();
            }
            System.setProperty(StaticCapabilitiesProvisioning.PROPERTY_PROVISIONED_CAPABILITIES_FILE,
                               tmpFile.getAbsolutePath());
        } catch (IOException e) {
            logger.error("Unable to create temporary file with provisioned capabilities JSON.", e);
        }
    }

    private static String createJsonFor(String directoriesUrl) throws JsonProcessingException {
        ObjectMapper objectMapper = new ObjectMapper();
        objectMapper.enableDefaultTypingAsProperty(DefaultTyping.JAVA_LANG_OBJECT, "_typeName");
        setObjectMapperOnCapabilitiesUtils(objectMapper);
        String channelId = "discoverydirectory_channelid";
        String participantId = "capabilitiesdirectory_participantid";
        GlobalDiscoveryEntry discoveryEntry = CapabilityUtils.newGlobalDiscoveryEntry(new Version(0, 1),
                                                                                      "io.joynr",
                                                                                      GlobalCapabilitiesDirectory.INTERFACE_NAME,
                                                                                      participantId,
                                                                                      new ProviderQos(),
                                                                                      System.currentTimeMillis(),
                                                                                      Long.MAX_VALUE,
                                                                                      "",
                                                                                      new ChannelAddress(directoriesUrl,
                                                                                                         channelId));
        String accessParticipantId = "domainaccesscontroller_participantid";
        GlobalDiscoveryEntry accessControlEntry = CapabilityUtils.newGlobalDiscoveryEntry(new Version(0, 1),
                                                                                          "io.joynr",
                                                                                          GlobalDomainAccessController.INTERFACE_NAME,
                                                                                          accessParticipantId,
                                                                                          new ProviderQos(),
                                                                                          System.currentTimeMillis(),
                                                                                          Long.MAX_VALUE,
                                                                                          "",
                                                                                          new InProcessAddress());
        List<DiscoveryEntry> entries = new ArrayList<>();
        entries.add(discoveryEntry);
        entries.add(accessControlEntry);
        return objectMapper.writeValueAsString(entries);
    }

    private static void setObjectMapperOnCapabilitiesUtils(ObjectMapper objectMapper) {
        try {
            Field objectMapperField = CapabilityUtils.class.getDeclaredField("objectMapper");
            objectMapperField.setAccessible(true);
            objectMapperField.set(CapabilityUtils.class, objectMapper);
        } catch (Exception e) {
            logger.error("Unable to set object mapper on CapabilitiesUtils class.", e);
        }
    }

    public static Server startServers() throws Exception {
        ContextHandlerCollection contexts = new ContextHandlerCollection();
        contexts.setHandlers(new Handler[]{ createBounceproxyWebApp(), discoveryWebApp(), accessControlWebApp() });

        System.setProperty("log4j.configuration",
                           Thread.currentThread()
                                 .getContextClassLoader()
                                 .getResource("log4j_backend.properties")
                                 .toString());

        Server server = startServer(contexts);
        return server;
    }

    public static Server startSSLServers(SSLSettings settings) throws Exception {
        final int port = ServletUtil.findFreePort();
        return startSSLServers(settings, port);
    }

    public static Server startSSLServers(SSLSettings settings, int port) throws Exception {
        ContextHandlerCollection contexts = new ContextHandlerCollection();
        contexts.setHandlers(new Handler[]{ createBounceproxyWebApp(), discoveryWebApp(), accessControlWebApp() });
        Server server = startSSLServer(contexts, settings, port);
        setBounceProxyUrl();
        setDirectoriesUrl();

        return server;
    }

    public static Server startBounceproxy() throws Exception {
        ContextHandlerCollection contexts = new ContextHandlerCollection();
        contexts.setHandlers(new Handler[]{ createBounceproxyWebApp() });

        final int port = ServletUtil.findFreePort();
        Server server = startServer(contexts, port);
        return server;
    }

    public static Server startControlledBounceproxy(String bpId) throws Exception {

        final int port = ServletUtil.findFreePort();
        final String bpUrl = "http://localhost:" + port + "/bounceproxy/";

        System.setProperty("joynr.bounceproxy.id", bpId);
        System.setProperty("joynr.bounceproxy.controller.baseurl",
                           System.getProperty(MessagingPropertyKeys.BOUNCE_PROXY_URL));
        System.setProperty("joynr.bounceproxy.url4cc", bpUrl);
        System.setProperty("joynr.bounceproxy.url4bpc", bpUrl);

        ContextHandlerCollection contexts = new ContextHandlerCollection();
        contexts.setHandlers(new Handler[]{ createControlledBounceproxyWebApp("", null) });
        Server server = startServer(contexts, port);

        System.clearProperty("joynr.bounceproxy.id");
        System.clearProperty("joynr.bounceproxy.controller.baseurl");
        System.clearProperty("joynr.bounceproxy.url4cc");
        System.clearProperty("joynr.bounceproxy.url4bpc");

        return server;
    }

    public static Server startBounceproxyController() throws Exception {
        return startBounceproxyController("bounceproxy-controller-nonclustered");
    }

    public static Server startClusteredBounceproxyController() throws Exception {
        return startBounceproxyController("bounceproxy-controller-clustered");
    }

    private static Server startBounceproxyController(String warFileName) throws Exception {
        ContextHandlerCollection contexts = new ContextHandlerCollection();
        contexts.setHandlers(new Handler[]{ createBounceproxyControllerWebApp(warFileName, "", null) });

        final int port = ServletUtil.findFreePort();
        Server server = startServer(contexts, port);
        String serverUrl = "http://localhost:" + port;
        String bounceProxyUrl = serverUrl + "/controller/";
        System.setProperty(MessagingPropertyKeys.BOUNCE_PROXY_URL, bounceProxyUrl);
        return server;
    }

    private static Server startServer(ContextHandlerCollection contexts) throws Exception {
        final int port = ServletUtil.findFreePort();
        return startServer(contexts, port);
    }

    private static Server startServer(ContextHandlerCollection contexts, int port) throws Exception {
        System.setProperty(MessagingPropertyKeys.PROPERTY_SERVLET_HOST_PATH, "http://localhost:" + port);
        setBounceProxyUrl();
        setDirectoriesUrl();
        logger.info("HOST PATH: {}", System.getProperty(MessagingPropertyKeys.PROPERTY_SERVLET_HOST_PATH));

        final Server jettyServer = new Server();
        ServerConnector connector = new ServerConnector(jettyServer,
                                                        new HttpConnectionFactory(new HttpConfiguration()));
        connector.setPort(port);
        connector.setAcceptQueueSize(1);
        jettyServer.setConnectors(new Connector[]{ connector });

        jettyServer.setHandler(contexts);
        jettyServer.start();

        logger.trace("Started jetty server: {}", jettyServer.dump());

        return jettyServer;
    }

    private static Server startSSLServer(ContextHandlerCollection contexts,
                                         SSLSettings settings,
                                         int port) throws IOException, Exception {

        System.setProperty(MessagingPropertyKeys.PROPERTY_SERVLET_HOST_PATH, "http://localhost:" + port);
        logger.info("PORT: {}", System.getProperty(MessagingPropertyKeys.PROPERTY_SERVLET_HOST_PATH));
        final Server jettyServer = new Server();

        HttpConfiguration https_config = new HttpConfiguration();
        https_config.setSecureScheme("https");
        https_config.setSecurePort(port);
        https_config.setOutputBufferSize(32768);
        https_config.addCustomizer(new SecureRequestCustomizer());

        // Configure SSL
        final SslContextFactory contextFactory = new SslContextFactory();
        contextFactory.setKeyStorePath(settings.getKeyStorePath());
        contextFactory.setTrustStorePath(settings.getTrustStorePath());
        contextFactory.setKeyStorePassword(settings.getKeyStorePassword());
        contextFactory.setTrustStorePassword(settings.getKeyStorePassword());
        contextFactory.setNeedClientAuth(true);

        // Create and use an SSL connector
        ServerConnector connector = new ServerConnector(jettyServer,
                                                        new SslConnectionFactory(contextFactory, "http/1.1"),
                                                        new HttpConnectionFactory(https_config));
        connector.setPort(port);
        connector.setAcceptQueueSize(1);
        jettyServer.setConnectors(new Connector[]{ connector });

        String serverUrl = "https://localhost:" + port;
        System.getProperties().setProperty(MessagingPropertyKeys.PROPERTY_SERVLET_HOST_PATH, serverUrl);

        jettyServer.setHandler(contexts);
        jettyServer.start();

        return jettyServer;
    }

    private static WebAppContext createBounceproxyWebApp() {
        WebAppContext bounceproxyWebapp = new WebAppContext();
        bounceproxyWebapp.setContextPath(BOUNCEPROXY_CONTEXT);
        bounceproxyWebapp.setWar("target/bounceproxy.war");
        return bounceproxyWebapp;
    }

    public static WebAppContext createControlledBounceproxyWebApp(String parentContext, Properties props) {
        WebAppContext bounceproxyWebapp = new WebAppContext();
        bounceproxyWebapp.setContextPath(createContextPath(parentContext, BOUNCEPROXY_CONTEXT));
        bounceproxyWebapp.setWar("target/controlled-bounceproxy.war");

        if (props != null) {
            bounceproxyWebapp.setConfigurations(new Configuration[]{ new WebInfConfiguration(),
                    new WebXmlConfiguration(), new SystemPropertyServletConfiguration(props) });
        }

        // Makes jetty load classes in the same order as JVM. Otherwise there's
        // a conflict loading loggers.
        bounceproxyWebapp.setParentLoaderPriority(true);

        return bounceproxyWebapp;
    }

    public static WebAppContext createBounceproxyControllerWebApp(String warFileName,
                                                                  String parentContext,
                                                                  Properties props) {
        WebAppContext bounceproxyWebapp = new WebAppContext();
        bounceproxyWebapp.setContextPath(createContextPath(parentContext, BOUNCEPROXYCONTROLLER_CONTEXT));
        bounceproxyWebapp.setWar("target/" + warFileName + ".war");

        if (props != null) {
            bounceproxyWebapp.setConfigurations(new Configuration[]{ new WebInfConfiguration(),
                    new WebXmlConfiguration(), new SystemPropertyServletConfiguration(props) });
        }
        // Makes jetty load classes in the same order as JVM. Otherwise there's
        // a conflict loading loggers.
        bounceproxyWebapp.setParentLoaderPriority(true);

        return bounceproxyWebapp;
    }

    /**
     * Creates a context path with slashes set at the right positions, i.e. a
     * leading slash, a single slash between each context and no slash at the
     * end.
     *
     * @param contexts
     *            the contexts to add to the path. The contexts are added in the
     *            same order as given as parameters.
     * @return
     */
    private static String createContextPath(String... contexts) {

        StringBuffer resultContext = new StringBuffer();

        for (String context : contexts) {

            if (!(context == null || context.isEmpty())) {
                if (!context.startsWith("/")) {
                    resultContext.append("/");
                }

                if (context.endsWith("/")) {
                    resultContext.append(context.substring(0, context.length() - 2));
                } else {
                    resultContext.append(context);
                }
            }
        }

        return resultContext.toString();
    }

    private static WebAppContext discoveryWebApp() {
        WebAppContext discoveryWebapp = new WebAppContext();
        discoveryWebapp.setContextPath(DISCOVERY_CONTEXT);
        discoveryWebapp.setWar("target/discovery.war");
        return discoveryWebapp;
    }

    private static WebAppContext accessControlWebApp() {
        WebAppContext accessControlWebapp = new WebAppContext();
        accessControlWebapp.setContextPath(ACCESSCONTROL_CONTEXT);
        accessControlWebapp.setWar("target/accesscontrol.war");
        return accessControlWebapp;
    }

    /**
     * Waits until all bounce proxies are registered with a single bounce proxy
     * controller or until the timeout is reached.
     *
     * @param timeout_ms
     *            the timeout in milliseconds
     * @param wait_time_ms
     *            the time to wait between two requests to the bounceproxy
     *            controller to get a list of registered bounceproxies (in
     *            milliseconds)
     * @param bpControllerUrl
     *            the bounceproxy controller base url to retrieve the list of
     *            registered bounceproxies from
     * @param bpIds
     *            the list of bounce proxy IDs to check for their registration
     * @return <code>true</code> if all bounceproxies are registered at the
     *         bounceproxy controller and the timeout hasn't expired yet,
     *         <code>false</code> if timeout has expired before all
     *         bounceproxies were registered.
     * @throws InterruptedException
     *             if {@link Thread#sleep(long)} between two calls to the
     *             bounceproxy controller was interrupted
     */
    public static boolean waitForBounceProxyRegistration(long timeout_ms,
                                                         long wait_time_ms,
                                                         String bpControllerUrl,
                                                         String... bpIds) throws InterruptedException {

        logger.debug("Wait for registration of bounceproxies at controller URL {} for {} ms",
                     bpControllerUrl,
                     timeout_ms);

        long start = System.currentTimeMillis();

        while (!areBounceProxiesRegisteredWithController(bpControllerUrl, bpIds)) {

            if (System.currentTimeMillis() - start > timeout_ms) {
                return false;
            }

            Thread.sleep(1000l);
        }
        return true;
    }

    private static boolean areBounceProxiesRegisteredWithController(String bpControllerUrl, String... bpIds) {

        List<Matcher<? super JsonPath>> containsBounceProxyMatcher = new LinkedList<Matcher<? super JsonPath>>();

        for (String bpId : bpIds) {
            Matcher<? super JsonPath> matcher = CoreMatchers.anyOf(containsBounceProxy(bpId, "ALIVE"),
                                                                   containsBounceProxy(bpId, "ACTIVE"));
            containsBounceProxyMatcher.add(matcher);
        }

        Matcher<JsonPath> matcher = CoreMatchers.allOf(containsBounceProxyMatcher);

        Response bounceProxiesResponse = getBounceProxies(bpControllerUrl);
        return matcher.matches(bounceProxiesResponse.jsonPath());
    }

    private static Response getBounceProxies(String bpControllerUrl) {

        String previousBaseUri = RestAssured.baseURI;
        RestAssured.baseURI = bpControllerUrl;
        Response bounceProxiesResponse = RestAssured.given().get("bounceproxies");
        RestAssured.baseURI = previousBaseUri;
        return bounceProxiesResponse;
    }

}
