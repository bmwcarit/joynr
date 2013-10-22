package io.joynr.integration.util;

import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.servlet.ServletUtil;

import org.eclipse.jetty.server.Handler;
import org.eclipse.jetty.server.Server;
import org.eclipse.jetty.server.handler.ContextHandlerCollection;
import org.eclipse.jetty.webapp.WebAppContext;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class ServersUtil {
    public static final String BOUNCEPROXY_CONTEXT = "/bounceproxy";

    public static final String DISCOVERY_CONTEXT = "/discovery";

    private static final Logger logger = LoggerFactory.getLogger(ServersUtil.class);

    public static Server startServers() throws Exception {

        int port = ServletUtil.findFreePort();
        logger.info("PORT: http://localhost:{}", port);
        Server jettyServer = new Server(port);

        WebAppContext discoveryWebapp = new WebAppContext();
        discoveryWebapp.setContextPath(DISCOVERY_CONTEXT);
        discoveryWebapp.setWar("target/discoverydirectoryservlet.war");
        jettyServer.setHandler(discoveryWebapp);

        WebAppContext bounceproxyWebapp = new WebAppContext();
        bounceproxyWebapp.setContextPath(BOUNCEPROXY_CONTEXT);
        bounceproxyWebapp.setWar("target/bounceproxy.war");

        ContextHandlerCollection contexts = new ContextHandlerCollection();
        contexts.setHandlers(new Handler[]{ bounceproxyWebapp, discoveryWebapp });

        jettyServer.setHandler(contexts);

        jettyServer.start();

        String serverUrl = "http://localhost:" + port;
        String bounceProxyUrl = serverUrl + BOUNCEPROXY_CONTEXT + "/";
        String directoriesUrl = serverUrl + DISCOVERY_CONTEXT + "/channels/discoverydirectory_channelid/";
        System.setProperty(MessagingPropertyKeys.BOUNCE_PROXY_URL, bounceProxyUrl);
        System.setProperty(MessagingPropertyKeys.CAPABILITIESDIRECTORYURL, directoriesUrl);
        System.setProperty(MessagingPropertyKeys.CHANNELURLDIRECTORYURL, directoriesUrl);

        return jettyServer;
    }

}
