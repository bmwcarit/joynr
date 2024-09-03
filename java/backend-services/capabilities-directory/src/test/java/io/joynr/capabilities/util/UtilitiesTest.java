/*
 * #%L
 * %%
 * Copyright (C) 2019 - 2024 BMW Car IT GmbH
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
package io.joynr.capabilities.util;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

import io.joynr.capabilities.CustomParameterPersisted;
import io.joynr.capabilities.GlobalDiscoveryEntryPersisted;
import io.joynr.capabilities.ProviderQosPersisted;
import joynr.types.CustomParameter;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderScope;
import joynr.types.Version;
import org.junit.Before;
import org.junit.Test;

import io.joynr.capabilities.directory.util.GcdUtilities;
import io.joynr.capabilities.directory.util.GcdUtilities.ValidateGBIDsEnum;

public class UtilitiesTest {
    private final String gcdGbId = "gcdGbId";
    private final Set<String> validGbids = new HashSet<>();

    @Before
    public void setUp() {
        validGbids.add(gcdGbId);
    }

    @Test(expected = IllegalStateException.class)
    public void testFailureOnNullGcdGbId() {
        GcdUtilities.validateGbids(new String[]{ gcdGbId }, null, validGbids);
    }

    @Test
    public void testReturnInvalidOnNullGbids() {
        assertEquals(ValidateGBIDsEnum.INVALID, GcdUtilities.validateGbids(null, gcdGbId, validGbids));
    }

    @Test
    public void testReturnInvalidEmptyGbids() {
        final String[] invalidGbids = {};
        assertEquals(ValidateGBIDsEnum.INVALID, GcdUtilities.validateGbids(invalidGbids, gcdGbId, validGbids));
    }

    @Test
    public void testReturnInvalidOnNullGbid() {
        final String[] invalidGbids = { null };
        assertEquals(ValidateGBIDsEnum.INVALID, GcdUtilities.validateGbids(invalidGbids, gcdGbId, validGbids));
    }

    @Test
    public void testReturnUnknownOnUnknownGbid() {
        final String[] invalidGbids = { "wrong-gbid" };
        assertEquals(ValidateGBIDsEnum.UNKNOWN, GcdUtilities.validateGbids(invalidGbids, gcdGbId, validGbids));
    }

    @Test
    public void testReturnOKOnValidGbid() {
        assertEquals(ValidateGBIDsEnum.OK, GcdUtilities.validateGbids(new String[]{ gcdGbId }, gcdGbId, validGbids));
    }

    @Test
    public void testReturnOKOnEmptyGbid() {
        assertEquals(ValidateGBIDsEnum.OK, GcdUtilities.validateGbids(new String[]{ "" }, gcdGbId, validGbids));
    }

    @Test
    public void testReplacePersistedCustomParameters() {
        final GlobalDiscoveryEntry persistedEntry = generateGlobalDiscoveryEntryPersisted();

        GcdUtilities.replaceCustomParameters(persistedEntry);

        assertCustomParametersAreNotPersistedOnes(persistedEntry);
    }

    @Test
    public void testChooseOneGlobalDiscoveryEntryReplacesPersistedCustomParameters() {
        final GlobalDiscoveryEntryPersisted entryPersisted = generateGlobalDiscoveryEntryPersisted();

        final GlobalDiscoveryEntry entry = GcdUtilities.chooseOneGlobalDiscoveryEntry(Arrays.asList(entryPersisted),
                                                                                      "gbId");

        assertCustomParametersAreNotPersistedOnes(entry);
    }

    private void assertCustomParametersAreNotPersistedOnes(final GlobalDiscoveryEntry entry) {
        final CustomParameter[] customParameters = entry.getQos().getCustomParameters();
        for (int i = 0; i < customParameters.length; i++) {
            final CustomParameter customParameter = customParameters[i];
            assertFalse(customParameter instanceof CustomParameterPersisted);
            assertTrue(customParameter instanceof CustomParameter);
        }
    }

    private GlobalDiscoveryEntryPersisted generateGlobalDiscoveryEntryPersisted() {
        final GlobalDiscoveryEntryPersisted persistedEntry = new GlobalDiscoveryEntryPersisted();
        persistedEntry.setAddress("address");
        persistedEntry.setProviderVersion(new Version(1, 1));
        persistedEntry.setDomain("domain");
        persistedEntry.setInterfaceName("interfaceName");
        persistedEntry.setParticipantId("participantId0");
        persistedEntry.setGbid("gbId");

        final ProviderQosPersisted providerQos = new ProviderQosPersisted();
        final CustomParameterPersisted[] customParameters = new CustomParameterPersisted[2];
        final CustomParameterPersisted firstParameter = new CustomParameterPersisted();
        firstParameter.setName("keyA");
        firstParameter.setValue("valueA");
        final CustomParameterPersisted secondParameter = new CustomParameterPersisted();
        secondParameter.setName("keyB");
        secondParameter.setValue("valueB");
        customParameters[0] = firstParameter;
        customParameters[1] = secondParameter;

        providerQos.setPriority(100L);
        providerQos.setScope(ProviderScope.GLOBAL);
        providerQos.setSupportsOnChangeSubscriptions(true);
        providerQos.setCustomParameters(customParameters);

        persistedEntry.setQos(providerQos);
        persistedEntry.setLastSeenDateMs(100L);
        persistedEntry.setExpiryDateMs(200L);
        persistedEntry.setPublicKeyId("publicKeyId");

        return persistedEntry;
    }
}
