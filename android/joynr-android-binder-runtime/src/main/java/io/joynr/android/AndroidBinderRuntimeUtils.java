/*-
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
package io.joynr.android;

import android.content.Context;
import android.content.Intent;
import android.content.pm.ResolveInfo;

import com.google.inject.Module;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.List;
import java.util.Properties;

import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.runtime.JoynrInjectorFactory;

public class AndroidBinderRuntimeUtils {

    private static final String STATIC_PERSISTENCE_FILE = "clustercontroller-joynr.properties";
    private static final String STATIC_PARTICIPANTS_FILE = "joynr.properties_participants";

    private static final Logger logger = LoggerFactory.getLogger(AndroidBinderRuntimeUtils.class);

    protected static String getClusterControllerServicePackageName(Context context) {
        Intent intent = new Intent("io.joynr.android.action.COMMUNICATE");

        List<ResolveInfo> services = context.getPackageManager().queryIntentServices(intent, 0);
        if (services == null || services.isEmpty()) {
            logger.error("There is no joynr cluster controller app installed!");
            return null;
        }
        return services.get(0).serviceInfo.applicationInfo.packageName;
    }

    protected static Properties getDefaultJoynrProperties(Context context) {

        final Properties config = new Properties();
        config.setProperty(MessagingPropertyKeys.PERSISTENCE_FILE,
                context.getFilesDir() + "/" + STATIC_PERSISTENCE_FILE);
        config.setProperty(ConfigurableMessagingSettings.PROPERTY_PARTICIPANTIDS_PERSISTENCE_FILE,
                context.getFilesDir() + "/" + STATIC_PARTICIPANTS_FILE);

        return config;
    }

    protected static JoynrInjectorFactory getJoynrInjectorFactory(Properties config, Module runtimeModule) {
        return new JoynrInjectorFactory(config, runtimeModule);
    }

}
