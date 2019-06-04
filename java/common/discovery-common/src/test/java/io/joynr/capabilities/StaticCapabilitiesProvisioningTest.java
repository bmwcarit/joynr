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

import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_CHANNEL_ID;
import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_PARTICIPANT_ID;
import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_DISCOVERY_DIRECTORIES_DOMAIN;
import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_DOMAIN_ACCESS_CONTROLLER_CHANNEL_ID;
import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_DOMAIN_ACCESS_CONTROLLER_PARTICIPANT_ID;
import static io.joynr.messaging.MessagingPropertyKeys.CHANNELID;
import static io.joynr.messaging.MessagingPropertyKeys.DISCOVERYDIRECTORYURL;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.lang.reflect.Field;
import java.util.Collection;
import java.util.HashSet;
import java.util.Set;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.databind.ObjectMapper.DefaultTyping;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.AbstractModule;
import com.google.inject.CreationException;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.name.Names;

import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.routing.RoutingTable;
import joynr.infrastructure.GlobalCapabilitiesDirectory;
import joynr.infrastructure.GlobalDomainAccessController;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.ChannelAddress;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.types.DiscoveryEntry;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.Version;

@RunWith(MockitoJUnitRunner.class)
public class StaticCapabilitiesProvisioningTest {

    private static final Logger logger = LoggerFactory.getLogger(StaticCapabilitiesProvisioningTest.class);

    private ObjectMapper objectMapper;

    @Mock
    private RoutingTable routingTable;

    @Before
    public void setUp() throws Exception {
        objectMapper = new ObjectMapper();
        objectMapper.enableDefaultTypingAsProperty(DefaultTyping.JAVA_LANG_OBJECT, "_typeName");
        Field objectMapperField = CapabilityUtils.class.getDeclaredField("objectMapper");
        objectMapperField.setAccessible(true);
        objectMapperField.set(CapabilityUtils.class, objectMapper);
    }

    private Set<DiscoveryEntry> createDiscoveryEntries(String domain, String... interfaceNames) {
        Set<DiscoveryEntry> discoveryEntries = new HashSet<DiscoveryEntry>();
        String participantId = "particpantId";
        ProviderQos qos = new ProviderQos();
        Long lastSeenDateMs = 0L;
        Long expiryDateMs = 0L;
        String publicKeyId = "publicKeyId";
        Address address = new MqttAddress("brokerUri", "topic");
        for (String interfaceName : interfaceNames) {
            GlobalDiscoveryEntry entry = CapabilityUtils.newGlobalDiscoveryEntry(new Version(0, 1),
                                                                                 domain,
                                                                                 interfaceName,
                                                                                 participantId,
                                                                                 qos,
                                                                                 lastSeenDateMs,
                                                                                 expiryDateMs,
                                                                                 publicKeyId,
                                                                                 address);
            discoveryEntries.add(entry);
        }
        return discoveryEntries;
    }

    @Test
    public void testLoadingExtraSerializedDiscoveryEntriesPlusLegacy() throws Exception {
        Set<DiscoveryEntry> discoveryEntries = createDiscoveryEntries("domain", "interfaceName1", "interfaceName2");

        LegacyCapabilitiesProvisioning.LegacyProvisioningPropertiesHolder properties = createLegacyProvisioningPropertiesHolder();

        final String serializedDiscoveryEntries = objectMapper.writeValueAsString(discoveryEntries);
        logger.debug("Serialised entries: " + serializedDiscoveryEntries);
        Injector injector = createInjectorForJsonValue(serializedDiscoveryEntries, properties);

        CapabilitiesProvisioning subject = injector.getInstance(CapabilitiesProvisioning.class);
        Collection<GlobalDiscoveryEntry> provisionedDiscoveryEntries = subject.getDiscoveryEntries();

        assertEquals(4, provisionedDiscoveryEntries.size());
        assertContainsEntryFor(provisionedDiscoveryEntries, "interfaceName1");
        assertContainsEntryFor(provisionedDiscoveryEntries, "interfaceName2");
        assertContainsEntryFor(provisionedDiscoveryEntries, GlobalCapabilitiesDirectory.INTERFACE_NAME);
        assertContainsEntryFor(provisionedDiscoveryEntries, GlobalDomainAccessController.INTERFACE_NAME);
    }

    private LegacyCapabilitiesProvisioning.LegacyProvisioningPropertiesHolder createLegacyProvisioningPropertiesHolder() {
        LegacyCapabilitiesProvisioning.LegacyProvisioningPropertiesHolder properties = new LegacyCapabilitiesProvisioning.LegacyProvisioningPropertiesHolder();
        properties.discoveryDirectoryUrl = "http://localhost:8080";
        properties.domainAccessControllerUrl = "http://localhost:9090";
        properties.capabilitiesDirectoryChannelId = "capdir_channel_id";
        properties.capabilitiesDirectoryParticipantId = "capdir_participant_id";
        properties.channelId = "local_channel_id";
        properties.discoveryDirectoriesDomain = "io.joynr";
        properties.domainAccessControllerChannelId = "acl_channel_id";
        properties.domainAccessControllerParticipantId = "acl_participant_id";
        return properties;
    }

