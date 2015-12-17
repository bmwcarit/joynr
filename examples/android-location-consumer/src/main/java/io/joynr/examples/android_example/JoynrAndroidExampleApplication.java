package io.joynr.examples.android_example;

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

import java.net.URI;
import java.net.URISyntaxException;
import java.util.Properties;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import android.app.Application;
import io.joynr.joynrandroidruntime.JoynrAndroidRuntime;
import io.joynr.messaging.websocket.WebsocketModule;

public class JoynrAndroidExampleApplication extends Application {
    private static final Logger logger = LoggerFactory.getLogger(JoynrAndroidExampleApplication.class);

    private final JoynrAndroidExampleLauncher joynrAndroidExampleLauncher = new JoynrAndroidExampleLauncher();

    private JoynrAndroidRuntime runtime;
    private Output output;

    @Override
    public void onCreate() {
        logger.info("onCreate JoynrAndroidExampleApplication");
        super.onCreate();
    }

    public void initJoynrRuntime(Properties joynrConfig) {
        logToOutput("Creating joynr Runtime.");
        Properties webSocketConfig = new Properties();
        String clusterControllerUrlString = "ws://10.0.2.2:4242";
        try {
            URI clusterControllerUrl = new URI(clusterControllerUrlString);
            webSocketConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_HOST,
                                        clusterControllerUrl.getHost());
            webSocketConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PORT, ""
                    + clusterControllerUrl.getPort());
            webSocketConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PROTOCOL,
                                        clusterControllerUrl.getScheme());
            webSocketConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PATH, "");
            joynrConfig.putAll(webSocketConfig);
        } catch (URISyntaxException e) {
            logger.info("Cluster Controller WebSocket URL is invalid: " + clusterControllerUrlString + " error: "
                    + e.getMessage());

        }

        runtime = new JoynrAndroidRuntime(getApplicationContext(), joynrConfig);

        joynrAndroidExampleLauncher.setJoynAndroidRuntime(runtime);

    }

    private void logToOutput(String string) {
        if (output != null) {
            output.append(string);
            output.append("\n");
        }
    }

    public void setOutput(Output output) {
        this.output = output;
        joynrAndroidExampleLauncher.setOutput(output);

    }

    public JoynrAndroidExampleLauncher getJoynAndroidExampleLauncher() {
        return joynrAndroidExampleLauncher;
    }
}
