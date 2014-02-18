package io.joynr.integration.util;

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

import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.servlet.ServletUtil;

import java.io.IOException;

import org.eclipse.jetty.server.AbstractConnector;
import org.eclipse.jetty.server.Connector;
import org.eclipse.jetty.server.Handler;
import org.eclipse.jetty.server.Server;
import org.eclipse.jetty.server.handler.ContextHandlerCollection;
import org.eclipse.jetty.server.nio.SelectChannelConnector;
import org.eclipse.jetty.webapp.WebAppContext;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class ServersUtil {
    public static final String BOUNCEPROXY_CONTEXT = "/bounceproxy";

    public static final String DISCOVERY_CONTEXT = "/discovery";

    public static final String BOUNCEPROXYCONTROLLER_CONTEXT = "";

    private static final Logger logger = LoggerFactory.getLogger(ServersUtil.class);

    private static void setBounceProxyUrl() {
        String serverUrl = System.getProperties().getProperty("hostPath");
        String bounceProxyUrl = serverUrl + BOUNCEPROXY_CONTEXT + "/";
        System.setProperty(MessagingPropertyKeys.BOUNCE_PROXY_URL, bounceProxyUrl);
    }

    private static void setDirectoriesUrl() {
        String serverUrl = System.getProperties().getProperty("hostPath");
        String directoriesUrl = serverUrl + DISCOVERY_CONTEXT + "/channels/discoverydirectory_channelid/";

        System.setProperty(MessagingPropertyKeys.CAPABILITIESDIRECTORYURL, directoriesUrl);
        System.setProperty(MessagingPropertyKeys.CHANNELURLDIRECTORYURL, directoriesUrl);
    }

    public static Server startServers() throws Exception {
        ContextHandlerCollection contexts = new ContextHandlerCollection();
        contexts.setHandlers(new Handler[]{ createBounceproxyWebApp(), discoveryWebApp() });

        Server server = startServer(contexts);
        setBounceProxyUrl();
        setDirectoriesUrl();
        return server;
    }

    public static Server startBounceproxy() throws Exception {
        ContextHandlerCollection contexts = new ContextHandlerCollection();
        contexts.setHandlers(new Handler[]{ createBounceproxyWebApp() });

        Server server = startServer(contexts);
        setBounceProxyUrl();
        return server;
    }

    public static Server startControlledBounceproxy() throws Exception {
        ContextHandlerCollection contexts = new ContextHandlerCollection();
        contexts.setHandlers(new Handler[]{ createControlledBounceproxyWebApp() });
        return startServer(contexts);
    }

    public static Server startBounceproxyController() throws Exception {
        ContextHandlerCollection contexts = new ContextHandlerCollection();
        contexts.setHandlers(new Handler[]{ createBounceproxyControllerWebApp() });

        Server server = startServer(contexts);
        String serverUrl = System.getProperties().getProperty("hostPath");
        String bounceProxyUrl = serverUrl + "/";
        System.setProperty(MessagingPropertyKeys.BOUNCE_PROXY_URL, bounceProxyUrl);
        return server;
    }

    private static Server startServer(ContextHandlerCollection contexts) throws IOException, Exception {
        final int port = ServletUtil.findFreePort();
        logger.info("PORT: http://localhost:{}", port);
        final Server jettyServer = new Server();
        AbstractConnector connector = new SelectChannelConnector();
        connector.setPort(port);
        connector.setAcceptors(1);
        jettyServer.setConnectors(new Connector[]{ connector });

        String serverUrl = "http://localhost:" + port;
        System.getProperties().setProperty("hostPath", serverUrl);

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

    private static WebAppContext createControlledBounceproxyWebApp() {
        WebAppContext bounceproxyWebapp = new WebAppContext();
        bounceproxyWebapp.setContextPath(BOUNCEPROXY_CONTEXT);
        bounceproxyWebapp.setWar("target/controlled-bounceproxy.war");
        return bounceproxyWebapp;
    }

    private static WebAppContext createBounceproxyControllerWebApp() {
        WebAppContext bounceproxyWebapp = new WebAppContext();
        bounceproxyWebapp.setContextPath(BOUNCEPROXYCONTROLLER_CONTEXT);
        bounceproxyWebapp.setWar("target/bounceproxy-controller.war");
        return bounceproxyWebapp;
    }

    private static WebAppContext discoveryWebApp() {
        WebAppContext discoveryWebapp = new WebAppContext();
        discoveryWebapp.setContextPath(DISCOVERY_CONTEXT);
        discoveryWebapp.setWar("target/discovery.war");
        return discoveryWebapp;
    }

}
