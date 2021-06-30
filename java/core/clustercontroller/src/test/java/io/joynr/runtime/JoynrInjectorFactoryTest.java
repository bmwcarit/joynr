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
package io.joynr.runtime;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import java.util.Properties;

import org.hamcrest.core.StringContains;
import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.ExpectedException;

import com.google.inject.CreationException;

import io.joynr.common.JoynrPropertiesModule;
import io.joynr.guice.IApplication;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.test.TestApplicationModule;
import io.joynr.test.TestJoynrApplication;

public class JoynrInjectorFactoryTest {

    private String creationTestConfigEntry;
    private JoynrInjectorFactory injectorfactory;
    private Properties applicationCreationProperties;
    private String systemTestConfigEntry;
    private Properties originalSystemproperties;
    private static final long factoryDiscoveryDefaultTimeoutValue = 42;

    @Before
    public void setUp() {
        originalSystemproperties = (Properties) System.getProperties().clone();

        Properties basicProperties = new Properties();
        basicProperties.setProperty(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_DEFAULT_TIMEOUT_MS,
                                    Long.toString(factoryDiscoveryDefaultTimeoutValue));
        injectorfactory = new JoynrInjectorFactory(basicProperties, new TestRuntimeModule());

        creationTestConfigEntry = "creation-test-value";
        applicationCreationProperties = new Properties();
        applicationCreationProperties.setProperty(TestJoynrApplication.PROPERTY_TEST_CONFIG_ENTRY,
                                                  creationTestConfigEntry);
        systemTestConfigEntry = "system-test-value";
    }

    @After
    public void tearDown() {
        System.setProperties(originalSystemproperties);
    }

    @Rule
    public ExpectedException creationException = ExpectedException.none();

    @Test
    /**
     * Would (but should not) fail with a guice exception if the property is added at the factory level and then again at the application level
     */
    public void applicationSystemPropertiesAreNotSetInJoynFactoryProperties() {
        System.getProperties().setProperty(TestJoynrApplication.PROPERTY_TEST_CONFIG_ENTRY, systemTestConfigEntry);
        injectorfactory.createApplication(new TestApplicationModule(applicationCreationProperties));
    }

    @Test
    public void applicationCreationPropertiesCannotOverrideJoynFactoryProperties() {
        creationException.expect(CreationException.class);
        creationException.expectMessage(StringContains.containsString("A binding to java.lang.String annotated with @com.google.inject.name.Named(value=joynr.discovery.defaulttimeoutms) was already configured"));

        applicationCreationProperties.setProperty(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_DEFAULT_TIMEOUT_MS,
                                                  "0");

        injectorfactory.createApplication(new TestApplicationModule(applicationCreationProperties));
    }

    @Test
    public void applicationLevelCreationPropertiesOverrideApplicationDefaultProperties() {
        IApplication application = injectorfactory.createApplication(new TestApplicationModule(applicationCreationProperties));
        assertTrue(application instanceof TestJoynrApplication);

        assertEquals(creationTestConfigEntry, ((TestJoynrApplication) application).testConfigEntry);
    }

    @Test
    public void applicationSystemPropertiesOverrideApplicationDefaultProperties() {
        System.getProperties().setProperty(TestJoynrApplication.PROPERTY_TEST_CONFIG_ENTRY, systemTestConfigEntry);

        IApplication application = injectorfactory.createApplication(new TestApplicationModule(applicationCreationProperties));
        assertTrue(application instanceof TestJoynrApplication);

        assertEquals(systemTestConfigEntry, ((TestJoynrApplication) application).testConfigEntry);
    }

    @Test
    public void applicationSystemPropertiesOverrideApplicationCreationProperties() {
        System.getProperties().setProperty(TestJoynrApplication.PROPERTY_TEST_CONFIG_ENTRY, systemTestConfigEntry);

        IApplication application = injectorfactory.createApplication(new TestApplicationModule(applicationCreationProperties));
        assertTrue(application instanceof TestJoynrApplication);

        assertEquals(systemTestConfigEntry, ((TestJoynrApplication) application).testConfigEntry);
    }

    @Test
    public void joynrFactoryPropertiesOverrideJoynDefaultProperties() {

        Properties defaultProperties = PropertyLoader.loadProperties(MessagingPropertyKeys.DEFAULT_MESSAGING_PROPERTIES_FILE);
        String defaultValue = defaultProperties.getProperty(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_DEFAULT_TIMEOUT_MS);

        assertFalse(defaultValue.equals(Long.toString(factoryDiscoveryDefaultTimeoutValue)));

        IApplication application = injectorfactory.createApplication(new TestApplicationModule(applicationCreationProperties));
        assertTrue(application instanceof TestJoynrApplication);

        assertEquals(factoryDiscoveryDefaultTimeoutValue,
                     ((TestJoynrApplication) application).defaultDiscoveryTimeoutMs);
    }

    @Test
    public void systemPropertiesOverrideJoynFactoryProperties() {
        long systemDiscoveryDefaultTimeoutValue = 24;
        assertFalse(systemDiscoveryDefaultTimeoutValue == factoryDiscoveryDefaultTimeoutValue);
        System.getProperties().setProperty(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_DEFAULT_TIMEOUT_MS,
                                           Long.toString(systemDiscoveryDefaultTimeoutValue));

        Properties propertiesBeforeModification = new Properties(originalSystemproperties);
        setUp(); //Re-init JoynrFactory with new system
        originalSystemproperties = propertiesBeforeModification;

        IApplication application = injectorfactory.createApplication(new TestApplicationModule(applicationCreationProperties));
        assertTrue(application instanceof TestJoynrApplication);

        assertEquals(systemDiscoveryDefaultTimeoutValue,
                     ((TestJoynrApplication) application).defaultDiscoveryTimeoutMs);
    }

    @Test
    public void getSystemPropertiesWithPattern() {
        String key = "joynrapp.myproperty.123";
        Properties properties = new Properties();
        properties.put(key, "1");
        String dontFindkey = "joynr.messaging.gdac.url";
        properties.put(dontFindkey, "whatever");

        Properties propertiesWithPattern = PropertyLoader.getPropertiesWithPattern(properties,
                                                                                   JoynrApplicationModule.PATTERN_STARTS_WITH_JOYNAPP);

        Properties propertiesWithoutPattern = PropertyLoader.getPropertiesWithPattern(properties,
                                                                                      JoynrPropertiesModule.PATTERN_DOESNT_START_WITH_JOYNAPP);

        assertTrue("system property starting with joynrapp found", propertiesWithPattern.containsKey(key));
        assertTrue(propertiesWithPattern.keySet().size() == 1);

        assertTrue("system property NOT starting with joynrapp found",
                   propertiesWithoutPattern.containsKey(dontFindkey));
        assertTrue(propertiesWithoutPattern.keySet().size() == 1);
    }
}
