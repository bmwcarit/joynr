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
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.mockito.Mockito.any;

import java.util.HashMap;
import java.util.Map;
import java.util.function.Consumer;
import java.util.function.Function;
import java.util.function.Supplier;

import io.joynr.accesscontrol.global.jee.persistence.ControlEntryType;
import joynr.infrastructure.DacTypes.ChangeType;
import joynr.infrastructure.DacTypes.MasterAccessControlEntry;
import joynr.infrastructure.DacTypes.MasterRegistrationControlEntry;
import joynr.infrastructure.DacTypes.OwnerAccessControlEntry;
import joynr.infrastructure.DacTypes.OwnerRegistrationControlEntry;
import joynr.infrastructure.DacTypes.Permission;
import joynr.infrastructure.DacTypes.TrustLevel;
import joynr.infrastructure.GlobalDomainAccessControllerSubscriptionPublisher;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.InjectMocks;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;

import java.util.concurrent.LinkedBlockingDeque;

@RunWith(MockitoJUnitRunner.class)
public class GlobalDomainAccessControlListEditorBeanTest {

    @Mock
    private MasterAccessControlEntryManager masterAccessControlEntryManagerMock;

    @Mock
    private MasterRegistrationControlEntryManager masterRegistrationControlEntryManagerMock;

    @Mock
    private OwnerAccessControlEntryManager ownerAccessControlEntryManagerMock;

    @Mock
    private OwnerRegistrationControlEntryManager ownerRegistrationControlEntryManagerMock;

    //Direct Multicast publication via another interface is currently not possible
    //@Mock
    //private GlobalDomainAccessControllerSubscriptionPublisher globalDomainAccessControllerSubscriptionPublisherMock;

    @Mock
    private GlobalDomainAccessControllerQueue globalDomainAccessControllerQueueMock;

    @InjectMocks
    private GlobalDomainAccessControlListEditorBean globalDomainAccessControlListEditorSubject = new GlobalDomainAccessControlListEditorBean();

    private final static String USER_ID = "user.name";
    private final static String USER_PARTITION = "username";
    private final static String DOMAIN = "domain1";
    private final static String[] DOMAINS = { DOMAIN };
    private final static String INTERFACE_NAME = "interfaceName";
    private final static String OPERATION = "operation";

    @Before
    public void setUp() {
        when(globalDomainAccessControllerQueueMock.add(any(GlobalDomainAccessControllerQueueJob.class))).thenReturn(true);
    }

    @Test
    public void testCreateMasterAccessControlEntry() {
        MasterAccessControlEntry mace = new MasterAccessControlEntry(USER_ID, DOMAIN, INTERFACE_NAME, TrustLevel.LOW,
            new TrustLevel[0], TrustLevel.HIGH, new TrustLevel[0], OPERATION,
            Permission.ASK, new Permission[0]);
        CreateOrUpdateResult<MasterAccessControlEntry> createResult = new CreateOrUpdateResult<>(mace, ChangeType.ADD);
        when(masterAccessControlEntryManagerMock.createOrUpdate(mace, ControlEntryType.MASTER)).thenReturn(
            createResult);

        Boolean result = globalDomainAccessControlListEditorSubject.updateMasterAccessControlEntry(mace);

        assertEquals(Boolean.TRUE, result);

        verify(masterAccessControlEntryManagerMock).createOrUpdate(eq(mace), eq(ControlEntryType.MASTER));
        //Direct Multicast publication via another interface is currently not possible
        //verify(globalDomainAccessControllerSubscriptionPublisherMock).fireMasterAccessControlEntryChanged(
        //    eq(ChangeType.ADD), eq(mace), eq(USER_PARTITION), eq(DOMAIN), eq(INTERFACE_NAME), eq(OPERATION));
    }

