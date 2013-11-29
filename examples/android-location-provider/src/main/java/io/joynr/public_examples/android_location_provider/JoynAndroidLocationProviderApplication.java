package io.joynr.public_examples.android_location_provider;

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
import joynr.vehicle.GpsProvider;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import android.app.Application;

public class JoynAndroidLocationProviderApplication extends Application {
    @SuppressWarnings("unused")
    private static final Logger logger = LoggerFactory.getLogger(JoynAndroidLocationProviderApplication.class);
    private static final String LOCATION_PROVIDER_DOMAIN = "android_location";
    private JoynrAndroidRuntime runtime;
    private Output output;
    private AndroidLocationProvider androidLocationProvider;

    @Override
    public void onCreate() {
        super.onCreate();
        runtime = new JoynrAndroidRuntime(getApplicationContext());
    }

    public void registerProvider() {
        if (androidLocationProvider == null) {
            logToOutput("Creating new location provider\n");
            androidLocationProvider = new AndroidLocationProvider("", this.getApplicationContext(), output);
        }
        if (runtime != null) {
            new Thread(new Runnable() {

                @Override
                public void run() {
                    logToOutput("Registering provider\n");
                    // registers the provider at the global capabilities directory
                    runtime.registerCapability(LOCATION_PROVIDER_DOMAIN,
                                               androidLocationProvider,
                                               GpsProvider.class,
                                               "android-location-provider");
                }
            }).start();
        } else {
            logToOutput("Failed to bind service. Can not register provider\n");
        }

    }

    private void logToOutput(String string) {
        if (output != null) {
            output.append(string);
        }
    }

    public void setOutput(Output output) {
        this.output = output;
    }

}
