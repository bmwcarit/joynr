package io.joynr.messaging.websocket;

import java.io.IOException;

import javax.inject.Named;

import org.eclipse.jetty.http.HttpVersion;
import org.eclipse.jetty.server.Server;
import org.eclipse.jetty.server.ServerConnector;
import org.eclipse.jetty.server.SslConnectionFactory;
import org.eclipse.jetty.servlet.ServletContextHandler;
import org.eclipse.jetty.servlet.ServletHolder;
import org.eclipse.jetty.util.ssl.SslContextFactory;
import org.eclipse.jetty.websocket.servlet.ServletUpgradeRequest;
import org.eclipse.jetty.websocket.servlet.ServletUpgradeResponse;
import org.eclipse.jetty.websocket.servlet.WebSocketCreator;
import org.eclipse.jetty.websocket.servlet.WebSocketServlet;
import org.eclipse.jetty.websocket.servlet.WebSocketServletFactory;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.Inject;

import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.routing.MessageRouter;
import joynr.system.RoutingTypes.WebSocketAddress;
import joynr.system.RoutingTypes.WebSocketClientAddress;
import joynr.system.RoutingTypes.WebSocketProtocol;

/*
 *
 * Creates a Jetty server with an websocket servlet. WebSocketMessagingStubs and LibWebSocketMassagingSkeletons will connect to it.
 */
public class CCWebSocketMessagingSkeleton extends WebSocketMessagingSkeleton {

    private static final Logger logger = LoggerFactory.getLogger(CCWebSocketMessagingSkeleton.class);
    private WebSocketAddress address;
    private WebSocketClientMessagingStubFactory webSocketMessagingStubFactory;

    @Inject
    public CCWebSocketMessagingSkeleton(@Named(ConfigurableMessagingSettings.PROPERTY_CC_MESSAGING_ADDRESS) WebSocketAddress address,
                                        ObjectMapper objectMapper,
                                        MessageRouter messageRouter,
                                        WebSocketClientMessagingStubFactory webSocketMessagingStubFactory) {
        super(objectMapper, messageRouter);
        this.address = address;
        this.webSocketMessagingStubFactory = webSocketMessagingStubFactory;
    }

    @Override
    public void onWebSocketText(String json) {
        if (isInitializationMessage(json)) {
            try {
                WebSocketClientAddress webSocketClientAddress = objectMapper.readValue(json,
                                                                                       WebSocketClientAddress.class);
                webSocketMessagingStubFactory.addSession(webSocketClientAddress, getSession());
            } catch (IOException e) {
                logger.error("Error parsing WebSocketClientAddress: ", e);
            }
        } else {
            super.onWebSocketText(json);
        }
    }

    /*
     * Checks if the message contains a WebSockeClientAddress of a libjoynr instance.
     * @return true if the json object's _typeName field contains joynr.system.WebSocketClientAddress
     */
    private boolean isInitializationMessage(String json) {
        return json.startsWith("{\"_typeName\":\"joynr.system.RoutingTypes.WebSocketClientAddress\"");
    }

    @Override
    public void initializeConnection() {
        Server server = new Server();
        ServerConnector connector;

        if (address.getProtocol().equals(WebSocketProtocol.WSS)) {
            SslContextFactory sslContextFactory = new SslContextFactory();
            SslConnectionFactory sslConnectionFactory = new SslConnectionFactory(sslContextFactory,
                                                                                 HttpVersion.HTTP_1_1.asString());
            connector = new ServerConnector(server, sslConnectionFactory);
        } else {
            connector = new ServerConnector(server);
        }
        connector.setPort(address.getPort());
        server.addConnector(connector);

        ServletContextHandler context = new ServletContextHandler(ServletContextHandler.SESSIONS);
        context.setContextPath("/");
        server.setHandler(context);

        ServletHolder holderEvents = new ServletHolder("ws-events", new WebSocketServlet() {
            @Override
            public void configure(WebSocketServletFactory webSocketServletFactory) {
                webSocketServletFactory.setCreator(new WebSocketCreator() {
                    @Override
                    public Object createWebSocket(ServletUpgradeRequest servletUpgradeRequest,
                                                  ServletUpgradeResponse servletUpgradeResponse) {
                        return CCWebSocketMessagingSkeleton.this;
                    }
                });
            }
        });

        context.addServlet(holderEvents, address.getPath());

        try {
            server.start();
        } catch (Throwable t) {
            logger.error("Error while starting websocket server: ", t);
        }
    }
}
