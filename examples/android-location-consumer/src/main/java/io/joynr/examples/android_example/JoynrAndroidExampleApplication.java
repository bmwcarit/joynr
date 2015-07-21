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

import io.joynr.joynrandroidruntime.JoynrAndroidRuntime;
import io.joynr.messaging.MessagingPropertyKeys;

import java.util.Properties;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import android.app.Application;

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
        logToOutput("Bounceproxy URL: " + joynrConfig.getProperty(MessagingPropertyKeys.BOUNCE_PROXY_URL));
        logToOutput("Channel URL Directory: " + joynrConfig.getProperty(MessagingPropertyKeys.CHANNELURLDIRECTORYURL));
        logToOutput("Capabilities Directory: "
                + joynrConfig.getProperty(MessagingPropertyKeys.CAPABILITIESDIRECTORYURL));
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
