package io.joynr.common;

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

import io.joynr.guice.PropertyLoadingModule;
import io.joynr.messaging.MessagingPropertiesPersistence;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.runtime.PropertyLoader;

import java.io.File;
import java.util.Properties;

import com.google.inject.Provides;
import com.google.inject.name.Named;

public class JoynrPropertiesModule extends PropertyLoadingModule {
    public static final String PATTERN_DOESNT_START_WITH_JOYNAPP = "^(?!joynrapp).*$";
    static final String PATTERN_STARTS_WITH_JOYN = "^(joynr)|(JOYNR).*$";

    public JoynrPropertiesModule(Properties customProperties) {
        Properties defaultMessagingProperties = PropertyLoader.loadProperties(MessagingPropertyKeys.DEFAULT_MESSAGING_PROPERTIES_FILE);
        Properties envPropertiesAll = new Properties();
        envPropertiesAll.putAll(System.getenv());
        Properties joynrEnvProperties = PropertyLoader.getPropertiesWithPattern(envPropertiesAll,
                                                                                PATTERN_STARTS_WITH_JOYN);

        Properties systemProperties = PropertyLoader.getPropertiesWithPattern(System.getProperties(),
                                                                              PATTERN_DOESNT_START_WITH_JOYNAPP);
        String persistenceFileName = getProperty(MessagingPropertyKeys.PERSISTENCE_FILE,
                                                 MessagingPropertyKeys.DEFAULT_PERSISTENCE_FILE,
                                                 defaultMessagingProperties,
                                                 customProperties,
                                                 joynrEnvProperties,
                                                 systemProperties);
        File persistencePropertiesFile = new File(persistenceFileName);
        MessagingPropertiesPersistence storage = new MessagingPropertiesPersistence(persistencePropertiesFile);

        mergeProperties(defaultMessagingProperties,
                        storage.getPersistedProperties(),
                        customProperties,
                        joynrEnvProperties,
                        systemProperties);

    }

    @Provides
    @Named(MessagingPropertyKeys.JOYNR_PROPERTIES)
    Properties provideProperties() {
        return properties;
    }

}
