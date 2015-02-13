package io.joynr.accesscontrol;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.capabilities.LocalCapabilitiesDirectory;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.ProxyInvocationHandler;
import io.joynr.proxy.ProxyInvocationHandlerFactory;

import java.lang.reflect.Method;
import java.util.Arrays;

import joynr.OnChangeSubscriptionQos;
import joynr.infrastructure.DomainRoleEntry;
import joynr.infrastructure.GlobalDomainAccessControllerBroadcastInterface.DomainRoleEntryChangedBroadcastFilterParameters;
import joynr.infrastructure.GlobalDomainAccessControllerBroadcastInterface.DomainRoleEntryChangedBroadcastListener;
import joynr.infrastructure.GlobalDomainAccessControllerProxy;
import joynr.infrastructure.MasterAccessControlEntry;
import joynr.infrastructure.OwnerAccessControlEntry;
import joynr.infrastructure.Permission;
import joynr.infrastructure.Role;
import joynr.infrastructure.TrustLevel;
import net.sf.ehcache.CacheManager;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
public class LocalDomainAccessControllerTest {
    private static final String WILDCARD = "*";
    private static final String UID1 = "uid1";
    private static final String DOMAIN1 = "domain1";
    private static final String INTERFACE1 = "interface1";
    private static final String OPEARATION1 = "operation1";

    private CacheManager cacheManager;
    private DomainAccessControlStore domainAccessControlStore;
    private LocalDomainAccessController localDomainAccessController;
    private MasterAccessControlEntry masterAce;
    private OwnerAccessControlEntry ownerAce;
    private DomainRoleEntry userDre;
    private DomainRoleEntry dummyUserDre;

    @Mock
    private ProxyInvocationHandlerFactory proxyInvocationHandlerFactoryMock;
    @Mock
    private ProxyInvocationHandler proxyInvocationHandlerMock;
    @Mock
    private LocalCapabilitiesDirectory localCapabilitiesDirectoryMock;

    @Before
    public void setup() {
        cacheManager = CacheManager.create();
        domainAccessControlStore = new DomainAccessControlStoreEhCache(cacheManager);

        String accessControlDomain = "accessControlDomain";
        when(proxyInvocationHandlerFactoryMock.create(any(String.class),
                                                      any(String.class),
                                                      any(String.class),
                                                      any(DiscoveryQos.class),
                                                      any(MessagingQos.class))).thenReturn(proxyInvocationHandlerMock);
        localDomainAccessController = new LocalDomainAccessControllerImpl(accessControlDomain,
                                                                          domainAccessControlStore,
                                                                          localCapabilitiesDirectoryMock,
                                                                          proxyInvocationHandlerFactoryMock);

        // instantiate some template objects
        userDre = new DomainRoleEntry(UID1, Arrays.asList(DOMAIN1), Role.OWNER);
        masterAce = new MasterAccessControlEntry(UID1,
                                                 DOMAIN1,
                                                 INTERFACE1,
                                                 TrustLevel.LOW,
                                                 Arrays.asList(TrustLevel.MID, TrustLevel.LOW),
                                                 TrustLevel.LOW,
                                                 Arrays.asList(TrustLevel.MID, TrustLevel.LOW),
                                                 OPEARATION1,
                                                 Permission.NO,
                                                 Arrays.asList(Permission.ASK, Permission.NO));
        ownerAce = new OwnerAccessControlEntry(UID1,
                                               DOMAIN1,
                                               INTERFACE1,
                                               TrustLevel.LOW,
                                               TrustLevel.LOW,
                                               OPEARATION1,
                                               Permission.YES);
        // dummyUser DRE to prepare for ACE validation workaround
        dummyUserDre = new DomainRoleEntry(DomainAccessControlStoreEhCache.DUMMY_USERID,
                                           Arrays.asList(DOMAIN1),
                                           Role.OWNER);
    }

    @After
    public void tearDown() {
        cacheManager.removeAllCaches();
    }

