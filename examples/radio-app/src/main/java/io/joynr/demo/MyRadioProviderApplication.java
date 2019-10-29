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
package io.joynr.demo;

import java.util.Properties;
import java.util.Scanner;

import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.CommandLineParser;
import org.apache.commons.cli.DefaultParser;
import org.apache.commons.cli.HelpFormatter;
import org.apache.commons.cli.Option;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.ObjectMapper.DefaultTyping;
import com.google.inject.Inject;
import com.google.inject.Module;
import com.google.inject.util.Modules;

import edu.umd.cs.findbugs.annotations.SuppressWarnings;
import io.joynr.accesscontrol.StaticDomainAccessControlProvisioning;
import io.joynr.accesscontrol.StaticDomainAccessControlProvisioningModule;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.AtmosphereMessagingModule;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.mqtt.paho.client.MqttPahoModule;
import io.joynr.messaging.websocket.WebsocketModule;
import io.joynr.provider.ProviderAnnotations;
import io.joynr.proxy.Future;
import io.joynr.runtime.AbstractJoynrApplication;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.CCWebSocketRuntimeModule;
import io.joynr.runtime.JoynrApplication;
import io.joynr.runtime.JoynrApplicationModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.LibjoynrWebSocketRuntimeModule;
import joynr.exceptions.ApplicationException;
import joynr.infrastructure.DacTypes.MasterAccessControlEntry;
import joynr.infrastructure.DacTypes.Permission;
import joynr.infrastructure.DacTypes.TrustLevel;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;

public class MyRadioProviderApplication extends AbstractJoynrApplication {
    private static final Logger LOG = LoggerFactory.getLogger(MyRadioProviderApplication.class);
    public static final String STATIC_PERSISTENCE_FILE = "provider-joynr.properties";

    private MyRadioProvider provider = null;

    @Inject
    private ObjectMapper jsonSerializer;

    @Inject
    private ProviderScope providerScope;

    public static void main(String[] args) throws Exception {
        // run application from cmd line using Maven:
        // mvn exec:java -Dexec.mainClass="io.joynr.demo.MyRadioProviderApplication" -Dexec.args="<arguments>"
        // where arguments provided as
        // -d provider-domain [-h websocket-host] [-p websocket-port] [-t [(websocket | websocketCC):[http]:[mqtt]] [-l]

        ProviderScope tmpProviderScope = ProviderScope.GLOBAL;
        String host = "localhost";
        int port = 4242;
        String localDomain = "domain";
        String transport = null;

        CommandLine line;
        Options options = new Options();
        Options helpOptions = new Options();
        setupOptions(options, helpOptions);
        CommandLineParser parser = new DefaultParser();

        // check for '-h' option alone first. This is required in order to avoid
        // reports about missing other args when using only '-h', which should supported
        // to just get help / usage info.
        try {
            line = parser.parse(helpOptions, args);

            if (line.hasOption('h')) {
                HelpFormatter formatter = new HelpFormatter();
                // use 'options' here to print help about all possible parameters
                formatter.printHelp(MyRadioProviderApplication.class.getName(), options, true);
                System.exit(0);
            }
        } catch (ParseException e) {
            // ignore, since any option except '-h' will cause this exception
        }

        try {
            line = parser.parse(options, args);

            if (line.hasOption('d')) {
                localDomain = line.getOptionValue('d');
                LOG.info("found domain = " + localDomain);
            }
            if (line.hasOption('H')) {
                host = line.getOptionValue('H');
                LOG.info("found host = " + host);
            }
            if (line.hasOption('l')) {
                tmpProviderScope = ProviderScope.LOCAL;
                LOG.info("found scope local");
            }
            if (line.hasOption('p')) {
                port = Integer.parseInt(line.getOptionValue('p'));
                LOG.info("found port = " + port);
            }
            if (line.hasOption('t')) {
                transport = line.getOptionValue('t').toLowerCase();
                LOG.info("found transport = " + transport);
            }
        } catch (ParseException e) {
            LOG.error("failed to parse command line: " + e);
            HelpFormatter formatter = new HelpFormatter();
            formatter.printHelp(MyRadioProviderApplication.class.getName(), options, true);
            System.exit(1);
        }

        Properties joynrConfig = new Properties();
        Module runtimeModule = getRuntimeModule(transport, host, port, joynrConfig);
        final ProviderScope providerScope = tmpProviderScope;
        LOG.info("Using the following runtime module: " + runtimeModule.getClass().getSimpleName());
        LOG.info("Registering provider with the following scope: " + providerScope.name());
        LOG.info("Registering provider on domain \"{}\"", localDomain);

        // joynr config properties are used to set joynr configuration at
        // compile time. They are set on the
        // JoynrInjectorFactory.
        // Set a custom static persistence file (default is joynr.properties in
        // the working dir) to store
        // joynr configuration. It allows for changing the joynr configuration
        // at runtime. Custom persistence
        // files support running the consumer and provider applications from
        // within the same directory.
        joynrConfig.setProperty(MessagingPropertyKeys.PERSISTENCE_FILE, STATIC_PERSISTENCE_FILE);

        // How to use custom infrastructure elements:

        // 1) Set them programmatically at compile time using the joynr
        // configuration properties at the
        // JoynInjectorFactory. E.g. uncomment the following lines to set a
        // certain joynr server
        // instance.
        // joynrConfig.setProperty(ConfigurableMessagingSettings.PROPERTY_GBIDS, "joynrdefaultgbid");
        // joynrConfig.setProperty(MqttModule.PROPERTY_MQTT_BROKER_URIS, "tcp://localhost:1883");

        // Each joynr instance has a local domain. It identifies the execution
        // device/platform, e.g. the
        // vehicle. Normally, providers on that instance are registered for the
        // local domain.
        joynrConfig.setProperty(PROPERTY_JOYNR_DOMAIN_LOCAL, localDomain);

        // 2) Or set them in the static persistence file (default:
        // joynr.properties in working dir) at
        // runtime. If not available in the working dir, it will be created
        // during the first launch
        // of the application. Copy the following lines to the custom
        // persistence file to set a
        // certain joynr server instance.
        // NOTE: This application uses a custom static persistence file
        // provider-joynr.properties.
        // Copy the following lines to the custom persistence file to set a
        // certain joynr server
        // instance.
        // joynr.messaging.gbids=joynrdefaultgbid
        // joynr.messaging.mqtt.brokeruris=tcp://localhost:1883

        // 3) Or set them in Java System properties.
        // -Djoynr.messaging.gbids=joynrdefaultgbid
        // -Djoynr.messaging.mqtt.brokeruris=tcp://localhost:1883

        // NOTE:
        // Programmatically set configuration properties override properties set
        // in the static persistence file.
        // Java system properties override both

        // Application-specific configuration properties are injected to the
        // application by setting
        // them on the JoynApplicationModule.
        Properties appConfig = new Properties();

        // Use injected static provisioning of access control entries to allow access to anyone to this interface
        provisionAccessControl(joynrConfig, localDomain);
        JoynrApplication joynrApplication = new JoynrInjectorFactory(joynrConfig,
                                                                     runtimeModule,
                                                                     new StaticDomainAccessControlProvisioningModule()).createApplication(new JoynrApplicationModule(MyRadioProviderApplication.class, appConfig) {
                                                                         @Override
                                                                         protected void configure() {
                                                                             super.configure();
                                                                             bind(ProviderScope.class).toInstance(providerScope);
                                                                         }
                                                                     });
        joynrApplication.run();

        joynrApplication.shutdown();
    }

