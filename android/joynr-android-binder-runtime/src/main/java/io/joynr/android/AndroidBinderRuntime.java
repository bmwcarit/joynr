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

import static io.joynr.android.AndroidBinderRuntimeUtils.getClusterControllerServicePackageName;
import static io.joynr.android.AndroidBinderRuntimeUtils.getDefaultJoynrProperties;
import static io.joynr.android.AndroidBinderRuntimeUtils.getJoynrInjectorFactory;

import android.content.Context;

import com.google.inject.Injector;
import com.google.inject.Module;
import com.google.inject.util.Modules;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.Properties;

import io.joynr.android.messaging.binder.util.BinderConstants;
import io.joynr.messaging.mqtt.hivemq.client.HivemqMqttClientModule;
import io.joynr.runtime.CCBinderRuntimeModule;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.runtime.LibjoynrBinderRuntimeModule;
import joynr.system.RoutingTypes.BinderAddress;

public class AndroidBinderRuntime {

    public static final String PROPERTY_CONTEXT_ANDROID = "joynr_context_android";

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
    public static JoynrRuntime initClusterController(Context context,
                                                     String brokerUri,
                                                     Properties properties,
                                                     Module... modules) {

        // set default joynr properties
        final Properties config = getDefaultJoynrProperties(context);

        if (properties != null && !properties.isEmpty()) {
            // override with possible developer specified properties
            config.putAll(properties);
        }

        config.put("joynr.messaging.mqtt.brokerUris", brokerUri);

        Module runtimeModule = new CCBinderRuntimeModule(context);
        final Module backendTransportModules = new HivemqMqttClientModule();
        runtimeModule = Modules.override(runtimeModule).with(backendTransportModules);
        for (Module module : modules) {
            runtimeModule = Modules.override(runtimeModule).with(module);
        }

        injector = getJoynrInjectorFactory(config, runtimeModule).getInjector();
        runtime = injector.getInstance(JoynrRuntime.class);

        logger.info("Started Android CC runtime...");

        return runtime;
    }

    /**
     * Static method that creates a {@link JoynrRuntime} for Android developers with default configurations.
     *
     * @param context The application context.
     * @return A {@link JoynrRuntime} object
     */
    public static JoynrRuntime init(Context context) {
        String clusterControllerPackageName = getClusterControllerServicePackageName(context);
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
        String clusterControllerPackageName = getClusterControllerServicePackageName(context);
        return init(context, properties, clusterControllerPackageName);
    }

    /**
     * Static method that creates a {@link JoynrRuntime} for Android developers with default configurations.
     *
     * @param context                      The application context.
     * @param properties                   Extra properties that can configure joynr runtime.
     * @param clusterControllerPackageName The package name of the joynr cluster controller.
     * @return A {@link JoynrRuntime} object or null if there is no Cluster Controller in the system
     */

    private static JoynrRuntime init(Context context, Properties properties, String clusterControllerPackageName) {

        if (clusterControllerPackageName != null && !clusterControllerPackageName.isEmpty()) {

            // set default joynr properties
            final Properties config = getDefaultJoynrProperties(context);

            if (properties != null && !properties.isEmpty()) {
                // override with possible developer specified properties
                config.putAll(properties);
            }

            BinderAddress ccBinderAddress = new BinderAddress(clusterControllerPackageName, BinderConstants.USER_ID_SYSTEM);
            injector = getJoynrInjectorFactory(config, new LibjoynrBinderRuntimeModule(context, ccBinderAddress))
                    .createChildInjector();
            runtime = injector.getInstance(JoynrRuntime.class);

            logger.info("Started Libjoynr runtime...");
        }

        return runtime;
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

    /**
     * Used for resetting static value of {@link JoynrRuntime} for testing
     * @param runtime
     */
    protected static void setRuntime(JoynrRuntime runtime) {
        AndroidBinderRuntime.runtime = runtime;
    }

    /**
     * Used for resetting static value of {@link Injector} for testing
     * @param injector
     */
    protected static void setInjector(Injector injector) {
        AndroidBinderRuntime.injector = injector;
    }
}
