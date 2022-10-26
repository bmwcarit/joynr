/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2022 BMW Car IT GmbH
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
package io.joynr.accesscontrol;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.assertNull;

import org.junit.Before;
import org.junit.Test;

import joynr.infrastructure.DacTypes.MasterRegistrationControlEntry;
import joynr.infrastructure.DacTypes.MasterAccessControlEntry;
import joynr.infrastructure.DacTypes.Permission;
import joynr.infrastructure.DacTypes.TrustLevel;

public class DomainWildcardAccessControlStoreTest {

    private DomainAccessControlStore store;

    @Before
    public void setup() {
        store = new DomainAccessControlStoreCqEngine(new DefaultDomainAccessControlProvisioning());
    }

    // Registration Control Entries

    @Test
    public void givenPartialRceDomainWithoutWildcardWhenGetEntryThenEntryIsNull() throws Exception {
        verifyWildcardDomainRegistrationControlEntry("QWE-", false);
    }

    @Test
    public void givenDifferentRceDomainWithoutWildcardWhenGetEntryThenEntryIsNull() throws Exception {
        verifyWildcardDomainRegistrationControlEntry("QWER", false);
    }

    @Test
    public void givenDifferentRceDomainWithWildcardWhenGetEntryThenEntryIsNull() throws Exception {
        verifyWildcardDomainRegistrationControlEntry("QWER*", false);
    }

    @Test
    public void givenDifferentRceDomainWhenGetEntryThenEntryIsNull() throws Exception {
        verifyWildcardDomainRegistrationControlEntry("QWE-4321FDSA", false);
    }

    @Test
    public void givenJustRceDomainWildcardWhenGetEntryThenEntryIsValid() throws Exception {
        verifyWildcardDomainRegistrationControlEntry("*", true);
    }

    @Test
    public void givenPartialRceDomainWithWildcardWhenGetEntryThenEntryIsValid() throws Exception {
        verifyWildcardDomainRegistrationControlEntry("QWE*", true);
    }

    @Test
    public void givenSameRceDomainWhenGetEntryThenEntryIsValid() throws Exception {
        verifyWildcardDomainRegistrationControlEntry("QWE-1234ASDF", true);
    }

    @Test
    public void givenRceDomainContainedWithinStoredDomainWithMissingStartWhenGetEntryThenEntryIsNull() throws Exception {
        verifyWildcardDomainRegistrationControlEntry("1234ASDF*", false);
    }

    /**
     Verifies if with a stored provider domain (with or without WILDCARD) a valid provider domain matches
     the registered provider domain.
     */
    private void verifyWildcardDomainRegistrationControlEntry(String storedAclDomain, boolean shouldValidate) {
        // GIVEN
        String domainFromCode = "QWE-1234ASDF";
        String storedUid = "some.uid";
        String storedInterfaceName = "someInterface";
        MasterRegistrationControlEntry storedEntryWithWildcard = new MasterRegistrationControlEntry(storedUid,
                                                                                                    storedAclDomain,
                                                                                                    storedInterfaceName,
                                                                                                    TrustLevel.LOW,
                                                                                                    new TrustLevel[]{
                                                                                                            TrustLevel.MID,
                                                                                                            TrustLevel.LOW },
                                                                                                    TrustLevel.LOW,
                                                                                                    new TrustLevel[]{
                                                                                                            TrustLevel.MID,
                                                                                                            TrustLevel.LOW },
                                                                                                    Permission.NO,
                                                                                                    new Permission[]{
                                                                                                            Permission.ASK,
                                                                                                            Permission.NO });
        store.updateMasterRegistrationControlEntry(storedEntryWithWildcard);

        // WHEN
        MasterRegistrationControlEntry obtainedEntry = store.getMasterRegistrationControlEntry(storedUid,
                                                                                               domainFromCode,
                                                                                               storedInterfaceName);

        // THEN
        if (shouldValidate) {
            assertEquals(storedEntryWithWildcard, obtainedEntry);
        } else {
            assertNull(obtainedEntry);
        }
    }

    // Access Control Entries

    @Test
    public void givenPartialAceDomainWithoutWildcardWhenGetEntryThenEntryIsNull() throws Exception {
        verifyWildcardDomainAccessControlEntry("QWE-", false);
    }

    @Test
    public void givenDifferentAceDomainWithoutWildcardWhenGetEntryThenEntryIsNull() throws Exception {
        verifyWildcardDomainAccessControlEntry("QWER", false);
    }

    @Test
    public void givenDifferentAceDomainWithWildcardWhenGetEntryThenEntryIsNull() throws Exception {
        verifyWildcardDomainAccessControlEntry("QWER*", false);
    }

    @Test
    public void givenDifferentAceDomainWhenGetEntryThenEntryIsNull() throws Exception {
        verifyWildcardDomainAccessControlEntry("QWE-4321FDSA", false);
    }

    @Test
    public void givenJustAceDomainWildcardWhenGetEntryThenEntryIsValid() throws Exception {
        verifyWildcardDomainAccessControlEntry("*", true);
    }

    @Test
    public void givenPartialAceDomainWithWildcardWhenGetEntryThenEntryIsValid() throws Exception {
        verifyWildcardDomainAccessControlEntry("QWE*", true);
    }

    @Test
    public void givenSameAceDomainWhenGetEntryThenEntryIsValid() throws Exception {
        verifyWildcardDomainAccessControlEntry("QWE-1234ASDF", true);
    }

    @Test
    public void givenAceDomainContainedWithinStoredDomainWithMissingStartWhenGetEntryThenEntryIsNull() throws Exception {
        verifyWildcardDomainAccessControlEntry("1234ASDF*", false);
    }

    /**
     Verifies if with a stored consumer domain (with or without WILDCARD) a valid consumer domain matches
     the registered consumer domain.
     */
    private void verifyWildcardDomainAccessControlEntry(String storedAclDomain, boolean shouldValidate) {
        // GIVEN
        String domainFromCode = "QWE-1234ASDF";
        String storedUid = "some.uid";
        String storedInterfaceName = "someInterface";
        String storedOperation = "*";
        MasterAccessControlEntry storedEntryWithWildcard = new MasterAccessControlEntry(storedUid,
                                                                                        storedAclDomain,
                                                                                        storedInterfaceName,
                                                                                        TrustLevel.LOW,
                                                                                        new TrustLevel[]{
                                                                                                TrustLevel.MID,
                                                                                                TrustLevel.LOW },
                                                                                        TrustLevel.LOW,
                                                                                        new TrustLevel[]{
                                                                                                TrustLevel.MID,
                                                                                                TrustLevel.LOW },
                                                                                        storedOperation,
                                                                                        Permission.NO,
                                                                                        new Permission[]{
                                                                                                Permission.ASK,
                                                                                                Permission.NO });
        store.updateMasterAccessControlEntry(storedEntryWithWildcard);

        // WHEN
        MasterAccessControlEntry obtainedEntry = store.getMasterAccessControlEntry(storedUid,
                                                                                   domainFromCode,
                                                                                   storedInterfaceName,
                                                                                   storedOperation);

        // THEN
        if (shouldValidate) {
            assertEquals(storedEntryWithWildcard, obtainedEntry);
        } else {
            assertNull(obtainedEntry);
        }
    }
}