    @Test
    public void testUpdateMasterAccessControlEntry() {
        MasterAccessControlEntry mace = new MasterAccessControlEntry(USER_ID, DOMAIN, INTERFACE_NAME, TrustLevel.LOW,
            new TrustLevel[0], TrustLevel.HIGH, new TrustLevel[0], OPERATION, Permission.ASK, new Permission[0]);
        CreateOrUpdateResult<MasterAccessControlEntry> updateResult = new CreateOrUpdateResult<>(mace,
            ChangeType.UPDATE);
        when(masterAccessControlEntryManagerMock.createOrUpdate(mace, ControlEntryType.MASTER)).thenReturn(
            updateResult);

        Boolean result = globalDomainAccessControlListEditorSubject.updateMasterAccessControlEntry(mace);

        assertEquals(Boolean.TRUE, result);

        verify(masterAccessControlEntryManagerMock).createOrUpdate(mace, ControlEntryType.MASTER);
        //Direct Multicast publication via another interface is currently not possible
        //verify(globalDomainAccessControllerSubscriptionPublisherMock).fireMasterAccessControlEntryChanged(
        //    ChangeType.UPDATE, mace, USER_PARTITION, DOMAIN, INTERFACE_NAME, OPERATION);
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
        Boolean result = globalDomainAccessControlListEditorSubject.removeMasterAccessControlEntry(USER_ID, DOMAIN, INTERFACE_NAME, OPERATION);

        assertEquals(Boolean.TRUE, result);

        verify(masterAccessControlEntryManagerMock).removeByUserIdDomainInterfaceNameAndOperation(eq(USER_ID),
                                                                                                  eq(DOMAIN),
                                                                                                  eq(INTERFACE_NAME),
                                                                                                  eq(OPERATION),
                                                                                                  eq(ControlEntryType.MASTER));
        //Direct Multicast publication via another interface is currently not possible
        //verify(globalDomainAccessControllerSubscriptionPublisherMock).fireMasterAccessControlEntryChanged(eq(ChangeType.REMOVE),
        //                                                                                                  eq(mace),
        //                                                                                                  eq(USER_PARTITION),
        //                                                                                                  eq(DOMAIN),
        //                                                                                                  eq(INTERFACE_NAME),
        //                                                                                                  eq(OPERATION));
    }

    @Test
    public void testCreateMediatorAccessControlEntry() {
        MasterAccessControlEntry mace = new MasterAccessControlEntry(USER_ID, DOMAIN, INTERFACE_NAME, TrustLevel.LOW,
            new TrustLevel[0], TrustLevel.HIGH, new TrustLevel[0], OPERATION,
            Permission.ASK, new Permission[0]);
        CreateOrUpdateResult<MasterAccessControlEntry> createResult = new CreateOrUpdateResult<>(mace, ChangeType.ADD);
        when(masterAccessControlEntryManagerMock.createOrUpdate(mace, ControlEntryType.MEDIATOR)).thenReturn(
            createResult);

        Boolean result = globalDomainAccessControlListEditorSubject.updateMediatorAccessControlEntry(mace);

        assertEquals(Boolean.TRUE, result);

        verify(masterAccessControlEntryManagerMock).createOrUpdate(eq(mace), eq(ControlEntryType.MEDIATOR));
        //Direct Multicast publication via another interface is currently not possible
        //verify(globalDomainAccessControllerSubscriptionPublisherMock).fireMediatorAccessControlEntryChanged(
        //    eq(ChangeType.ADD), eq(mace), eq(USER_PARTITION), eq(DOMAIN), eq(INTERFACE_NAME), eq(OPERATION));
    }

    @Test
    public void testUpdateMediatorAccessControlEntry() {
        MasterAccessControlEntry mace = new MasterAccessControlEntry(USER_ID, DOMAIN, INTERFACE_NAME, TrustLevel.LOW,
            new TrustLevel[0], TrustLevel.HIGH, new TrustLevel[0], OPERATION, Permission.ASK, new Permission[0]);
        CreateOrUpdateResult<MasterAccessControlEntry> updateResult = new CreateOrUpdateResult<>(mace,
            ChangeType.UPDATE);
        when(masterAccessControlEntryManagerMock.createOrUpdate(mace, ControlEntryType.MEDIATOR)).thenReturn(
            updateResult);

        Boolean result = globalDomainAccessControlListEditorSubject.updateMediatorAccessControlEntry(mace);

        assertEquals(Boolean.TRUE, result);

        verify(masterAccessControlEntryManagerMock).createOrUpdate(mace, ControlEntryType.MEDIATOR);
        //Direct Multicast publication via another interface is currently not possible
        //verify(globalDomainAccessControllerSubscriptionPublisherMock).fireMediatorAccessControlEntryChanged(
        //    ChangeType.UPDATE, mace, USER_PARTITION, DOMAIN, INTERFACE_NAME, OPERATION);
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
        Boolean result = globalDomainAccessControlListEditorSubject.removeMediatorAccessControlEntry(USER_ID, DOMAIN, INTERFACE_NAME, OPERATION);

        assertEquals(Boolean.TRUE, result);

        verify(masterAccessControlEntryManagerMock).removeByUserIdDomainInterfaceNameAndOperation(eq(USER_ID),
                                                                                                  eq(DOMAIN),
                                                                                                  eq(INTERFACE_NAME),
                                                                                                  eq(OPERATION),
                                                                                                  eq(ControlEntryType.MEDIATOR));
        //Direct Multicast publication via another interface is currently not possible
        //verify(globalDomainAccessControllerSubscriptionPublisherMock).fireMediatorAccessControlEntryChanged(eq(ChangeType.REMOVE),
        //                                                                                                    eq(mace),
        //                                                                                                    eq(USER_PARTITION),
        //                                                                                                    eq(DOMAIN),
        //                                                                                                    eq(INTERFACE_NAME),
        //                                                                                                    eq(OPERATION));
    }

