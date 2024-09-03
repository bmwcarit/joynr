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
package io.joynr.messaging;

import static io.joynr.util.JoynrUtil.createUuidString;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.Properties;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.runtime.PropertyLoader;

public class MessagingPropertiesPersistence {
    private static final Logger logger = LoggerFactory.getLogger(MessagingPropertiesPersistence.class);
    private File persistenceFile;
    private Properties storage;

    public MessagingPropertiesPersistence(File persistenceFile) {
        this.persistenceFile = persistenceFile;
        makeDirectories(persistenceFile);
        storage = PropertyLoader.loadProperties(persistenceFile);

        generateIfAbsent(MessagingPropertyKeys.CHANNELID);
        generateIfAbsent(MessagingPropertyKeys.RECEIVERID);

        persistProperties();
    }

    public Properties getPersistedProperties() {
        if (storage == null) {
            return new Properties();
        }
        Properties propertiesCopy = new Properties();
        propertiesCopy.putAll(storage);
        return propertiesCopy;
    }

    private void persistProperties() {
        FileOutputStream fileOutputStream = null;
        try {
            File parentDirectory = persistenceFile.getParentFile();
            if (parentDirectory != null && !parentDirectory.exists() && !parentDirectory.mkdirs()) {
                throw new IllegalStateException("Could not create parent directory: " + parentDirectory);
            }

            if (!persistenceFile.exists()) {
                boolean created = persistenceFile.createNewFile();
                if (!created) {
                    throw new IllegalStateException("Could not create persistenceFile: " + persistenceFile);
                }
            }

            fileOutputStream = new FileOutputStream(persistenceFile);
            storage.store(fileOutputStream, null);
        } catch (IOException e1) {
            logger.error("Couldn't write properties file {}", persistenceFile.getPath(), e1);
        } finally {
            if (fileOutputStream != null) {
                try {
                    fileOutputStream.close();
                } catch (IOException e) {
                }
            }
        }
    }

    private void makeDirectories(File persistenceFile) {
        File parentFile = persistenceFile.getParentFile();
        if (parentFile != null && parentFile.exists() == false) {
            boolean madeDirs = parentFile.mkdirs();
            if (!madeDirs) {
                throw new IllegalStateException("Couldn't create dir: " + parentFile);
            }
        }
    }

    private void generateIfAbsent(String propertyName) {
        if (!storage.containsKey(propertyName)) {
            String propertyValue = createUuidString();
            storage.put(propertyName, propertyValue);
        }
    }
}
