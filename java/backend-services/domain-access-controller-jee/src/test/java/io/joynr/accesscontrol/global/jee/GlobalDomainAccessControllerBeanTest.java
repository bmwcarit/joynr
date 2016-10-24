package io.joynr.accesscontrol.global.jee;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import io.joynr.accesscontrol.global.jee.persistence.ControlEntryType;
import joynr.infrastructure.DacTypes.ChangeType;
import joynr.infrastructure.DacTypes.DomainRoleEntry;
import joynr.infrastructure.DacTypes.MasterAccessControlEntry;
import joynr.infrastructure.DacTypes.Permission;
import joynr.infrastructure.DacTypes.Role;
import joynr.infrastructure.DacTypes.TrustLevel;
import joynr.infrastructure.GlobalDomainAccessControllerSubscriptionPublisher;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.InjectMocks;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
public class GlobalDomainAccessControllerBeanTest {

    @Mock
    private DomainRoleEntryManager domainRoleEntryManagerMock;

    @Mock
    private MasterAccessControlEntryManager masterAccessControlEntryManagerMock;

    @Mock
    private MasterRegistrationControlEntryManager masterRegistrationControlEntryManagerMock;

    @Mock
    private OwnerAccessControlEntryManager ownerAccessControlEntryManagerMock;

    @Mock
    private OwnerRegistrationControlEntryManager ownerRegistrationControlEntryManagerMock;

    @Mock
    private GlobalDomainAccessControllerSubscriptionPublisher globalDomainAccessControllerSubscriptionPublisherMock;

    @InjectMocks
    private GlobalDomainAccessControllerBean subject = new GlobalDomainAccessControllerBean();

    private final static String USER_ID = "user.name";
    private final static String USER_PARTITION = "username";
    private final static String DOMAIN = "domain1";
    private final static String[] DOMAINS = { DOMAIN };
    private final static String INTERFACE_NAME = "interfaceName";
    private final static Role ROLE = Role.MASTER;
    private final static String OPERATION = "operation";

    @Test
    public void testGetDomainRoleEntries() {
        DomainRoleEntry[] result = new DomainRoleEntry[0];
        Mockito.when(domainRoleEntryManagerMock.findByUserId(USER_ID)).thenReturn(result);
        DomainRoleEntry[] domainRoles = subject.getDomainRoles(USER_ID);
        verify(domainRoleEntryManagerMock).findByUserId(USER_ID);
        assertArrayEquals(result, domainRoles);
    }

    @Test
    public void testCreate() {
        DomainRoleEntry domainRoleEntry = new DomainRoleEntry(USER_ID, DOMAINS, ROLE);
        CreateOrUpdateResult<DomainRoleEntry> createOrUpdateResult = new CreateOrUpdateResult<>(domainRoleEntry,
            ChangeType.ADD);

        when(domainRoleEntryManagerMock.createOrUpdate(domainRoleEntry)).thenReturn(createOrUpdateResult);

        assertTrue(subject.updateDomainRole(domainRoleEntry));

        verify(domainRoleEntryManagerMock).createOrUpdate(eq(domainRoleEntry));
        verify(globalDomainAccessControllerSubscriptionPublisherMock).fireDomainRoleEntryChanged(eq(ChangeType.ADD),
            eq(domainRoleEntry), eq(
                USER_PARTITION));
    }

    @Test
    public void testUpdate() {
        DomainRoleEntry domainRoleEntry = new DomainRoleEntry(USER_ID, DOMAINS, ROLE);
        CreateOrUpdateResult<DomainRoleEntry> createOrUpdateResult = new CreateOrUpdateResult<>(domainRoleEntry,
            ChangeType.UPDATE);

        when(domainRoleEntryManagerMock.createOrUpdate(domainRoleEntry)).thenReturn(createOrUpdateResult);

        assertTrue(subject.updateDomainRole(domainRoleEntry));

        verify(domainRoleEntryManagerMock).createOrUpdate(eq(domainRoleEntry));
        verify(globalDomainAccessControllerSubscriptionPublisherMock).fireDomainRoleEntryChanged(eq(ChangeType.UPDATE),
            eq(domainRoleEntry),
            eq(USER_PARTITION));
    }

