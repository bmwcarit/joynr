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

import com.google.inject.Binder;
import com.google.inject.Injector;
import com.google.inject.Module;
import com.google.inject.util.Modules;
import io.joynr.accesscontrol.AccessControlClientModule;
import io.joynr.common.JoynrPropertiesModule;
import io.joynr.discovery.DiscoveryClientModule;
import io.joynr.dispatching.subscription.PubSubModule;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.LongPollingMessagingModule;
import io.joynr.messaging.websocket.WebsocketModule;
import io.joynr.runtime.CCWebSocketRuntimeModule;
import io.joynr.runtime.ClusterControllerRuntime;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.runtime.LibJoynrRuntime;
import io.joynr.runtime.LibJoynrRuntimeModule;
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
        Injector injectorCC = new JoynrInjectorFactory(new ClusterControllerModule(ccConfig), new WebsocketModule()).getInjector();
        return (ClusterControllerRuntime) injectorCC.getInstance(JoynrRuntime.class);
    }

    @Override
    protected JoynrRuntime getRuntime(final Properties joynrConfig, final Module... modules) {
        if (ccJoynrRuntime == null) {
            ccJoynrRuntime = createClusterController(webSocketConfig);
        }
        joynrConfig.putAll(webSocketConfig);
        joynrConfig.setProperty(ConfigurableMessagingSettings.PROPERTY_CC_CONNECTION_TYPE, "WEBSOCKET");
        Injector injectorLib = new JoynrInjectorFactory(new LibJoynrModule(joynrConfig, modules)).getInjector();
        return injectorLib.getInstance(LibJoynrRuntime.class);
    }

    private static class ClusterControllerModule implements Module {

        private Module module = null;
        private Properties ccConfig;

        private ClusterControllerModule(Properties ccConfig) {
            this.ccConfig = ccConfig;
        }

        @Override
        public void configure(Binder binder) {

            module = Modules.override(new JoynrPropertiesModule(ccConfig),
                                      new LongPollingMessagingModule(),
                                      new PubSubModule(),
                                      new DiscoveryClientModule(),
                                      new CCWebSocketRuntimeModule(),
                                      new AccessControlClientModule()).with();
            module.configure(binder);
        }
    }

    private static class LibJoynrModule implements Module {

        private final Properties joynrConfig;
        private final Module[] modules;
        private Module module;

        public LibJoynrModule(Properties joynrConfig, Module... modules) {
            this.joynrConfig = joynrConfig;
            this.modules = modules;
            module = null;
        }

        @Override
        public void configure(Binder binder) {
            module = Modules.override(new JoynrPropertiesModule(joynrConfig),
                                      new PubSubModule(),
                                      new DiscoveryClientModule(),
                                      new LongPollingMessagingModule(),
                                      //override AccessController with dummy implementation
                                      Modules.override(new AccessControlClientModule())
                                             .with(new WebsocketModule(), new LibJoynrRuntimeModule())).with(modules);
            module.configure(binder);
        }
    }

}
