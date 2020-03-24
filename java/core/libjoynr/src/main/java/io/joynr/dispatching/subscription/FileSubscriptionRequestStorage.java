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
package io.joynr.dispatching.subscription;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.util.MultiMap;
import joynr.SubscriptionRequest;

@Singleton
public class FileSubscriptionRequestStorage implements SubscriptionRequestStorage {

    private static final Logger logger = LoggerFactory.getLogger(FileSubscriptionRequestStorage.class);
    MultiMap<String, PersistedSubscriptionRequest> persistedSubscriptionRequests = new MultiMap<>();
    private String persistenceFileName;

    @Inject
    public FileSubscriptionRequestStorage(@Named(ConfigurableMessagingSettings.PROPERTY_SUBSCRIPTIONREQUESTS_PERSISTENCE_FILE) String persistenceFileName) {
        this.persistenceFileName = persistenceFileName;
        deserializeFromFile();
    }

    @Override
    synchronized public MultiMap<String, PersistedSubscriptionRequest> getSavedSubscriptionRequests() {
        return persistedSubscriptionRequests;
    }

    @Override
    public synchronized void persistSubscriptionRequest(String proxyId,
                                                        String providerId,
                                                        SubscriptionRequest subscriptionRequest) {
        persistedSubscriptionRequests.put(providerId,
                                          new PersistedSubscriptionRequest(proxyId, providerId, subscriptionRequest));
        persistSubscriptionRequestsToFile();
    }

    @Override
    public synchronized void removeSubscriptionRequest(String providerId,
                                                       PersistedSubscriptionRequest subscriptionRequest) {
        persistedSubscriptionRequests.remove(providerId, subscriptionRequest);
        persistSubscriptionRequestsToFile();
    }

    @SuppressWarnings("unchecked")
    synchronized private void deserializeFromFile() {
        ObjectInputStream inputStream = null;
        try {
            inputStream = new ObjectInputStream(new FileInputStream(persistenceFileName));
            persistedSubscriptionRequests = (MultiMap<String, PersistedSubscriptionRequest>) inputStream.readObject();
        } catch (Exception e) {
            logger.warn("unable to read saved subscription requests: " + e.getMessage());
            deleteCorruptedPersistenceFile();
            persistedSubscriptionRequests = new MultiMap<>();
        } finally {
            if (inputStream != null) {
                try {
                    inputStream.close();

                } catch (IOException e) {
                    logger.error("unable to close subscription requests persistence file", e);
                }
            }
        }
    }

    private void deleteCorruptedPersistenceFile() {
        try {
            new File(persistenceFileName).delete();
        } catch (Exception e) {
            logger.error("unable to delete unparsable persistence file: " + persistenceFileName, e);
        }
    }

    private void persistSubscriptionRequestsToFile() {
        ObjectOutputStream outputStream = null;
        try {
            outputStream = new ObjectOutputStream(new FileOutputStream(persistenceFileName, false));
            outputStream.writeObject(persistedSubscriptionRequests);
        } catch (Exception e) {
            logger.error("unable to write to saved subscripton requests", e);
        } finally {
            if (outputStream != null) {
                try {
                    outputStream.close();
                } catch (Exception e) {
                    logger.error("unable to close saved subscripton requests", e);
                }
            }
        }
    }
}
