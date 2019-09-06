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
package io.joynr.systemintegrationtest;

import java.io.IOException;
import java.util.Arrays;
import java.util.Properties;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

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

import edu.umd.cs.findbugs.annotations.SuppressWarnings;

import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.exceptions.DiscoveryException;
import io.joynr.exceptions.JoynrCommunicationException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.websocket.WebsocketModule;
import io.joynr.proxy.ProxyBuilder.ProxyCreatedCallback;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.runtime.AbstractJoynrApplication;
import io.joynr.runtime.JoynrApplication;
import io.joynr.runtime.JoynrApplicationModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.LibjoynrWebSocketRuntimeModule;
import joynr.test.SystemIntegrationTestProxy;

public class ConsumerApplication extends AbstractJoynrApplication {
    private static final Logger LOG = LoggerFactory.getLogger(ConsumerApplication.class);
    public static final String SYSTEMINTEGRATIONTEST_PROVIDER_DOMAIN = "system-integration-test.provider.domain";
    private static final String STATIC_PERSISTENCE_FILE = "java-consumer.persistence_file";

    private static String providerDomain;
    private static String gbidsParameter = "";
    private static String[] gbids;
    private static boolean globalOnly = false;
    private SystemIntegrationTestProxy systemIntegrationTestProxy;
    private static Semaphore proxyCreated = new Semaphore(0);

    /**
     * Main method.
     *
     * @throws IOException
     */
    public static void main(String[] args) throws IOException {
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
                providerDomain = line.getOptionValue('d');
                LOG.info("found domain = " + providerDomain);
            }
            if (line.hasOption('g')) {
                gbidsParameter = line.getOptionValue('g');
                LOG.info("found gbids = " + gbidsParameter);
            }
            if (line.hasOption('G')) {
                globalOnly = true;
                LOG.info("globalOnly = " + globalOnly);
            }
        } catch (ParseException e) {
            LOG.error("failed to parse command line: " + e);
            HelpFormatter formatter = new HelpFormatter();
            formatter.printHelp(ProviderApplication.class.getName(), options, true);
            System.exit(1);
        }

        if (!gbidsParameter.isEmpty()) {
            gbids = Arrays.stream(gbidsParameter.split(",")).map(a -> a.trim()).toArray(String[]::new);
            LOG.debug("Searching for providers on domain \"{}\", gbids \"{}\"", providerDomain, gbids);
        } else {
            gbids = new String[0];
            LOG.debug("Searching for providers on domain \"{}\"", providerDomain);
        }

        Properties joynrConfig = new Properties();
        Module runtimeModule = getRuntimeModule(args, joynrConfig);
        LOG.debug("Using the following runtime module: " + runtimeModule.getClass().getSimpleName());
        LOG.debug("Searching for providers on domain \"{}\"", providerDomain);

        joynrConfig.setProperty(MessagingPropertyKeys.PERSISTENCE_FILE, STATIC_PERSISTENCE_FILE);
        joynrConfig.setProperty(PROPERTY_JOYNR_DOMAIN_LOCAL, providerDomain);
        Properties appConfig = new Properties();
        appConfig.setProperty(SYSTEMINTEGRATIONTEST_PROVIDER_DOMAIN, providerDomain);

        JoynrApplication myConsumerApp = new JoynrInjectorFactory(joynrConfig,
                                                                  runtimeModule).createApplication(new JoynrApplicationModule(ConsumerApplication.class, appConfig));
        myConsumerApp.run();

        myConsumerApp.shutdown();
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
        Option optionGbids = Option.builder("g")
                                   .required(false)
                                   .desc("optional, if present, provider is looked up for these gbids")
                                   .longOpt("gbids")
                                   .hasArg(true)
                                   .numberOfArgs(1)
                                   .type(String.class)
                                   .build();
        Option optionGlobalOnly = Option.builder("G")
                                        .required(false)
                                        .argName("globalOnly")
                                        .desc("search only globally for provider")
                                        .longOpt("global-only")
                                        .hasArg(false)
                                        .build();
        options.addOption(optionDomain);
        options.addOption(optionHelp);
        options.addOption(optionGbids);
        options.addOption(optionGlobalOnly);
        helpOptions.addOption(optionHelp);
    }

    private static Module getRuntimeModule(String[] args, Properties joynrConfig) {
        joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_HOST, "localhost");
        joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PORT, "4242");
        joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PROTOCOL, "ws");
        joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PATH, "");
        return new LibjoynrWebSocketRuntimeModule();
    }

    @Override
    @SuppressWarnings(value = "DM_EXIT", justification = "WORKAROUND to be removed")
    public void shutdown() {
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

    @SuppressWarnings({ "checkstyle:methodlength", "DM_EXIT" })
    @Override
    public void run() {
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryTimeoutMs(10000);
        if (globalOnly == true) {
            discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);
            discoveryQos.setCacheMaxAgeMs(0L);
        } else {
            discoveryQos.setCacheMaxAgeMs(Long.MAX_VALUE);
        }
        discoveryQos.setArbitrationStrategy(ArbitrationStrategy.HighestPriority);

        boolean success = false;
        try {
            ProxyBuilder<SystemIntegrationTestProxy> proxyBuilder = runtime.getProxyBuilder(providerDomain,
                                                                                            SystemIntegrationTestProxy.class)
                                                                           .setMessagingQos(new MessagingQos(10000))
                                                                           .setDiscoveryQos(discoveryQos);
            if (gbids.length > 0) {
                proxyBuilder = proxyBuilder.setGbids(gbids);
            }
            systemIntegrationTestProxy = proxyBuilder.build(new ProxyCreatedCallback<SystemIntegrationTestProxy>() {
                @Override
                public void onProxyCreationFinished(SystemIntegrationTestProxy result) {
                    LOG.info("proxy created");
                    proxyCreated.release();
                }

                @Override
                public void onProxyCreationError(JoynrRuntimeException error) {
                    LOG.error("error creating proxy");
                }
            });
            try {
                if (proxyCreated.tryAcquire(11000, TimeUnit.MILLISECONDS) == false) {
                    throw new DiscoveryException("proxy not created in time");
                }
            } catch (InterruptedException e) {
                throw new DiscoveryException("proxy not created in time");
            }

            try {
                int addendA = 3333333;
                int addendB = 4444444;
                Integer sum = systemIntegrationTestProxy.add(addendA, addendB);
                if (sum != null && sum == (addendA + addendB)) {
                    LOG.info("SIT RESULT success: Java consumer -> " + providerDomain + " (" + addendA + " + " + addendB
                            + " =  " + sum + ")");
                    success = true;
                }
            } catch (Exception e) {
                // fallthrough
            }
        } catch (DiscoveryException | JoynrCommunicationException e) {
            // fallthrough
        }
        if (!success) {
            LOG.info("SIT RESULT error: Java consumer -> " + providerDomain);
        }
        System.exit((success) ? 0 : 1);
    }
}