    private static void setupOptions(Options options, Options helpOptions) {
        Option optionDomain = Option.builder("d")
                                    .required(true)
                                    .argName("domain")
                                    .desc("the domain of the provider (required)")
                                    .longOpt("domain")
                                    .hasArg(true)
                                    .numberOfArgs(1)
                                    .type(String.class)
                                    .build();
        Option optionHost = Option.builder("H")
                                  .required(false)
                                  .argName("host")
                                  .desc("the websocket host (optional, used in case of websocket transport, default: localhost)")
                                  .longOpt("host")
                                  .hasArg(true)
                                  .numberOfArgs(1)
                                  .type(String.class)
                                  .build();
        Option optionHelp = Option.builder("h")
                                  .required(false)
                                  .desc("print this message")
                                  .longOpt("help")
                                  .hasArg(false)
                                  .build();
        Option optionLocal = Option.builder("l")
                                   .required(false)
                                   .desc("optional, if present, the provider is registered only locally")
                                   .longOpt("local")
                                   .hasArg(false)
                                   .build();
        Option optionPort = Option.builder("p")
                                  .required(false)
                                  .argName("port")
                                  .desc("the websocket port (optional, used in case of websocket transport, default: 4242)")
                                  .longOpt("port")
                                  .hasArg(true)
                                  .numberOfArgs(1)
                                  .type(Number.class)
                                  .build();
        Option optionTransport = Option.builder("t")
                                       .required(false)
                                       .argName("transport")
                                       .desc("the transport (optional, combination of websocket, http, mqtt with colon as separator, default: mqtt, any combination without websocket uses an embedded cluster controller)")
                                       .longOpt("transport")
                                       .hasArg(true)
                                       .numberOfArgs(1)
                                       .type(String.class)
                                       .build();

        options.addOption(optionDomain);
        options.addOption(optionHelp);
        options.addOption(optionHost);
        options.addOption(optionLocal);
        options.addOption(optionPort);
        options.addOption(optionTransport);
        helpOptions.addOption(optionHelp);
    }

