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
package io.joynr.accesscontrol;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import org.junit.Before;
import org.junit.Test;

import joynr.infrastructure.DacTypes.DomainRoleEntry;
import joynr.infrastructure.DacTypes.MasterAccessControlEntry;
import joynr.infrastructure.DacTypes.MasterRegistrationControlEntry;
import joynr.infrastructure.DacTypes.OwnerAccessControlEntry;
import joynr.infrastructure.DacTypes.OwnerRegistrationControlEntry;
import joynr.infrastructure.DacTypes.Permission;
import joynr.infrastructure.DacTypes.Role;
import joynr.infrastructure.DacTypes.TrustLevel;

public class DomainAccessControlStoreTest {

    private static final String WILDCARD = "*";
    private static final String UID1 = "uid1";
    private static final String UID2 = "uid2";
    private static final String DOMAIN1 = "domain1";
    private static final String INTERFACE1 = "interface1";
    private static final String INTERFACEX = "interfaceX";
    private static final String OPERATION1 = "operation1";
    private static final String OPERATIONX = "operationX";

    private DomainAccessControlStore store;
    private MasterAccessControlEntry expectedMasterAccessControlEntry;
    private OwnerAccessControlEntry expectedOwnerAccessControlEntry;
    private MasterRegistrationControlEntry expectedMasterRegistrationControlEntry;
    private OwnerRegistrationControlEntry expectedOwnerRegistrationControlEntry;
    private DomainRoleEntry expectedUserDomainRoleEntry;

    @Before
    public void setup() {
        store = new DomainAccessControlStoreCqEngine(new DefaultDomainAccessControlProvisioning());
        // instantiate some template objects
        expectedUserDomainRoleEntry = new DomainRoleEntry(UID1, new String[0], Role.OWNER);
        expectedMasterAccessControlEntry = new MasterAccessControlEntry(UID1,
                                                                        DOMAIN1,
                                                                        INTERFACE1,
                                                                        TrustLevel.LOW,
                                                                        new TrustLevel[]{ TrustLevel.MID,
                                                                                TrustLevel.LOW },
                                                                        TrustLevel.LOW,
                                                                        new TrustLevel[]{ TrustLevel.MID,
                                                                                TrustLevel.LOW },
                                                                        OPERATION1,
                                                                        Permission.NO,
                                                                        new Permission[]{ Permission.ASK,
                                                                                Permission.NO });
        expectedOwnerAccessControlEntry = new OwnerAccessControlEntry(UID1,
                                                                      DOMAIN1,
                                                                      INTERFACE1,
                                                                      TrustLevel.LOW,
                                                                      TrustLevel.LOW,
                                                                      OPERATION1,
                                                                      Permission.NO);

        expectedMasterRegistrationControlEntry = new MasterRegistrationControlEntry(UID1,
                                                                                    DOMAIN1,
                                                                                    INTERFACE1,
                                                                                    TrustLevel.LOW,
                                                                                    new TrustLevel[]{ TrustLevel.MID,
                                                                                            TrustLevel.LOW },
                                                                                    TrustLevel.LOW,
                                                                                    new TrustLevel[]{ TrustLevel.MID,
                                                                                            TrustLevel.LOW },
                                                                                    Permission.NO,
                                                                                    new Permission[]{ Permission.ASK,
                                                                                            Permission.NO });
        expectedOwnerRegistrationControlEntry = new OwnerRegistrationControlEntry(UID1,
                                                                                  DOMAIN1,
                                                                                  INTERFACE1,
                                                                                  TrustLevel.LOW,
                                                                                  TrustLevel.LOW,
                                                                                  Permission.NO);
    }

    @Test
    public void testGetDomainRoles() throws Exception {
        store.updateDomainRole(expectedUserDomainRoleEntry);

        assertEquals("DRE for UID1 should be the same as expectedOwnerAccessControlEntry",
                     expectedUserDomainRoleEntry,
                     store.getDomainRoles(UID1).get(0));
        assertEquals("DRE for UID1 and Role.OWNER should be the same as expectedOwnerAccessControlEntry",
                     expectedUserDomainRoleEntry,
                     store.getDomainRole(UID1, Role.OWNER));
    }

    @Test
    public void testUpdateDomainRole() throws Exception {
        store.updateDomainRole(expectedUserDomainRoleEntry);
        // update dre role to MASTER
        DomainRoleEntry dre = store.getDomainRoles(UID1).get(0);
        dre.setRole(Role.MASTER);
        store.updateDomainRole(dre);

        boolean found = false;
        for (DomainRoleEntry entry : store.getDomainRoles(UID1)) {
            if (entry.getRole().equals(Role.MASTER)) {
                found = true;
            }
        }

        assertTrue("DomainRoleEntry for UID1 with role MASTER should exist", found);
    }

    @Test
    public void testRemoveDomainRole() throws Exception {
        store.updateDomainRole(expectedUserDomainRoleEntry);

        assertTrue("Remove UID1 DRE should return true", store.removeDomainRole(UID1, Role.OWNER));
        assertTrue("There should be no UID1 DREs in DRT any more", store.getDomainRoles(UID1).isEmpty());
    }

