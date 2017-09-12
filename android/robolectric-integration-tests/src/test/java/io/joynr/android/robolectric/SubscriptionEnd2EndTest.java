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
package io.joynr.android.robolectric;

import com.google.inject.Injector;
import com.google.inject.Module;
import io.joynr.android.test.TestActivity;
import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.integration.AbstractSubscriptionEnd2EndTest;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.websocket.WebsocketModule;
import io.joynr.runtime.CCWebSocketRuntimeModule;
import io.joynr.runtime.ClusterControllerRuntime;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.servlet.ServletUtil;
import joynr.types.CustomParameter;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;

import org.junit.After;
import org.junit.Before;
import org.junit.Ignore;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;

import java.util.Properties;

@RunWith(RobolectricTestRunner.class)
@Config(manifest = "./src/test/AndroidManifest.xml")
@Ignore
public class SubscriptionEnd2EndTest extends AbstractSubscriptionEnd2EndTest {

    private TestActivity activity;
    private ClusterControllerRuntime ccJoynrRuntime;
    private Properties webSocketConfig;

    @Override
    @Before
    public void setUp() throws Exception {
        final int port = ServletUtil.findFreePort();
        webSocketConfig = new Properties();
        webSocketConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_HOST, "localhost");
        webSocketConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PORT, "" + port);
        webSocketConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PROTOCOL, "ws");
        webSocketConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PATH, "");

        providerQos = new ProviderQos(new CustomParameter[0], System.currentTimeMillis(), ProviderScope.LOCAL, false);
        discoveryQos = new DiscoveryQos(10000,
                                        ArbitrationStrategy.HighestPriority,
                                        Long.MAX_VALUE,
                                        DiscoveryScope.LOCAL_ONLY);
        super.setUp();
        // Uncomment to log the verbose android logs to stdout
        //ShadowLog.stream = System.out;
    }

    @Override
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
    protected JoynrRuntime getRuntime(Properties joynrConfig, Module... modules) {
        if (ccJoynrRuntime == null) {
            ccJoynrRuntime = createClusterController(webSocketConfig);
        }
        if (activity == null) {
            activity = Robolectric.buildActivity(TestActivity.class).create().get();
        }

        joynrConfig.putAll(webSocketConfig);
        return activity.createRuntime(joynrConfig, modules);
    }
}
