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
package io.joynr.accesscontrol.global.jee;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.util.HashMap;
import java.util.Map;
import java.util.function.Function;

import io.joynr.accesscontrol.global.jee.persistence.ControlEntryType;
import joynr.infrastructure.DacTypes.MasterAccessControlEntry;
import joynr.infrastructure.DacTypes.MasterRegistrationControlEntry;
import joynr.infrastructure.DacTypes.OwnerAccessControlEntry;
import joynr.infrastructure.DacTypes.OwnerRegistrationControlEntry;
import joynr.infrastructure.GlobalDomainAccessControllerSubscriptionPublisher;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.InjectMocks;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
public class GlobalDomainAccessControllerBeanTest {

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
    private GlobalDomainAccessControllerBean globalDomainAccessControllerSubject = new GlobalDomainAccessControllerBean();

    private final static String USER_ID = "user.name";
    private final static String DOMAIN = "domain1";
    private final static String INTERFACE_NAME = "interfaceName";

    @Test
    public void testFindMasterAccessControlEntriesByUserId() {
        when(masterAccessControlEntryManagerMock.findByUserId(USER_ID, ControlEntryType.MASTER)).thenReturn(new MasterAccessControlEntry[0]);
        MasterAccessControlEntry[] result = globalDomainAccessControllerSubject.getMasterAccessControlEntries(USER_ID);
        assertNotNull(result);
        assertEquals(0, result.length);
        verify(masterAccessControlEntryManagerMock).findByUserId(eq(USER_ID), eq(ControlEntryType.MASTER));
    }

    @Test
    public void testFindMasterAccessControlEntriesByDomainAndInterfaceName() {
        when(masterAccessControlEntryManagerMock.findByDomainAndInterfaceName(DOMAIN,
                                                                              INTERFACE_NAME,
                                                                              ControlEntryType.MASTER)).thenReturn(new MasterAccessControlEntry[0]);
        MasterAccessControlEntry[] result = globalDomainAccessControllerSubject.getMasterAccessControlEntries(DOMAIN, INTERFACE_NAME);
        assertNotNull(result);
        assertEquals(0, result.length);
        verify(masterAccessControlEntryManagerMock).findByDomainAndInterfaceName(DOMAIN,
                                                                                 INTERFACE_NAME,
                                                                                 ControlEntryType.MASTER);
    }

    @Test
    public void testFindMediatorAccessControlEntriesByUserId() {
        when(masterAccessControlEntryManagerMock.findByUserId(USER_ID, ControlEntryType.MEDIATOR)).thenReturn(new MasterAccessControlEntry[0]);
        MasterAccessControlEntry[] result = globalDomainAccessControllerSubject.getMediatorAccessControlEntries(USER_ID);
        assertNotNull(result);
        assertEquals(0, result.length);
        verify(masterAccessControlEntryManagerMock).findByUserId(eq(USER_ID), eq(ControlEntryType.MEDIATOR));
    }

    @Test
    public void testFindMediatorAccessControlEntriesByDomainAndInterfaceName() {
        when(masterAccessControlEntryManagerMock.findByDomainAndInterfaceName(DOMAIN,
                                                                              INTERFACE_NAME,
                                                                              ControlEntryType.MEDIATOR)).thenReturn(new MasterAccessControlEntry[0]);
        MasterAccessControlEntry[] result = globalDomainAccessControllerSubject.getMediatorAccessControlEntries(DOMAIN, INTERFACE_NAME);
        assertNotNull(result);
        assertEquals(0, result.length);
        verify(masterAccessControlEntryManagerMock).findByDomainAndInterfaceName(DOMAIN,
                                                                                 INTERFACE_NAME,
                                                                                 ControlEntryType.MEDIATOR);
    }

    @Test
    public void testFindOwnerAccessControlEntryByUserId() {
        when(ownerAccessControlEntryManagerMock.findByUserId(USER_ID)).thenReturn(new OwnerAccessControlEntry[0]);
        OwnerAccessControlEntry[] result = globalDomainAccessControllerSubject.getOwnerAccessControlEntries(USER_ID);
        assertNotNull(result);
        assertEquals(0, result.length);
        verify(ownerAccessControlEntryManagerMock).findByUserId(eq(USER_ID));
    }

    @Test
    public void testFindOwnerAccessControlEntryByDomainAndInterfaceName() {
        when(ownerAccessControlEntryManagerMock.findByDomainAndInterfaceName(DOMAIN, INTERFACE_NAME)).thenReturn(new OwnerAccessControlEntry[0]);
        OwnerAccessControlEntry[] result = globalDomainAccessControllerSubject.getOwnerAccessControlEntries(DOMAIN, INTERFACE_NAME);
        assertNotNull(result);
        assertEquals(0, result.length);
        verify(ownerAccessControlEntryManagerMock).findByDomainAndInterfaceName(eq(DOMAIN), eq(INTERFACE_NAME));
    }

    @Test
    public void testFindMasterAndMediatorRegistrationControlEntryByUserId() {
        Map<ControlEntryType, Function<String, MasterRegistrationControlEntry[]>> getters = new HashMap<>();
        getters.put(ControlEntryType.MASTER, (String userId) -> { return globalDomainAccessControllerSubject.getMasterRegistrationControlEntries(userId);});
        getters.put(ControlEntryType.MEDIATOR, (String userId) -> { return globalDomainAccessControllerSubject.getMediatorRegistrationControlEntries(userId);});
        for (ControlEntryType type : new ControlEntryType[] { ControlEntryType.MASTER, ControlEntryType.MEDIATOR}) {
            when(masterRegistrationControlEntryManagerMock.findByUserIdAndType(USER_ID,
                type)).thenReturn(new MasterRegistrationControlEntry[0]);
            MasterRegistrationControlEntry[] result = getters.get(type).apply(USER_ID);
            assertNotNull(result);
            assertEquals(0, result.length);
            verify(masterRegistrationControlEntryManagerMock).findByUserIdAndType(eq(USER_ID),
                eq(type));
        }
    }

    @Test
    public void testFindOwnerRegistrationControlEntriesByUserId() {
        when(ownerRegistrationControlEntryManagerMock.findByUserId(USER_ID)).thenReturn(new OwnerRegistrationControlEntry[0]);
        OwnerRegistrationControlEntry[] result = globalDomainAccessControllerSubject.getOwnerRegistrationControlEntries(USER_ID);
        assertNotNull(result);
        assertEquals(0, result.length);
        verify(ownerRegistrationControlEntryManagerMock).findByUserId(eq(USER_ID));
    }
}