    @Test
    public void testGetMasterAce() throws Exception {
        store.updateMasterAccessControlEntry(expectedMasterAccessControlEntry);

        assertEquals("Master ACE associated to UID1 from Master ACL should be the same as expectedMasterAccessControlEntry",
                     expectedMasterAccessControlEntry,
                     store.getMasterAccessControlEntries(UID1).get(0));
        assertEquals("Master ACE associated to DOMAIN1 and INTERFACE1 should be the same as expectedMasterAccessControlEntry",
                     expectedMasterAccessControlEntry,
                     store.getMasterAccessControlEntries(DOMAIN1, INTERFACE1).get(0));
        assertEquals("Master ACE associated to UID1, DOMAIN1 and INTERFACE1 should be the same as expectedMasterAccessControlEntry",
                     expectedMasterAccessControlEntry,
                     store.getMasterAccessControlEntries(UID1, DOMAIN1, INTERFACE1).get(0));

        MasterAccessControlEntry masterAceWithWildcardOperation = new MasterAccessControlEntry(expectedMasterAccessControlEntry);
        masterAceWithWildcardOperation.setOperation(WILDCARD);
        store.updateMasterAccessControlEntry(masterAceWithWildcardOperation);
        int expectedAceCount = 2;
        assertEquals("There are two (2) master ACEs associated to UID1, DOMAIN1 and INTERFACE1",
                     expectedAceCount,
                     store.getMasterAccessControlEntries(UID1, DOMAIN1, INTERFACE1).size());

        MasterAccessControlEntry returnedMasterAce = store.getMasterAccessControlEntry(UID1,
                                                                                       DOMAIN1,
                                                                                       INTERFACE1,
                                                                                       OPERATION1);
        assertEquals("Master ACE associated to UID1, DOMAIN1, INTERFACE1 and OPERATION1 should be the same as expectedMasterAccessControlEntry",
                     expectedMasterAccessControlEntry,
                     returnedMasterAce);
    }

    @Test
    public void testGetMasterAceWithWildcardOperation() throws Exception {
        expectedMasterAccessControlEntry.setOperation(WILDCARD);
        store.updateMasterAccessControlEntry(expectedMasterAccessControlEntry);

        assertEquals("Master ACE associated to UID1, DOMAIN1, INTERFACE1 and OPERATION1 should be the same as expectedMasterAccessControlEntry",
                     expectedMasterAccessControlEntry,
                     store.getMasterAccessControlEntry(UID1, DOMAIN1, INTERFACE1, OPERATION1));
    }

    @Test
    public void testGetEditableMasterAcl() throws Exception {
        expectedUserDomainRoleEntry.setDomains(new String[]{ DOMAIN1 });
        expectedUserDomainRoleEntry.setRole(Role.MASTER);
        store.updateDomainRole(expectedUserDomainRoleEntry);
        store.updateMasterAccessControlEntry(expectedMasterAccessControlEntry);

        assertEquals("Editable master ACE for UID1 should be equal to expectedMasterAccessControlEntry",
                     expectedMasterAccessControlEntry,
                     store.getEditableMasterAccessControlEntries(UID1).get(0));
    }

    @Test
    public void testEditableMasterAccessControlEntryNoMatchingDre() throws Exception {
        expectedMasterAccessControlEntry.setUid(UID2);
        store.updateMasterAccessControlEntry(expectedMasterAccessControlEntry);

        assertTrue("There should be no editable master ACE for UID1 in Master ACL",
                   store.getEditableMasterAccessControlEntries(UID1).isEmpty());
    }

    @Test
    public void testUpdateMasterAccessControlEntry() throws Exception {
        boolean expectedUpdateResult = true;
        expectedMasterAccessControlEntry.setDefaultConsumerPermission(Permission.NO);
        assertEquals("Insert master ACE should return true",
                     expectedUpdateResult,
                     store.updateMasterAccessControlEntry(expectedMasterAccessControlEntry));
        assertTrue("Before update master ACE for UID1 should have default Permission.NO",
                   store.getMasterAccessControlEntries(UID1)
                        .get(0)
                        .getDefaultConsumerPermission()
                        .equals(Permission.NO));
        expectedMasterAccessControlEntry.setDefaultConsumerPermission(Permission.YES);
        assertEquals("Update master ACE should return true",
                     expectedUpdateResult,
                     store.updateMasterAccessControlEntry(expectedMasterAccessControlEntry));
        assertTrue("After update master ACE for UID1 should have default Permission.YES",
                   store.getMasterAccessControlEntries(UID1)
                        .get(0)
                        .getDefaultConsumerPermission()
                        .equals(Permission.YES));
    }

