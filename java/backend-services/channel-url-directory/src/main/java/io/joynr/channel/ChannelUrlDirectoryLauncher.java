package io.joynr.channel;

/*
 * #%L
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

import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.runtime.AbstractJoynrApplication;
import io.joynr.runtime.JoynrApplication;
import io.joynr.runtime.JoynrApplicationModule;
import io.joynr.runtime.JoynrInjectorFactory;

import java.util.Properties;

import joynr.infrastructure.ChannelUrlDirectoryAbstractProvider;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.name.Named;

public class ChannelUrlDirectoryLauncher extends AbstractJoynrApplication {

    private static final String AUTH_TOKEN = "ChannelUrlDirectoryLauncher";
    Logger logger = LoggerFactory.getLogger(ChannelUrlDirectoryLauncher.class);

    @Inject
    private ChannelUrlDirectoryAbstractProvider channelUrlDirectoryProvider;

    @Inject
    @Named(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_DIRECTORIES_DOMAIN)
    String discoveryDirectoriesDomain;

    // private static String channelId;

    public static void main(String[] args) {

        start(new Properties());
    }

    public static void start(Properties joynrConfig) {
        JoynrInjectorFactory injectorFactory = new JoynrInjectorFactory(joynrConfig, new ChannelUrlDirectoryModule());
        JoynrApplication channelUrlDirectoryLauncher = injectorFactory.createApplication(new JoynrApplicationModule(ChannelUrlDirectoryLauncher.class));
        channelUrlDirectoryLauncher.run();
    }

    @Override
    public void run() {
        runtime.registerProvider(discoveryDirectoriesDomain, channelUrlDirectoryProvider, AUTH_TOKEN);
    }

    @Override
    public void shutdown() {
        logger.error("SHUTDOWN!");
        // runtime.unregisterProvider(channelUrlDirectoryDomain,
        // channelUrlDirectoryProvider,
        // ChannelUrlDirectoryProvider.class);
    }

}
