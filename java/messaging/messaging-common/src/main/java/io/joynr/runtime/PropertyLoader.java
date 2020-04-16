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

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.util.Enumeration;
import java.util.Optional;
import java.util.Properties;
import java.util.regex.Pattern;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.guice.LowerCaseProperties;

/**
 * Loads properties using this class's classloader
 *
 * @author david.katz
 *
 */

public class PropertyLoader {
    private static Logger logger = LoggerFactory.getLogger(PropertyLoader.class);

    public static Optional<InputStream> loadResource(String resourceName) {
        ClassLoader classLoader = Thread.currentThread().getContextClassLoader();
        URL url = classLoader.getResource(resourceName);

        // try opening from classpath
        if (url != null) {
            InputStream urlStream = null;
            try {
                urlStream = url.openStream();
                return Optional.of(urlStream);
            } catch (IOException e) {
            } finally {
            }
        }

        // try opening from file system
        FileInputStream fileInputStream = null;
        try {
            fileInputStream = new FileInputStream(resourceName);
            return Optional.ofNullable(fileInputStream);
        } catch (IOException e) {
        } finally {
        }
        return Optional.empty();
    }

    /**
     * load properties from file
     * @param fileName The filename of the file in which the properties are stored
     * @return The loaded properties.
     */
    public static Properties loadProperties(String fileName) {
        LowerCaseProperties properties = new LowerCaseProperties();
        ClassLoader classLoader = Thread.currentThread().getContextClassLoader();
        URL url = classLoader.getResource(fileName);

        // try opening from classpath
        if (url != null) {
            InputStream urlStream = null;
            try {
                urlStream = url.openStream();
                properties.load(urlStream);

                logger.info("Properties from classpath loaded file {}", fileName);
            } catch (IOException e) {
                logger.info("Properties from classpath file {} could not be loaded. Error:", fileName, e);
            } finally {
                if (urlStream != null) {
                    try {
                        urlStream.close();
                    } catch (IOException e) {
                    }
                }
            }
        }

        // Override properties found on the classpath with any found in a file
        // of the same name
        LowerCaseProperties propertiesFromFileSystem = loadProperties(new File(fileName));
        properties.putAll(propertiesFromFileSystem);
        return properties;
    }

    public static LowerCaseProperties loadProperties(File file) {
        LowerCaseProperties properties = new LowerCaseProperties();

        // try opening from file system
        FileInputStream fileInputStream = null;
        try {
            fileInputStream = new FileInputStream(file);
            properties.load(fileInputStream);
            logger.info("Properties from runtime directory loaded file: {}", file.getAbsolutePath());
        } catch (IOException e) {
            logger.info("Properties from runtime directory not loaded because: {}", e.getMessage());
        } finally {
            if (fileInputStream != null)
                try {
                    fileInputStream.close();
                } catch (IOException e) {
                }
        }

        return properties;
    }

    public static Properties loadProperties(Properties properties, String fileName) {
        properties.putAll(loadProperties(fileName));
        return properties;
    }

    // public static boolean propertiesFileExists(String fileName) {
    // ClassLoader classLoader = Thread.currentThread().getContextClassLoader();
    // URL url = classLoader.getResource(fileName);
    // return url != null;
    //
    // }

    public static Properties getPropertiesWithPattern(Properties originalProperties, String regex) {
        Properties properties = new Properties();
        Enumeration<Object> keys = originalProperties.keys();
        while (keys.hasMoreElements()) {
            String nextKey = (String) keys.nextElement();
            if (Pattern.compile(regex).matcher(nextKey).find()) {
                properties.put(nextKey.replace("_", "."), originalProperties.getProperty(nextKey));
            }
        }

        return properties;
    }

}