    @Test
    public void testFindEditableOwnerAccessControlEntry() {
        when(ownerAccessControlEntryManagerMock.findByUserIdThatAreEditable(USER_ID)).thenReturn(new OwnerAccessControlEntry[0]);
        OwnerAccessControlEntry[] result = globalDomainAccessControlListEditorSubject.getEditableOwnerAccessControlEntries(USER_ID);
        assertNotNull(result);
        assertEquals(0, result.length);
        verify(ownerAccessControlEntryManagerMock).findByUserIdThatAreEditable(eq(USER_ID));
    }

    @Test
    public void testCreateOwnerAccessControlEntry() {
        OwnerAccessControlEntry oace = new OwnerAccessControlEntry(USER_ID, DOMAIN, INTERFACE_NAME, TrustLevel.HIGH, TrustLevel.LOW, OPERATION, Permission.YES);
        CreateOrUpdateResult<OwnerAccessControlEntry> createResult = new CreateOrUpdateResult<>(oace, ChangeType.ADD);
        when(ownerAccessControlEntryManagerMock.createOrUpdate(oace)).thenReturn(createResult);
        Boolean result = globalDomainAccessControlListEditorSubject.updateOwnerAccessControlEntry(oace);
        assertEquals(Boolean.TRUE, result);
        verify(ownerAccessControlEntryManagerMock).createOrUpdate(eq(oace));
        //Direct Multicast publication via another interface is currently not possible
        //verify(globalDomainAccessControllerSubscriptionPublisherMock).fireOwnerAccessControlEntryChanged(eq(ChangeType.ADD), eq(oace), eq(USER_PARTITION), eq(DOMAIN), eq(INTERFACE_NAME), eq(OPERATION));
    }

    @Test
    public void testUpdateOwnerAccessControlEntry() {
        OwnerAccessControlEntry oace = new OwnerAccessControlEntry(USER_ID, DOMAIN, INTERFACE_NAME, TrustLevel.HIGH, TrustLevel.LOW, OPERATION, Permission.YES);
        CreateOrUpdateResult<OwnerAccessControlEntry> createResult = new CreateOrUpdateResult<>(oace, ChangeType.UPDATE);
        when(ownerAccessControlEntryManagerMock.createOrUpdate(oace)).thenReturn(createResult);
        Boolean result = globalDomainAccessControlListEditorSubject.updateOwnerAccessControlEntry(oace);
        assertEquals(Boolean.TRUE, result);
        verify(ownerAccessControlEntryManagerMock).createOrUpdate(eq(oace));
        //Direct Multicast publication via another interface is currently not possible
        //verify(globalDomainAccessControllerSubscriptionPublisherMock).fireOwnerAccessControlEntryChanged(eq(ChangeType.UPDATE), eq(oace), eq(USER_PARTITION), eq(DOMAIN), eq(INTERFACE_NAME), eq(OPERATION));
    }

    @Test
    public void testFindEditableMasterAndMediatorRegistrationControlEntryByUserId() {
        Map<ControlEntryType, Function<String, MasterRegistrationControlEntry[]>> getters = new HashMap<>();
        getters.put(ControlEntryType.MASTER, (String userId) -> { return globalDomainAccessControlListEditorSubject.getEditableMasterRegistrationControlEntries(userId);});
        getters.put(ControlEntryType.MEDIATOR, (String userId) -> { return globalDomainAccessControlListEditorSubject.getEditableMediatorRegistrationControlEntries(userId);});
        for (ControlEntryType type : new ControlEntryType[] { ControlEntryType.MASTER, ControlEntryType.MEDIATOR}) {
            when(masterRegistrationControlEntryManagerMock.findByUserIdAndThatAreEditable(USER_ID,
                type)).thenReturn(new MasterRegistrationControlEntry[0]);
            MasterRegistrationControlEntry[] result = getters.get(type).apply(USER_ID);
            assertNotNull(result);
            assertEquals(0, result.length);
            verify(masterRegistrationControlEntryManagerMock).findByUserIdAndThatAreEditable(eq(USER_ID),
                eq(type));
        }
    }

