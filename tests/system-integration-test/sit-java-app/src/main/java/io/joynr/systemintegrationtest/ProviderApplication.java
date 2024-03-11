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
package io.joynr.systemintegrationtest;

import java.util.Arrays;
import java.util.Properties;

import joynr.types.CustomParameter;
import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.CommandLineParser;
import org.apache.commons.cli.DefaultParser;
import org.apache.commons.cli.HelpFormatter;
import org.apache.commons.cli.Option;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Module;

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.websocket.WebsocketModule;
import io.joynr.proxy.Future;
import io.joynr.runtime.AbstractJoynrApplication;
import io.joynr.runtime.JoynrApplication;
import io.joynr.runtime.JoynrApplicationModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.LibjoynrWebSocketRuntimeModule;
import io.joynr.runtime.ProviderRegistrar;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;

public class ProviderApplication extends AbstractJoynrApplication {
    private static final Logger logger = LoggerFactory.getLogger(ProviderApplication.class);
    public static final String STATIC_PERSISTENCE_FILE = "java-provider.persistence_file";

    private Provider provider = null;
    private static boolean runForever = false;
    private static boolean shutdownHookCondition = false;
    private static String localDomain;
    private static String gbidsParameter = "";
    private static String[] gbids;
    private static boolean registerGlobally = false;
    private static boolean expectedFailure = false;
    private static boolean providerHasCustomParameters = false;
    private static CustomParameter[] providerCustomParameters;

