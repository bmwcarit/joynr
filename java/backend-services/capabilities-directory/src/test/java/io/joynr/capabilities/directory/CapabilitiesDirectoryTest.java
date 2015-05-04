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
import io.joynr.provider.PromiseKeeper;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Properties;
import java.util.UUID;

import joynr.types.CapabilityInformation;
import joynr.types.CustomParameter;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;

import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

public class CapabilitiesDirectoryTest {

    private static final List<CustomParameter> CUSTOM_PARAMETERS = new ArrayList<CustomParameter>();

    private static CapabilitiesDirectoryImpl capabilitiesDirectory;

    String channelId = "capabilitiesProvider";
    String domain = "com";
    String interface1 = "interface1";
    String interface2 = "interface2";
    String interface3 = "Interface3";

    ProviderQos providerQos = new ProviderQos(CUSTOM_PARAMETERS, 1, 1L, ProviderScope.GLOBAL, true);
    CapabilityInformation capInfo1;
    CapabilityInformation capInfo2;
    CapabilityInformation capInfo3;
    String postFix = "" + System.currentTimeMillis();

    @BeforeClass
    public static void start() {
        capabilitiesDirectory = startCapabilitiesDirectory();

    }

    @Before
    public void setUp() {

        String participantId1 = "testParticipantId1_" + UUID.randomUUID().toString();
        String participantId2 = "testParticipantId2_" + UUID.randomUUID().toString();
        String participantId3 = "testParticipantId3_" + UUID.randomUUID().toString();

        capInfo1 = new CapabilityInformation(domain, interface1, providerQos, channelId, participantId1);
        capInfo2 = new CapabilityInformation(domain, interface2, providerQos, channelId, participantId2);
        capInfo3 = new CapabilityInformation(domain, interface3, providerQos, channelId, participantId3);

    }

    private static CapabilitiesDirectoryImpl startCapabilitiesDirectory() {
        CapabilitiesDirectoryLauncher.start(new Properties());
        return CapabilitiesDirectoryLauncher.getCapabilitiesDirctory();
    }

    @AfterClass
    public static void tearDown() {
        CapabilitiesDirectoryLauncher.stop();
    }

    @Test
    public void registerMultipleCapabilitiesAsList() throws InterruptedException {

        List<CapabilityInformation> interfaces2And3 = Arrays.asList(capInfo2, capInfo3);
        capabilitiesDirectory.add(interfaces2And3);

        List<CapabilityInformation> expectedCapInfo2 = Arrays.asList(capInfo2);
        PromiseKeeper lookupCapInfo2 = new PromiseKeeper();
        capabilitiesDirectory.lookup(domain, interface2).then(lookupCapInfo2);
        assertEquals(expectedCapInfo2, lookupCapInfo2.getValues()[0]);

        List<CapabilityInformation> expectedCapInfo3 = Arrays.asList(capInfo3);
        PromiseKeeper lookupCapInfo3 = new PromiseKeeper();
        capabilitiesDirectory.lookup(domain, interface3).then(lookupCapInfo3);
        assertEquals(expectedCapInfo3, lookupCapInfo3.getValues()[0]);
    }

    @Test
    public void registerCapabilityAndRequestChannels() throws Exception {

        List<CapabilityInformation> expectedCapInfo1 = Arrays.asList(capInfo1);

        capabilitiesDirectory.add(capInfo1);

        PromiseKeeper lookupCapInfo1 = new PromiseKeeper();
        capabilitiesDirectory.lookup(domain, interface1).then(lookupCapInfo1);
        lookupCapInfo1.waitForSettlement();
        assertEquals(expectedCapInfo1, lookupCapInfo1.getValues()[0]);

    }
}