    private static Module getRuntimeModule(String transport, String host, int port, Properties joynrConfig) {
        Module runtimeModule;
        if (transport != null) {
            if (transport.contains("websocketcc")) {
                configureWebSocket(host, port, joynrConfig);
                runtimeModule = new CCWebSocketRuntimeModule();
            } else if (transport.contains("websocket")) {
                configureWebSocket(host, port, joynrConfig);
                runtimeModule = new LibjoynrWebSocketRuntimeModule();
            } else {
                runtimeModule = new CCInProcessRuntimeModule();
            }

            Module backendTransportModules = Modules.EMPTY_MODULE;
            if (transport.contains("http")) {
                backendTransportModules = Modules.combine(backendTransportModules, new AtmosphereMessagingModule());
            }

            if (transport.contains("mqtt")) {
                configureMqtt(joynrConfig);
                backendTransportModules = Modules.combine(backendTransportModules, new MqttPahoModule());
            }
            return Modules.override(runtimeModule).with(backendTransportModules);
        }
        return Modules.override(new CCInProcessRuntimeModule()).with(new MqttPahoModule());
    }

    private static void configureWebSocket(String host, int port, Properties joynrConfig) {
        joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_HOST, host);
        joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PORT, "" + port);
        joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PROTOCOL, "ws");
        joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PATH, "");
    }

    private static void configureMqtt(Properties joynrConfig) {
        joynrConfig.put("joynr.messaging.mqtt.brokerUri", "tcp://localhost:1883");
        joynrConfig.put(MessagingPropertyKeys.PROPERTY_MESSAGING_PRIMARYGLOBALTRANSPORT, "mqtt");
    }

    @Override
    public void run() {
        provider = new MyRadioProvider();
        provider.addBroadcastFilter(new TrafficServiceBroadcastFilter());
        provider.addBroadcastFilter(new GeocastBroadcastFilter(jsonSerializer));
        ProviderQos providerQos = new ProviderQos();
        providerQos.setPriority(System.currentTimeMillis());
        providerQos.setScope(providerScope);
        Future<Void> future = runtime.getProviderRegistrar(localDomain, provider)
                                     .withProviderQos(providerQos)
                                     .awaitGlobalRegistration()
                                     .register();
        try {
            future.get();
        } catch (JoynrRuntimeException | ApplicationException | InterruptedException e) {
            LOG.error("runtime.registerProvider failed: ", e);
            return;
        }

        Scanner scanner = new Scanner(System.in, "UTF-8");
        String key = "";
        while (!key.equals("q")) {
            key = scanner.nextLine();

            switch (key) {
            case "s":
                provider.shuffleStations();
                break;
            case "p":
                provider.fireWeakSignalEventWithPartition();
                break;
            case "w":
                provider.fireWeakSignalEvent();
                break;
            case "n":
                provider.fireNewStationDiscoveredEvent();
                break;
            default:
                LOG.info("\n\nUSAGE press\n" + " q\tto quit\n" + " s\tto shuffle stations\n"
                        + " w\tto fire weak signal event\n"
                        + " p\tto fire weak signal event with country of current station as partition\n"
                        + " n\tto fire station discovered event\n");
                break;
            }
        }
    }

    @Override
    @SuppressWarnings(value = "DM_EXIT", justification = "WORKAROUND to be removed")
    public void shutdown() {
        LOG.info("shutting down");
        if (provider != null) {
            try {
                runtime.unregisterProvider(localDomain, provider);
            } catch (JoynrRuntimeException e) {
                LOG.error("unable to unregister capabilities {}", e.getMessage());
            }
        }
        runtime.shutdown(true);
        // TODO currently there is a bug preventing all threads being stopped
        // WORKAROUND
        try {
            Thread.sleep(3000);
        } catch (InterruptedException e) {
            // do nothing; exiting application
        }
        System.exit(0);
    }

    private static void provisionAccessControl(Properties properties, String domain) throws Exception {
        ObjectMapper objectMapper = new ObjectMapper();
        objectMapper.enableDefaultTypingAsProperty(DefaultTyping.JAVA_LANG_OBJECT, "_typeName");
        MasterAccessControlEntry newMasterAccessControlEntry = new MasterAccessControlEntry("*",
                                                                                            domain,
                                                                                            ProviderAnnotations.getInterfaceName(MyRadioProvider.class),
                                                                                            TrustLevel.LOW,
                                                                                            new TrustLevel[]{
                                                                                                    TrustLevel.LOW },
                                                                                            TrustLevel.LOW,
                                                                                            new TrustLevel[]{
                                                                                                    TrustLevel.LOW },
                                                                                            "*",
                                                                                            Permission.YES,
                                                                                            new Permission[]{
                                                                                                    Permission.YES });

        MasterAccessControlEntry[] provisionedAccessControlEntries = { newMasterAccessControlEntry };
        String provisionedAccessControlEntriesAsJson = objectMapper.writeValueAsString(provisionedAccessControlEntries);
        properties.setProperty(StaticDomainAccessControlProvisioning.PROPERTY_PROVISIONED_MASTER_ACCESSCONTROLENTRIES,
                               provisionedAccessControlEntriesAsJson);
    }
}
