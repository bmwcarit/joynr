/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
package io.joynr.proxy;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import java.util.Arrays;
import java.util.Collection;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.junit.MockitoJUnitRunner;

import io.joynr.arbitration.ArbitrationConstants;
import joynr.types.CustomParameter;
import joynr.types.DiscoveryEntry;
import joynr.types.DiscoveryEntryWithMetaInfo;
import joynr.types.ProviderQos;
import joynr.types.Version;

@RunWith(MockitoJUnitRunner.class)
public class DiscoveryResultTest {

    @Test
    public void testConstructorWithDiscoveryEntriesWithMetaInfo() {
        String participantId1 = "participant1";
        String participantId2 = "participant2";
        DiscoveryEntryWithMetaInfo entry1 = new DiscoveryEntryWithMetaInfo();
        entry1.setParticipantId(participantId1);
        DiscoveryEntryWithMetaInfo entry2 = new DiscoveryEntryWithMetaInfo();
        entry2.setParticipantId(participantId2);
        DiscoveryResult subject = new DiscoveryResult(Arrays.asList(entry1, entry2));
        Collection<DiscoveryEntry> entries = subject.getAllDiscoveryEntries();
        boolean entry1found = false;
        boolean entry2found = false;
        for (DiscoveryEntry entry : entries) {
            if (entry.getParticipantId() == entry1.getParticipantId()) {
                entry1found = true;
            }
            if (entry.getParticipantId() == entry2.getParticipantId()) {
                entry2found = true;
            }
        }
        assertTrue(entry1found && entry2found);
        assertEquals(2, entries.size());
    }

    @Test
    public void testReturnLastSeen() {
        Long lastSeenDateMs = Long.valueOf(10000);
        Long anotherDateMs = Long.valueOf(500);
        DiscoveryEntryWithMetaInfo entry1 = new DiscoveryEntryWithMetaInfo();
        entry1.setLastSeenDateMs(lastSeenDateMs);
        DiscoveryEntryWithMetaInfo entry2 = new DiscoveryEntryWithMetaInfo();
        entry2.setLastSeenDateMs(anotherDateMs);
        DiscoveryResult subject = new DiscoveryResult(Arrays.asList(entry1, entry2));
        DiscoveryEntry lastSeenEntry = subject.getLastSeen();
        assertEquals(lastSeenDateMs, lastSeenEntry.getLastSeenDateMs());
    }

    @Test
    public void testReturnHighestPriority() {
        Long highPriority = Long.valueOf(10000);
        Long lowPriority = Long.valueOf(500);
        ProviderQos highPriorityProviderQos = new ProviderQos();
        highPriorityProviderQos.setPriority(highPriority);
        ProviderQos lowPriorityProviderQos = new ProviderQos();
        lowPriorityProviderQos.setPriority(lowPriority);
        DiscoveryEntryWithMetaInfo entry1 = new DiscoveryEntryWithMetaInfo();
        entry1.setQos(highPriorityProviderQos);
        DiscoveryEntryWithMetaInfo entry2 = new DiscoveryEntryWithMetaInfo();
        entry2.setQos(lowPriorityProviderQos);
        DiscoveryResult subject = new DiscoveryResult(Arrays.asList(entry1, entry2));
        DiscoveryEntry highPriorityEntry = subject.getHighestPriority();
        assertEquals(highPriorityProviderQos.getPriority(), highPriorityEntry.getQos().getPriority());
    }

    @Test
    public void testReturnLatestVersionByMajor() {
        Version latestVersion = new Version(2, 0);
        Version olderVersion = new Version(1, 0);
        DiscoveryEntryWithMetaInfo entry1 = new DiscoveryEntryWithMetaInfo();
        entry1.setProviderVersion(latestVersion);
        DiscoveryEntryWithMetaInfo entry2 = new DiscoveryEntryWithMetaInfo();
        entry2.setProviderVersion(olderVersion);
        DiscoveryResult subject = new DiscoveryResult(Arrays.asList(entry1, entry2));
        DiscoveryEntry entryWithLatestVersion = subject.getLatestVersion();
        assertEquals(latestVersion, entryWithLatestVersion.getProviderVersion());
    }

