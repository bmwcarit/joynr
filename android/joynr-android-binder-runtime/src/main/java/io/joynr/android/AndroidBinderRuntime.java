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

import com.google.inject.Injector;
import com.google.inject.Module;
import com.google.inject.util.Modules;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.List;
import java.util.Properties;

import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.mqtt.paho.client.MqttPahoModule;
import io.joynr.runtime.CCBinderRuntimeModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.runtime.LibjoynrBinderRuntimeModule;
import joynr.system.RoutingTypes.BinderAddress;

public class AndroidBinderRuntime {

    public static final String PROPERTY_CONTEXT_ANDROID = "joynr_context_android";

    private static final String STATIC_PERSISTENCE_FILE = "clustercontroller-joynr.properties";
    private static final String STATIC_PARTICIPANTS_FILE = "joynr.properties_participants";
    private static final String STATIC_PERSISTENCE_SUSBCRIPTION_REQUESTS = "joynr_persistedSubscriptionRequests";

    private static final Logger logger = LoggerFactory.getLogger(AndroidBinderRuntime.class);

    /**
     * The guice injector used to instantiate the {@code runtime}
     */
    private static Injector injector;
    /**
     * The current {@link JoynrRuntime} being used
     */
    private static JoynrRuntime runtime;

    /**
     * Static method that creates a cluster controller {@link JoynrRuntime} for Android developers with default configurations.
     * ATTENTION: This method shouldn't be called by joynr app developers as the cluster controller is usually running in another app/process.
     *
     * @param context    The Application context
     * @param brokerUri  The mqtt broker uri
     * @param properties Extra properties that can configure joynr runtime.
     * @param modules    Extra modules that can configure joynr runtime.
     * @return A {@link JoynrRuntime} object
     */
    public static JoynrRuntime initClusterController(Context context, String brokerUri, Properties properties, Module... modules) {

        // set default joynr properties
        final Properties config = getDefaultJoynrProperties(context);

        // override with possible developer specified properties
        config.putAll(properties);

        config.put("joynr.messaging.mqtt.brokerUris", brokerUri);

        Module runtimeModule = new CCBinderRuntimeModule(context);
        final Module backendTransportModules = new MqttPahoModule();
        runtimeModule = Modules.override(runtimeModule).with(backendTransportModules);
        for (Module module : modules) {
            runtimeModule = Modules.override(runtimeModule).with(module);
        }

        injector = new JoynrInjectorFactory(config, runtimeModule).getInjector();
        runtime = injector.getInstance(JoynrRuntime.class);

        logger.debug("Started Android CC runtime...");

        return runtime;
    }


    /**
     * Static method that creates a {@link JoynrRuntime} for Android developers with default configurations.
     *
     * @param context The application context.
     * @return A {@link JoynrRuntime} object
     */
    public static JoynrRuntime init(Context context) {
        String clusterControllerPackageName = getClusterControlerServicePackagename(context);
        return init(context, new Properties(), clusterControllerPackageName);
    }

    /**
     * Static method that creates a {@link JoynrRuntime} for Android developers with extra properties.
     *
     * @param context    The application context.
     * @param properties Extra properties that can configure joynr runtime.
     * @return A {@link JoynrRuntime} object
     */
    public static JoynrRuntime init(Context context, Properties properties) {
        String clusterControllerPackageName = getClusterControlerServicePackagename(context);
        return init(context, properties, clusterControllerPackageName);
    }


    /**
     * Static method that creates a {@link JoynrRuntime} for Android developers with default configurations.
     *
     * @param context                      The application context.
     * @param properties                   Extra properties that can configure joynr runtime.
     * @param clusterControllerPackageName The package name of the joynr cluster controller.
     * @return A {@link JoynrRuntime} object
     */

    private static JoynrRuntime init(Context context, Properties properties, String clusterControllerPackageName) {

        // set default joynr properties
        final Properties config = getDefaultJoynrProperties(context);

        // override with possible developer specified properties
        config.putAll(properties);

        BinderAddress ccBinderAddress = new BinderAddress(clusterControllerPackageName);
        injector = new JoynrInjectorFactory(config, new LibjoynrBinderRuntimeModule(context, ccBinderAddress))
                .createChildInjector();
        runtime = injector.getInstance(JoynrRuntime.class);

        logger.debug("Started Libjoynr runtime...");

        return runtime;
    }

    private static String getClusterControlerServicePackagename(Context context) {
        Intent intent = new Intent("io.joynr.android.action.COMMUNICATE");

        List<ResolveInfo> services = context.getPackageManager().queryIntentServices(intent, 0);
        if (services == null || services.isEmpty()) {
            logger.error("There is no joynr cluster controller app installed!");
            throw new io.joynr.exceptions.JoynrRuntimeException("There is no joynr cluster controller app installed!");
        }
        return services.get(0).serviceInfo.applicationInfo.packageName;
    }

    private static Properties getDefaultJoynrProperties(Context context) {

        final Properties config = new Properties();
        config.setProperty(MessagingPropertyKeys.PERSISTENCE_FILE,
                context.getFilesDir() + "/" + STATIC_PERSISTENCE_FILE);
        config.setProperty(ConfigurableMessagingSettings.PROPERTY_PARTICIPANTIDS_PERSISISTENCE_FILE,
                context.getFilesDir() + "/" + STATIC_PARTICIPANTS_FILE);

        config.setProperty(ConfigurableMessagingSettings.PROPERTY_SUBSCRIPTIONREQUESTS_PERSISISTENCE_FILE,
                context.getFilesDir() + "/" + STATIC_PERSISTENCE_SUSBCRIPTION_REQUESTS);

        return config;
    }


    /**
     * @return The guice injector used to instantiate the {@code runtime}
     */
    public static Injector getInjector() {
        return injector;
    }

    /**
     * @return The current {@link JoynrRuntime} being used
     */
    public static JoynrRuntime getRuntime() {
        return runtime;
    }
}