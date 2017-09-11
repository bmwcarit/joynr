package io.joynr.accesscontrol.global.jee;

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

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import joynr.infrastructure.DacTypes.ChangeType;
import joynr.infrastructure.DacTypes.DomainRoleEntry;
import joynr.infrastructure.DacTypes.Role;
import joynr.infrastructure.GlobalDomainRoleControllerSubscriptionPublisher;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.InjectMocks;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
public class GlobalDomainRoleControllerBeanTest {

    @Mock
    private DomainRoleEntryManager domainRoleEntryManagerMock;

    @Mock
    private GlobalDomainRoleControllerSubscriptionPublisher globalDomainRoleControllerSubscriptionPublisherMock;

    @InjectMocks
    private GlobalDomainRoleControllerBean globalDomainRoleControllerSubject = new GlobalDomainRoleControllerBean();

    private final static String USER_ID = "user.name";
    private final static String USER_PARTITION = "username";
    private final static String DOMAIN = "domain1";
    private final static String[] DOMAINS = { DOMAIN };
    private final static Role ROLE = Role.MASTER;

    @Test
    public void testGetDomainRoleEntries() {
        DomainRoleEntry[] result = new DomainRoleEntry[0];
        Mockito.when(domainRoleEntryManagerMock.findByUserId(USER_ID)).thenReturn(result);
        DomainRoleEntry[] domainRoles = globalDomainRoleControllerSubject.getDomainRoles(USER_ID);
        verify(domainRoleEntryManagerMock).findByUserId(USER_ID);
        assertArrayEquals(result, domainRoles);
    }

    @Test
    public void testCreate() {
        DomainRoleEntry domainRoleEntry = new DomainRoleEntry(USER_ID, DOMAINS, ROLE);
        CreateOrUpdateResult<DomainRoleEntry> createOrUpdateResult = new CreateOrUpdateResult<>(domainRoleEntry,
            ChangeType.ADD);

        when(domainRoleEntryManagerMock.createOrUpdate(domainRoleEntry)).thenReturn(createOrUpdateResult);

        assertTrue(globalDomainRoleControllerSubject.updateDomainRole(domainRoleEntry));

        verify(domainRoleEntryManagerMock).createOrUpdate(eq(domainRoleEntry));
        verify(globalDomainRoleControllerSubscriptionPublisherMock).fireDomainRoleEntryChanged(eq(ChangeType.ADD),
            eq(domainRoleEntry), eq(
                USER_PARTITION));
    }

    @Test
    public void testUpdate() {
        DomainRoleEntry domainRoleEntry = new DomainRoleEntry(USER_ID, DOMAINS, ROLE);
        CreateOrUpdateResult<DomainRoleEntry> createOrUpdateResult = new CreateOrUpdateResult<>(domainRoleEntry,
            ChangeType.UPDATE);

        when(domainRoleEntryManagerMock.createOrUpdate(domainRoleEntry)).thenReturn(createOrUpdateResult);

        assertTrue(globalDomainRoleControllerSubject.updateDomainRole(domainRoleEntry));

        verify(domainRoleEntryManagerMock).createOrUpdate(eq(domainRoleEntry));
        verify(globalDomainRoleControllerSubscriptionPublisherMock).fireDomainRoleEntryChanged(eq(ChangeType.UPDATE),
            eq(domainRoleEntry),
            eq(USER_PARTITION));
    }

    @Test
    public void testRemove() {
        DomainRoleEntry domainRoleEntry = new DomainRoleEntry(USER_ID, DOMAINS, ROLE);

        when(domainRoleEntryManagerMock.removeByUserIdAndRole(USER_ID, ROLE)).thenReturn(domainRoleEntry);

        assertTrue(globalDomainRoleControllerSubject.removeDomainRole(USER_ID, ROLE));

        verify(domainRoleEntryManagerMock).removeByUserIdAndRole(eq(USER_ID), eq(ROLE));
        verify(globalDomainRoleControllerSubscriptionPublisherMock).fireDomainRoleEntryChanged(eq(ChangeType.REMOVE),
                                                                                                 eq(domainRoleEntry),
                                                                                                 eq(USER_PARTITION));
    }
}
