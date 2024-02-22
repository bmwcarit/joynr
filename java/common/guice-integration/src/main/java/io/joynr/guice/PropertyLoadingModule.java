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
package io.joynr.guice;

import java.util.Map.Entry;
import java.util.Properties;
import java.util.Set;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.AbstractModule;
import com.google.inject.name.Names;

public class PropertyLoadingModule extends AbstractModule {

    Logger logger = LoggerFactory.getLogger(PropertyLoadingModule.class);

    protected final Properties properties = new LowerCaseProperties();

    /**
     * @param allProperties
     *            multiple properties which will be combined. last item has highest priority
     */
    public PropertyLoadingModule(Properties... allProperties) {

        mergeProperties(allProperties);

    }

    protected void mergeProperties(Properties... allProperties) {
        for (Properties specialProperties : allProperties) {
            if (specialProperties != null) {
                properties.putAll(specialProperties);
            }
        }
    }

    protected String getProperty(String key, String defaultValue, Properties... allProperties) {
        String value = defaultValue;
        for (Properties aProperties : allProperties) {
            value = aProperties.getProperty(key, value);
        }
        return value;
    }

    protected void logProperties() {
        Properties filteredProperties = getFilteredProperties("joynr.");
        filteredProperties.putAll(getFilteredProperties("javax.net.ssl"));
        maskPasswordProperties(filteredProperties);
        if (logger.isInfoEnabled()) {
            String formattedPropertiesOutput = filteredProperties.toString().replaceAll(",", "\n");
            logger.info("Properties loaded:\n{}", formattedPropertiesOutput);
        }
    }

    private void maskPasswordProperties(final Properties properties) {
        properties.keySet()
                  .stream()
                  .map(Object::toString)
                  .filter(this::isPasswordKey)
                  .forEach(key -> properties.put(key, "********"));
    }

    private boolean isPasswordKey(final String key) {
        return key != null && key.contains("password");
    }

    protected Properties getFilteredProperties(String prefix) {
        Set<Entry<Object, Object>> entrySet = properties.entrySet();
        Properties filteredProperties = new Properties();
        for (Entry<Object, Object> entry : entrySet) {
            if (entry.getKey().toString().startsWith(prefix)) {
                filteredProperties.put(entry.getKey(), entry.getValue());
            }
        }

        return filteredProperties;
    }

    @Override
    protected void configure() {
        Names.bindProperties(binder(), properties);
        logProperties();
    }

}