    @Test
    public void testRemoveMasterAccessControlEntry() throws Exception {
        store.updateMasterAccessControlEntry(expectedMasterAccessControlEntry);
        boolean expectedRemoveResult = true;

        assertEquals("Remove master ACE for given userId, domain, interface and operation should return true",
                     expectedRemoveResult,
                     store.removeMasterAccessControlEntry(UID1, DOMAIN1, INTERFACE1, OPERATION1));
        assertTrue("In Master ACL no master ACE for given domain, interface should remain",
                   store.getMasterAccessControlEntries(DOMAIN1, INTERFACE1).isEmpty());
        assertTrue("In Master ACL no master ACE for UID1 should remain",
                   store.getMasterAccessControlEntries(UID1).isEmpty());
    }

    // Mediator ACEs have the same datatype as master ACEs, which is why expectedMasterAccessControlEntry
    // will be reused, but not renamed, in the following tests.

    @Test
    public void testGetMediatorAce() throws Exception {
        store.updateMasterAccessControlEntry(expectedMasterAccessControlEntry);
        store.updateMediatorAccessControlEntry(expectedMasterAccessControlEntry);

        assertEquals("Mediator ACE associated to UID1 from Mediator ACL should be the same as expectedMediatorAccessControlEntry",
                     expectedMasterAccessControlEntry,
                     store.getMediatorAccessControlEntries(UID1).get(0));
        assertEquals("Mediator ACE associated to DOMAIN1 and INTERFACE1 should be the same as expectedMediatorAccessControlEntry",
                     expectedMasterAccessControlEntry,
                     store.getMediatorAccessControlEntries(DOMAIN1, INTERFACE1).get(0));
        assertEquals("Mediator ACE associated to UID1, DOMAIN1 and INTERFACE1 should be the same as expectedMediatorAccessControlEntry",
                     expectedMasterAccessControlEntry,
                     store.getMediatorAccessControlEntries(UID1, DOMAIN1, INTERFACE1).get(0));

        MasterAccessControlEntry mediatorAceWithWildcardOperation = new MasterAccessControlEntry(expectedMasterAccessControlEntry);
        mediatorAceWithWildcardOperation.setOperation(WILDCARD);
        store.updateMasterAccessControlEntry(mediatorAceWithWildcardOperation);
        store.updateMediatorAccessControlEntry(mediatorAceWithWildcardOperation);
        int expectedAceCount = 2;
        assertEquals("There are two (2) mediator ACEs associated to UID1, DOMAIN1 and INTERFACE1",
                     expectedAceCount,
                     store.getMediatorAccessControlEntries(UID1, DOMAIN1, INTERFACE1).size());

        MasterAccessControlEntry returnedMediatorAce = store.getMediatorAccessControlEntry(UID1,
                                                                                           DOMAIN1,
                                                                                           INTERFACE1,
                                                                                           OPERATION1);
        assertEquals("Mediator ACE associated to UID1, DOMAIN1, INTERFACE1 and OPERATION1 should be the same as expectedMediatorAccessControlEntry",
                     expectedMasterAccessControlEntry,
                     returnedMediatorAce);
    }

    @Test
    public void testGetMediatorAceWithWildcardOperation() throws Exception {
        expectedMasterAccessControlEntry.setOperation(WILDCARD);
        store.updateMasterAccessControlEntry(expectedMasterAccessControlEntry);
        store.updateMediatorAccessControlEntry(expectedMasterAccessControlEntry);

        assertEquals("Mediator ACE associated to UID1, DOMAIN1, INTERFACE1 and OPERATION1 should be the same as expectedMediatorAccessControlEntry",
                     expectedMasterAccessControlEntry,
                     store.getMediatorAccessControlEntry(UID1, DOMAIN1, INTERFACE1, OPERATION1));
    }

    @Test
    public void testGetEditableMediatorAcl() throws Exception {
        expectedUserDomainRoleEntry.setDomains(new String[]{ DOMAIN1 });
        expectedUserDomainRoleEntry.setRole(Role.MASTER);
        store.updateDomainRole(expectedUserDomainRoleEntry);
        store.updateMasterAccessControlEntry(expectedMasterAccessControlEntry);
        store.updateMediatorAccessControlEntry(expectedMasterAccessControlEntry);

        assertEquals("Editable mediator ACE for UID1 should be equal to expectedMediatorAccessControlEntry",
                     expectedMasterAccessControlEntry,
                     store.getEditableMasterAccessControlEntries(UID1).get(0));
    }

    @Test
    public void testEditableMediatorAccessControlEntryNoMatchingDre() throws Exception {
        expectedMasterAccessControlEntry.setUid(UID2);
        store.updateMasterAccessControlEntry(expectedMasterAccessControlEntry);
        store.updateMediatorAccessControlEntry(expectedMasterAccessControlEntry);

        assertTrue("There should be no editable mediator ACE for UID1 in Master ACL",
                   store.getEditableMediatorAccessControlEntries(UID1).isEmpty());
    }

