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
package io.joynr.capabilities.directory;

import java.util.Properties;

import com.google.inject.Inject;
import com.google.inject.persist.PersistService;
import com.google.inject.persist.jpa.JpaPersistModule;
import com.google.inject.util.Modules;
import com.google.inject.Module;

import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.mqtt.paho.client.MqttPahoModule;
import io.joynr.runtime.AbstractJoynrApplication;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrApplication;
import io.joynr.runtime.JoynrApplicationModule;
import io.joynr.runtime.JoynrInjectorFactory;
import joynr.infrastructure.GlobalCapabilitiesDirectoryAbstractProvider;
import joynr.types.ProviderQos;

public class CapabilitiesDirectoryLauncher extends AbstractJoynrApplication {

    private static CapabilitiesDirectoryImpl capabilitiesDirectory;
    private static JoynrApplication capabilitiesDirectoryLauncher;

    private static Module getRuntimeModule(Properties joynrConfig) {
        joynrConfig.put("joynr.messaging.mqtt.brokerUri", "tcp://localhost:1883");
        joynrConfig.put(MessagingPropertyKeys.PROPERTY_MESSAGING_PRIMARYGLOBALTRANSPORT, "mqtt");

        return Modules.override(new JpaPersistModule("CapabilitiesDirectory"), new CCInProcessRuntimeModule())
                      .with(new MqttPahoModule(), new CapabilitiesDirectoryModule());
    }

    @Inject
    private GlobalCapabilitiesDirectoryAbstractProvider capabilitiesDirectoryProvider;

    private PersistService persistService;

    public static void start(Properties joynrConfig) {
        Module runtimeModule = getRuntimeModule(joynrConfig);
        JoynrInjectorFactory injectorFactory = new JoynrInjectorFactory(joynrConfig, runtimeModule);
        capabilitiesDirectoryLauncher = injectorFactory.createApplication(new JoynrApplicationModule("capabilitiesDirectoryLauncher",
                                                                                                     CapabilitiesDirectoryLauncher.class));
        capabilitiesDirectoryLauncher.run();
        capabilitiesDirectory = injectorFactory.getInjector().getInstance(CapabilitiesDirectoryImpl.class);
    }

    public static void stop() {
        capabilitiesDirectoryLauncher.shutdown();
    }

    static CapabilitiesDirectoryImpl getCapabilitiesDirectory() {
        return capabilitiesDirectory;
    }

    @Inject
    public CapabilitiesDirectoryLauncher(PersistService persistService) {
        this.persistService = persistService;
    }

    @Override
    public void run() {
        ProviderQos providerQos = new ProviderQos();
        runtime.registerProvider(localDomain, capabilitiesDirectoryProvider, providerQos);
    }

    @Override
    public void shutdown() {
        persistService.stop();
        runtime.shutdown(true);
    }

    public static void main(String[] args) {
        start(new Properties());
    }
}
