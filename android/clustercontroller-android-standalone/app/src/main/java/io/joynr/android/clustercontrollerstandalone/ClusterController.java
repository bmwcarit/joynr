/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
package io.joynr.android.clustercontrollerstandalone;

import android.content.Context;

import com.google.inject.Injector;
import com.google.inject.Module;
import com.google.inject.util.Modules;

import java.util.Properties;

import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.mqtt.paho.client.MqttPahoModule;
import io.joynr.messaging.websocket.WebsocketModule;
import io.joynr.runtime.CCWebSocketRuntimeModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.JoynrRuntime;

public class ClusterController {

    public static final String STATIC_PERSISTENCE_FILE = "clustercontroller-joynr.properties";
    public static final String STATIC_PARTICIPANTS_FILE = "joynr.properties_participants";

    public static final int PORT = 4242;
    public static final String HOST = "localhost";

    public static JoynrRuntime run(Context context, String brokerUri) {

        Properties ccConfig = new Properties();
        ccConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_HOST, HOST);
        ccConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PORT, "" + PORT);
        ccConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PROTOCOL, "ws");
        ccConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PATH, "");

        ccConfig.setProperty(MessagingPropertyKeys.PERSISTENCE_FILE, context.getCacheDir() + "/" + STATIC_PERSISTENCE_FILE);
        ccConfig.setProperty(ConfigurableMessagingSettings.PROPERTY_PARTICIPANTIDS_PERSISISTENCE_FILE,
                context.getCacheDir() + "/" + STATIC_PARTICIPANTS_FILE);


        ccConfig.setProperty(ConfigurableMessagingSettings.PROPERTY_CC_CONNECTION_TYPE, "WEBSOCKET");
        ccConfig.put("joynr.messaging.mqtt.brokerUris", brokerUri);

        Module runtimeModule = new CCWebSocketRuntimeModule();
        Module backendTransportModules = new MqttPahoModule();
        runtimeModule = Modules.override(runtimeModule).with(backendTransportModules);

        Injector injectorCC = new JoynrInjectorFactory(ccConfig, runtimeModule).getInjector();

        return injectorCC.getInstance(JoynrRuntime.class);
    }

}
