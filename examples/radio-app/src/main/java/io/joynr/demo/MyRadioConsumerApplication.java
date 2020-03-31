/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2020 BMW Car IT GmbH
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

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.Inject;
import com.google.inject.Module;
import com.google.inject.name.Named;
import com.google.inject.util.Modules;

import edu.umd.cs.findbugs.annotations.SuppressWarnings;
import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.exceptions.DiscoveryException;
import io.joynr.exceptions.JoynrCommunicationException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.AtmosphereMessagingModule;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.mqtt.paho.client.MqttPahoModule;
import io.joynr.messaging.websocket.WebsocketModule;
import io.joynr.proxy.CallbackWithModeledError;
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.pubsub.subscription.AttributeSubscriptionAdapter;
import io.joynr.runtime.AbstractJoynrApplication;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrApplication;
import io.joynr.runtime.JoynrApplicationModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.LibjoynrWebSocketRuntimeModule;
import joynr.MulticastSubscriptionQos;
import joynr.OnChangeSubscriptionQos;
import joynr.OnChangeWithKeepAliveSubscriptionQos;
import joynr.exceptions.ApplicationException;
import joynr.exceptions.ProviderRuntimeException;
import joynr.vehicle.Country;
import joynr.vehicle.GeoPosition;
import joynr.vehicle.Radio.AddFavoriteStationErrorEnum;
import joynr.vehicle.RadioBroadcastInterface;
import joynr.vehicle.RadioBroadcastInterface.NewStationDiscoveredBroadcastFilterParameters;
import joynr.vehicle.RadioBroadcastInterface.WeakSignalBroadcastAdapter;
import joynr.vehicle.RadioProxy;
import joynr.vehicle.RadioStation;
import joynr.vehicle.RadioSync.GetLocationOfCurrentStationReturned;

public class MyRadioConsumerApplication extends AbstractJoynrApplication {
    private static final String PRINT_BORDER = "\n####################\n";
    private static final Logger LOG = LoggerFactory.getLogger(MyRadioConsumerApplication.class);
    public static final String APP_CONFIG_PROVIDER_DOMAIN = "javademoapp.provider.domain";
    private static final String STATIC_PERSISTENCE_FILE = "consumer-joynr.properties";
    public static final String MISSING_NAME = "MISSING_NAME";

    @Inject
    @Named(APP_CONFIG_PROVIDER_DOMAIN)
    private String providerDomain;
    private Future<String> subscriptionFutureCurrentStation;
    private Future<String> weakSignalFuture;
    private Future<String> weakSignalWithPartitionFuture;
    private Future<String> newStationDiscoveredFuture;
    private RadioProxy radioProxy;
    @Inject
    private ObjectMapper objectMapper;
    @Inject
    private DiscoveryScope discoveryScope;

