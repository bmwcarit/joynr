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

import com.fasterxml.jackson.databind.ObjectMapper.DefaultTyping;
import com.google.inject.Inject;
import com.google.inject.Module;
import com.google.inject.util.Modules;

import io.joynr.accesscontrol.StaticDomainAccessControlProvisioning;
import io.joynr.accesscontrol.StaticDomainAccessControlProvisioningModule;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.mqtt.hivemq.client.HivemqMqttClientModule;
import io.joynr.messaging.websocket.WebsocketModule;
import io.joynr.provider.ProviderAnnotations;
import io.joynr.runtime.AbstractJoynrApplication;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.CCWebSocketRuntimeModule;
import io.joynr.runtime.JoynrApplication;
import io.joynr.runtime.JoynrApplicationModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.LibjoynrWebSocketRuntimeModule;
import io.joynr.util.ObjectMapper;
import joynr.infrastructure.DacTypes.MasterAccessControlEntry;
import joynr.infrastructure.DacTypes.Permission;
import joynr.infrastructure.DacTypes.TrustLevel;
import joynr.types.ProviderQos;

public class IltProviderApplication extends AbstractJoynrApplication {
    private static final Logger logger = LoggerFactory.getLogger(IltProviderApplication.class);
    public static final String STATIC_PERSISTENCE_FILE = "java-provider.persistence_file";

    private IltProvider provider = null;
    @Inject
    private ObjectMapper jsonSerializer;

    private boolean shutDownRequested = false;

    public static void main(String[] args) throws Exception {
        if (args.length != 1 && args.length != 2) {
            logger.error("\n\nUSAGE: java {} <local-domain>\n\n NOTE: Providers are registered on the local domain.",
                         IltProviderApplication.class.getName());
            return;
        }
        String localDomain = args[0];
        logger.debug("Registering provider on domain \"{}\"", localDomain);

        Properties joynrConfig = new Properties();
        Module runtimeModule = getRuntimeModule(args, joynrConfig);

        logger.debug("Using the following runtime module: " + runtimeModule.getClass().getSimpleName());
        logger.debug("Registering provider on domain \"{}\"", localDomain);

        joynrConfig.setProperty(MessagingPropertyKeys.PERSISTENCE_FILE, STATIC_PERSISTENCE_FILE);
        joynrConfig.setProperty(PROPERTY_JOYNR_DOMAIN_LOCAL, localDomain);
        Properties appConfig = new Properties();

        // Use injected static provisioning of access control entries to allow access to anyone to this interface
        provisionAccessControl(joynrConfig, localDomain);
        JoynrApplication joynrApplication = new JoynrInjectorFactory(joynrConfig,
                                                                     runtimeModule,
                                                                     new StaticDomainAccessControlProvisioningModule()).createApplication(new JoynrApplicationModule(IltProviderApplication.class, appConfig));
        joynrApplication.run();
        joynrApplication.shutdown();
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
                joynrConfig.put(MessagingPropertyKeys.PROPERTY_MESSAGING_PRIMARYGLOBALTRANSPORT, "mqtt");
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
        joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PATH, "");
    }

    @Override
    public void run() {
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

    @Override
    public void shutdown() {
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

    private static void provisionAccessControl(Properties properties, String domain) throws Exception {
        ObjectMapper objectMapper = new ObjectMapper();
        objectMapper.enableDefaultTypingAsProperty(DefaultTyping.JAVA_LANG_OBJECT, "_typeName");
        MasterAccessControlEntry newMasterAccessControlEntry = new MasterAccessControlEntry("*",
                                                                                            domain,
                                                                                            ProviderAnnotations.getInterfaceName(IltProvider.class),
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