    @Test
    public void testRemove() {
        DomainRoleEntry domainRoleEntry = new DomainRoleEntry(USER_ID, DOMAINS, ROLE);

        when(domainRoleEntryManagerMock.removeByUserIdAndRole(USER_ID, ROLE)).thenReturn(domainRoleEntry);

        assertTrue(subject.removeDomainRole(USER_ID, ROLE));

        verify(domainRoleEntryManagerMock).removeByUserIdAndRole(eq(USER_ID), eq(ROLE));
        verify(globalDomainAccessControllerSubscriptionPublisherMock).fireDomainRoleEntryChanged(eq(ChangeType.REMOVE),
                                                                                                 eq(domainRoleEntry),
                                                                                                 eq(USER_PARTITION));
    }

    @Test
    public void testFindMasterAccessControlEntriesByUserId() {
        when(masterAccessControlEntryManagerMock.findByUserId(USER_ID, ControlEntryType.MASTER)).thenReturn(new MasterAccessControlEntry[0]);
        MasterAccessControlEntry[] result = subject.getMasterAccessControlEntries(USER_ID);
        assertNotNull(result);
        assertEquals(0, result.length);
        verify(masterAccessControlEntryManagerMock).findByUserId(eq(USER_ID), eq(ControlEntryType.MASTER));
    }

    @Test
    public void testFindMasterAccessControlEntriesByDomainAndInterfaceName() {
        when(masterAccessControlEntryManagerMock.findByDomainAndInterfaceName(DOMAIN,
                                                                              INTERFACE_NAME,
                                                                              ControlEntryType.MASTER)).thenReturn(new MasterAccessControlEntry[0]);
        MasterAccessControlEntry[] result = subject.getMasterAccessControlEntries(DOMAIN, INTERFACE_NAME);
        assertNotNull(result);
        assertEquals(0, result.length);
        verify(masterAccessControlEntryManagerMock).findByDomainAndInterfaceName(DOMAIN,
                                                                                 INTERFACE_NAME,
                                                                                 ControlEntryType.MASTER);
    }

    @Test
    public void testCreateMasterAccessControlEntry() {
        MasterAccessControlEntry mace = new MasterAccessControlEntry(USER_ID, DOMAIN, INTERFACE_NAME, TrustLevel.LOW,
            new TrustLevel[0], TrustLevel.HIGH, new TrustLevel[0], OPERATION,
            Permission.ASK, new Permission[0]);
        CreateOrUpdateResult<MasterAccessControlEntry> createResult = new CreateOrUpdateResult<>(mace, ChangeType.ADD);
        when(masterAccessControlEntryManagerMock.createOrUpdate(mace, ControlEntryType.MASTER)).thenReturn(
            createResult);

        Boolean result = subject.updateMasterAccessControlEntry(mace);

        assertEquals(Boolean.TRUE, result);

        verify(masterAccessControlEntryManagerMock).createOrUpdate(eq(mace), eq(ControlEntryType.MASTER));
        verify(globalDomainAccessControllerSubscriptionPublisherMock).fireMasterAccessControlEntryChanged(
            eq(ChangeType.ADD), eq(mace), eq(USER_PARTITION), eq(DOMAIN), eq(INTERFACE_NAME), eq(OPERATION));
    }

