package io.joynr.integration.setup;

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

import java.util.Properties;

import org.eclipse.jetty.webapp.AbstractConfiguration;
import org.eclipse.jetty.webapp.WebAppContext;

/**
 * Configuration for a Jetty server that allows a configuration by system
 * properties per web application even though multiple web application run in
 * the same JVM.
 * 
 * @author christina.strobel
 * 
 */
public class SystemPropertyServletConfiguration extends AbstractConfiguration {

    private Properties properties;

    public SystemPropertyServletConfiguration(Properties properties) {
        this.properties = properties;
    }

    @Override
    public void preConfigure(WebAppContext context) throws Exception {
        // we have to set properties one by one as otherwise current properties
        // are overridden
        for (String key : this.properties.stringPropertyNames()) {
            System.setProperty(key, this.properties.get(key).toString());
        }
    }

    @Override
    public void postConfigure(WebAppContext context) throws Exception {
        for (String key : this.properties.stringPropertyNames()) {
            System.clearProperty(key);
        }
    }

}