package io.joynr.example;

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

    @Override
    public void onCreate() {
        super.onCreate();
        // Replace with your bounceproxy's host name
        String backendHost = "YOURHOSTHERE:8080"; //TODO make this configurable
        Properties joynrConfig = new Properties();
        joynrConfig.setProperty(MessagingPropertyKeys.BOUNCE_PROXY_URL, "http://" + backendHost + "/bounceproxy/");
        joynrConfig.setProperty(MessagingPropertyKeys.CHANNELURLDIRECTORYURL, "http://" + backendHost
                + "/discovery/channels/discoverydirectory_channelid/");
        joynrConfig.setProperty(MessagingPropertyKeys.CAPABILITIESDIRECTORYURL, "http://" + backendHost
                + "/discovery/channels/discoverydirectory_channelid/");
        runtime = new JoynrAndroidRuntime(getApplicationContext(), joynrConfig);
        logger.info("onCreate JoynAndroidExampleApplication");
        joynrAndroidExampleLauncher.setJoynAndroidRuntime(runtime);

    }

    public void setOutput(Output output) {
        joynrAndroidExampleLauncher.setOutput(output);

    }

    public JoynrAndroidExampleLauncher getJoynAndroidExampleLauncher() {
        return joynrAndroidExampleLauncher;
    }
}
