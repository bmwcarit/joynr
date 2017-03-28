package io.joynr.runtime;

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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import io.joynr.common.JoynrPropertiesModule;
import io.joynr.guice.IApplication;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.test.TestApplicationModule;
import io.joynr.test.TestJoynrApplication;

import java.util.Properties;

import org.hamcrest.core.StringContains;
import org.junit.After;
import org.junit.Before;
import org.junit.Ignore;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.ExpectedException;

import com.google.inject.CreationException;

public class JoynrInjectorFactoryTest {

    private String creationTestConfigEntry;
    private JoynrInjectorFactory injectorfactory;
    private Properties applicationCreationProperties;
    private String systemTestConfigEntry;
    private Properties originalSystemproperties;
    private static final String factoryBounceProxyUrl = "http://factory-test-value";
    private static final String systemBounceProxyUrl = "http://system-test-value";

    @Before
    public void setUp() {
        originalSystemproperties = (Properties) System.getProperties().clone();

        Properties basicProperties = new Properties();
        basicProperties.setProperty(AbstractJoynrApplication.PROPERTY_JOYNR_DOMAIN_LOCAL, "localdomain");
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
        creationException.expectMessage(StringContains.containsString("A binding to java.lang.String annotated with @com.google.inject.name.Named(value=joynr.messaging.bounceproxyurl) was already configured"));

        applicationCreationProperties.setProperty(MessagingPropertyKeys.BOUNCE_PROXY_URL,
                                                  "http://test-bounce-proxy-url");

        injectorfactory.createApplication(new TestApplicationModule(applicationCreationProperties));
    }

    @Test
    public void applicationLevelDefaultPropertiesCannotOverrideJoynDefaultProperties() {
        creationException.expect(CreationException.class);
        creationException.expectMessage(StringContains.containsString("A binding to java.lang.String annotated with @com.google.inject.name.Named(value=joynr.messaging.bounceproxyurl) was already configured"));

        injectorfactory.createApplication(new TestApplicationModule(applicationCreationProperties, true));
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

    @Ignore("Will not run in environments with MessagingPropertyKeys.BOUNCE_PROXY_URL set as environment variable (e.g. on build server).")
    @Test
    public void joynrFactoryPropertiesOverrideJoynDefaultProperties() {

        Properties defaultProperties = PropertyLoader.loadProperties(MessagingPropertyKeys.DEFAULT_MESSAGING_PROPERTIES_FILE);
        String defaultBounceProxyUrl = defaultProperties.getProperty(MessagingPropertyKeys.BOUNCE_PROXY_URL);

        assertFalse(factoryBounceProxyUrl.equals(defaultBounceProxyUrl));

        IApplication application = injectorfactory.createApplication(new TestApplicationModule(applicationCreationProperties));
        assertTrue(application instanceof TestJoynrApplication);

        assertEquals(factoryBounceProxyUrl, ((TestJoynrApplication) application).bounceProxyUrl);
    }

    @Ignore("Will not run in environments with MessagingPropertyKeys.BOUNCE_PROXY_URL set as environment variable (e.g. on build server).")
    @Test
    public void joynrSystemPropertiesOverrideJoynFactoryProperties() {
        System.getProperties().setProperty(MessagingPropertyKeys.BOUNCE_PROXY_URL, systemBounceProxyUrl);

        IApplication application = injectorfactory.createApplication(new TestApplicationModule(applicationCreationProperties));
        assertTrue(application instanceof TestJoynrApplication);

        assertEquals(systemBounceProxyUrl, ((TestJoynrApplication) application).bounceProxyUrl);
    }

    @Test
    public void getSystemPropertiesWithPattern() {
        String key = "joynrapp.myproperty.123";
        Properties properties = new Properties();
        properties.put(key, "1");
        String dontFindkey = "joynr.messaging.bounceProxyUrl";
        properties.put(dontFindkey, "http://system-test-value");

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
