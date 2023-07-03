/*
 * #%L
 * %%
 * Copyright (C) 2023 BMW Car IT GmbH
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
package io.joynr.examples.spring.service;

import io.joynr.examples.spring.consumer.ConsumerApp;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.runtime.JoynrApplicationModule;
import io.joynr.runtime.JoynrInjectorFactory;
import joynr.types.ProviderScope;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.beans.factory.config.ConfigurableBeanFactory;
import org.springframework.context.annotation.Scope;
import org.springframework.stereotype.Service;

import java.util.Properties;
import java.util.concurrent.Executors;

import static io.joynr.runtime.AbstractJoynrApplication.PROPERTY_JOYNR_DOMAIN_LOCAL;

@Scope(value = ConfigurableBeanFactory.SCOPE_SINGLETON)
@Service
public class ConsumerService {
    @Value("${joynr.consumer.persistence.file:consumer-joynr.properties}")
    private String staticPersistenceFile;
    @Value("${joynr.local.domain:domain}")
    private String localDomain;

    @Autowired
    private JoynrRuntimeService runtimeService;

    private ConsumerApp consumer;

    public ConsumerApp getConsumer() {
        if (consumer == null) {
            final Properties appConfig = new Properties();
            final Properties joynrConfig = new Properties();
            joynrConfig.setProperty(MessagingPropertyKeys.PERSISTENCE_FILE, staticPersistenceFile);
            joynrConfig.setProperty(PROPERTY_JOYNR_DOMAIN_LOCAL, localDomain);
            consumer = (ConsumerApp) new JoynrInjectorFactory(joynrConfig,
                                                              runtimeService.createRuntimeModule(joynrConfig)).createApplication(
                    new JoynrApplicationModule(ConsumerApp.class, appConfig) {
                        @Override
                        protected void configure() {
                            super.configure();
                            bind(ProviderScope.class).toInstance(ProviderScope.GLOBAL);
                        }
                    });

            Executors.newFixedThreadPool(1).submit(consumer);
        }
        return consumer;
    }
}