    @Test
    public void testUpdateMasterAccessControlEntry() {
        MasterAccessControlEntry mace = new MasterAccessControlEntry(USER_ID, DOMAIN, INTERFACE_NAME, TrustLevel.LOW,
            new TrustLevel[0], TrustLevel.HIGH, new TrustLevel[0], OPERATION, Permission.ASK, new Permission[0]);
        CreateOrUpdateResult<MasterAccessControlEntry> updateResult = new CreateOrUpdateResult<>(mace,
            ChangeType.UPDATE);
        when(masterAccessControlEntryManagerMock.createOrUpdate(mace, ControlEntryType.MASTER)).thenReturn(
            updateResult);

        Boolean result = subject.updateMasterAccessControlEntry(mace);

        assertEquals(Boolean.TRUE, result);

        verify(masterAccessControlEntryManagerMock).createOrUpdate(mace, ControlEntryType.MASTER);
        verify(globalDomainAccessControllerSubscriptionPublisherMock).fireMasterAccessControlEntryChanged(
            ChangeType.UPDATE, mace, USER_PARTITION, DOMAIN, INTERFACE_NAME, OPERATION);
    }

    @Test
    public void testRemoveMasterAccessControlEntry() {
        MasterAccessControlEntry mace = new MasterAccessControlEntry(USER_ID,
                                                                     DOMAIN,
                                                                     INTERFACE_NAME,
                                                                     TrustLevel.LOW,
                                                                     new TrustLevel[0],
                                                                     TrustLevel.HIGH,
                                                                     new TrustLevel[0],
                                                                     OPERATION,
                                                                     Permission.ASK,
                                                                     new Permission[0]);
        when(masterAccessControlEntryManagerMock.removeByUserIdDomainInterfaceNameAndOperation(USER_ID,
                                                                                               DOMAIN,
                                                                                               INTERFACE_NAME,
                                                                                               OPERATION,
                                                                                               ControlEntryType.MASTER)).thenReturn(mace);
        Boolean result = subject.removeMasterAccessControlEntry(USER_ID, DOMAIN, INTERFACE_NAME, OPERATION);

        assertEquals(Boolean.TRUE, result);

        verify(masterAccessControlEntryManagerMock).removeByUserIdDomainInterfaceNameAndOperation(eq(USER_ID),
                                                                                                  eq(DOMAIN),
                                                                                                  eq(INTERFACE_NAME),
                                                                                                  eq(OPERATION),
                                                                                                  eq(ControlEntryType.MASTER));
        verify(globalDomainAccessControllerSubscriptionPublisherMock).fireMasterAccessControlEntryChanged(eq(ChangeType.REMOVE),
                                                                                                          eq(mace),
                                                                                                          eq(USER_PARTITION),
                                                                                                          eq(DOMAIN),
                                                                                                          eq(INTERFACE_NAME),
                                                                                                          eq(OPERATION));
    }

    @Test
    public void testFindMediatorAccessControlEntriesByUserId() {
        when(masterAccessControlEntryManagerMock.findByUserId(USER_ID, ControlEntryType.MEDIATOR)).thenReturn(new MasterAccessControlEntry[0]);
        MasterAccessControlEntry[] result = subject.getMediatorAccessControlEntries(USER_ID);
        assertNotNull(result);
        assertEquals(0, result.length);
        verify(masterAccessControlEntryManagerMock).findByUserId(eq(USER_ID), eq(ControlEntryType.MEDIATOR));
    }

    @Test
    public void testFindMediatorAccessControlEntriesByDomainAndInterfaceName() {
        when(masterAccessControlEntryManagerMock.findByDomainAndInterfaceName(DOMAIN,
                                                                              INTERFACE_NAME,
                                                                              ControlEntryType.MEDIATOR)).thenReturn(new MasterAccessControlEntry[0]);
        MasterAccessControlEntry[] result = subject.getMediatorAccessControlEntries(DOMAIN, INTERFACE_NAME);
        assertNotNull(result);
        assertEquals(0, result.length);
        verify(masterAccessControlEntryManagerMock).findByDomainAndInterfaceName(DOMAIN,
                                                                                 INTERFACE_NAME,
                                                                                 ControlEntryType.MEDIATOR);
    }