    public static void main(String[] args) throws Exception {
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
                formatter.printHelp(ProviderApplication.class.getName(), options, true);
                System.exit(0);
            }
        } catch (ParseException e) {
            // ignore, since any option except '-h' will cause this exception
        }

        try {
            line = parser.parse(options, args);

            if (line.hasOption('d')) {
                localDomain = line.getOptionValue('d');
                logger.info("found domain = " + localDomain);
            }
            if (line.hasOption('g')) {
                gbidsParameter = line.getOptionValue('g');
                logger.info("found gbids = " + gbidsParameter);
            }
            if (line.hasOption('r')) {
                runForever = true;
                logger.info("found runForever = " + runForever);
            }
            if (line.hasOption('G')) {
                registerGlobally = true;
                logger.info("registerGlobally = " + registerGlobally);
            }
            if (line.hasOption('f')) {
                expectedFailure = true;
                logger.info("expectedFailure = " + expectedFailure);
            }
            if (line.hasOption('P')) {
                providerHasCustomParameters = true;
                providerCustomParameters = parseCustomParameters(line.getOptionValue('P'));
            }
        } catch (ParseException e) {
            logger.error("failed to parse command line: " + e);
            HelpFormatter formatter = new HelpFormatter();
            formatter.printHelp(ProviderApplication.class.getName(), options, true);
            System.exit(1);
        }

        if (!gbidsParameter.isEmpty()) {
            gbids = Arrays.stream(gbidsParameter.split(",")).map(a -> a.trim()).toArray(String[]::new);
        } else {
            gbids = new String[0];
        }
        logger.info("gbidsParameter = " + gbidsParameter + ", gbids = " + Arrays.toString(gbids));

        Properties joynrConfig = new Properties();
        Module runtimeModule = getRuntimeModule(joynrConfig);

        logger.debug("Using the following runtime module: " + runtimeModule.getClass().getSimpleName());
        logger.debug("Registering provider on domain \"{}\"", localDomain);

        joynrConfig.setProperty(MessagingPropertyKeys.PERSISTENCE_FILE, STATIC_PERSISTENCE_FILE);
        joynrConfig.setProperty(PROPERTY_JOYNR_DOMAIN_LOCAL, localDomain);
        Properties appConfig = new Properties();

        JoynrApplication joynrApplication = new JoynrInjectorFactory(joynrConfig,
                                                                     runtimeModule).createApplication(new JoynrApplicationModule(ProviderApplication.class, appConfig));

        joynrApplication.run();
        joynrApplication.shutdown();
    }

    private static CustomParameter[] parseCustomParameters(final String initialValue) {
        logger.info("Parsing provider custom parameters. Initial value: {};", initialValue);
        final String[] parameters = initialValue.split(";");
        return Arrays.stream(parameters).map(ProviderApplication::parseCustomParameter).toArray(CustomParameter[]::new);
    }

    private static CustomParameter parseCustomParameter(final String rawValue) {
        final String[] keyAndValue = rawValue.split("=");
        if (keyAndValue.length == 2) {
            return new CustomParameter(keyAndValue[0], keyAndValue[1]);
        } else {
            throw new IllegalArgumentException("Unable to parse Provider Custom Parameter \"" + rawValue
                    + "\". It is expected to like key=value string separated by semicolon.");
        }
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
        Option optionHelp = Option.builder("h")
                                  .required(false)
                                  .desc("print this message")
                                  .longOpt("help")
                                  .hasArg(false)
                                  .build();
        Option optionRunforever = Option.builder("r")
                                        .required(false)
                                        .desc("optional, if present, the provider is running forever")
                                        .longOpt("runforever")
                                        .hasArg(false)
                                        .build();
        Option optionGbids = Option.builder("g")
                                   .required(false)
                                   .desc("optional, if present, provider is registered for these gbids")
                                   .longOpt("gbids")
                                   .hasArg(true)
                                   .numberOfArgs(1)
                                   .type(String.class)
                                   .build();
        Option optionGlobal = Option.builder("G")
                                    .required(false)
                                    .argName("global")
                                    .desc("register provider globally")
                                    .longOpt("global")
                                    .hasArg(false)
                                    .build();
        Option optionExpectedFailure = Option.builder("f")
                                             .required(false)
                                             .argName("expectedFailure")
                                             .desc("expect failure")
                                             .longOpt("expected-failure")
                                             .hasArg(false)
                                             .build();
        final Option optionCustomParameters = Option.builder("P")
                                                    .required(false)
                                                    .argName("providerCustomParameters")
                                                    .desc("provider custom parameters. It is expected as key=value sequence separated by semicolon. For example: keyA=valueA;keyB=valueB")
                                                    .longOpt("providerCustomParameters")
                                                    .hasArg(true)
                                                    .numberOfArgs(1)
                                                    .type(String.class)
                                                    .build();

        options.addOption(optionDomain);
        options.addOption(optionHelp);
        options.addOption(optionRunforever);
        options.addOption(optionGbids);
        options.addOption(optionGlobal);
        options.addOption(optionExpectedFailure);
        options.addOption(optionCustomParameters);
        helpOptions.addOption(optionHelp);
    }

    private static Module getRuntimeModule(Properties joynrConfig) {
        configureWebSocket(joynrConfig);
        return new LibjoynrWebSocketRuntimeModule();
    }

    private static void configureWebSocket(Properties joynrConfig) {
        if (expectedFailure) {
            joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_HOST, "localhost");
            joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PORT, "4245");
            joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PROTOCOL, "ws");
            joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PATH, "/");
        } else {
            joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_HOST, "localhost");
            joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PORT, "4242");
            joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PROTOCOL, "ws");
            joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PATH, "/");
        }
    }

    @Override
    public void run() {
        provider = new Provider();
        ProviderQos providerQos = new ProviderQos();
        // by default, ProviderScope is set to GLOBAL
        providerQos.setScope(registerGlobally == true ? ProviderScope.GLOBAL : ProviderScope.LOCAL);
        providerQos.setPriority(System.currentTimeMillis());
        if (providerHasCustomParameters) {
            providerQos.setCustomParameters(providerCustomParameters);
        }

        // access to provider is needed inside the hook, so it must be added here
        Thread shutdownHook = new Thread() {
            @Override
            public void run() {
                logger.info("executing shutdown hook");
                synchronized (this) {
                    logger.info("notifying any waiting thread from shutdown hook");
                    shutdownHookCondition = true;
                    notifyAll();
                }
                logger.info("shutting down");
                if (provider != null) {
                    try {
                        runtime.unregisterProvider(localDomain, provider);
                    } catch (JoynrRuntimeException e) {
                        logger.error("unable to unregister capabilities {}", e.getMessage());
                    }
                }
                runtime.shutdown(false);
                logger.info("shutdown completed");
            }
        };
        logger.info("adding shutdown hook");
        Runtime.getRuntime().addShutdownHook(shutdownHook);

        ProviderRegistrar providerRegistrar = runtime.getProviderRegistrar(localDomain, provider)
                                                     .withProviderQos(providerQos);
        if (gbids.length > 0) {
            logger.info("gbids.length > 0, gbids.length = " + gbids.length);
            providerRegistrar.withGbids(gbids);
        }

        if (expectedFailure) {
            providerRegistrar.awaitGlobalRegistration();
        }

        Future<Void> registrationFuture = providerRegistrar.register();

        if (expectedFailure) {
            try {
                registrationFuture.get(30000);
            } catch (Exception e) {
                logger.info("SIT RESULT success: Java provider failed as expected!");
                provider = null;
                return;
            }
        }

        if (expectedFailure) {
            if (registrationFuture.getStatus().successful()) {
                logger.info("SIT RESULT failure: Java provider did not fail as expected!");
            } else {
                logger.info("SIT RESULT success: Java provider failed as expected!");
                provider = null;
                return;
            }
        }

        if (!runForever) {
            try {
                Thread.sleep(30000);
            } catch (InterruptedException e) {
                // terminate execution by continuing
            }
        } else {
            synchronized (shutdownHook) {
                while (!shutdownHookCondition) {
                    try {
                        shutdownHook.wait();
                    } catch (InterruptedException e) {
                        // ignore
                    }
                }
            }
        }
    }

    @Override
    public void shutdown() {
        System.exit(0);
    }

}
