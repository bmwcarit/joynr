/*
 * #%L
 * %%
 * Copyright (C) 2022 BMW Car IT GmbH
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
package io.joynr.messaging.mqtt;

import static org.junit.Assert.assertNotNull;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.util.Properties;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.Key;
import com.google.inject.name.Names;

import io.joynr.capabilities.ParticipantIdStorage;
import io.joynr.common.JoynrPropertiesModule;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingPropertyKeys;
import joynr.types.GlobalDiscoveryEntry;

@RunWith(MockitoJUnitRunner.class)
public class MqttPropertiesFileParticipantIdStorageTest {
    private final int majorVersion = 42;

    private ParticipantIdStorage storage;

    @Mock
    private GlobalDiscoveryEntry capabilitiesDirectoryEntry;

    @Mock
    private GlobalDiscoveryEntry domainAccessControllerEntry;

    @Test(expected = JoynrRuntimeException.class)
    public void testRegisterProviderWithSharedSubscriptionsThrowsWhenNoFixedParticipantId() throws IOException {
        Injector injector = Guice.createInjector(new AbstractModule() {
            @Override
            protected void configure() {
                bind(GlobalDiscoveryEntry.class).annotatedWith(Names.named(MessagingPropertyKeys.CAPABILITIES_DIRECTORY_DISCOVERY_ENTRY))
                                                .toInstance(capabilitiesDirectoryEntry);
                bind(ParticipantIdStorage.class).to(MqttPropertiesFileParticipantIdStorage.class);
                bind(Boolean.class).annotatedWith(Names.named(MqttModule.PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS))
                                   .toInstance(true);
            }
        }, new JoynrPropertiesModule(new Properties()));
        String persistenceFile = injector.getInstance(Key.get(String.class,
                                                              Names.named(ConfigurableMessagingSettings.PROPERTY_PARTICIPANTIDS_PERSISTENCE_FILE)));
        Files.deleteIfExists(new File(persistenceFile).toPath());
        storage = injector.getInstance(ParticipantIdStorage.class);
        storage.getProviderParticipantId("newDomain", "newInterfaceName", majorVersion, null);
    }

    @Test
    public void testRegisterProviderWithoutSharedSubscriptionsReturnsRandomParticipantId() throws IOException {
        Injector injector = Guice.createInjector(new AbstractModule() {
            @Override
            protected void configure() {
                bind(GlobalDiscoveryEntry.class).annotatedWith(Names.named(MessagingPropertyKeys.CAPABILITIES_DIRECTORY_DISCOVERY_ENTRY))
                                                .toInstance(capabilitiesDirectoryEntry);
                bind(ParticipantIdStorage.class).to(MqttPropertiesFileParticipantIdStorage.class);
                bind(Boolean.class).annotatedWith(Names.named(MqttModule.PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS))
                                   .toInstance(false);
            }
        }, new JoynrPropertiesModule(new Properties()));
        String persistenceFile = injector.getInstance(Key.get(String.class,
                                                              Names.named(ConfigurableMessagingSettings.PROPERTY_PARTICIPANTIDS_PERSISTENCE_FILE)));
        Files.deleteIfExists(new File(persistenceFile).toPath());
        storage = injector.getInstance(ParticipantIdStorage.class);
        assertNotNull(storage.getProviderParticipantId("newDomain", "newInterfaceName", majorVersion, null));
    }
}
