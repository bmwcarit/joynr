package io.joynr.messaging.bounceproxy.runtime;

/*
 * #%L
 * joynr::java::messaging::bounceproxy::controlled-bounceproxy
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

import io.joynr.exceptions.JoynrException;
import io.joynr.messaging.bounceproxy.BounceProxyPropertyKeys;

import java.util.Properties;

/**
 * Property loader for properties that are set as system properties.
 * 
 * @author christina.strobel
 * 
 */
public class BounceProxySystemPropertyLoader {

    /**
     * Loads all properties for the bounce proxy that have to be set as system
     * properties (see
     * {@link BounceProxyPropertyKeys#bounceProxySystemPropertyKeys}).
     * 
     * @return
     * @throws JoynrException
     *             if not all of the properties were set so that bounce proxy
     *             won't be able to start up correctly
     */
    public static Properties loadProperties() {

        Properties properties = new Properties();

        for (String key : BounceProxyPropertyKeys.getPropertyKeysForSystemProperties()) {

            String value = System.getProperty(key);

            if (value == null) {
                // property not set
                throw new JoynrException("No value for system property '" + key + "' set. Unable to start Bounce Proxy");
            }

            properties.put(key, value);
        }

        return properties;
    }

}
