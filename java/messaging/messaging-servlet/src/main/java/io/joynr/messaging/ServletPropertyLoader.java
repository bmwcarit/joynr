package io.joynr.messaging;

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

import java.io.IOException;
import java.io.InputStream;
import java.util.Properties;

import javax.servlet.ServletContext;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class ServletPropertyLoader {

    private static final Logger logger = LoggerFactory.getLogger(ServletPropertyLoader.class);

    public static Properties loadProperties(String filename, ServletContext servletContext) {
        Properties properties = new Properties();
        InputStream in = null;
        try {
            ClassLoader classLoader = Thread.currentThread().getContextClassLoader();
            in = classLoader.getResourceAsStream(filename);
            if (in == null) {
                // commented, because this was a dead store
                // in = servletContext.getResourceAsStream(filename);
                logger.error("properties file not found");
            } else {
                properties.load(in);
            }
        } catch (IOException e) {
            logger.error("properties file not loaded", e);
        }
        return properties;
    }

}
