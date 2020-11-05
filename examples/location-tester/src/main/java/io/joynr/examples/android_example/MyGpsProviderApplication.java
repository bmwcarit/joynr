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
package io.joynr.examples.android_example;

import java.io.Console;
import java.util.Properties;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.databind.ObjectMapper.DefaultTyping;
import com.google.inject.Module;
import com.google.inject.util.Modules;

import io.joynr.accesscontrol.StaticDomainAccessControlProvisioning;
import io.joynr.accesscontrol.StaticDomainAccessControlProvisioningModule;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.AtmosphereMessagingModule;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.websocket.WebsocketModule;
import io.joynr.provider.ProviderAnnotations;
import io.joynr.runtime.AbstractJoynrApplication;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrApplication;
import io.joynr.runtime.JoynrApplicationModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.LibjoynrWebSocketRuntimeModule;
import io.joynr.util.ObjectMapper;
import joynr.infrastructure.DacTypes.MasterAccessControlEntry;
import joynr.infrastructure.DacTypes.Permission;
import joynr.infrastructure.DacTypes.TrustLevel;
import joynr.types.ProviderQos;

public class MyGpsProviderApplication extends AbstractJoynrApplication {
    private static final Logger logger = LoggerFactory.getLogger(MyGpsProviderApplication.class);
    public static final String STATIC_PERSISTENCE_FILE = "gps-provider-joynr.properties";

    private MyGpsProvider provider = null;

    public static void main(String[] args) throws Exception {
        // run application from cmd line using Maven:
        // mvn exec:java -Dexec.mainClass="io.joynr.demo.MyRadioProviderApplication" -Dexec.args="<local-domain>"
        // Get the provider domain from the command line
        if (args.length != 1 && args.length != 2) {
            logger.error("\n\nUSAGE: java {} <local-domain> [websocket] \n\n NOTE: Providers are registered on the local domain.",
                         MyGpsProviderApplication.class.getName());
            return;
        }
        String localDomain = args[0];
        logger.debug("Registering provider on domain \"{}\"", localDomain);

        // joynr config properties are used to set joynr configuration at
        // compile time. They are set on the
        // JoynrInjectorFactory.
        Properties joynrConfig = new Properties();
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
        // joynrConfig.setProperty(MqttModule.PROPERTY_MQTT_BROKER_URIS, "tcp://localhost:1883/");

        // Each joynr instance has a local domain. It identifies the execution
        // device/platform, e.g. the
        // vehicle. Normally, providers on that instance are registered for the
        // local domain.
        joynrConfig.setProperty(PROPERTY_JOYNR_DOMAIN_LOCAL, localDomain);

        // NOTE: When running this application to test the android-location-provider, you must use
        //       the concrete hostname (and _not_ localhost) in the bounceproxy URL, since this URL
        //       is registered in the global discovery directory and must be resolvable by the Android
        //       device.
        joynrConfig.setProperty(MessagingPropertyKeys.BOUNCE_PROXY_URL, "http://<concrete host>:8080/bounceproxy/");

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
        // joynr.messaging.mqtt.brokeruris=tcp://localhost:1883/

        // 3) Or set them in Java System properties.
        // -Djoynr.messaging.gbids=joynrdefaultgbid
        // -Djoynr.messaging.mqtt.brokeruris=tcp://localhost:1883/

        // NOTE:
        // Programmatically set configuration properties override properties set
        // in the static persistence file.
        // Java system properties override both

        // Application-specific configuration properties are injected to the
        // application by setting
        // them on the JoynApplicationModule.
        Properties appConfig = new Properties();
        provisionAccessControl(joynrConfig, localDomain);

        Module runtimeModule = null;
        if (args.length == 2 && args[1].equalsIgnoreCase("websocket")) {
            joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_HOST, "localhost");
            joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PORT, "4242");
            joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PROTOCOL, "ws");
            joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PATH, "");
            runtimeModule = new LibjoynrWebSocketRuntimeModule();
        } else {
            runtimeModule = Modules.override(new CCInProcessRuntimeModule()).with(new AtmosphereMessagingModule());
        }
        logger.debug("Using the following runtime module: " + runtimeModule.getClass().getSimpleName());

        JoynrApplication joynrApplication = new JoynrInjectorFactory(joynrConfig,
                                                                     new StaticDomainAccessControlProvisioningModule(),
                                                                     runtimeModule).createApplication(new JoynrApplicationModule(MyGpsProviderApplication.class, appConfig));
        joynrApplication.run();

        joynrApplication.shutdown();
    }

    @Override
    public void run() {
        ProviderQos providerQos = new ProviderQos();
        provider = new MyGpsProvider();
        providerQos.setPriority(System.currentTimeMillis());
        runtime.getProviderRegistrar(localDomain, provider).withProviderQos(providerQos).register();

        Console console = System.console();
        if (console != null) {
            String key = "";
            while (!key.equals("q")) {
                key = console.readLine();

                switch (key) {
                case "l":
                    provider.notifyLocationUpdate();
                    break;
                default:
                    logger.info("\n\nUSAGE press\n" + " q\tto quit\n" + " l\tto update location\n");
                    break;
                }
            }
        } else {
            logger.info("\n\nNon-interactive mode detected.\n");
        }
    }

    @Override
    public void shutdown() {
        logger.info("shutting down");
        if (provider != null) {
            try {
                runtime.unregisterProvider(localDomain, provider);
            } catch (JoynrRuntimeException e) {
                logger.error("unable to unregister capabilities", e);
            }
        }
        runtime.shutdown(true);
        // TODO currently there is a bug preventing all threads being stopped
        // WORKAROUND
        try {
            Thread.sleep(3000);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }
        System.exit(0);
    }

    private static void provisionAccessControl(Properties properties, String domain) throws Exception {
        ObjectMapper objectMapper = new ObjectMapper();
        objectMapper.enableDefaultTypingAsProperty(DefaultTyping.JAVA_LANG_OBJECT, "_typeName");
        MasterAccessControlEntry newMasterAccessControlEntry = new MasterAccessControlEntry("*",
                                                                                            domain,
                                                                                            ProviderAnnotations.getInterfaceName(MyGpsProvider.class),
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
