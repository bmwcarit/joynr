/*
 * #%L
 * joynr::java::messaging::bounceproxy::controlled-bounceproxy
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
package io.joynr.messaging.bounceproxy.runtime;

import java.util.Optional;
import java.util.Properties;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.bounceproxy.BounceProxyPropertyKeys;
import io.joynr.runtime.PropertyLoader;

/**
 * Property loader for properties that are set as system properties or in a file
 * called {@value #CONTROLLED_BOUNCE_PROXY_SYSTEM_PROPERTIES} located on the
 * classpath. System properties have precedence over properties defined in the
 * file<br>
 *
 * The class supports the specification of variables in the properties file,
 * e.g.
 * <p>
 * <code>
 * joynr.bounceproxy.id = bounceproxy.${hostname}.instance0
 * </code>
 * </p>
 * This allows that dynamic properties can be set on a server for which either
 * system properties can't be set directly or the property can't be set at the
 * time of building the application.<br>
 * All variables used in {@value #CONTROLLED_BOUNCE_PROXY_SYSTEM_PROPERTIES}
 * have to be available by calling {@link System#getProperty(String)} with the
 * variable as property key.
 *
 * @author christina.strobel
 *
 */
public class BounceProxySystemPropertyLoader {

    private static final Logger logger = LoggerFactory.getLogger(BounceProxySystemPropertyLoader.class);

    private static final String CONTROLLED_BOUNCE_PROXY_SYSTEM_PROPERTIES = "controlledBounceProxySystem.properties";
    private static Properties systemPropertiesFromFile = null;

    /**
     * Loads all properties for the bounce proxy that have to be set as system
     * properties (see
     * {@link BounceProxyPropertyKeys#bounceProxySystemPropertyKeys}) or in the
     * file {@value #CONTROLLED_BOUNCE_PROXY_SYSTEM_PROPERTIES}. System
     * properties have precedence over the file.
     *
     * @return
     *   all properties for the bounce proxy that have to be set as
     *   system properties
     * @throws JoynrRuntimeException
     *   if not all of the properties were set so that bounce proxy
     *   won't be able to start up correctly
     */
    public static Properties loadProperties() {

        Properties properties = new Properties();

        for (String key : BounceProxyPropertyKeys.getPropertyKeysForSystemProperties()) {

            String value = System.getProperty(key);

            if (value == null) {

                Optional<String> optionalValue = loadPropertyFromFile(key);

                if (!optionalValue.isPresent()) {
                    throw new JoynrRuntimeException("No value for system property '" + key
                            + "' set. Unable to start Bounce Proxy");
                }
                value = optionalValue.get();
            }

            properties.put(key, value);
        }

        return properties;
    }

    private static Optional<String> loadPropertyFromFile(String key) {

        if (systemPropertiesFromFile == null) {
            logger.info("Loading system properties file " + CONTROLLED_BOUNCE_PROXY_SYSTEM_PROPERTIES);
            systemPropertiesFromFile = PropertyLoader.loadProperties(CONTROLLED_BOUNCE_PROXY_SYSTEM_PROPERTIES);
        }

        if (systemPropertiesFromFile != null) {
            String value = systemPropertiesFromFile.getProperty(key);

            logger.info("Trying to load property '" + key + "' from system properties file");

            if (value != null) {
                return replaceVariableBySystemProperty(value);
            }
        }

        return Optional.empty();
    }

    /**
     * Replaces a variable defined by <code>${variable}</code> by a system
     * property with the key <code>variable</code>.
     *
     * @param value
     *            a string with any number of variables defined
     * @return an Optional containing string in which each occurrence of <code>${variable}</code> is
     *         replaced by {@link System#getProperty(String)} with
     *         <code>variable</code> as property key.
     * @throws JoynrRuntimeException
     *             if the property key for the variable does not exist
     */
    static Optional<String> replaceVariableBySystemProperty(String value) {

        if (value == null)
            return Optional.empty();

        int startVariableIndex = value.indexOf("${");

        while (startVariableIndex >= 0) {
            int endVariableIndex = value.indexOf("}", startVariableIndex);

            String propertyName = value.substring(startVariableIndex + 2, endVariableIndex);

            String systemProperty = System.getProperty(propertyName);

            if (systemProperty == null) {
                throw new JoynrRuntimeException("No value for system property '" + propertyName
                        + "' set as defined in property file. Unable to start Bounce Proxy");
            } else {
                value = value.substring(0, startVariableIndex) + systemProperty + value.substring(endVariableIndex + 1);
            }
            startVariableIndex = value.indexOf("${");
        }

        return Optional.of(value);
    }
}