    @Test
    public void testHasRole() throws Throwable {
        domainAccessControlStore.updateDomainRole(userDre);

        assertTrue("UID1 should have role OWNER in DRT", localDomainAccessController.hasRole(UID1, DOMAIN1, Role.OWNER));
        assertFalse("UID1 should not have role MASTER in DRT", localDomainAccessController.hasRole(UID1,
                                                                                                   DOMAIN1,
                                                                                                   Role.MASTER));

        Method method = GlobalDomainAccessControllerProxy.class.getMethod("subscribeToDomainRoleEntryChangedBroadcast",
                                                                          DomainRoleEntryChangedBroadcastListener.class,
                                                                          OnChangeSubscriptionQos.class,
                                                                          DomainRoleEntryChangedBroadcastFilterParameters.class);
        verify(proxyInvocationHandlerMock, times(1)).invoke(any(Object.class), eq(method), any(Object[].class));
    }

    @Test
    public void testConsumerPermission() throws Exception {
        domainAccessControlStore.updateDomainRole(dummyUserDre);
        domainAccessControlStore.updateOwnerAccessControlEntry(ownerAce);

        assertEquals("UID1 should have Permission YES",
                     Permission.YES,
                     localDomainAccessController.getConsumerPermission(UID1,
                                                                       DOMAIN1,
                                                                       INTERFACE1,
                                                                       OPEARATION1,
                                                                       TrustLevel.HIGH));
    }

    @Test
    public void testConsumerPermissionInvalidOwnerAce() throws Exception {
        masterAce.setDefaultConsumerPermission(Permission.ASK);
        domainAccessControlStore.updateDomainRole(dummyUserDre);
        domainAccessControlStore.updateOwnerAccessControlEntry(ownerAce);
        DomainRoleEntry dummyUserDomainRoleEntryMaster = new DomainRoleEntry(DomainAccessControlStoreEhCache.DUMMY_USERID,
                                                                             Arrays.asList(DOMAIN1),
                                                                             Role.MASTER);
        domainAccessControlStore.updateDomainRole(dummyUserDomainRoleEntryMaster);
        domainAccessControlStore.updateMasterAccessControlEntry(masterAce);

        assertEquals("UID1 should have Permission NO",
                     Permission.NO,
                     localDomainAccessController.getConsumerPermission(UID1,
                                                                       DOMAIN1,
                                                                       INTERFACE1,
                                                                       OPEARATION1,
                                                                       TrustLevel.HIGH));
    }

    @Test
    public void testConsumerPermissionOwnerAceOverrulesMaster() throws Exception {
        domainAccessControlStore.updateDomainRole(dummyUserDre);
        ownerAce.setRequiredTrustLevel(TrustLevel.MID);
        ownerAce.setConsumerPermission(Permission.ASK);
        domainAccessControlStore.updateOwnerAccessControlEntry(ownerAce);
        DomainRoleEntry dummyUserDomainRoleEntryMaster = new DomainRoleEntry(DomainAccessControlStoreEhCache.DUMMY_USERID,
                                                                             Arrays.asList(DOMAIN1),
                                                                             Role.MASTER);
        domainAccessControlStore.updateDomainRole(dummyUserDomainRoleEntryMaster);
        domainAccessControlStore.updateMasterAccessControlEntry(masterAce);

        assertEquals("UID1 should have Permission ASK",
                     Permission.ASK,
                     localDomainAccessController.getConsumerPermission(UID1,
                                                                       DOMAIN1,
                                                                       INTERFACE1,
                                                                       OPEARATION1,
                                                                       TrustLevel.HIGH));
        assertEquals("UID1 should have Permission NO",
                     Permission.NO,
                     localDomainAccessController.getConsumerPermission(UID1,
                                                                       DOMAIN1,
                                                                       INTERFACE1,
                                                                       OPEARATION1,
                                                                       TrustLevel.LOW));
    }

    @Test
    public void testConsumerPermissionOperationWildcard() throws Exception {
        domainAccessControlStore.updateDomainRole(dummyUserDre);
        ownerAce.setOperation(WILDCARD);
        domainAccessControlStore.updateOwnerAccessControlEntry(ownerAce);
        assertEquals("UID1 should have Permission YES",
                     Permission.YES,
                     localDomainAccessController.getConsumerPermission(UID1,
                                                                       DOMAIN1,
                                                                       INTERFACE1,
                                                                       OPEARATION1,
                                                                       TrustLevel.HIGH));
    }
}