    @Test
    public void testCreateMediatorAccessControlEntry() {
        MasterAccessControlEntry mace = new MasterAccessControlEntry(USER_ID, DOMAIN, INTERFACE_NAME, TrustLevel.LOW,
            new TrustLevel[0], TrustLevel.HIGH, new TrustLevel[0], OPERATION,
            Permission.ASK, new Permission[0]);
        CreateOrUpdateResult<MasterAccessControlEntry> createResult = new CreateOrUpdateResult<>(mace, ChangeType.ADD);
        when(masterAccessControlEntryManagerMock.createOrUpdate(mace, ControlEntryType.MEDIATOR)).thenReturn(
            createResult);

        Boolean result = subject.updateMediatorAccessControlEntry(mace);

        assertEquals(Boolean.TRUE, result);

        verify(masterAccessControlEntryManagerMock).createOrUpdate(eq(mace), eq(ControlEntryType.MEDIATOR));
        verify(globalDomainAccessControllerSubscriptionPublisherMock).fireMediatorAccessControlEntryChanged(
            eq(ChangeType.ADD), eq(mace), eq(USER_PARTITION), eq(DOMAIN), eq(INTERFACE_NAME), eq(OPERATION));
    }

    @Test
    public void testUpdateMediatorAccessControlEntry() {
        MasterAccessControlEntry mace = new MasterAccessControlEntry(USER_ID, DOMAIN, INTERFACE_NAME, TrustLevel.LOW,
            new TrustLevel[0], TrustLevel.HIGH, new TrustLevel[0], OPERATION, Permission.ASK, new Permission[0]);
        CreateOrUpdateResult<MasterAccessControlEntry> updateResult = new CreateOrUpdateResult<>(mace,
            ChangeType.UPDATE);
        when(masterAccessControlEntryManagerMock.createOrUpdate(mace, ControlEntryType.MEDIATOR)).thenReturn(
            updateResult);

        Boolean result = subject.updateMediatorAccessControlEntry(mace);

        assertEquals(Boolean.TRUE, result);

        verify(masterAccessControlEntryManagerMock).createOrUpdate(mace, ControlEntryType.MEDIATOR);
        verify(globalDomainAccessControllerSubscriptionPublisherMock).fireMediatorAccessControlEntryChanged(
            ChangeType.UPDATE, mace, USER_PARTITION, DOMAIN, INTERFACE_NAME, OPERATION);
    }

    @Test
    public void testRemoveMediatorAccessControlEntry() {
        MasterAccessControlEntry mace = new MasterAccessControlEntry(USER_ID,
                                                                     DOMAIN,
                                                                     INTERFACE_NAME,
                                                                     TrustLevel.LOW,
                                                                     new TrustLevel[0],
                                                                     TrustLevel.HIGH,
                                                                     new TrustLevel[0],
                                                                     OPERATION,
                                                                     Permission.ASK,
                                                                     new Permission[0]);
        when(masterAccessControlEntryManagerMock.removeByUserIdDomainInterfaceNameAndOperation(USER_ID,
                                                                                               DOMAIN,
                                                                                               INTERFACE_NAME,
                                                                                               OPERATION,
                                                                                               ControlEntryType.MEDIATOR)).thenReturn(mace);
        Boolean result = subject.removeMediatorAccessControlEntry(USER_ID, DOMAIN, INTERFACE_NAME, OPERATION);

        assertEquals(Boolean.TRUE, result);

        verify(masterAccessControlEntryManagerMock).removeByUserIdDomainInterfaceNameAndOperation(eq(USER_ID),
                                                                                                  eq(DOMAIN),
                                                                                                  eq(INTERFACE_NAME),
                                                                                                  eq(OPERATION),
                                                                                                  eq(ControlEntryType.MEDIATOR));
        verify(globalDomainAccessControllerSubscriptionPublisherMock).fireMediatorAccessControlEntryChanged(eq(ChangeType.REMOVE),
                                                                                                            eq(mace),
                                                                                                            eq(USER_PARTITION),
                                                                                                            eq(DOMAIN),
                                                                                                            eq(INTERFACE_NAME),
                                                                                                            eq(OPERATION));
    }
}