    @Test
    public void testUpdateMediatorAccessControlEntry() throws Exception {
        boolean expectedUpdateResult = true;
        expectedMasterAccessControlEntry.setDefaultConsumerPermission(Permission.NO);
        assertEquals("Insert master ACE should return true",
                     expectedUpdateResult,
                     store.updateMasterAccessControlEntry(expectedMasterAccessControlEntry));
        assertEquals("Insert mediator ACE should return true",
                     expectedUpdateResult,
                     store.updateMediatorAccessControlEntry(expectedMasterAccessControlEntry));
        assertTrue("Before update mediator ACE for UID1 should have default Permission.NO",
                   store.getMediatorAccessControlEntries(UID1)
                        .get(0)
                        .getDefaultConsumerPermission()
                        .equals(Permission.NO));
        expectedMasterAccessControlEntry.setDefaultConsumerPermission(Permission.YES);
        assertEquals("Update mediator ACE should return false",
                     false,
                     store.updateMediatorAccessControlEntry(expectedMasterAccessControlEntry));
        expectedMasterAccessControlEntry.setPossibleConsumerPermissions(new Permission[]{ Permission.ASK, Permission.NO,
                Permission.YES });
        assertEquals("Update master ACE should return true",
                     expectedUpdateResult,
                     store.updateMasterAccessControlEntry(expectedMasterAccessControlEntry));
        assertEquals("Update mediator ACE should return true",
                     expectedUpdateResult,
                     store.updateMediatorAccessControlEntry(expectedMasterAccessControlEntry));
        assertTrue("After update mediator ACE for UID1 should have default Permission.YES",
                   store.getMediatorAccessControlEntries(UID1)
                        .get(0)
                        .getDefaultConsumerPermission()
                        .equals(Permission.YES));
    }

    @Test
    public void testRemoveMediatorAccessControlEntry() throws Exception {
        store.updateMasterAccessControlEntry(expectedMasterAccessControlEntry);
        store.updateMediatorAccessControlEntry(expectedMasterAccessControlEntry);
        boolean expectedRemoveResult = true;

        assertEquals("Remove mediator ACE for given userId, domain, interface and operation should return true",
                     expectedRemoveResult,
                     store.removeMediatorAccessControlEntry(UID1, DOMAIN1, INTERFACE1, OPERATION1));
        assertTrue("In Mediator ACL no mediator ACE for given domain, interface should remain",
                   store.getMediatorAccessControlEntries(DOMAIN1, INTERFACE1).isEmpty());
        assertTrue("In Mediator ACL no mediator ACE for UID1 should remain",
                   store.getMediatorAccessControlEntries(UID1).isEmpty());
    }

    @Test
    public void testGetOwnerAccessControlEntry() throws Exception {
        store.updateOwnerAccessControlEntry(expectedOwnerAccessControlEntry);

        assertEquals("Owner ACE for UID1 should be equal to expectedOwnerAccessControlEntry",
                     expectedOwnerAccessControlEntry,
                     store.getOwnerAccessControlEntries(UID1).get(0));
        assertEquals("Owner ACE associated to DOMAIN1 and INTERFACE1 should be the same as expectedOwnerAccessControlEntry",
                     expectedOwnerAccessControlEntry,
                     store.getOwnerAccessControlEntries(DOMAIN1, INTERFACE1).get(0));
        assertEquals("Owner ACE associated to UID1, DOMAIN1 and INTERFACE1 should be the same as expectedOwnerAccessControlEntry",
                     expectedOwnerAccessControlEntry,
                     store.getOwnerAccessControlEntries(UID1, DOMAIN1, INTERFACE1).get(0));
        OwnerAccessControlEntry returnedOwnerAce = store.getOwnerAccessControlEntry(UID1,
                                                                                    DOMAIN1,
                                                                                    INTERFACE1,
                                                                                    OPERATION1);
        assertEquals("Owner ACE associated to UID1, DOMAIN1, INTERFACE1 and OPERATION1 should be the same as expectedOwnerAccessControlEntry",
                     expectedOwnerAccessControlEntry,
                     returnedOwnerAce);
    }

    @Test
    public void testEditableOwnerAccessControlEntry() throws Exception {
        expectedUserDomainRoleEntry.setDomains(new String[]{ DOMAIN1 });
        store.updateDomainRole(expectedUserDomainRoleEntry);
        store.updateOwnerAccessControlEntry(expectedOwnerAccessControlEntry);

        assertEquals("Editable Owner ACE for UID1 should be equal to expectedOwnerAccessControlEntry",
                     expectedOwnerAccessControlEntry,
                     store.getEditableOwnerAccessControlEntries(UID1).get(0));
    }

    @Test
    public void testEditableOwnerAccessControlEntryNoMatchingDre() throws Exception {
        store.updateDomainRole(expectedUserDomainRoleEntry);
        expectedOwnerAccessControlEntry.setUid(UID2);
        store.updateOwnerAccessControlEntry(expectedOwnerAccessControlEntry);

        assertTrue("No editable owner ACEs for UID2 should be in Owner ACL",
                   store.getEditableOwnerAccessControlEntries(UID2).isEmpty());
    }

