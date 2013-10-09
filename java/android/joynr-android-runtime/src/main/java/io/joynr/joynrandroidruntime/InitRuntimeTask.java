package io.joynr.joynrandroidruntime;

/*
 * #%L
 * joynr::java::android::joynr-android-runtime
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

import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.runtime.JoynrRuntimeImpl;
import io.joynr.runtime.PropertyLoader;

import java.io.File;
import java.util.Properties;

import android.content.Context;
import android.os.AsyncTask;
import android.util.Log;

import com.google.inject.Injector;

public class InitRuntimeTask extends AsyncTask<Object, String, JoynrRuntime> {

    public static final long INIT_TIMEOUT = 30000;
    private UILogger uiLogger;
    private Context applicationContext;

    public InitRuntimeTask(Context applicationContext, UILogger uiLogger) {
        this.applicationContext = applicationContext;
        this.uiLogger = uiLogger;
    }

    @Override
    protected JoynrRuntime doInBackground(Object... params) {
        try {
            Log.d("JAS", "starting Joyn Runtime");
            publishProgress("Starting Joyn Runtime...\n");

            // TODO get properties path from params
            Properties properties = PropertyLoader.loadProperties("res/raw/demo.properties");

            // create/make persistence file absolute
            File appWorkingDir = applicationContext.getFilesDir();
            String persistenceFileName = appWorkingDir.getPath()
                    + File.separator
                    + properties.getProperty(MessagingPropertyKeys.PERSISTENCE_FILE,
                                             MessagingPropertyKeys.DEFAULT_PERSISTENCE_FILE);
            properties.setProperty(MessagingPropertyKeys.PERSISTENCE_FILE, persistenceFileName);

            // create/make participant ID persistence file absolute
            String participantIdPersistenceFileName = appWorkingDir.getPath()
                    + File.separator
                    + properties.getProperty(ConfigurableMessagingSettings.PROPERTY_PARTICIPANTIDS_PERSISISTENCE_FILE,
                                             ConfigurableMessagingSettings.DEFAULT_PARTICIPANTIDS_PERSISTENCE_FILE);
            properties.setProperty(ConfigurableMessagingSettings.PROPERTY_PARTICIPANTIDS_PERSISISTENCE_FILE,
                                   participantIdPersistenceFileName);

            publishProgress("Properties loaded\n");

            properties.setProperty(ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_CLIENT_REQUEST_TIMEOUT, "120000");

            Injector injectorA = new JoynrInjectorFactory(properties).createChildInjector();

            JoynrRuntimeImpl runtime = injectorA.getInstance(JoynrRuntimeImpl.class);
            if (runtime != null) {
                Log.d("JAS", "Joyn Runtime started");
            } else {
                Log.e("JAS", "Joyn runtime not started");
            }
            publishProgress("Joyn Runtime started.\n");

            return runtime;

        } catch (Exception e) {
            e.printStackTrace();
            publishProgress(e.getMessage());
        }
        return null;
    }

    protected void onProgressUpdate(String... progress) {
        uiLogger.logText(progress);
    }

};
