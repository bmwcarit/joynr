package io.joynr.runtime;

import java.io.IOException;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
import java.util.Set;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Injector;

import io.joynr.capabilities.CapabilitiesStore;
import io.joynr.capabilities.CapabilityEntry;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.websocket.WebsocketModule;
import jline.console.ConsoleReader;

public class ClusterController {
    private static final Logger LOG = LoggerFactory.getLogger(ClusterController.class);
    private static Properties webSocketConfig;
    private static JoynrRuntime runtime;

    public static void main(String[] args) {
        final int port = 4242;
        webSocketConfig = new Properties();
        webSocketConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_HOST, "localhost");
        webSocketConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PORT, "" + port);
        webSocketConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PROTOCOL, "ws");
        webSocketConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PATH, "");
        Properties ccConfig = new Properties();
        ccConfig.putAll(webSocketConfig);
        ccConfig.setProperty(ConfigurableMessagingSettings.PROPERTY_CC_CONNECTION_TYPE, "WEBSOCKET");
        Injector injectorCC = new JoynrInjectorFactory(ccConfig, new CCWebSocketRuntimeModule()).getInjector();

        runtime = injectorCC.getInstance(JoynrRuntime.class);
        CapabilitiesStore capabilities = injectorCC.getInstance(CapabilitiesStore.class);

        ConsoleReader console;
        try {
            console = new ConsoleReader();
            String command = "";
            while (!command.equals("q")) {
                command = console.readLine();

                if (command.equals("caps")) {
                    Set<CapabilityEntry> allCapabilities = capabilities.getAllCapabilities();
                    StringBuffer capabilitiesAsText = new StringBuffer();
                    for (CapabilityEntry capability : allCapabilities) {
                        capabilitiesAsText.append(capability.toString()).append('\n');
                    }
                    LOG.info(capabilitiesAsText.toString());
                } else {
                    LOG.info("\n\nUSAGE press\n" + " q\tto quit\n caps\tto list registered providers\n");
                }
            }
        } catch (IOException e) {
            LOG.error("error reading input from console", e);
        }
        LOG.info("shutting down");
        runtime.shutdown(false);
        System.exit(0);
    }
}
