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

import io.joynr.provider.ProviderAnnotations;
import io.joynr.accesscontrol.StaticDomainAccessControlProvisioning;
import io.joynr.accesscontrol.StaticDomainAccessControlProvisioningModule;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.websocket.WebsocketModule;
import io.joynr.runtime.AbstractJoynrApplication;
import io.joynr.runtime.JoynrApplication;
import io.joynr.runtime.JoynrApplicationModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.LibjoynrWebSocketRuntimeModule;
import joynr.types.ProviderQos;

import java.util.Properties;

import joynr.infrastructure.DacTypes.MasterAccessControlEntry;
import joynr.infrastructure.DacTypes.Permission;
import joynr.infrastructure.DacTypes.TrustLevel;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.ObjectMapper.DefaultTyping;
import com.google.inject.Module;

import edu.umd.cs.findbugs.annotations.SuppressWarnings;

public class ProviderApplication extends AbstractJoynrApplication {
    private static final Logger LOG = LoggerFactory.getLogger(ProviderApplication.class);
    public static final String STATIC_PERSISTENCE_FILE = "java-provider.persistence_file";

    private Provider provider = null;
    private static boolean runForever = false;
    private static String localDomain;

    public static void main(String[] args) throws Exception {
        if ((args.length != 1 && args.length != 2) || (args.length == 2 && !args[1].equals("runForever"))) {
            LOG.error("\n\nUSAGE: java {} <local-domain> [runForever]\n\n NOTE: Providers are registered on the local domain.",
                      ProviderApplication.class.getName());
            return;
        }
        localDomain = args[0];

        Properties joynrConfig = new Properties();
        Module runtimeModule = getRuntimeModule(args, joynrConfig);

        LOG.debug("Using the following runtime module: " + runtimeModule.getClass().getSimpleName());
        LOG.debug("Registering provider on domain \"{}\"", localDomain);

        joynrConfig.setProperty(MessagingPropertyKeys.PERSISTENCE_FILE, STATIC_PERSISTENCE_FILE);
        joynrConfig.setProperty(PROPERTY_JOYNR_DOMAIN_LOCAL, localDomain);
        Properties appConfig = new Properties();

        // Use injected static provisioning of access control entries to allow access to anyone to this interface
        provisionAccessControl(joynrConfig, localDomain);
        JoynrApplication joynrApplication = new JoynrInjectorFactory(joynrConfig,
                                                                     runtimeModule,
                                                                     new StaticDomainAccessControlProvisioningModule()).createApplication(new JoynrApplicationModule(ProviderApplication.class, appConfig));

        if (args.length == 2) {
            runForever = true;
        }
        joynrApplication.run();
        joynrApplication.shutdown();
    }

    private static Module getRuntimeModule(String[] args, Properties joynrConfig) {
        configureWebSocket(joynrConfig);
        return new LibjoynrWebSocketRuntimeModule();
    }

    private static void configureWebSocket(Properties joynrConfig) {
        joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_HOST, "localhost");
        joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PORT, "4242");
        joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PROTOCOL, "ws");
        joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PATH, "");
    }

    @Override
    public void run() {
        provider = new Provider();
        ProviderQos providerQos = new ProviderQos();
        providerQos.setPriority(System.currentTimeMillis());

        // access to provider is needed inside the hook, so it must be added here
        Thread shutdownHook = new Thread() {
            @Override
            public void run() {
                LOG.info("executing shutdown hook");
                synchronized (this) {
                    LOG.info("notifying any waiting thread from shutdown hook");
                    notifyAll();
                }
                LOG.info("shutting down");
                if (provider != null) {
                    try {
                        runtime.unregisterProvider(localDomain, provider);
                    } catch (JoynrRuntimeException e) {
                        LOG.error("unable to unregister capabilities {}", e.getMessage());
                    }
                }
                runtime.shutdown(false);
                LOG.info("shutdown completed");
            }
        };
        LOG.info("adding shutdown hook");
        Runtime.getRuntime().addShutdownHook(shutdownHook);

        runtime.registerProvider(localDomain, provider, providerQos);

        try {
            if (!runForever) {
                Thread.sleep(30000);
            } else {
                synchronized (shutdownHook) {
                    shutdownHook.wait();
                }
            }
        } catch (Exception e) {
            // terminate execution by continuing
        }
    }

    @Override
    @SuppressWarnings(value = "DM_EXIT", justification = "WORKAROUND to be removed")
    public void shutdown() {
        System.exit(0);
    }

    private static void provisionAccessControl(Properties properties, String domain) throws Exception {
        ObjectMapper objectMapper = new ObjectMapper();
        objectMapper.enableDefaultTypingAsProperty(DefaultTyping.JAVA_LANG_OBJECT, "_typeName");
        MasterAccessControlEntry newMasterAccessControlEntry = new MasterAccessControlEntry("*",
                                                                                            domain,
                                                                                            ProviderAnnotations.getInterfaceName(Provider.class),
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
