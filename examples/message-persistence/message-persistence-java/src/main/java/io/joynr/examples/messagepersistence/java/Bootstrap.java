/*-
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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
package io.joynr.examples.messagepersistence.java;

import static io.joynr.runtime.AbstractJoynrApplication.PROPERTY_JOYNR_DOMAIN_LOCAL;

import java.util.Properties;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.AbstractModule;
import com.google.inject.Module;
import com.google.inject.util.Modules;

import io.joynr.examples.messagepersistence.SimpleFileBasedMessagePersister;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.mqtt.hivemq.client.HivemqMqttClientModule;
import io.joynr.messaging.persistence.MessagePersister;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrApplication;
import io.joynr.runtime.JoynrInjectorFactory;

public class Bootstrap {

    private static final Logger logger = LoggerFactory.getLogger(Bootstrap.class);

    public static final void main(String[] args) {
        logger.info("Starting java provider application ...");

        Properties joynrProperties = new Properties();
        joynrProperties.setProperty(MessagingPropertyKeys.PERSISTENCE_FILE, "java-application-joynr.properties");
        joynrProperties.setProperty(PROPERTY_JOYNR_DOMAIN_LOCAL, "io.joynr.examples.messagepersistence");
        // limit parallel processing of requests
        joynrProperties.setProperty(ConfigurableMessagingSettings.PROPERTY_MESSAGING_MAXIMUM_PARALLEL_SENDS, "2");

        Module runtimeModule = Modules.combine(new CCInProcessRuntimeModule(), new HivemqMqttClientModule());
        runtimeModule = Modules.override(runtimeModule).with(new AbstractModule() {
            @Override
            protected void configure() {
                bind(MessagePersister.class).toInstance(new SimpleFileBasedMessagePersister());
            }
        });

        JoynrInjectorFactory joynrInjectorFactory = new JoynrInjectorFactory(joynrProperties, runtimeModule);

        JoynrApplication application = joynrInjectorFactory.createApplication(MessagePersistenceJavaApplication.class);
        logger.info("Application created.");

        application.run();

        logger.info("Application finished. Shutting down.");
        application.shutdown();
    }
}