    @Test
    public void testUpdateOwnerAccessControlEntry() throws Exception {
        boolean expectedUpdateResult = true;
        expectedOwnerAccessControlEntry.setConsumerPermission(Permission.NO);
        assertEquals("Insert owner ACE should return true",
                     expectedUpdateResult,
                     store.updateOwnerAccessControlEntry(expectedOwnerAccessControlEntry));
        assertTrue("Before update owner ACE for UID1 should have default Permission.NO",
                   store.getOwnerAccessControlEntries(UID1).get(0).getConsumerPermission().equals(Permission.NO));
        expectedOwnerAccessControlEntry.setConsumerPermission(Permission.YES);
        assertEquals("Update owner ACE should return true",
                     expectedUpdateResult,
                     store.updateOwnerAccessControlEntry(expectedOwnerAccessControlEntry));
        assertTrue("After update owner ACE for UID1 should have default Permission.YES",
                   store.getOwnerAccessControlEntries(UID1).get(0).getConsumerPermission().equals(Permission.YES));
    }

    @Test
    public void testRemoveOwnerAccessControlEntry() throws Exception {
        store.updateOwnerAccessControlEntry(expectedOwnerAccessControlEntry);
        boolean expectedRemoveResult = true;

        assertEquals("Remove owner ACE for given userId, domain, interface and operation should return true",
                     expectedRemoveResult,
                     store.removeOwnerAccessControlEntry(UID1, DOMAIN1, INTERFACE1, OPERATION1));
        assertTrue("In Owner ACL no owner ACE for given domain, interface should remain",
                   store.getOwnerAccessControlEntries(DOMAIN1, INTERFACE1).isEmpty());
        assertTrue("In Owner ACL no owner ACE for UID1 should remain",
                   store.getOwnerAccessControlEntries(UID1).isEmpty());
    }

    @Test
    public void testGetWildcardOwnerAce() throws Exception {
        OwnerAccessControlEntry expectedOwnerAccessControlEntryWildcard = new OwnerAccessControlEntry(WILDCARD,
                                                                                                      DOMAIN1,
                                                                                                      INTERFACEX,
                                                                                                      TrustLevel.HIGH,
                                                                                                      TrustLevel.HIGH,
                                                                                                      OPERATIONX,
                                                                                                      Permission.YES);
        store.updateOwnerAccessControlEntry(expectedOwnerAccessControlEntryWildcard);

        assertTrue("Exactly one owner ACE for WILDCARD user should be in Owner ACL",
                   store.getOwnerAccessControlEntries(WILDCARD).size() == 1);
        assertTrue("In case no USER2_ID ACE found, WILDCARD user ACE should be returned",
                   store.getOwnerAccessControlEntries(UID2).get(0).getUid().equals(WILDCARD));
        assertTrue("Uid of returned owner ACEs associated to DOMAIN1 and INTERFACEX should be WILDCARD",
                   store.getOwnerAccessControlEntries(DOMAIN1, INTERFACEX).get(0).getUid().equals(WILDCARD));
    }

    @Test
    public void testGetWildcardMasterAce() throws Exception {
        MasterAccessControlEntry expectedMasterAccessControlEntryWildcard = new MasterAccessControlEntry(WILDCARD,
                                                                                                         DOMAIN1,
                                                                                                         INTERFACEX,
                                                                                                         TrustLevel.LOW,
                                                                                                         new TrustLevel[]{
                                                                                                                 TrustLevel.MID,
                                                                                                                 TrustLevel.LOW },
                                                                                                         TrustLevel.LOW,
                                                                                                         new TrustLevel[]{
                                                                                                                 TrustLevel.MID,
                                                                                                                 TrustLevel.LOW },
                                                                                                         OPERATIONX,
                                                                                                         Permission.NO,
                                                                                                         new Permission[]{
                                                                                                                 Permission.ASK,
                                                                                                                 Permission.NO });
        store.updateMasterAccessControlEntry(expectedMasterAccessControlEntryWildcard);

        assertTrue("Exactly one master RCE for WILDCARD user should be in Master RCL",
                   store.getMasterAccessControlEntries(WILDCARD).size() == 1);
        assertTrue("In case no USER2_ID RCE found, WILDCARD user RCE should be returned",
                   store.getMasterAccessControlEntries(UID2).get(0).getUid().equals(WILDCARD));
        assertTrue("Uid of returned master RCEs associated to DOMAIN1 and INTERFACEX should be WILDCARD",
                   store.getMasterAccessControlEntries(DOMAIN1, INTERFACEX).get(0).getUid().equals(WILDCARD));
    }

    // RCE

    @Test
    public void testGetMasterRce() throws Exception {
        store.updateMasterRegistrationControlEntry(expectedMasterRegistrationControlEntry);

        assertEquals("Master RCE associated to UID1 from Master RCL should be the same as expectedMasterRegistrationControlEntry",
                     expectedMasterRegistrationControlEntry,
                     store.getMasterRegistrationControlEntries(UID1).get(0));
        assertEquals("Master RCE associated to DOMAIN1 and INTERFACE1 should be the same as expectedMasterRegistrationControlEntry",
                     expectedMasterRegistrationControlEntry,
                     store.getMasterRegistrationControlEntries(DOMAIN1, INTERFACE1).get(0));
        assertEquals("Master RCE associated to UID1, DOMAIN1 and INTERFACE1 should be the same as expectedMasterRegistrationControlEntry",
                     expectedMasterRegistrationControlEntry,
                     store.getMasterRegistrationControlEntry(UID1, DOMAIN1, INTERFACE1));
    }

