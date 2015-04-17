package io.joynr.capabilities.directory;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2014 BMW Car IT GmbH
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

import static org.junit.Assert.assertEquals;
import io.joynr.capabilities.CapabilitiesStoreImpl;
import io.joynr.capabilities.DefaultCapabilitiesProvisioning;
import io.joynr.provider.PromiseKeeper;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

import joynr.types.CapabilityInformation;
import joynr.types.ProviderQos;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

public class CapabilitiesDirectoryTest {

    CapabilitiesDirectoryImpl capabilitiesDirectory;
    ArrayList<CapabilityInformation> singleInterface = new ArrayList<CapabilityInformation>();
    ArrayList<CapabilityInformation> multipleInterfaces = new ArrayList<CapabilityInformation>();

    String mcId = "capabilitiesProvider";
    String domain = "com";
    String thisInterface = "registerThisInterface";
    String anotherInterface = "registerAnotherInterface";
    String participantId1 = "testParticipantId1";
    String participantId2 = "testParticipantId2";
    ProviderQos providerQos = new ProviderQos();
    CapabilityInformation capInfo1;
    CapabilityInformation capInfo2;
    String postFix = "" + System.currentTimeMillis();

    @Before
    public void setUp() {
        capabilitiesDirectory = new CapabilitiesDirectoryImpl(new CapabilitiesStoreImpl(new DefaultCapabilitiesProvisioning()));
        providerQos.setPriority((long) 123);
        capInfo1 = new CapabilityInformation(domain, thisInterface, providerQos, mcId, participantId1);
        capInfo2 = new CapabilityInformation(domain, anotherInterface, providerQos, mcId, participantId2);
        singleInterface.add(capInfo1);
        multipleInterfaces.add(capInfo1);
        multipleInterfaces.add(capInfo2);
    }

    @After
    public void tearDown() {
    }

    @Test
    public void registerMultipleCapabilities() throws InterruptedException {
        List<CapabilityInformation> expectedLookupThisInterface = new ArrayList<CapabilityInformation>();
        expectedLookupThisInterface.add(capInfo1);
        List<CapabilityInformation> expectedLookupAnotherInterface = new ArrayList<CapabilityInformation>();
        expectedLookupAnotherInterface.add(capInfo2);

        capabilitiesDirectory.add(singleInterface);
        capabilitiesDirectory.add(multipleInterfaces);

        PromiseKeeper lookupThisInterface = new PromiseKeeper();
        capabilitiesDirectory.lookup(domain, thisInterface).then(lookupThisInterface);
        assertEquals(expectedLookupThisInterface, lookupThisInterface.getValues()[0]);

        PromiseKeeper lookupAnotherInterface = new PromiseKeeper();
        capabilitiesDirectory.lookup(domain, anotherInterface).then(lookupAnotherInterface);
        assertEquals(expectedLookupAnotherInterface, lookupAnotherInterface.getValues()[0]);
    }

    @Test
    public void registerCapabilityAndRequestChannels() throws Exception {
        List<CapabilityInformation> expectedLookupThisInterface = new ArrayList<CapabilityInformation>();
        expectedLookupThisInterface.add(capInfo1);

        PromiseKeeper addSingleInterface = new PromiseKeeper();
        capabilitiesDirectory.add(singleInterface).then(addSingleInterface);
        addSingleInterface.waitForSettlement();

        PromiseKeeper lookupThisInterface = new PromiseKeeper();
        capabilitiesDirectory.lookup(domain, thisInterface).then(lookupThisInterface);
        assertEquals(expectedLookupThisInterface, lookupThisInterface.getValues()[0]);
    }

    String getRandomParticipantId() {
        return "participantId-" + String.valueOf(UUID.randomUUID().getLeastSignificantBits()).substring(4);
    }

    String getRandomChannelId() {
        return "channel-" + String.valueOf(UUID.randomUUID().getLeastSignificantBits()).substring(4);
    }

    String getRandomInterface(String prefix) {
        return prefix + "-" + "interface-" + String.valueOf(UUID.randomUUID().getLeastSignificantBits()).substring(4);
    }

    String getRandomDomain(String type) {
        return type + "." + "testdomains." + String.valueOf(UUID.randomUUID().getLeastSignificantBits()).substring(4);
    }

}
