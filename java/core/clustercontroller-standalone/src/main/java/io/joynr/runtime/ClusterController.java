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
package io.joynr.runtime;

import java.io.Console;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.Properties;
import java.util.Set;

import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.CommandLineParser;
import org.apache.commons.cli.DefaultParser;
import org.apache.commons.cli.HelpFormatter;
import org.apache.commons.cli.Option;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Injector;
import com.google.inject.Module;
import com.google.inject.util.Modules;

import io.joynr.capabilities.LocalCapabilitiesDirectory;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.mqtt.hivemq.client.HivemqMqttClientModule;
import io.joynr.messaging.websocket.WebsocketModule;
import joynr.types.DiscoveryEntry;

public class ClusterController {
    private static final Logger logger = LoggerFactory.getLogger(ClusterController.class);
    private static Properties webSocketConfig;
    private static JoynrRuntime runtime;
    private static final Object lock = new Object();

    public static void main(String[] args) {
        int port = 4242;
        String host = "localhost";
        String transport = null;
        String brokerUri = null;
        Options options = new Options();
        Options helpOptions = new Options();
        setupOptions(options, helpOptions);
        CommandLine line;
        CommandLineParser parser = new DefaultParser();

        // check for '-h' option alone first. This is required in order to avoid
        // reports about missing other args when using only '-h', which should supported
        // to just get help / usage info.
        try {
            line = parser.parse(helpOptions, args);

            if (line.hasOption('h')) {
                HelpFormatter formatter = new HelpFormatter();
                // use 'options' here to print help about all possible parameters
                formatter.printHelp(ClusterController.class.getName(), options, true);
                System.exit(0);
            }
        } catch (ParseException e) {
            // ignore, since any option except '-h' will cause this exception
        }

        try {
            line = parser.parse(options, args);

            if (line.hasOption('p')) {
                port = Integer.parseInt(line.getOptionValue('p'));
                logger.info("found port = " + port);
            }
            if (line.hasOption('H')) {
                host = line.getOptionValue('H');
                logger.info("found host = " + host);
            }
            if (line.hasOption('t')) {
                transport = line.getOptionValue('t').toLowerCase();
                logger.info("found transport = " + transport);
            }
            if (line.hasOption('b')) {
                brokerUri = line.getOptionValue('b');
                logger.info("found brokerUri = " + brokerUri);
            }
        } catch (ParseException e) {
            logger.error("failed to parse command line: " + e);
            HelpFormatter formatter = new HelpFormatter();
            formatter.printHelp(ClusterController.class.getName(), options, true);
            System.exit(1);
        }

        webSocketConfig = new Properties();
        webSocketConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_HOST, host);
        webSocketConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PORT, "" + port);
        webSocketConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PROTOCOL, "ws");
        webSocketConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PATH, "/");
        Properties ccConfig = new Properties();
        ccConfig.putAll(webSocketConfig);
        ccConfig.setProperty(ConfigurableMessagingSettings.PROPERTY_CC_CONNECTION_TYPE, "WEBSOCKET");
        Module runtimeModule = new CCWebSocketRuntimeModule();

        if (transport != null) {
            Module backendTransportModules = Modules.EMPTY_MODULE;
            if (transport.contains("atmosphere") || transport.contains("http")) {
                logger.error("Atmosphere/HTTP transport is no longer supported.");
            }
            if (transport.contains("mqtt")) {
                if (brokerUri != null) {
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
                }
                backendTransportModules = Modules.combine(backendTransportModules, new HivemqMqttClientModule());
            }
            runtimeModule = Modules.override(runtimeModule).with(backendTransportModules);
        }

        Injector injectorCC = new JoynrInjectorFactory(ccConfig, runtimeModule).getInjector();

        runtime = injectorCC.getInstance(JoynrRuntime.class);
        LocalCapabilitiesDirectory capabilitiesDirectory = injectorCC.getInstance(LocalCapabilitiesDirectory.class);

        Thread shutdownHook = new Thread() {
            @Override
            public void run() {
                logger.info("executing shutdown hook");
                synchronized (lock) {
                    logger.info("notifying any waiting thread from shutdown hook");
                    lock.notifyAll();
                }
                logger.info("shutting down");
                runtime.shutdown(false);
                logger.info("shutdown completed");
            }
        };
        logger.info("adding shutdown hook");
        Runtime.getRuntime().addShutdownHook(shutdownHook);

        Console console = System.console();
        if (console != null) {
            String command = "";
            while (!command.equals("q")) {
                command = console.readLine();

                if (command.equals("caps")) {
                    Set<DiscoveryEntry> allLocalDiscoveryEntries = capabilitiesDirectory.listLocalCapabilities();
                    StringBuffer discoveryEntriesAsText = new StringBuffer();
                    for (DiscoveryEntry capability : allLocalDiscoveryEntries) {
                        discoveryEntriesAsText.append(capability.toString()).append('\n');
                    }
                    logger.info(discoveryEntriesAsText.toString());
                } else {
                    logger.info("\n\nUSAGE press\n" + " q\tto quit\n caps\tto list registered providers\n");
                }
            }
        } else {
            logger.info("\n\nNon-interactive mode detected.\n"
                    + "This cluster controller will continue to run until its JVM gets terminated\n"
                    + "by the operating system. This can be triggered by sending a SIGTERM signal\n"
                    + "to the process running the JVM.");
            synchronized (lock) {
                logger.info("waiting on shutdown hook");
                while (shutdownHook.isAlive()) {
                    try {
                        lock.wait();
                    } catch (InterruptedException e) {
                        logger.error("Thread interrupted.", e);
                        Thread.currentThread().interrupt();
                    }
                }
            }
        }
        System.exit(0);
    }

    private static void setupOptions(Options options, Options helpOptions) {
        Option optionPort = Option.builder("p")
                                  .required(false)
                                  .argName("port")
                                  .desc("the websocket port (optional, default: 4242)")
                                  .longOpt("port")
                                  .hasArg(true)
                                  .numberOfArgs(1)
                                  .type(Number.class)
                                  .build();
        Option optionHelp = Option.builder("h")
                                  .required(false)
                                  .desc("print this message")
                                  .longOpt("help")
                                  .hasArg(false)
                                  .build();
        Option optionHost = Option.builder("H")
                                  .required(false)
                                  .argName("host")
                                  .desc("the websocket host (optional, default: localhost)")
                                  .longOpt("host")
                                  .hasArg(true)
                                  .numberOfArgs(1)
                                  .type(String.class)
                                  .build();
        Option optionTransport = Option.builder("t")
                                       .required(false)
                                       .argName("transport")
                                       .desc("the transport (optional, select 'mqtt' to establish connection with broker)")
                                       .longOpt("transport")
                                       .hasArg(true)
                                       .numberOfArgs(1)
                                       .type(String.class)
                                       .build();
        Option optionBrokerUri = Option.builder("b")
                                       .required(false)
                                       .argName("brokerUri")
                                       .desc("the broker uri")
                                       .longOpt("brokerUri")
                                       .hasArg(true)
                                       .numberOfArgs(1)
                                       .type(String.class)
                                       .build();

        options.addOption(optionPort);
        options.addOption(optionHelp);
        options.addOption(optionHost);
        options.addOption(optionTransport);
        options.addOption(optionBrokerUri);
        helpOptions.addOption(optionHelp);
    }
}
