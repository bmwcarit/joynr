package io.joynr.runtime;

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

import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.servlet.ServletUtil;

import java.util.Random;
import java.util.Scanner;
import java.util.UUID;
import java.util.regex.Pattern;

import org.eclipse.jetty.server.Server;
import org.eclipse.jetty.servlet.DefaultServlet;
import org.eclipse.jetty.servlet.ServletContextHandler;
import org.eclipse.jetty.servlet.ServletHolder;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.servlet.GuiceFilter;

public class MessagingJettyLauncher {

    protected static final String ROOT = "/*";
    private static final Logger logger = LoggerFactory.getLogger(MessagingJettyLauncher.class);
    String contextId = UUID.randomUUID().toString();

    Random generator = new Random();
    private static String providerChannelId;
    private static int port = 0;
    private static Server server;

    @edu.umd.cs.findbugs.annotations.SuppressWarnings(
                                                      value = "DM_DEFAULT_ENCODING",
                                                      justification = "Just reading key-input, encoding does not matter here")
    public static void main(String[] args) throws Exception {

        String host = System.getProperty("host");
        if (host == null) {
            throw new IllegalArgumentException("host must be set. eg. -Dhost=http://myserver");
        }

        String portString = System.getProperty("port");
        if (portString != null) {
            try {
                port = Integer.parseInt(portString);
            } catch (NumberFormatException e) {
                logger.error("port must be a valid integer");
            }
        }

        if (port == 0) {
            port = ServletUtil.findFreePort();
        }

        logger.info("starting server on port: {}", port);

        String hostPath = host + ":" + port;
        System.setProperty(MessagingPropertyKeys.PROPERTY_SERVLET_HOST_PATH, hostPath);

        // Create the server.
        server = new Server(port);
        providerChannelId = "provider-channel-" + UUID.randomUUID().toString();

        ServletContextHandler servletContextHandler = new ServletContextHandler(server, "/");
        servletContextHandler.setInitParameter("channelId", providerChannelId);

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

        // stop the server when q is pressed
        Scanner input = new Scanner(System.in);
        Pattern pattern = Pattern.compile("q");
        // wait until the user types q to quit
        input.next(pattern);

        input.close();
        try {
            server.stop();
            server.join();
        } catch (Exception e) {
            // do nothing as we don't want tests to fail only because
            // stopping of the server did not work
        }
    }
}
