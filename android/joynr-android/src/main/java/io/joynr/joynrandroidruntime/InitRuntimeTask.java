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
package io.joynr.joynrandroidruntime;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Properties;

import android.content.Context;
import android.os.AsyncTask;
import android.util.Log;

import com.google.inject.Injector;
import com.google.inject.Module;
import com.google.inject.util.Modules;

import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.runtime.LibjoynrWebSocketRuntimeModule;
import io.joynr.runtime.PropertyLoader;

public class InitRuntimeTask extends AsyncTask<Object, String, JoynrRuntime> {

    public static final long INIT_TIMEOUT = 30000;
    private UILogger uiLogger;
    private Context applicationContext;
    private Properties joynrConfig;
    private List<Module> modules;

    public InitRuntimeTask(Context applicationContext, UILogger uiLogger) {
        this(PropertyLoader.loadProperties("res/raw/demo.properties"),
             applicationContext,
             uiLogger,
             Collections.<Module> emptyList());
    }

    public InitRuntimeTask(Properties joynrConfig,
                           Context applicationContext,
                           UILogger uiLogger,
                           List<Module> joynrModules) {
        this.joynrConfig = joynrConfig;
        this.applicationContext = applicationContext;
        this.uiLogger = uiLogger;

        // Make note of custom modules and add what is needed for Android long polling
        this.modules = new ArrayList<Module>();
        this.modules.addAll(joynrModules);
    }

    @Override
    protected JoynrRuntime doInBackground(Object... params) {
        try {
            Log.d("JAS", "starting joynr runtime");
            publishProgress("Starting joynr runtime...\n");

            // create/make persistence file absolute
            File appWorkingDir = applicationContext.getFilesDir();
            String persistenceFileName = appWorkingDir.getPath()
                    + File.separator
                    + joynrConfig.getProperty(MessagingPropertyKeys.PERSISTENCE_FILE,
                                              MessagingPropertyKeys.DEFAULT_PERSISTENCE_FILE);
            joynrConfig.setProperty(MessagingPropertyKeys.PERSISTENCE_FILE, persistenceFileName);

            // create/make participant ID persistence file absolute
            String participantIdPersistenceFileName = appWorkingDir.getPath()
                    + File.separator
                    + joynrConfig.getProperty(ConfigurableMessagingSettings.PROPERTY_PARTICIPANTIDS_PERSISISTENCE_FILE,
                                              ConfigurableMessagingSettings.DEFAULT_PARTICIPANTIDS_PERSISTENCE_FILE);
            joynrConfig.setProperty(ConfigurableMessagingSettings.PROPERTY_PARTICIPANTIDS_PERSISISTENCE_FILE,
                                    participantIdPersistenceFileName);

            publishProgress("Properties loaded\n");

            // Create an injector with all the required custom modules.
            // CCInProcessRuntimeModule is used by default but can be overwritten by custom modules
            Module combinedModules = Modules.override(new LibjoynrWebSocketRuntimeModule()).with(modules);
            Injector injectorA = new JoynrInjectorFactory(joynrConfig, combinedModules).createChildInjector();

            JoynrRuntime runtime = injectorA.getInstance(JoynrRuntime.class);
            if (runtime != null) {
                Log.d("JAS", "joynr runtime started");
            } else {
                Log.e("JAS", "joynr runtime not started");
            }
            publishProgress("joynr runtime started.\n");

            return runtime;

        } catch (Exception e) {
            Log.e("JAS", "joynr runtime not started", e);
            publishProgress(e.getMessage());
        }
        return null;
    }

    @Override
    protected void onProgressUpdate(String... progress) {
        uiLogger.logText(progress);
    }

};
