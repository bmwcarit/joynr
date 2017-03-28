package io.joynr.examples.android_location_provider;

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

import io.joynr.joynrandroidruntime.JoynrAndroidRuntime;

import java.util.Properties;
import joynr.types.ProviderQos;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import android.app.Application;

public class JoynrAndroidLocationProviderApplication extends Application {
    @SuppressWarnings("unused")
    private static final Logger logger = LoggerFactory.getLogger(JoynrAndroidLocationProviderApplication.class);
    private JoynrAndroidRuntime runtime;
    private Output output;
    private AndroidLocationProvider androidLocationProvider;

    @Override
    public void onCreate() {
        super.onCreate();
    }

    public void initJoynrRuntime(Properties joynrConfig) {
        logToOutput("Creating joynr Runtime.");
        runtime = new JoynrAndroidRuntime(getApplicationContext(), joynrConfig);
        logToOutput("Finished creating joynr Runtime.");
    }

    public void registerProvider(final String domain) {
        if (androidLocationProvider == null) {
            logToOutput("Creating new location provider.");
            androidLocationProvider = new AndroidLocationProvider(this.getApplicationContext(), output);
        }
        if (runtime != null) {
            logToOutput("Registering provider on domain \"" + domain + "\".");
            // registers the provider at the global capabilities directory
            ProviderQos providerQos = new ProviderQos();
            providerQos.setPriority(System.currentTimeMillis());
            runtime.registerProvider(domain, androidLocationProvider, providerQos);
        } else {
            logToOutput("Failed to bind service. Can not register provider\n");
        }
    }

    public void unregisterProvider(final String domain) {
        if (androidLocationProvider == null) {
            return;
        }
        if (runtime != null) {
            logToOutput("Unegistering provider from domain \"" + domain + "\".");
            // registers the provider at the global capabilities directory
            runtime.unregisterProvider(domain, androidLocationProvider);
        } else {
            logToOutput("Failed to bind service. Can not register provider\n");
        }

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