    @Test
    public void testReturnLatestVersionByMinor() {
        Version latestVersion = new Version(2, 5);
        Version olderVersion = new Version(2, 0);
        DiscoveryEntryWithMetaInfo entry1 = new DiscoveryEntryWithMetaInfo();
        entry1.setProviderVersion(latestVersion);
        DiscoveryEntryWithMetaInfo entry2 = new DiscoveryEntryWithMetaInfo();
        entry2.setProviderVersion(olderVersion);
        DiscoveryResult subject = new DiscoveryResult(Arrays.asList(entry1, entry2));
        DiscoveryEntry entryWithLatestVersion = subject.getLatestVersion();
        assertEquals(latestVersion, entryWithLatestVersion.getProviderVersion());
    }

    @Test
    public void testGetByParticipantId() {
        String participantId1 = "participant1";
        String participantId2 = "participant2";
        DiscoveryEntryWithMetaInfo entry1 = new DiscoveryEntryWithMetaInfo();
        entry1.setParticipantId(participantId1);
        DiscoveryEntryWithMetaInfo entry2 = new DiscoveryEntryWithMetaInfo();
        entry2.setParticipantId(participantId2);
        DiscoveryResult subject = new DiscoveryResult(Arrays.asList(entry1, entry2));
        DiscoveryEntry foundEntry = subject.getParticipantId(participantId1);
        assertEquals(participantId1, foundEntry.getParticipantId());
    }

    @Test
    public void testGetByParticipantIdReturnsNullOnNonexistent() {
        String participantId1 = "participant1";
        String participantId2 = "participant2";
        String participantId3 = "participant3";
        DiscoveryEntryWithMetaInfo entry1 = new DiscoveryEntryWithMetaInfo();
        entry1.setParticipantId(participantId1);
        DiscoveryEntryWithMetaInfo entry2 = new DiscoveryEntryWithMetaInfo();
        entry2.setParticipantId(participantId2);
        DiscoveryResult subject = new DiscoveryResult(Arrays.asList(entry1, entry2));
        DiscoveryEntry foundEntry = subject.getParticipantId(participantId3);
        assertNull(foundEntry);
    }

    @Test
    public void testGetByKeyword() {
        String keywordValue = "test";
        String participantId1 = "participant1";
        String participantId2 = "participant2";
        String participantId3 = "participant3";
        CustomParameter keywordParameter = new CustomParameter(ArbitrationConstants.KEYWORD_PARAMETER, keywordValue);
        ProviderQos providerQosWithKeyword = new ProviderQos();
        providerQosWithKeyword.setCustomParameters(new CustomParameter[]{ keywordParameter });
        ProviderQos providerQosWithoutKeyword = new ProviderQos();
        DiscoveryEntryWithMetaInfo entry1 = new DiscoveryEntryWithMetaInfo();
        entry1.setQos(providerQosWithKeyword);
        entry1.setParticipantId(participantId1);
        DiscoveryEntryWithMetaInfo entry2 = new DiscoveryEntryWithMetaInfo();
        entry2.setQos(providerQosWithoutKeyword);
        entry2.setParticipantId(participantId2);
        DiscoveryEntryWithMetaInfo entry3 = new DiscoveryEntryWithMetaInfo();
        entry3.setQos(providerQosWithKeyword);
        entry3.setParticipantId(participantId3);
        DiscoveryResult subject = new DiscoveryResult(Arrays.asList(entry1, entry2, entry3));
        Collection<DiscoveryEntry> entriesWithKeyword = subject.getWithKeyword(keywordValue);
        assertEquals(2, entriesWithKeyword.size());
        boolean entry1found = false;
        boolean entry3found = false;
        for (DiscoveryEntry entry : entriesWithKeyword) {
            if (entry.getParticipantId() == entry1.getParticipantId()) {
                entry1found = true;
            }
            if (entry.getParticipantId() == entry3.getParticipantId()) {
                entry3found = true;
            }
        }
        assertTrue(entry1found && entry3found);
    }
}