    @Test
    public void testLoadingSerializedDiscoveryEntriesNoLegacy() throws Exception {
        Set<DiscoveryEntry> discoveryEntries = createDiscoveryEntries("io.joynr",
                                                                      GlobalCapabilitiesDirectory.INTERFACE_NAME,
                                                                      GlobalDomainAccessController.INTERFACE_NAME);

        final String serializedDiscoveryEntries = objectMapper.writeValueAsString(discoveryEntries);
        Injector injector = createInjectorForJsonValue(serializedDiscoveryEntries);

        CapabilitiesProvisioning subject = injector.getInstance(CapabilitiesProvisioning.class);
        Collection<GlobalDiscoveryEntry> provisionedDiscoveryEntries = subject.getDiscoveryEntries();

        assertEquals(2, provisionedDiscoveryEntries.size());
        assertContainsEntryFor(provisionedDiscoveryEntries, GlobalCapabilitiesDirectory.INTERFACE_NAME);
        assertContainsEntryFor(provisionedDiscoveryEntries, GlobalDomainAccessController.INTERFACE_NAME);
    }

    @Test
    public void testOverrideJsonWithLegacy() throws IOException {
        Set<DiscoveryEntry> discoveryEntries = createDiscoveryEntries("io.joynr",
                                                                      GlobalCapabilitiesDirectory.INTERFACE_NAME,
                                                                      GlobalDomainAccessController.INTERFACE_NAME);

        LegacyCapabilitiesProvisioning.LegacyProvisioningPropertiesHolder properties = createLegacyProvisioningPropertiesHolder();

        final String serializedDiscoveryEntries = objectMapper.writeValueAsString(discoveryEntries);
        Injector injector = createInjectorForJsonValue(serializedDiscoveryEntries, properties);

        CapabilitiesProvisioning subject = injector.getInstance(CapabilitiesProvisioning.class);
        Collection<GlobalDiscoveryEntry> provisionedDiscoveryEntries = subject.getDiscoveryEntries();

        assertEquals(2, provisionedDiscoveryEntries.size());
        assertContainsEntryFor(provisionedDiscoveryEntries,
                               GlobalCapabilitiesDirectory.INTERFACE_NAME,
                               properties.capabilitiesDirectoryParticipantId,
                               properties.discoveryDirectoryUrl);
        assertContainsEntryFor(provisionedDiscoveryEntries,
                               GlobalDomainAccessController.INTERFACE_NAME,
                               properties.domainAccessControllerParticipantId,
                               properties.domainAccessControllerUrl);
    }

    @Test(expected = CreationException.class)
    public void testIncompleteLegacySettings() throws IOException {
        LegacyCapabilitiesProvisioning.LegacyProvisioningPropertiesHolder properties = createLegacyProvisioningPropertiesHolder();
        properties.capabilitiesDirectoryParticipantId = "";
        Set<DiscoveryEntry> discoveryEntries = createDiscoveryEntries("io.joynr",
                                                                      GlobalCapabilitiesDirectory.INTERFACE_NAME,
                                                                      GlobalDomainAccessController.INTERFACE_NAME);
        final String serializedDiscoveryEntries = objectMapper.writeValueAsString(discoveryEntries);
        Injector injector = createInjectorForJsonValue(serializedDiscoveryEntries, properties);
        fail("Expecting legacy capabilities provisioning to fail fast.");
    }

    private void assertContainsEntryFor(Collection<GlobalDiscoveryEntry> entries, String interfaceName) {
        assertContainsEntryFor(entries, interfaceName, null, null);
    }

    private void assertContainsEntryFor(Collection<GlobalDiscoveryEntry> entries,
                                        String interfaceName,
                                        String participantId,
                                        String channelAddressUri) {
        assertContainsEntryFor(entries, interfaceName, participantId, channelAddressUri, null);
    }

