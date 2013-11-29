package io.joynr.runtime;

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

import io.joynr.guice.ApplicationModule;

import java.util.Properties;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Provides;
import com.google.inject.name.Named;

public class JoynrApplicationModule extends ApplicationModule {
    Logger logger = LoggerFactory.getLogger(JoynrApplicationModule.class);

    // private Properties joynrApplicationProperties = null;
    static final String PATTERN_STARTS_WITH_JOYNAPP = "^(joynrapp).*$";
    public static final String KEY_JOYNR_APPLICATION_PROPERTIES = "joynr.application.properties";

    @Provides
    @Named(KEY_JOYNR_APPLICATION_PROPERTIES)
    Properties joynrApplcationProperties() {
        return properties;
    }

    public JoynrApplicationModule(Class<? extends JoynrApplication> applicationClass) {
        this(applicationClass.getName(), applicationClass);
    }

    public JoynrApplicationModule(Class<? extends JoynrApplication> applicationClass,
                                  Properties applicationSpecificProperties) {
        this(applicationClass.getName(), applicationClass, applicationSpecificProperties);
    }

    public JoynrApplicationModule(String appId, Class<? extends JoynrApplication> applicationClass) {
        this(appId, applicationClass, new Properties());
    }

    public JoynrApplicationModule(String appId,
                                  Class<? extends JoynrApplication> applicationClass,
                                  Properties applicationSpecificProperties) {
        super(appId, applicationClass, combineProperties(applicationSpecificProperties));

    }

    private static Properties combineProperties(Properties applicationSpecificProperties) {
        // System properties ()set on java startup using -Djoynrapp.propertykey=propertyvalue
        if (applicationSpecificProperties == null) {
            applicationSpecificProperties = new Properties();
        }
        applicationSpecificProperties.putAll(PropertyLoader.getPropertiesWithPattern(System.getProperties(),
                                                                                     PATTERN_STARTS_WITH_JOYNAPP));
        return applicationSpecificProperties;
    }

}