    @Test
    public void testGetEditableMasterRcl() throws Exception {
        expectedUserDomainRoleEntry.setDomains(new String[]{ DOMAIN1 });
        expectedUserDomainRoleEntry.setRole(Role.MASTER);
        store.updateDomainRole(expectedUserDomainRoleEntry);
        store.updateMasterRegistrationControlEntry(expectedMasterRegistrationControlEntry);

        assertEquals("Editable master RCE for UID1 should be equal to expectedMasterRegistrationControlEntry",
                     expectedMasterRegistrationControlEntry,
                     store.getEditableMasterRegistrationControlEntries(UID1).get(0));
    }

    @Test
    public void testEditableMasterRegistrationControlEntryNoMatchingDre() throws Exception {
        expectedMasterRegistrationControlEntry.setUid(UID2);
        store.updateMasterRegistrationControlEntry(expectedMasterRegistrationControlEntry);

        assertTrue("There should be no editable master RCE for UID1 in Master RCL",
                   store.getEditableMasterRegistrationControlEntries(UID1).isEmpty());
    }

    @Test
    public void testUpdateMasterRegistrationControlEntry() throws Exception {
        boolean expectedUpdateResult = true;
        expectedMasterRegistrationControlEntry.setDefaultProviderPermission(Permission.NO);
        assertEquals("Insert master RCE should return true",
                     expectedUpdateResult,
                     store.updateMasterRegistrationControlEntry(expectedMasterRegistrationControlEntry));
        assertTrue("Before update master RCE for UID1 should have default Permission.NO",
                   store.getMasterRegistrationControlEntries(UID1)
                        .get(0)
                        .getDefaultProviderPermission()
                        .equals(Permission.NO));
        expectedMasterRegistrationControlEntry.setDefaultProviderPermission(Permission.YES);
        assertEquals("Update master RCE should return true",
                     expectedUpdateResult,
                     store.updateMasterRegistrationControlEntry(expectedMasterRegistrationControlEntry));
        assertTrue("After update master RCE for UID1 should have default Permission.YES",
                   store.getMasterRegistrationControlEntries(UID1)
                        .get(0)
                        .getDefaultProviderPermission()
                        .equals(Permission.YES));

    }

    @Test
    public void testRemoveMasterRegistrationControlEntry() throws Exception {
        store.updateMasterRegistrationControlEntry(expectedMasterRegistrationControlEntry);
        boolean expectedRemoveResult = true;

        assertEquals("Remove master RCE for given userId, domain and interface should return true",
                     expectedRemoveResult,
                     store.removeMasterRegistrationControlEntry(UID1, DOMAIN1, INTERFACE1));
        assertTrue("In Master RCL no master RCE for given domain, interface should remain",
                   store.getMasterRegistrationControlEntries(DOMAIN1, INTERFACE1).isEmpty());
        assertTrue("In Master RCL no master RCE for UID1 should remain",
                   store.getMasterRegistrationControlEntries(UID1).isEmpty());
    }

    @Test
    public void testGetMediatorRce() throws Exception {
        store.updateMasterRegistrationControlEntry(expectedMasterRegistrationControlEntry);
        store.updateMediatorRegistrationControlEntry(expectedMasterRegistrationControlEntry);

        assertEquals("Mediator RCE associated to UID1 from Mediator RCL should be the same as expectedMediatorRegistrationControlEntry",
                     expectedMasterRegistrationControlEntry,
                     store.getMediatorRegistrationControlEntries(UID1).get(0));
        assertEquals("Mediator RCE associated to DOMAIN1 and INTERFACE1 should be the same as expectedMediatorRegistrationControlEntry",
                     expectedMasterRegistrationControlEntry,
                     store.getMediatorRegistrationControlEntries(DOMAIN1, INTERFACE1).get(0));
        assertEquals("Mediator RCE associated to UID1, DOMAIN1 and INTERFACE1 should be the same as expectedMediatorRegistrationControlEntry",
                     expectedMasterRegistrationControlEntry,
                     store.getMediatorRegistrationControlEntry(UID1, DOMAIN1, INTERFACE1));
    }

    @Test
    public void testGetEditableMediatorRcl() throws Exception {
        expectedUserDomainRoleEntry.setDomains(new String[]{ DOMAIN1 });
        expectedUserDomainRoleEntry.setRole(Role.MASTER);
        store.updateDomainRole(expectedUserDomainRoleEntry);
        store.updateMasterRegistrationControlEntry(expectedMasterRegistrationControlEntry);
        store.updateMediatorRegistrationControlEntry(expectedMasterRegistrationControlEntry);

        assertEquals("Editable mediator RCE for UID1 should be equal to expectedMediatorRegistrationControlEntry",
                     expectedMasterRegistrationControlEntry,
                     store.getEditableMediatorRegistrationControlEntries(UID1).get(0));
    }

