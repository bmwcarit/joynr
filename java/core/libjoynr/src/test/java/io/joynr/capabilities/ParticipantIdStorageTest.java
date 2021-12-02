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
package io.joynr.capabilities;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.util.Properties;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.Key;
import com.google.inject.name.Names;

import io.joynr.common.JoynrPropertiesModule;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingPropertyKeys;
import joynr.tests.test;
import joynr.types.GlobalDiscoveryEntry;

@RunWith(MockitoJUnitRunner.class)
public class ParticipantIdStorageTest {
    private static final String TOKEN1_PARTICIPANT = "token1Participant";
    private static final String TOKEN2_PARTICIPANT = "token2Participant";
    private static final String INTERFACE_NAME_FIXED_PARTICIPANT_ID = "interfaceName/testFixedParticipantId";
    private static final String domain = "testDomain";
    private static final String participantIdFixedParticipantId = "testFixedParticipantId";
    private final int majorVersion = 42;
    private Properties testProperties;

    private ParticipantIdStorage storage;

    @Mock
    private GlobalDiscoveryEntry capabilitiesDirectoryEntry;

    @Mock
    private GlobalDiscoveryEntry domainAccessControllerEntry;

    @Before
    public void setUp() throws IOException {
        testProperties = new Properties();
        testProperties.put(ParticipantIdKeyUtil.getProviderParticipantIdKey(domain,
                                                                            INTERFACE_NAME_FIXED_PARTICIPANT_ID,
                                                                            majorVersion),
                           participantIdFixedParticipantId);
        Injector injector = Guice.createInjector(new AbstractModule() {
            @Override
            protected void configure() {
                bind(GlobalDiscoveryEntry.class).annotatedWith(Names.named(MessagingPropertyKeys.CAPABILITIES_DIRECTORY_DISCOVERY_ENTRY))
                                                .toInstance(capabilitiesDirectoryEntry);
                bind(ParticipantIdStorage.class).to(PropertiesFileParticipantIdStorage.class);
            }
        }, new JoynrPropertiesModule(testProperties));
        String persistenceFile = injector.getInstance(Key.get(String.class,
                                                              Names.named(ConfigurableMessagingSettings.PROPERTY_PARTICIPANTIDS_PERSISTENCE_FILE)));
        Files.deleteIfExists(new File(persistenceFile).toPath());
        storage = injector.getInstance(ParticipantIdStorage.class);
    }

    @Test
    public void testGetProviderParticipantIdReturnsDefaultValue() {
        int majorVersion = 42;
        String participant1 = storage.getProviderParticipantId(domain,
                                                               test.INTERFACE_NAME,
                                                               majorVersion,
                                                               TOKEN1_PARTICIPANT);
        assertEquals(TOKEN1_PARTICIPANT, participant1);

        String participant2 = storage.getProviderParticipantId(domain,
                                                               test.INTERFACE_NAME,
                                                               majorVersion,
                                                               TOKEN2_PARTICIPANT);
        assertEquals(TOKEN2_PARTICIPANT, participant2);
    }

    @Test
    public void testGetProviderParticipantIdDoesNotReturnDefaultValueIfPersistedEntryExists() {
        int majorVersion = 42;
        String participant1 = storage.getProviderParticipantId(domain, test.INTERFACE_NAME, majorVersion, null);
        assertNotEquals(TOKEN1_PARTICIPANT, participant1);

        String participant2 = storage.getProviderParticipantId(domain,
                                                               test.INTERFACE_NAME,
                                                               majorVersion,
                                                               TOKEN1_PARTICIPANT);
        assertEquals(participant1, participant2);
    }

    @Test
    public void testGetProviderParticipantIdReturnsDefaultValueFromProperties() {
        String participant = storage.getProviderParticipantId(domain,
                                                              INTERFACE_NAME_FIXED_PARTICIPANT_ID,
                                                              majorVersion);
        assertEquals(participantIdFixedParticipantId, participant);
    }

    @Test
    public void testGetProviderParticipantIdDoesNotReturnDefaultValueFromPropertiesIfPersistedEntryExists() {
        final String participantId1 = storage.getProviderParticipantId(domain,
                                                                       INTERFACE_NAME_FIXED_PARTICIPANT_ID,
                                                                       majorVersion,
                                                                       null);
        assertNotEquals(participantIdFixedParticipantId, participantId1);

        String participant2 = storage.getProviderParticipantId(domain,
                                                               INTERFACE_NAME_FIXED_PARTICIPANT_ID,
                                                               majorVersion);
        assertEquals(participantId1, participant2);
    }
}