    @Test
    public void testCreateAndUpdateMasterAndMediatorRegistrationControlEntry() {
        for (ChangeType changeType : new ChangeType[] { ChangeType.ADD, ChangeType.UPDATE}) {
            Map<ControlEntryType, Function<MasterRegistrationControlEntry, Boolean>> globalDomainAccessControllerSubjectCalls = new HashMap<>();
            globalDomainAccessControllerSubjectCalls.put(ControlEntryType.MASTER, (updatedEntry) -> {
                return globalDomainAccessControlListEditorSubject.updateMasterRegistrationControlEntry(updatedEntry);
            });
            globalDomainAccessControllerSubjectCalls.put(ControlEntryType.MEDIATOR, (updatedEntry) -> {
                return globalDomainAccessControlListEditorSubject.updateMediatorRegistrationControlEntry(updatedEntry);
            });

            Map<ControlEntryType, Consumer<MasterRegistrationControlEntry>> multicastVerifiers = new HashMap<>();
            multicastVerifiers.put(ControlEntryType.MASTER, (mrce) -> {
                //Direct Multicast publication via another interface is currently not possible
                //verify(globalDomainAccessControllerSubscriptionPublisherMock).fireMasterRegistrationControlEntryChanged(
                //    eq(changeType), eq(mrce), eq(USER_PARTITION), eq(DOMAIN), eq(INTERFACE_NAME));
            });
            multicastVerifiers.put(ControlEntryType.MEDIATOR, (mrce) -> {
                //Direct Multicast publication via another interface is currently not possible
                //verify(
                //    globalDomainAccessControllerSubscriptionPublisherMock).fireMediatorRegistrationControlEntryChanged(
                //    eq(changeType), eq(mrce), eq(USER_PARTITION), eq(DOMAIN), eq(INTERFACE_NAME));
            });

            for (ControlEntryType type : new ControlEntryType[]{ControlEntryType.MASTER, ControlEntryType.MEDIATOR}) {
                reset(masterRegistrationControlEntryManagerMock);
                //reset(globalDomainAccessControllerSubscriptionPublisherMock);
                MasterRegistrationControlEntry mrce = new MasterRegistrationControlEntry(USER_ID, DOMAIN,
                    INTERFACE_NAME,
                    TrustLevel.LOW, new TrustLevel[0], TrustLevel.HIGH, new TrustLevel[0], Permission.ASK,
                    new Permission[0]);
                when(masterRegistrationControlEntryManagerMock.createOrUpdate(mrce, type)).thenReturn(
                    new CreateOrUpdateResult<MasterRegistrationControlEntry>(mrce, changeType));
                assertTrue(globalDomainAccessControllerSubjectCalls.get(type).apply(mrce));
                verify(masterRegistrationControlEntryManagerMock).createOrUpdate(mrce, type);
                multicastVerifiers.get(type).accept(mrce);
            }
        }
    }

    @Test
    public void testRemoveMasterAndMediatorRegistrationControlEntry() {
        Map<ControlEntryType, Supplier<Boolean>> globalDomainAccessControllerSubjectCalls = new HashMap<>();
        globalDomainAccessControllerSubjectCalls.put(ControlEntryType.MASTER, () -> { return globalDomainAccessControlListEditorSubject.removeMasterRegistrationControlEntry(USER_ID, DOMAIN, INTERFACE_NAME);});
        globalDomainAccessControllerSubjectCalls.put(ControlEntryType.MEDIATOR, () -> { return globalDomainAccessControlListEditorSubject.removeMediatorRegistrationControlEntry(USER_ID, DOMAIN, INTERFACE_NAME);});

        Map<ControlEntryType, Consumer<MasterRegistrationControlEntry>> multicastVerifiers = new HashMap<>();
        multicastVerifiers.put(ControlEntryType.MASTER, (mrce) -> {
            //Direct Multicast publication via another interface is currently not possible
            //verify(globalDomainAccessControllerSubscriptionPublisherMock).fireMasterRegistrationControlEntryChanged(eq(ChangeType.REMOVE), eq(mrce), eq(USER_PARTITION), eq(DOMAIN), eq(INTERFACE_NAME));
        });
        multicastVerifiers.put(ControlEntryType.MEDIATOR, (mrce) -> {
            //Direct Multicast publication via another interface is currently not possible
            //verify(globalDomainAccessControllerSubscriptionPublisherMock).fireMediatorRegistrationControlEntryChanged(eq(ChangeType.REMOVE), eq(mrce), eq(USER_PARTITION), eq(DOMAIN), eq(INTERFACE_NAME));
        });

        for (ControlEntryType type : new ControlEntryType[] { ControlEntryType.MASTER, ControlEntryType.MEDIATOR}) {
            MasterRegistrationControlEntry mrce = new MasterRegistrationControlEntry(USER_ID, DOMAIN, INTERFACE_NAME, TrustLevel.LOW, new TrustLevel[0], TrustLevel.HIGH, new TrustLevel[0], Permission.ASK, new Permission[0]);
            when(masterRegistrationControlEntryManagerMock.removeByUserIdDomainInterfaceNameAndType(USER_ID, DOMAIN, INTERFACE_NAME, type)).thenReturn(mrce);
            assertTrue(globalDomainAccessControllerSubjectCalls.get(type).get());
            verify(masterRegistrationControlEntryManagerMock).removeByUserIdDomainInterfaceNameAndType(eq(USER_ID), eq(DOMAIN), eq(INTERFACE_NAME), eq(type));
            multicastVerifiers.get(type).accept(mrce);
        }
    }

