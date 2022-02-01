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
package io.joynr.test.interlanguage;

import java.util.Properties;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.Module;
import com.google.inject.util.Modules;

import io.joynr.common.JoynrPropertiesModule;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.mqtt.hivemq.client.HivemqMqttClientModule;
import io.joynr.messaging.websocket.WebsocketModule;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.CCWebSocketRuntimeModule;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.runtime.LibjoynrWebSocketRuntimeModule;
import io.joynr.util.ObjectMapper;
import joynr.types.ProviderQos;

public class IltProviderApplication {
    private static final Logger logger = LoggerFactory.getLogger(IltProviderApplication.class);
    public static final String STATIC_PERSISTENCE_FILE = "java-provider.persistence_file";

    private static String localDomain;
    private static JoynrRuntime runtime;
    private static IltProvider provider = null;
    private static ObjectMapper jsonSerializer;

    private static boolean shutDownRequested = false;

    public static void main(String[] args) throws Exception {
        if (args.length != 1 && args.length != 2) {
            logger.error("\n\nUSAGE: java {} <local-domain>\n\n NOTE: Providers are registered on the local domain.",
                         IltProviderApplication.class.getName());
            return;
        }
        localDomain = args[0];
        logger.debug("Registering provider on domain \"{}\"", localDomain);

        Properties joynrConfig = new Properties();
        Module runtimeModule = getRuntimeModule(args, joynrConfig);

        logger.debug("Using the following runtime module: " + runtimeModule.getClass().getSimpleName());
        logger.debug("Registering provider on domain \"{}\"", localDomain);

        joynrConfig.setProperty(MessagingPropertyKeys.PERSISTENCE_FILE, STATIC_PERSISTENCE_FILE);

        Injector injector = Guice.createInjector(runtimeModule, new JoynrPropertiesModule(joynrConfig));
        runtime = injector.getInstance(JoynrRuntime.class);
        jsonSerializer = injector.getInstance(ObjectMapper.class);
        run();
        shutdown();
    }

    private static Module getRuntimeModule(String[] args, Properties joynrConfig) {
        Module runtimeModule;
        if (args.length > 1) {
            String transport = args[1].toLowerCase();
            if (transport.contains("websocketcc")) {
                configureWebSocket(joynrConfig);
                runtimeModule = new CCWebSocketRuntimeModule();
            } else if (transport.contains("websocket")) {
                configureWebSocket(joynrConfig);
                runtimeModule = new LibjoynrWebSocketRuntimeModule();
            } else {
                runtimeModule = new CCInProcessRuntimeModule();
            }

            Module backendTransportModules = Modules.EMPTY_MODULE;

            if (transport.contains("mqtt")) {
                logger.info("Configuring MQTT...");
                backendTransportModules = Modules.combine(backendTransportModules, new HivemqMqttClientModule());
            }
            return Modules.override(runtimeModule).with(backendTransportModules);
        }
        return new CCInProcessRuntimeModule();
    }

    private static void configureWebSocket(Properties joynrConfig) {
        joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_HOST, "localhost");
        joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PORT, "4242");
        joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PROTOCOL, "ws");
        joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PATH, "/");
    }

    public static void run() {
        provider = new IltProvider();
        provider.addBroadcastFilter(new IltStringBroadcastFilter(jsonSerializer));
        ProviderQos providerQos = new ProviderQos();
        providerQos.setPriority(System.currentTimeMillis());

        runtime.getProviderRegistrar(localDomain, provider).withProviderQos(providerQos).register();

        while (!shutDownRequested) {
            try {
                Thread.sleep(1000);
            } catch (Exception e) {
                // no handling
            }
        }
    }

    public static void shutdown() {
        logger.info("shutting down");
        if (provider != null) {
            try {
                runtime.unregisterProvider(localDomain, provider);
            } catch (JoynrRuntimeException e) {
                logger.error("unable to unregister capabilities {}", e.getMessage());
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

}
