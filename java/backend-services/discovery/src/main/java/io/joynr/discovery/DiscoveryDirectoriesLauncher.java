package io.joynr.discovery;

/*
 * #%L
 * joynr::java::backend-services::capabilities directory
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

import io.joynr.runtime.AbstractJoynrApplication;
import io.joynr.runtime.JoynrApplication;
import io.joynr.runtime.JoynrApplicationModule;
import io.joynr.runtime.JoynrInjectorFactory;

import java.util.Properties;

import joynr.infrastructure.ChannelUrlDirectoryAbstractProvider;
import joynr.infrastructure.ChannelUrlDirectoryProvider;
import joynr.infrastructure.GlobalCapabilitiesDirectoryAbstractProvider;
import joynr.infrastructure.GlobalCapabilitiesDirectoryProvider;

import com.google.inject.Inject;

public class DiscoveryDirectoriesLauncher extends AbstractJoynrApplication {

    private static final String AUTH_TOKEN = "DiscoveryDirectoryLauncher";

    @Inject
    private ChannelUrlDirectoryAbstractProvider channelUrlDirectoryProvider;

    @Inject
    private GlobalCapabilitiesDirectoryAbstractProvider capabilitiesDirectoryProvider;

    public static void main(String[] args) {
        DiscoveryDirectoriesLauncher.start();
    }

    public static DiscoveryDirectoriesLauncher start() {
        return start(new Properties());
    }

    public static DiscoveryDirectoriesLauncher start(Properties joynrConfig) {

        JoynrInjectorFactory injectorFactory = new JoynrInjectorFactory(joynrConfig, new DiscoveryDirectoriesModule());

        JoynrApplication discoveryDirectoryLauncher = injectorFactory.createApplication(new JoynrApplicationModule(AUTH_TOKEN,
                                                                                                                   DiscoveryDirectoriesLauncher.class));
        discoveryDirectoryLauncher.run();

        return (DiscoveryDirectoriesLauncher) discoveryDirectoryLauncher;
    }

    public DiscoveryDirectoriesLauncher() {
    }

    @Override
    public void run() {
        runtime.registerCapability(localDomain,
                                   channelUrlDirectoryProvider,
                                   ChannelUrlDirectoryProvider.class,
                                   AUTH_TOKEN);

        runtime.registerCapability(localDomain,
                                   capabilitiesDirectoryProvider,
                                   GlobalCapabilitiesDirectoryProvider.class,
                                   AUTH_TOKEN);

    }

    @Override
    public void shutdown() {
        // TODO Auto-generated method stub
        super.shutdown();
    }

}