    @Test
    public void testFindEditableOwnerRegistrationControlEntriesByUserId() {
        when(ownerRegistrationControlEntryManagerMock.findByUserIdAndThatIsEditable(USER_ID)).thenReturn(new OwnerRegistrationControlEntry[0]);
        OwnerRegistrationControlEntry[] result = globalDomainAccessControlListEditorSubject.getEditableOwnerRegistrationControlEntries(USER_ID);
        assertNotNull(result);
        assertEquals(0, result.length);
        verify(ownerRegistrationControlEntryManagerMock).findByUserIdAndThatIsEditable(eq(USER_ID));
    }

    @Test
    public void testCreateAndUpdateOwnerRegistrationControlEntry() {
        for (ChangeType changeType : new ChangeType[]{ ChangeType.ADD, ChangeType.UPDATE }) {
            reset(ownerRegistrationControlEntryManagerMock);
            OwnerRegistrationControlEntry orce = new OwnerRegistrationControlEntry(USER_ID,
                                                                                   DOMAIN,
                                                                                   INTERFACE_NAME,
                                                                                   TrustLevel.HIGH,
                                                                                   TrustLevel.LOW,
                                                                                   Permission.ASK);
            when(ownerRegistrationControlEntryManagerMock.createOrUpdate(orce)).thenReturn(new CreateOrUpdateResult<OwnerRegistrationControlEntry>(orce,
                                                                                                                                                   changeType));
            assertTrue(globalDomainAccessControlListEditorSubject.updateOwnerRegistrationControlEntry(orce));
            verify(ownerRegistrationControlEntryManagerMock).createOrUpdate(eq(orce));
            //Direct Multicast publication via another interface is currently not possible
            //verify(globalDomainAccessControllerSubscriptionPublisherMock).fireOwnerRegistrationControlEntryChanged(eq(changeType),
            //                                                                                                       eq(orce),
            //                                                                                                       eq(USER_PARTITION),
            //                                                                                                       eq(DOMAIN),
            //                                                                                                       eq(INTERFACE_NAME));
        }
    }

    @Test
    public void testRemoveOwnerRegistrationControlEntry() {
        OwnerRegistrationControlEntry orce = new OwnerRegistrationControlEntry(USER_ID,
                                                                               DOMAIN,
                                                                               INTERFACE_NAME,
                                                                               TrustLevel.HIGH,
                                                                               TrustLevel.LOW,
                                                                               Permission.ASK);
        when(ownerRegistrationControlEntryManagerMock.removeByUserIdDomainAndInterfaceName(USER_ID,
                                                                                           DOMAIN,
                                                                                           INTERFACE_NAME)).thenReturn(orce);
        assertTrue(globalDomainAccessControlListEditorSubject.removeOwnerRegistrationControlEntry(USER_ID, DOMAIN, INTERFACE_NAME));
        verify(ownerRegistrationControlEntryManagerMock).removeByUserIdDomainAndInterfaceName(eq(USER_ID),
                                                                                              eq(DOMAIN),
                                                                                              eq(INTERFACE_NAME));
        //Direct Multicast publication via another interface is currently not possible
        //verify(globalDomainAccessControllerSubscriptionPublisherMock).fireOwnerRegistrationControlEntryChanged(eq(ChangeType.REMOVE),
        //                                                                                                       eq(orce),
        //                                                                                                       eq(USER_PARTITION),
        //                                                                                                       eq(DOMAIN),
        //                                                                                                       eq(INTERFACE_NAME));
    }
}
