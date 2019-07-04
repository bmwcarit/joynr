/*
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
package io.joynr.discovery.jee;

import static io.joynr.capabilities.directory.CapabilitiesDirectoryImpl.GCD_GBID;
import static io.joynr.capabilities.directory.CapabilitiesDirectoryImpl.VALID_GBIDS;

import java.util.Properties;

import javax.ejb.Singleton;
import javax.enterprise.inject.Produces;
import javax.inject.Named;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.jeeintegration.api.JoynrLocalDomain;
import io.joynr.jeeintegration.api.JoynrProperties;
import io.joynr.messaging.MessagingPropertyKeys;

/**
 * This is the singleton bean which will provide the configuration values at runtime for the
 * service, also allowing you to specify your own values via OS environment variables and
 * Java system properties to override the defaults provided here.
 * <p>
 *     In order to override values via environment variables, take the joynr property name
 *     (see the
 *     <a href="https://github.com/bmwcarit/joynr/blob/develop/wiki/JavaSettings.md">Java Configuration Reference</a>
 *     for details) and replace all period
 *     characters ('.') with underscores ('_'). Hence, <code>joynr.servlet.hostpath</code>
 *     becomes <code>joynr_servlet_hostpath</code>. Case is ignored, so you can feel free
 *     to use upper-case to make any of the longer names more readable in your setup.
 * </p>
 */
@Singleton
public class TestJoynrConfigurationProvider {

    private static final Logger logger = LoggerFactory.getLogger(JoynrConfigurationProvider.class);

    static final String VALID_GBIDS_ARRAY = "joynrdefaultgbid,testGbid2,testGbid3";
    static final String JOYNR_DEFAULT_GCD_GBID = "joynrdefaultgbid";

    @Produces
    @JoynrProperties
    public Properties getJoynrProperties() {
        Properties joynrProperties = new Properties();
        readAndSetProperty(joynrProperties,
                           MessagingPropertyKeys.PROPERTY_SERVLET_CONTEXT_ROOT,
                           "/discovery-directory-jee/messaging");
        readAndSetProperty(joynrProperties, MessagingPropertyKeys.CHANNELID, "discoverydirectory_channelid");
        readAndSetProperty(joynrProperties,
                           MessagingPropertyKeys.PERSISTENCE_FILE,
                           "test-discovery-directory-joynr.properties");
        return joynrProperties;
    }

    private void readAndSetProperty(Properties joynrProperties, String propertyKey, String defaultValue) {
        String value = System.getProperty(propertyKey, defaultValue);
        logger.debug("Setting property {} to value {}.", propertyKey, value);
        joynrProperties.setProperty(propertyKey, value);
    }

    @Produces
    @JoynrLocalDomain
    public String getJoynrLocalDomain() {
        return "io.joynrtest";
    }

    @Produces
    @Named(GCD_GBID)
    public String getGcdGbid() {
        return JOYNR_DEFAULT_GCD_GBID;
    }

    @Produces
    @Named(VALID_GBIDS)
    public String getValidGbids() {
        return VALID_GBIDS_ARRAY;
    }
}
