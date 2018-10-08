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

import java.io.IOException;
import java.net.ServerSocket;
import java.util.Properties;

import com.google.inject.Module;
import com.google.inject.util.Modules;

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.mqtt.paho.client.MqttPahoModule;
import io.joynr.runtime.AbstractJoynrApplication;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrApplication;
import io.joynr.runtime.JoynrApplicationModule;
import io.joynr.runtime.JoynrInjectorFactory;

import joynr.exceptions.ApplicationException;
import joynr.infrastructure.GlobalDomainAccessControllerAbstractProvider;
import joynr.infrastructure.GlobalDomainRoleControllerAbstractProvider;
import joynr.infrastructure.GlobalDomainAccessControlListEditorAbstractProvider;
import joynr.types.ProviderQos;

import com.google.inject.Inject;

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

        return Modules.override(new CCInProcessRuntimeModule()).with(new MqttPahoModule(),
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
            ProviderQos providerQos = new ProviderQos();
            runtime.registerProvider(localDomain, globalDomainAccessSyncProvider, providerQos).get();
            globalDomainAccessSyncProviderRegistered = true;
            runtime.registerProvider(localDomain, globalDomainRoleSyncProvider, providerQos).get();
            globalDomainRoleSyncProviderRegistered = true;
            runtime.registerProvider(localDomain, globalDomainAccessControlListEditorSyncProvider, providerQos).get();
            globalDomainAccessControlListEditorSyncProviderRegistered = true;
            startOk = true;
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
