package io.joynr.integration;

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

import com.google.inject.Injector;
import com.google.inject.Module;
import com.google.inject.util.Modules;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.websocket.WebsocketModule;
import io.joynr.runtime.CCWebSocketRuntimeModule;
import io.joynr.runtime.ClusterControllerRuntime;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.runtime.LibJoynrRuntime;
import io.joynr.runtime.LibjoynrWebSocketRuntimeModule;
import io.joynr.servlet.ServletUtil;
import org.junit.After;
import org.junit.Before;

import java.util.Properties;

/**
 *
 */
public class WebSocketProviderProxyEnd2EndTest extends ProviderProxyEnd2EndTest {

    private ClusterControllerRuntime ccJoynrRuntime;
    private Properties webSocketConfig;

    @Before
    @Override
    public void baseSetup() throws Exception {
        final int port = ServletUtil.findFreePort();
        webSocketConfig = new Properties();
        webSocketConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_HOST, "localhost");
        webSocketConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PORT, "" + port);
        webSocketConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PROTOCOL, "ws");
        webSocketConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PATH, "");

        //init websocket properties before libjoynr runtimes are created
        super.baseSetup();
    }

    @After
    public void tearDown() throws InterruptedException {
        super.tearDown();
        ccJoynrRuntime.shutdown(false);
    }

    private ClusterControllerRuntime createClusterController(Properties webSocketConfig) {
        Properties ccConfig = new Properties();
        ccConfig.putAll(webSocketConfig);
        ccConfig.setProperty(ConfigurableMessagingSettings.PROPERTY_CC_CONNECTION_TYPE, "WEBSOCKET");
        Injector injectorCC = new JoynrInjectorFactory(ccConfig, new CCWebSocketRuntimeModule()).getInjector();
        return (ClusterControllerRuntime) injectorCC.getInstance(JoynrRuntime.class);
    }

    @Override
    protected JoynrRuntime getRuntime(final Properties joynrConfig, final Module... modules) {
        if (ccJoynrRuntime == null) {
            ccJoynrRuntime = createClusterController(webSocketConfig);
        }
        joynrConfig.putAll(webSocketConfig);
        joynrConfig.setProperty(ConfigurableMessagingSettings.PROPERTY_CC_CONNECTION_TYPE, "WEBSOCKET");
        Injector injectorLib = new JoynrInjectorFactory(joynrConfig, Modules.override(modules)
                                                                            .with(new LibjoynrWebSocketRuntimeModule())).getInjector();
        return injectorLib.getInstance(LibJoynrRuntime.class);
    }
}