    private void assertContainsEntryFor(Collection<GlobalDiscoveryEntry> entries,
                                        String interfaceName,
                                        String participantId,
                                        String channelAddressUri,
                                        String mqttAddressUri) {
        boolean found = false;
        for (DiscoveryEntry entry : entries) {
            if (entry instanceof GlobalDiscoveryEntry) {
                GlobalDiscoveryEntry globalDiscoveryEntry = (GlobalDiscoveryEntry) entry;
                if (globalDiscoveryEntry.getInterfaceName().equals(interfaceName)
                        && (participantId == null || participantId.equals(globalDiscoveryEntry.getParticipantId()))) {
                    if (channelAddressUri != null) {
                        Address address = CapabilityUtils.getAddressFromGlobalDiscoveryEntry(globalDiscoveryEntry);
                        assertTrue(address instanceof ChannelAddress);
                        assertEquals(channelAddressUri, ((ChannelAddress) address).getMessagingEndpointUrl());
                    }
                    if (mqttAddressUri != null) {
                        Address address = CapabilityUtils.getAddressFromGlobalDiscoveryEntry(globalDiscoveryEntry);
                        assertTrue(address instanceof MqttAddress);
                        assertEquals(mqttAddressUri, ((MqttAddress) address).getBrokerUri());
                    }
                    found = true;
                }
            }
        }
        assertTrue("Couldn't find " + interfaceName + ((participantId == null ? "" : " / " + participantId)) + " in "
                + entries, found);
    }

    @Test(expected = CreationException.class)
    public void testInvalidJson() throws IOException {
        Injector injector = createInjectorForJsonValue("this is not json");
        injector.getInstance(CapabilitiesProvisioning.class);
    }

    private Injector createInjectorForJsonValue(final String jsonValue) throws IOException {
        LegacyCapabilitiesProvisioning.LegacyProvisioningPropertiesHolder propertiesHolder = new LegacyCapabilitiesProvisioning.LegacyProvisioningPropertiesHolder();
        propertiesHolder.domainAccessControllerParticipantId = "";
        propertiesHolder.capabilitiesDirectoryParticipantId = "";
        propertiesHolder.domainAccessControllerChannelId = "";
        propertiesHolder.discoveryDirectoriesDomain = "";
        propertiesHolder.channelId = "";
        propertiesHolder.capabilitiesDirectoryChannelId = "";
        propertiesHolder.discoveryDirectoryUrl = "";
        propertiesHolder.domainAccessControllerUrl = "";
        return createInjectorForJsonValue(jsonValue, propertiesHolder);
    }

    private Injector createInjectorForJsonValue(final String jsonValue,
                                                final LegacyCapabilitiesProvisioning.LegacyProvisioningPropertiesHolder provisioningProperties) throws IOException {
        final File tmpFile = File.createTempFile("capprovtest", "json");
        logger.trace("Writing serialised JSON {} to file {}", jsonValue, tmpFile);
        tmpFile.deleteOnExit();
        try (FileWriter writer = new FileWriter(tmpFile)) {
            writer.write(jsonValue + "\n");
            writer.flush();
        } catch (IOException e) {
            fail("Unable to write test data to file: " + tmpFile);
        }
        Injector injector = Guice.createInjector(new AbstractModule() {
            @Override
            protected void configure() {
                bind(String.class).annotatedWith(Names.named(PROPERTY_CAPABILITIES_DIRECTORY_CHANNEL_ID))
                                  .toInstance(provisioningProperties.capabilitiesDirectoryChannelId);
                bind(String.class).annotatedWith(Names.named(PROPERTY_CAPABILITIES_DIRECTORY_PARTICIPANT_ID))
                                  .toInstance(provisioningProperties.capabilitiesDirectoryParticipantId);
                bind(String.class).annotatedWith(Names.named(PROPERTY_DISCOVERY_DIRECTORIES_DOMAIN))
                                  .toInstance(provisioningProperties.discoveryDirectoriesDomain);
                bind(String.class).annotatedWith(Names.named(PROPERTY_DOMAIN_ACCESS_CONTROLLER_CHANNEL_ID))
                                  .toInstance(provisioningProperties.domainAccessControllerChannelId);
                bind(String.class).annotatedWith(Names.named(PROPERTY_DOMAIN_ACCESS_CONTROLLER_PARTICIPANT_ID))
                                  .toInstance(provisioningProperties.domainAccessControllerParticipantId);
                bind(String.class).annotatedWith(Names.named(CHANNELID)).toInstance(provisioningProperties.channelId);
                bind(String.class).annotatedWith(Names.named(DISCOVERYDIRECTORYURL))
                                  .toInstance(provisioningProperties.discoveryDirectoryUrl);
                bind(String.class).annotatedWith(Names.named(MessagingPropertyKeys.DOMAINACCESSCONTROLLERURL))
                                  .toInstance(provisioningProperties.domainAccessControllerUrl);

                bind(ObjectMapper.class).toInstance(objectMapper);
                bind(RoutingTable.class).toInstance(routingTable);
                bind(String.class).annotatedWith(Names.named(StaticCapabilitiesProvisioning.PROPERTY_PROVISIONED_CAPABILITIES_FILE))
                                  .toInstance(tmpFile.getAbsolutePath());
                requestStaticInjection(CapabilityUtils.class);
            }
        }, new StaticCapabilitiesProvisioningModule());
        return injector;
    }
}
