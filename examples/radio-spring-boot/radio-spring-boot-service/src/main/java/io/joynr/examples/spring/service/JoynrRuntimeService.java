/*
 * #%L
 * %%
 * Copyright (C) 2023 BMW Car IT GmbH
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
package io.joynr.examples.spring.service;

import com.google.inject.Module;
import com.google.inject.util.Modules;
import io.joynr.messaging.mqtt.hivemq.client.HivemqMqttClientModule;
import io.joynr.messaging.websocket.WebsocketModule;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.CCWebSocketRuntimeModule;
import io.joynr.runtime.LibjoynrWebSocketRuntimeModule;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.beans.factory.config.ConfigurableBeanFactory;
import org.springframework.context.annotation.Scope;
import org.springframework.stereotype.Service;

import java.util.Properties;

@Scope(value = ConfigurableBeanFactory.SCOPE_SINGLETON)
@Service
public class JoynrRuntimeService {

    @Value("${joynr.transport:}")
    private String transport;
    @Value("${joynr.host:localhost}")
    private String host;
    @Value("${joynr.port:4242}")
    private int port;

    private Module getRuntimeModule(final String transport,
                                    final String host,
                                    final int port,
                                    final Properties joynrConfig) {
        Module runtimeModule;
        if (transport != null && !transport.isEmpty()) {
            if (transport.contains("websocketcc")) {
                configureWebSocket(host, port, joynrConfig);
                runtimeModule = new CCWebSocketRuntimeModule();
            } else if (transport.contains("websocket")) {
                configureWebSocket(host, port, joynrConfig);
                runtimeModule = new LibjoynrWebSocketRuntimeModule();
            } else {
                runtimeModule = new CCInProcessRuntimeModule();
            }

            Module backendTransportModules = Modules.EMPTY_MODULE;

            if (transport.contains("mqtt")) {
                backendTransportModules = Modules.combine(backendTransportModules, new HivemqMqttClientModule());
            }
            return Modules.override(runtimeModule).with(backendTransportModules);
        }
        return Modules.override(new CCInProcessRuntimeModule()).with(new HivemqMqttClientModule());
    }

    private void configureWebSocket(final String host, final int port, final Properties joynrConfig) {
        joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_HOST, host);
        joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PORT, String.valueOf(port));
        joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PROTOCOL, "ws");
        joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PATH, "/");
    }

    public Module createRuntimeModule(final Properties joynrConfig) {
        return getRuntimeModule(transport, host, port, joynrConfig);
    }
}
