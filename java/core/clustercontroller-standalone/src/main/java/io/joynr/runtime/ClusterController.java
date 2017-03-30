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

import io.joynr.capabilities.LocalCapabilitiesDirectory;
import io.joynr.messaging.AtmosphereMessagingModule;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.mqtt.paho.client.MqttPahoModule;
import io.joynr.messaging.websocket.WebsocketModule;

import java.io.IOException;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.Properties;
import java.util.Set;

import jline.console.ConsoleReader;
import joynr.types.DiscoveryEntry;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Injector;
import com.google.inject.Module;
import com.google.inject.util.Modules;

public class ClusterController {
    private static final Logger LOG = LoggerFactory.getLogger(ClusterController.class);
    private static Properties webSocketConfig;
    private static JoynrRuntime runtime;

    public static void main(String[] args) {
        final int port = 4242;
        webSocketConfig = new Properties();
        webSocketConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_HOST, "localhost");
        webSocketConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PORT, "" + port);
        webSocketConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PROTOCOL, "ws");
        webSocketConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PATH, "");
        Properties ccConfig = new Properties();
        ccConfig.putAll(webSocketConfig);
        ccConfig.setProperty(ConfigurableMessagingSettings.PROPERTY_CC_CONNECTION_TYPE, "WEBSOCKET");
        Module runtimeModule = new CCWebSocketRuntimeModule();

        if (args.length > 0) {
            String transport = args[0].toLowerCase();
            Module backendTransportModules = Modules.EMPTY_MODULE;
            if (transport.contains("atmosphere") || transport.contains("http")) {
                backendTransportModules = Modules.combine(backendTransportModules, new AtmosphereMessagingModule());
            }
            if (transport.contains("mqtt")) {
                if (args.length > 1) {
                    String brokerUri = args[1];
                    try {
                        URI uri = new URI(brokerUri);
                        if (uri.getAuthority() == null || uri.getHost() == null || uri.getPort() < 0) {
                            throw new URISyntaxException(brokerUri, "host, authority or port was not set");
                        }
                    } catch (URISyntaxException e) {
                        System.err.println(brokerUri
                                + " is not a valid URI for the MQTT broker. Expecting for example: tcp://localhost:1883 Error: "
                                + e.getMessage());
                        System.exit(1);
                    }
                    ccConfig.put("joynr.messaging.mqtt.brokerUri", brokerUri);
                    ccConfig.put("joynr.messaging.primaryglobaltransport", "mqtt");
                }
                backendTransportModules = Modules.combine(backendTransportModules, new MqttPahoModule());
            }
            runtimeModule = Modules.override(runtimeModule).with(backendTransportModules);
        }

        Injector injectorCC = new JoynrInjectorFactory(ccConfig, runtimeModule).getInjector();

        runtime = injectorCC.getInstance(JoynrRuntime.class);
        LocalCapabilitiesDirectory capabilitiesDirectory = injectorCC.getInstance(LocalCapabilitiesDirectory.class);

        Thread shutdownHook = new Thread() {
            @Override
            public void run() {
                LOG.info("executing shutdown hook");
                synchronized (this) {
                    LOG.info("notifying any waiting thread from shutdown hook");
                    notifyAll();
                }
                LOG.info("shutting down");
                runtime.shutdown(false);
                LOG.info("shutdown completed");
            }
        };
        LOG.info("adding shutdown hook");
        Runtime.getRuntime().addShutdownHook(shutdownHook);

        if (System.console() != null) {
            ConsoleReader console;
            try {
                console = new ConsoleReader();
                String command = "";
                while (!command.equals("q")) {
                    command = console.readLine();

                    if (command.equals("caps")) {
                        Set<DiscoveryEntry> allLocalDiscoveryEntries = capabilitiesDirectory.listLocalCapabilities();
                        StringBuffer discoveryEntriesAsText = new StringBuffer();
                        for (DiscoveryEntry capability : allLocalDiscoveryEntries) {
                            discoveryEntriesAsText.append(capability.toString()).append('\n');
                        }
                        LOG.info(discoveryEntriesAsText.toString());
                    } else {
                        LOG.info("\n\nUSAGE press\n" + " q\tto quit\n caps\tto list registered providers\n");
                    }
                }
            } catch (IOException e) {
                LOG.error("error reading input from console", e);
            }
        } else {
            LOG.info("\n\nNon-interactive mode detected.\n"
                    + "This cluster controller will continue to run until its JVM gets terminated\n"
                    + "by the operating system. This can be triggered by sending a SIGTERM signal\n"
                    + "to the process running the JVM.");
            synchronized (shutdownHook) {
                LOG.info("waiting on shutdown hook");
                try {
                    shutdownHook.wait();
                } catch (InterruptedException e) {
                    // ignore
                }
            }
        }
        System.exit(0);
    }
}
