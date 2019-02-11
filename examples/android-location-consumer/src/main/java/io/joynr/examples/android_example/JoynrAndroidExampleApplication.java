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
package io.joynr.examples.android_example;

import java.util.Properties;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import android.app.Application;

import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.joynrandroidruntime.JoynrAndroidRuntime;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.ProxyBuilder;
import joynr.vehicle.GpsProxy;

public class JoynrAndroidExampleApplication extends Application {
    private static final Logger logger = LoggerFactory.getLogger(JoynrAndroidExampleApplication.class);
    private static final String PROVIDER_DOMAIN = "android-domain";

    private JoynrAndroidRuntime runtime;
    public GpsProxy proxy;
    private Output output;

    @Override
    public void onCreate() {
        logger.info("onCreate JoynrAndroidExampleApplication");
        super.onCreate();
    }

    public void initJoynrRuntime(Properties joynrConfig) {
        logToOutput("Creating joynr Runtime.");
        runtime = new JoynrAndroidRuntime(getApplicationContext(), joynrConfig);
    }

    public void createProxy() {
        if (runtime == null) {
            logger.error("runtime has not been initialized!");
            logToOutput("runtime has not been initialized!\n");
        }

        logToOutput("Creating joynr GPS proxy and requesting location...\n");
        MessagingQos messagingQos = new MessagingQos(2 * 60 * 1000); // 2 minutes ttl
        DiscoveryQos discoveryQos = new DiscoveryQos(30 * 1000, // 30 second timeout to find a provider
                                                     ArbitrationStrategy.HighestPriority,
                                                     Integer.MAX_VALUE,
                                                     DiscoveryScope.LOCAL_ONLY);
        try {
            ProxyBuilder<GpsProxy> builder = runtime.getProxyBuilder(PROVIDER_DOMAIN, GpsProxy.class);

            builder.setDiscoveryQos(discoveryQos)
                   .setMessagingQos(messagingQos)
                   .build(new ProxyBuilder.ProxyCreatedCallback<GpsProxy>() {

                       @Override
                       public void onProxyCreationFinished(GpsProxy newProxy) {
                           logToOutput("Proxy created");
                           proxy = newProxy;

                       }

                       @Override
                       public void onProxyCreationError(JoynrRuntimeException error) {
                           logToOutput("Error during proxy creation: " + error.getMessage() + "\n");

                       }
                   });

        } catch (Exception e) {
            logToOutput("ERROR: create proxy failed: " + e.getMessage() + "\n");
            logger.error("create proxy failed: ", e);
        }

    }

    public void getGpsLocation() {
        new GetGpsTask(output).execute(proxy);
    }

    public void subscribeToGpsLocation() {
        new CreateSubscriptionTask(output).execute(proxy);
    }

    private void logToOutput(String string) {
        if (output != null) {
            output.append(string);
            output.append("\n");
        }
    }

    public void setOutput(Output output) {
        this.output = output;
    }
}
