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
package io.joynr.accesscontrol.global;

import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_DOMAIN_ACCESS_CONTROLLER_PARTICIPANT_ID;
import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_DOMAIN_ACCESS_CONTROL_LISTEDITOR_PARTICIPANT_ID;
import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_DOMAIN_ROLE_CONTROLLER_PARTICIPANT_ID;

import java.io.IOException;
import java.net.ServerSocket;
import java.util.Properties;

import com.google.inject.Inject;
import com.google.inject.Module;
import com.google.inject.util.Modules;

import io.joynr.capabilities.ParticipantIdKeyUtil;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.mqtt.hivemq.client.HivemqMqttClientModule;
import io.joynr.runtime.AbstractJoynrApplication;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrApplication;
import io.joynr.runtime.JoynrApplicationModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.PropertyLoader;
import joynr.exceptions.ApplicationException;
import joynr.infrastructure.GlobalDomainAccessControlListEditorAbstractProvider;
import joynr.infrastructure.GlobalDomainAccessControlListEditorProvider;
import joynr.infrastructure.GlobalDomainAccessControllerAbstractProvider;
import joynr.infrastructure.GlobalDomainAccessControllerProvider;
import joynr.infrastructure.GlobalDomainRoleControllerAbstractProvider;
import joynr.infrastructure.GlobalDomainRoleControllerProvider;

public class GlobalDomainAccessControllerLauncher extends AbstractJoynrApplication {
    private static final String APP_ID = "GlobalDomainAccessControllerLauncher";
    private static int shutdownPort = Integer.parseInt(System.getProperty("joynr.globaldomainaccesscontrollerlauncher.shutdownport",
                                                                          "9998"));

    @Inject
    private GlobalDomainAccessControllerAbstractProvider globalDomainAccessSyncProvider;

    @Inject
    private GlobalDomainRoleControllerAbstractProvider globalDomainRoleSyncProvider;

    @Inject
    private GlobalDomainAccessControlListEditorAbstractProvider globalDomainAccessControlListEditorSyncProvider;

    private static boolean startOk;

    private boolean globalDomainAccessSyncProviderRegistered;
    private boolean globalDomainRoleSyncProviderRegistered;
    private boolean globalDomainAccessControlListEditorSyncProviderRegistered;

    private static JoynrApplication globalDomainAccessControllerLauncher;

    private static Module getRuntimeModule(Properties joynrConfig) {
        joynrConfig.put("joynr.messaging.mqtt.brokerUri", "tcp://localhost:1883");
        joynrConfig.put(MessagingPropertyKeys.PROPERTY_MESSAGING_PRIMARYGLOBALTRANSPORT, "mqtt");

        Properties joynrDefaultProperties = PropertyLoader.loadProperties(MessagingPropertyKeys.DEFAULT_MESSAGING_PROPERTIES_FILE);

        if (joynrDefaultProperties.containsKey(PROPERTY_DOMAIN_ACCESS_CONTROLLER_PARTICIPANT_ID)) {
            joynrConfig.put(ParticipantIdKeyUtil.getProviderParticipantIdKey("io.joynr",
                                                                             GlobalDomainAccessControllerProvider.class),
                            joynrDefaultProperties.getProperty(PROPERTY_DOMAIN_ACCESS_CONTROLLER_PARTICIPANT_ID));
        }

        if (joynrDefaultProperties.containsKey(PROPERTY_DOMAIN_ACCESS_CONTROL_LISTEDITOR_PARTICIPANT_ID)) {
            joynrConfig.put(ParticipantIdKeyUtil.getProviderParticipantIdKey("io.joynr",
                                                                             GlobalDomainAccessControlListEditorProvider.class),
                            joynrDefaultProperties.getProperty(PROPERTY_DOMAIN_ACCESS_CONTROL_LISTEDITOR_PARTICIPANT_ID));
        }

        if (joynrDefaultProperties.containsKey(PROPERTY_DOMAIN_ROLE_CONTROLLER_PARTICIPANT_ID)) {
            joynrConfig.put(ParticipantIdKeyUtil.getProviderParticipantIdKey("io.joynr",
                                                                             GlobalDomainRoleControllerProvider.class),
                            joynrDefaultProperties.getProperty(PROPERTY_DOMAIN_ROLE_CONTROLLER_PARTICIPANT_ID));
        }

        return Modules.override(new CCInProcessRuntimeModule()).with(new HivemqMqttClientModule(),
                                                                     new GlobalDomainAccessControllerModule());
    }

    private static void waitUntilShutdownRequested() {
        try {
            ServerSocket serverSocket = new ServerSocket(shutdownPort);
            serverSocket.accept();
            serverSocket.close();
        } catch (IOException e) {
            return;
        }
        return;
    }

    private static void setStartOk(boolean value) {
        startOk = value;
    }

    public static void main(String[] args) {
        GlobalDomainAccessControllerLauncher.start();
    }

    public static void start() {
        start(new Properties());
        if (startOk) {
            waitUntilShutdownRequested();
        }
        globalDomainAccessControllerLauncher.shutdown();
    }

    public static void start(Properties joynrConfig) {

        Module runtimeModule = getRuntimeModule(joynrConfig);
        JoynrInjectorFactory injectorFactory = new JoynrInjectorFactory(joynrConfig, runtimeModule);

        JoynrApplication domainAccessControllerLauncherApp = injectorFactory.createApplication(new JoynrApplicationModule(APP_ID,
                                                                                                                          GlobalDomainAccessControllerLauncher.class));
        domainAccessControllerLauncherApp.run();

        globalDomainAccessControllerLauncher = domainAccessControllerLauncherApp;
    }

    @Override
    public void run() {
        try {
            runtime.getProviderRegistrar(localDomain, globalDomainAccessSyncProvider).register().get();
            globalDomainAccessSyncProviderRegistered = true;
            runtime.getProviderRegistrar(localDomain, globalDomainRoleSyncProvider).register().get();
            globalDomainRoleSyncProviderRegistered = true;
            runtime.getProviderRegistrar(localDomain, globalDomainAccessControlListEditorSyncProvider).register().get();
            globalDomainAccessControlListEditorSyncProviderRegistered = true;
            GlobalDomainAccessControllerLauncher.setStartOk(true);
        } catch (JoynrRuntimeException | ApplicationException | InterruptedException e) {
            // ignore
        }
    }

    @Override
    public void shutdown() {
        if (globalDomainAccessSyncProviderRegistered) {
            runtime.unregisterProvider(localDomain, globalDomainAccessSyncProvider);
            if (globalDomainRoleSyncProviderRegistered) {
                runtime.unregisterProvider(localDomain, globalDomainRoleSyncProvider);
                if (globalDomainAccessControlListEditorSyncProviderRegistered) {
                    runtime.unregisterProvider(localDomain, globalDomainAccessControlListEditorSyncProvider);
                }
            }
        }

        // wait some time to let the runtime handle the unregistration
        // requests before the external connection gets terminated by a
        // shutdown
        try {
            Thread.sleep(5000);
        } catch (InterruptedException e) {
            // ignore
        }
        runtime.shutdown(false);
    }
}