    @Test
    public void testEditableMediatorRegistrationControlEntryNoMatchingDre() throws Exception {
        expectedMasterRegistrationControlEntry.setUid(UID2);
        store.updateMasterRegistrationControlEntry(expectedMasterRegistrationControlEntry);
        store.updateMediatorRegistrationControlEntry(expectedMasterRegistrationControlEntry);

        assertTrue("There should be no editable mediator RCE for UID1 in Mediator RCL",
                   store.getEditableMediatorRegistrationControlEntries(UID1).isEmpty());
    }

    @Test
    public void testUpdateMediatorRegistrationControlEntry() throws Exception {
        boolean expectedUpdateResult = true;
        expectedMasterRegistrationControlEntry.setDefaultProviderPermission(Permission.NO);
        assertEquals("Insert master RCE should return true",
                     expectedUpdateResult,
                     store.updateMasterRegistrationControlEntry(expectedMasterRegistrationControlEntry));
        assertEquals("Insert mediator RCE should return true",
                     expectedUpdateResult,
                     store.updateMediatorRegistrationControlEntry(expectedMasterRegistrationControlEntry));
        assertTrue("Before update mediator RCE for UID1 should have default Permission.NO",
                   store.getMediatorRegistrationControlEntries(UID1)
                        .get(0)
                        .getDefaultProviderPermission()
                        .equals(Permission.NO));
        expectedMasterRegistrationControlEntry.setDefaultProviderPermission(Permission.YES);
        assertEquals("Update mediator RCE should return false",
                     false,
                     store.updateMediatorRegistrationControlEntry(expectedMasterRegistrationControlEntry));
        expectedMasterRegistrationControlEntry.setPossibleProviderPermissions(new Permission[]{ Permission.ASK,
                Permission.NO, Permission.YES });
        assertEquals("Update master RCE should return true",
                     expectedUpdateResult,
                     store.updateMasterRegistrationControlEntry(expectedMasterRegistrationControlEntry));
        assertEquals("Update mediator RCE should return true",
                     true,
                     store.updateMediatorRegistrationControlEntry(expectedMasterRegistrationControlEntry));
        assertTrue("After update mediator RCE for UID1 should have default Permission.YES",
                   store.getMediatorRegistrationControlEntries(UID1)
                        .get(0)
                        .getDefaultProviderPermission()
                        .equals(Permission.YES));
    }

    @Test
    public void testRemoveMediatorRegistrationControlEntry() throws Exception {
        store.updateMasterRegistrationControlEntry(expectedMasterRegistrationControlEntry);
        store.updateMediatorRegistrationControlEntry(expectedMasterRegistrationControlEntry);
        boolean expectedRemoveResult = true;

        assertEquals("Remove mediator RCE for given userId, domain and interface should return true",
                     expectedRemoveResult,
                     store.removeMediatorRegistrationControlEntry(UID1, DOMAIN1, INTERFACE1));
        assertTrue("In Mediator RCL no mediator RCE for given domain, interface should remain",
                   store.getMediatorRegistrationControlEntries(DOMAIN1, INTERFACE1).isEmpty());
        assertTrue("In Mediator RCL no mediator RCE for UID1 should remain",
                   store.getMediatorRegistrationControlEntries(UID1).isEmpty());
    }

    @Test
    public void testGetOwnerRegistrationControlEntry() throws Exception {
        store.updateOwnerRegistrationControlEntry(expectedOwnerRegistrationControlEntry);

        assertEquals("Owner RCE for UID1 should be equal to expectedOwnerRegistrationControlEntry",
                     expectedOwnerRegistrationControlEntry,
                     store.getOwnerRegistrationControlEntries(UID1).get(0));
        assertEquals("Owner RCE associated to DOMAIN1 and INTERFACE1 should be the same as expectedOwnerRegistrationControlEntry",
                     expectedOwnerRegistrationControlEntry,
                     store.getOwnerRegistrationControlEntries(DOMAIN1, INTERFACE1).get(0));
        assertEquals("Owner RCE associated to UID1, DOMAIN1 and INTERFACE1 should be the same as expectedOwnerRegistrationControlEntry",
                     expectedOwnerRegistrationControlEntry,
                     store.getOwnerRegistrationControlEntry(UID1, DOMAIN1, INTERFACE1));
        OwnerRegistrationControlEntry returnedOwnerRce = store.getOwnerRegistrationControlEntry(UID1,
                                                                                                DOMAIN1,
                                                                                                INTERFACE1);
        assertEquals("Owner RCE associated to UID1, DOMAIN1 and INTERFACE1 should be the same as expectedOwnerRegistrationControlEntry",
                     expectedOwnerRegistrationControlEntry,
                     returnedOwnerRce);
    }