    /**
     * Main method. This method is responsible for: 1. Instantiating the consumer application. 2. Injecting the instance
     * with Guice bindings 3. Starting the application. 4. Ending the application so that the necessary clean up calls
     * are made.
     *
     * @param args arguments give when calling the main method
     */
    public static void main(String[] args) {
        // run application from cmd line using Maven:
        // mvn exec:java -Dexec.mainClass="io.joynr.demo.MyRadioConsumerApplication" -Dexec.args="<arguments>"
        DiscoveryScope tmpDiscoveryScope = DiscoveryScope.LOCAL_AND_GLOBAL;
        String host = "localhost";
        int port = 4242;
        String providerDomain = "domain";
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
                formatter.printHelp(MyRadioConsumerApplication.class.getName(), options, true);
                System.exit(0);
            }
        } catch (ParseException e) {
            // ignore, since any option except '-h' will cause this exception
        }

        try {
            line = parser.parse(options, args);

            if (line.hasOption('d')) {
                providerDomain = line.getOptionValue('d');
                LOG.info("found domain = " + providerDomain);
            }
            if (line.hasOption('H')) {
                host = line.getOptionValue('H');
                LOG.info("found host = " + host);
            }
            if (line.hasOption('l')) {
                tmpDiscoveryScope = DiscoveryScope.LOCAL_ONLY;
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
            formatter.printHelp(MyRadioConsumerApplication.class.getName(), options, true);
            System.exit(1);
        }

        // joynr config properties are used to set joynr configuration at compile time. They are set on the
        // JoynInjectorFactory.
        Properties joynrConfig = new Properties();
        Module runtimeModule = getRuntimeModule(transport, host, port, joynrConfig);

        LOG.debug("Using the following runtime module: " + runtimeModule.getClass().getSimpleName());
        LOG.debug("Searching for providers on domain \"{}\"", providerDomain);

        // Set a custom static persistence file (default is joynr.properties in the working dir) to store
        // joynr configuration. It allows for changing the joynr configuration at runtime. Custom persistence
        // files support running the consumer and provider applications from within the same directory.
        joynrConfig.setProperty(MessagingPropertyKeys.PERSISTENCE_FILE, STATIC_PERSISTENCE_FILE);

        // How to use custom infrastructure elements:

        // 1) Set them programmatically at compile time using the joynr configuration properties at the
        // JoynInjectorFactory. E.g. uncomment the following lines to set a certain joynr server
        // instance.
        // joynrConfig.setProperty(MessagingPropertyKeys.BOUNCE_PROXY_URL, "http://localhost:8080/bounceproxy/");
        // joynrConfig.setProperty(MessagingPropertyKeys.DISCOVERYDIRECTORYURL, "http://localhost:8080/discovery/channels/discoverydirectory_channelid/");
        joynrConfig.setProperty(PROPERTY_JOYNR_DOMAIN_LOCAL, "radioapp_consumer_local_domain");

        // 2) Or set them in the static persistence file (default: joynr.properties in working dir) at
        // runtime. If not available in the working dir, it will be created during the first launch
        // of the application. Copy the following lines to the custom persistence file to set a
        // certain joynr server instance.
        // NOTE: This application uses a custom static persistence file consumer-joynr.properties.
        // Copy the following lines to the custom persistence file to set a certain joynr server
        // instance.
        // joynr.messaging.bounceproxyurl=http://localhost:8080/bounceproxy/
        // joynr.messaging.discoverydirectoryurl=http://localhost:8080/discovery/channels/discoverydirectory_channelid/

        // 3) Or set them in Java System properties.
        // -Djoynr.messaging.bounceProxyUrl=http://localhost:8080/bounceproxy/
        // -Djoynr.messaging.capabilitiesDirectoryUrl=http://localhost:8080/discovery/channels/discoverydirectory_channelid/

        // NOTE:
        // Programmatically set configuration properties override properties set in the static persistence file.
        // Java system properties override both

        // Application-specific configuration properties are injected to the application by setting
        // them on the JoynApplicationModule.
        Properties appConfig = new Properties();
        appConfig.setProperty(APP_CONFIG_PROVIDER_DOMAIN, providerDomain);
        final DiscoveryScope discoveryScope = tmpDiscoveryScope;

        JoynrApplication myRadioConsumerApp = new JoynrInjectorFactory(joynrConfig,
                                                                       runtimeModule).createApplication(new JoynrApplicationModule(MyRadioConsumerApplication.class, appConfig) {
                                                                           @Override
                                                                           protected void configure() {
                                                                               super.configure();
                                                                               bind(DiscoveryScope.class).toInstance(discoveryScope);
                                                                           }
                                                                       });
        myRadioConsumerApp.run();

        myRadioConsumerApp.shutdown();
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
                                   .desc("optional, if present, the provider is discovered only locally")
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
            if (transport.contains("websocket")) {
                joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_HOST, host);
                joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PORT, "" + port);
                joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PROTOCOL, "ws");
                joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PATH, "");
                runtimeModule = new LibjoynrWebSocketRuntimeModule();
            } else {
                runtimeModule = new CCInProcessRuntimeModule();
            }

            Module backendTransportModules = Modules.EMPTY_MODULE;
            if (transport.contains("http")) {
                backendTransportModules = Modules.combine(backendTransportModules, new AtmosphereMessagingModule());
            }

            if (transport.contains("mqtt")) {
                joynrConfig.put("joynr.messaging.mqtt.brokerUri", "tcp://localhost:1883");
                joynrConfig.put(MessagingPropertyKeys.PROPERTY_MESSAGING_PRIMARYGLOBALTRANSPORT, "mqtt");
                backendTransportModules = Modules.combine(backendTransportModules, new MqttPahoModule());
            }

            return Modules.override(runtimeModule).with(backendTransportModules);
        }

        return Modules.override(new CCInProcessRuntimeModule()).with(new MqttPahoModule());
    }

    @Override
    @SuppressWarnings(value = "DM_EXIT", justification = "WORKAROUND to be removed")
    public void shutdown() {
        if (radioProxy != null) {
            if (subscriptionFutureCurrentStation != null) {
                try {
                    radioProxy.unsubscribeFromCurrentStation(subscriptionFutureCurrentStation.get());
                } catch (JoynrRuntimeException | InterruptedException | ApplicationException e) {
                    LOG.error(e.getMessage());
                }
            }
            if (weakSignalFuture != null) {
                try {
                    radioProxy.unsubscribeFromWeakSignalBroadcast(weakSignalFuture.get());
                } catch (JoynrRuntimeException | InterruptedException | ApplicationException e) {
                    LOG.error(e.getMessage());
                }
            }
            if (weakSignalWithPartitionFuture != null) {
                try {
                    radioProxy.unsubscribeFromWeakSignalBroadcast(weakSignalWithPartitionFuture.get());
                } catch (JoynrRuntimeException | InterruptedException | ApplicationException e) {
                    LOG.error(e.getMessage());
                }
            }
            if (newStationDiscoveredFuture != null) {
                try {
                    radioProxy.unsubscribeFromNewStationDiscoveredBroadcast(newStationDiscoveredFuture.get());
                } catch (JoynrRuntimeException | InterruptedException | ApplicationException e) {
                    LOG.error(e.getMessage());
                }
            }
        }

        // Add any clean up code here for your application.
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

    private Future<String> subscribeToWeakSignal(MulticastSubscriptionQos qos, String... partitions) {
        return radioProxy.subscribeToWeakSignalBroadcast(new WeakSignalBroadcastAdapter() {
            @Override
            public void onReceive(RadioStation weakSignalStation) {
                LOG.info(PRINT_BORDER + "BROADCAST SUBSCRIPTION: weak signal: " + weakSignalStation + PRINT_BORDER);
            }
        }, qos, partitions);
    }

    @SuppressWarnings("checkstyle:methodlength")
    @Override
    public void run() {
        DiscoveryQos discoveryQos = new DiscoveryQos();
        // As soon as the arbitration QoS is set on the proxy builder, discovery of suitable providers
        // is triggered. If the discovery process does not find matching providers within the
        // arbitration timeout duration it will be terminated and you will get an arbitration exception.
        discoveryQos.setDiscoveryTimeoutMs(10000);
        discoveryQos.setDiscoveryScope(discoveryScope);
        // Provider entries in the global capabilities directory are cached locally. Discovery will
        // consider entries in this cache valid if they are younger as the max age of cached
        // providers as defined in the QoS. All valid entries will be processed by the arbitrator when searching
        // for and arbitrating the "best" matching provider.
        // NOTE: Valid cache entries might prevent triggering a lookup in the global capabilities
        // directory. Therefore, not all providers registered with the global capabilities
        // directory might be taken into account during arbitration.
        discoveryQos.setCacheMaxAgeMs(Long.MAX_VALUE);
        // The discovery process outputs a list of matching providers. The arbitration strategy then
        // chooses one or more of them to be used by the proxy.
        discoveryQos.setArbitrationStrategy(ArbitrationStrategy.HighestPriority);

        // The provider will maintain at least a minimum interval idle time in milliseconds between
        // successive notifications, even if on-change notifications are enabled and the value changes more
        // often. This prevents the consumer from being flooded by updated values. The filtering happens on
        // the provider's side, thus also preventing excessive network traffic.
        int minInterval_ms = 0;
        // The provider will send notifications every maximum interval in milliseconds, even if the value didn't
        // change. It will send notifications more often if on-change notifications are enabled,
        // the value changes more often, and the minimum interval QoS does not prevent it. The maximum interval
        // can thus be seen as a sort of heart beat.
        int maxInterval_ms = 10000;

        // The provider will send notifications until the end date is reached. The consumer will not receive any
        // notifications (neither value notifications nor missed publication notifications) after
        // this date.
        long validityMs = 60000;
        // If no notification was received within the last alert interval, a missed publication
        // notification will be raised.
        int alertAfterInterval_ms = 20000;
        // Notification messages will be sent with this time-to-live. If a notification message can not be
        // delivered within its TTL, it will be deleted from the system.
        // NOTE: If a notification message is not delivered due to an expired TTL, it might raise a
        // missed publication notification (depending on the value of the alert interval QoS).
        int publicationTtl_ms = 5000;

        OnChangeWithKeepAliveSubscriptionQos subscriptionQos = new OnChangeWithKeepAliveSubscriptionQos();
        subscriptionQos.setMinIntervalMs(minInterval_ms).setMaxIntervalMs(maxInterval_ms).setValidityMs(validityMs);
        subscriptionQos.setAlertAfterIntervalMs(alertAfterInterval_ms).setPublicationTtlMs(publicationTtl_ms);

        ProxyBuilder<RadioProxy> proxyBuilder = runtime.getProxyBuilder(providerDomain, RadioProxy.class);

        try {
            // getting an attribute
            radioProxy = proxyBuilder.setMessagingQos(new MessagingQos()).setDiscoveryQos(discoveryQos).build();
            RadioStation currentStation = radioProxy.getCurrentStation();
            LOG.info(PRINT_BORDER + "ATTRIBUTE GET: current station: " + currentStation + PRINT_BORDER);

            // subscribe to an attribute
            subscriptionFutureCurrentStation = radioProxy.subscribeToCurrentStation(new AttributeSubscriptionAdapter<RadioStation>() {

                @Override
                public void onReceive(RadioStation value) {
                    LOG.info(PRINT_BORDER + "ATTRIBUTE SUBSCRIPTION: current station: " + value + PRINT_BORDER);
                }

                @Override
                public void onError(JoynrRuntimeException error) {
                    LOG.info(PRINT_BORDER + "ATTRIBUTE SUBSCRIPTION: " + error + PRINT_BORDER);
                }
            }, subscriptionQos);

            // broadcast subscription
            // The provider will send a notification whenever the value changes.
            MulticastSubscriptionQos weakSignalBroadcastSubscriptionQos;
            // The consumer will be subscribed to the multicast until the end date is reached, after which the
            // consumer will be automatically unsubscribed, and will not receive any further notifications
            // this date.
            long wsbValidityMs = 60 * 1000;
            weakSignalBroadcastSubscriptionQos = new MulticastSubscriptionQos();
            weakSignalBroadcastSubscriptionQos.setValidityMs(wsbValidityMs);

            weakSignalFuture = subscribeToWeakSignal(weakSignalBroadcastSubscriptionQos);

            //susbcribe to weak signal with partition "GERMANY"
            weakSignalWithPartitionFuture = subscribeToWeakSignal(weakSignalBroadcastSubscriptionQos, "GERMANY");

            // selective broadcast subscription
            OnChangeSubscriptionQos newStationDiscoveredBroadcastSubscriptionQos;
            int nsdbMinIntervalMs = 2 * 1000;
            long nsdbValidityMs = 180 * 1000;
            int nsdbPublicationTtlMs = 5 * 1000;
            newStationDiscoveredBroadcastSubscriptionQos = new OnChangeSubscriptionQos();
            newStationDiscoveredBroadcastSubscriptionQos.setMinIntervalMs(nsdbMinIntervalMs)
                                                        .setValidityMs(nsdbValidityMs)
                                                        .setPublicationTtlMs(nsdbPublicationTtlMs);
            NewStationDiscoveredBroadcastFilterParameters newStationDiscoveredBroadcastFilterParams = new NewStationDiscoveredBroadcastFilterParameters();
            newStationDiscoveredBroadcastFilterParams.setHasTrafficService("true");
            GeoPosition positionOfInterest = new GeoPosition(48.1351250, 11.5819810); // Munich
            String positionOfInterestJson = null;
            try {
                positionOfInterestJson = objectMapper.writeValueAsString(positionOfInterest);
            } catch (JsonProcessingException e1) {
                LOG.error("Unable to write position of interest filter parameter to JSON", e1);
            }
            newStationDiscoveredBroadcastFilterParams.setPositionOfInterest(positionOfInterestJson);
            newStationDiscoveredBroadcastFilterParams.setRadiusOfInterestArea("200000"); // 200 km
            newStationDiscoveredFuture = radioProxy.subscribeToNewStationDiscoveredBroadcast(new RadioBroadcastInterface.NewStationDiscoveredBroadcastAdapter() {
                @Override
                public void onReceive(RadioStation discoveredStation, GeoPosition geoPosition) {
                    LOG.info(PRINT_BORDER + "BROADCAST SUBSCRIPTION: new station discovered: " + discoveredStation
                            + " at " + geoPosition + PRINT_BORDER);
                }
            }, newStationDiscoveredBroadcastSubscriptionQos, newStationDiscoveredBroadcastFilterParams);

            boolean success;

            try {
                // add favorite radio station
                RadioStation favoriteStation = new RadioStation("99.3 The Fox Rocks", false, Country.CANADA);
                success = radioProxy.addFavoriteStation(favoriteStation);
                LOG.info(PRINT_BORDER + "METHOD: added favorite station: " + favoriteStation + ": " + success
                        + PRINT_BORDER);
                success = radioProxy.addFavoriteStation(favoriteStation);
            } catch (ApplicationException exception) {
                AddFavoriteStationErrorEnum error = exception.getError();
                switch (error) {
                case DUPLICATE_RADIOSTATION:
                    LOG.info(PRINT_BORDER + "METHOD: addFavoriteStation failed with the following excpected error: "
                            + error);
                    break;
                default:
                    LOG.error(PRINT_BORDER + "METHOD: addFavoriteStation failed with an unexpected error: " + error);
                    break;
                }
            }

            try {
                // add favorite radio station
                RadioStation favoriteStation = new RadioStation("", false, Country.GERMANY);
                success = radioProxy.addFavoriteStation(favoriteStation);
                LOG.info(PRINT_BORDER + "METHOD: addFavoriteStation completed unexpected with the following output: "
                        + success);
            } catch (ApplicationException exception) {
                String errorName = exception.getError().name();
                LOG.info(PRINT_BORDER
                        + "METHOD: addFavoriteStation failed with the following unexpected ApplicationException: "
                        + errorName);
            } catch (ProviderRuntimeException exception) {
                String errorName = exception.getMessage();
                String expectation = errorName.equals(MISSING_NAME) ? "expected" : "unexpected";
                LOG.info(PRINT_BORDER + "METHOD: addFavoriteStation failed with the following " + expectation
                        + " exception: " + errorName);
            }

            // shuffle the stations
            radioProxy.shuffleStations();
            currentStation = radioProxy.getCurrentStation();
            LOG.info(PRINT_BORDER + "The current radio station after shuffling is: " + currentStation + PRINT_BORDER);

            // add favorite radio station async
            RadioStation radioStation = new RadioStation("99.4 AFN", false, Country.GERMANY);
            Future<Boolean> future = radioProxy.addFavoriteStation(new CallbackWithModeledError<Boolean, AddFavoriteStationErrorEnum>() {
                @Override
                public void onSuccess(Boolean result) {
                    LOG.info(PRINT_BORDER + "ASYNC METHOD: added favorite station: callback onSuccess" + PRINT_BORDER);
                }

                @Override
                public void onFailure(JoynrRuntimeException error) {
                    LOG.info(PRINT_BORDER + "ASYNC METHOD: added favorite station: callback onFailure: "
                            + error.getMessage() + PRINT_BORDER);
                }

                @Override
                public void onFailure(AddFavoriteStationErrorEnum errorEnum) {
                    switch (errorEnum) {
                    case DUPLICATE_RADIOSTATION:
                        LOG.info(PRINT_BORDER + "ASYNC METHOD: added favorite station failed: Duplicate Station!"
                                + PRINT_BORDER);
                        break;

                    default:
                        LOG.error(PRINT_BORDER + "ASYNC METHOD: added favorite station failed: unknown errorEnum:"
                                + errorEnum + PRINT_BORDER);
                        break;
                    }
                    LOG.info(PRINT_BORDER + "ASYNC METHOD: added favorite station: callback onFailure: " + errorEnum
                            + PRINT_BORDER);
                }
            }, radioStation);

            try {
                long timeoutInMilliseconds = 8000;
                Boolean reply = future.get(timeoutInMilliseconds);
                LOG.info(PRINT_BORDER + "ASYNC METHOD: added favorite station: " + radioStation + ": " + reply
                        + PRINT_BORDER);
            } catch (InterruptedException | JoynrRuntimeException | ApplicationException e) {
                LOG.info(PRINT_BORDER + "ASYNC METHOD: added favorite station: " + radioStation + ": "
                        + e.getClass().getSimpleName() + "!");
            }

            Scanner scanner = new Scanner(System.in, "UTF-8");
            String key = "";
            while (!key.equals("q")) {
                key = scanner.nextLine();

                switch (key) {
                case "s":
                    try {
                        radioProxy.shuffleStations();
                        LOG.info("called shuffleStations()");
                    } catch (Exception e) {
                        LOG.error("Exception from shuffleStations()", e);
                    }
                    break;
                case "g":
                    try {
                        currentStation = radioProxy.getCurrentStation();
                        LOG.info("called getCurrentStation(), result: {}", currentStation);
                    } catch (Exception e) {
                        LOG.error("Exception from getCurrentStation()", e);
                    }
                    break;
                case "m":
                    try {
                        GetLocationOfCurrentStationReturned locationOfCurrentStation = radioProxy.getLocationOfCurrentStation();
                        LOG.info("called getLocationOfCurrentStation. country: " + locationOfCurrentStation.country
                                + ", location: " + locationOfCurrentStation.location);
                    } catch (Exception e) {
                        LOG.error("Exception from getLocationOfCurrentStation()", e);
                    }
                    break;
                default:
                    LOG.info("\n\nUSAGE press\n" + " q\tto quit\n" + " s\tto shuffle stations\n");
                    break;
                }
            }
            scanner.close();
        } catch (DiscoveryException e) {
            LOG.error("No provider found", e);
        } catch (JoynrCommunicationException e) {
            LOG.error("The message was not sent: ", e);
        }
    }
}
