package io.joynr.capabilities.directory;

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

import java.util.Properties;

import com.google.inject.Inject;
import com.google.inject.persist.PersistService;
import com.google.inject.persist.jpa.JpaPersistModule;
import com.google.inject.util.Modules;

import io.joynr.messaging.routing.GlobalAddressFactory;
import io.joynr.messaging.routing.TestGlobalAddressModule;
import io.joynr.runtime.AbstractJoynrApplication;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrApplication;
import io.joynr.runtime.JoynrApplicationModule;
import io.joynr.runtime.JoynrInjectorFactory;
import joynr.infrastructure.GlobalCapabilitiesDirectoryAbstractProvider;
import joynr.system.RoutingTypes.ChannelAddress;
import joynr.types.ProviderQos;

class MockChannelAddressFactory extends GlobalAddressFactory<ChannelAddress> {

    @Override
    public ChannelAddress create() {
        return new ChannelAddress("messagingEndpointUrl", "channelId");
    }

    @Override
    public boolean supportsTransport(String transport) {
        return "longpolling".equals(transport);
    }
}

public class CapabilitiesDirectoryLauncher extends AbstractJoynrApplication {

    private static CapabilitiesDirectoryImpl capabilitiesDirectory;

    private static JoynrApplication capabilitiesDirectoryLauncher;

    @Inject
    private GlobalCapabilitiesDirectoryAbstractProvider capabilitiesDirectoryProvider;

    private PersistService persistService;

    public static void main(String[] args) {

        start(new Properties());
    }

    public static void start(Properties joynrConfig) {

        // LongPollingMessagingModule is only added in main(), since the servletMessagingModule will be used otherwise
        JoynrInjectorFactory injectorFactory = new JoynrInjectorFactory(joynrConfig,
                                                                        Modules.override(new JpaPersistModule("CapabilitiesDirectory"),
                                                                                         new CCInProcessRuntimeModule())
                                                                               .with(new TestGlobalAddressModule(),
                                                                                     new CapabilitiesDirectoryModule()));
        capabilitiesDirectoryLauncher = injectorFactory.createApplication(new JoynrApplicationModule("capabilitiesDirectoryLauncher",
                                                                                                     CapabilitiesDirectoryLauncher.class));
        capabilitiesDirectoryLauncher.run();
        capabilitiesDirectory = injectorFactory.getInjector().getInstance(CapabilitiesDirectoryImpl.class);
    }

    public static void stop() {
        capabilitiesDirectoryLauncher.shutdown();
    }

    @Inject
    public CapabilitiesDirectoryLauncher(PersistService persistService) {
        this.persistService = persistService;
    }

    @Override
    public void run() {
        // LongPollingMessagingModule is only added in main(), since the servletMessagingModule will be used otherwise
        ProviderQos providerQos = new ProviderQos();
        runtime.registerProvider(localDomain, capabilitiesDirectoryProvider, providerQos);
    }

    @Override
    public void shutdown() {
        persistService.stop();
        runtime.shutdown(true);
    }

    static CapabilitiesDirectoryImpl getCapabilitiesDirctory() {
        return capabilitiesDirectory;
    }

}