    @Test
    public void testEditableOwnerRegistrationControlEntry() throws Exception {
        expectedUserDomainRoleEntry.setDomains(new String[]{ DOMAIN1 });
        store.updateDomainRole(expectedUserDomainRoleEntry);
        store.updateOwnerRegistrationControlEntry(expectedOwnerRegistrationControlEntry);

        assertEquals("Editable Owner RCE for UID1 should be equal to expectedOwnerRegistrationControlEntry",
                     expectedOwnerRegistrationControlEntry,
                     store.getEditableOwnerRegistrationControlEntries(UID1).get(0));
    }

    @Test
    public void testEditableOwnerRegistrationControlEntryNoMatchingDre() throws Exception {
        store.updateDomainRole(expectedUserDomainRoleEntry);
        expectedOwnerRegistrationControlEntry.setUid(UID2);
        store.updateOwnerRegistrationControlEntry(expectedOwnerRegistrationControlEntry);

        assertTrue("No editable owner RCEs for UID2 should be in Owner RCL",
                   store.getEditableOwnerRegistrationControlEntries(UID2).isEmpty());
    }

    @Test
    public void testUpdateOwnerRegistrationControlEntry() throws Exception {
        boolean expectedUpdateResult = true;
        expectedOwnerRegistrationControlEntry.setProviderPermission(Permission.NO);
        assertEquals("Insert owner RCE should return true",
                     expectedUpdateResult,
                     store.updateOwnerRegistrationControlEntry(expectedOwnerRegistrationControlEntry));
        assertTrue("Before update owner RCE for UID1 should have default Permission.NO",
                   store.getOwnerRegistrationControlEntries(UID1).get(0).getProviderPermission().equals(Permission.NO));
        expectedOwnerRegistrationControlEntry.setProviderPermission(Permission.YES);
        assertEquals("Update owner RCE should return true",
                     expectedUpdateResult,
                     store.updateOwnerRegistrationControlEntry(expectedOwnerRegistrationControlEntry));
        assertTrue("After update owner RCE for UID1 should have default Permission.YES",
                   store.getOwnerRegistrationControlEntries(UID1)
                        .get(0)
                        .getProviderPermission()
                        .equals(Permission.YES));
    }

    @Test
    public void testRemoveOwnerRegistrationControlEntry() throws Exception {
        store.updateOwnerRegistrationControlEntry(expectedOwnerRegistrationControlEntry);
        boolean expectedRemoveResult = true;

        assertEquals("Remove owner RCE for given userId, domain and interface should return true",
                     expectedRemoveResult,
                     store.removeOwnerRegistrationControlEntry(UID1, DOMAIN1, INTERFACE1));
        assertTrue("In Owner RCL no owner RCE for given domain, interface should remain",
                   store.getOwnerRegistrationControlEntries(DOMAIN1, INTERFACE1).isEmpty());
        assertTrue("In Owner RCL no owner RCE for UID1 should remain",
                   store.getOwnerRegistrationControlEntries(UID1).isEmpty());
    }

    @Test
    public void testGetWildcardOwnerRce() throws Exception {
        OwnerRegistrationControlEntry expectedOwnerRegistrationControlEntryWildcard = new OwnerRegistrationControlEntry(WILDCARD,
                                                                                                                        DOMAIN1,
                                                                                                                        INTERFACEX,
                                                                                                                        TrustLevel.HIGH,
                                                                                                                        TrustLevel.HIGH,
                                                                                                                        Permission.YES);
        store.updateOwnerRegistrationControlEntry(expectedOwnerRegistrationControlEntryWildcard);

        assertTrue("Exactly one owner RCE for WILDCARD user should be in Owner RCL",
                   store.getOwnerRegistrationControlEntries(WILDCARD).size() == 1);
        assertTrue("In case no USER2_ID RCE found, WILDCARD user RCE should be returned",
                   store.getOwnerRegistrationControlEntries(UID2).get(0).getUid().equals(WILDCARD));
        assertTrue("Uid of returned owner RCEs associated to DOMAIN1 and INTERFACEX should be WILDCARD",
                   store.getOwnerRegistrationControlEntries(DOMAIN1, INTERFACEX).get(0).getUid().equals(WILDCARD));
    }

    @Test
    public void testGetWildcardMasterRce() throws Exception {
        MasterRegistrationControlEntry expectedMasterRegistrationControlEntryWildcard = new MasterRegistrationControlEntry(WILDCARD,
                                                                                                                           DOMAIN1,
                                                                                                                           INTERFACEX,
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
        store.updateMasterRegistrationControlEntry(expectedMasterRegistrationControlEntryWildcard);

        assertTrue("Exactly one master RCE for WILDCARD user should be in Master RCL",
                   store.getMasterRegistrationControlEntries(WILDCARD).size() == 1);
        assertTrue("In case no USER2_ID RCE found, WILDCARD user RCE should be returned",
                   store.getMasterRegistrationControlEntries(UID2).get(0).getUid().equals(WILDCARD));
        assertTrue("Uid of returned master RCEs associated to DOMAIN1 and INTERFACEX should be WILDCARD",
                   store.getMasterRegistrationControlEntries(DOMAIN1, INTERFACEX).get(0).getUid().equals(WILDCARD));
    }
}
