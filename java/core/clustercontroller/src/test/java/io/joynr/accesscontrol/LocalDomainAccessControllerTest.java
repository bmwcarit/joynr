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
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.lang.reflect.Method;
import java.util.Set;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;

import io.joynr.arbitration.DiscoveryQos;
import io.joynr.discovery.LocalDiscoveryAggregator;
import io.joynr.dispatching.Dispatcher;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.ProxyBuilderFactoryImpl;
import io.joynr.proxy.ProxyInvocationHandler;
import io.joynr.proxy.ProxyInvocationHandlerFactory;
import io.joynr.proxy.StatelessAsyncCallback;
import io.joynr.proxy.StatelessAsyncCallbackDirectory;
import io.joynr.runtime.ShutdownNotifier;
import joynr.MulticastSubscriptionQos;
import joynr.infrastructure.DacTypes.DomainRoleEntry;
import joynr.infrastructure.DacTypes.MasterAccessControlEntry;
import joynr.infrastructure.DacTypes.OwnerAccessControlEntry;
import joynr.infrastructure.DacTypes.Permission;
import joynr.infrastructure.DacTypes.Role;
import joynr.infrastructure.DacTypes.TrustLevel;
import joynr.types.GlobalDiscoveryEntry;

import net.sf.ehcache.CacheManager;

@RunWith(MockitoJUnitRunner.class)
public class LocalDomainAccessControllerTest {
    private static final String WILDCARD = "*";
    private static final String UID1 = "uid1";
    private static final String DOMAIN1 = "domain1";
    private static final String INTERFACE1 = "interface1";
    private static final String OPEARATION1 = "operation1";
    private static final long MAX_TTL = 2592000000L;
    private static final long DEFAULT_DISCOVERY_TIMEOUT_MS = 30000L;
    private static final long DEFAULT_RETRY_INTERVAL_MS = 2000L;
    private static final long ARBITRATION_MINIMUMRETRYDELAY = 2000L;

    private CacheManager cacheManager;
    private DomainAccessControlStore domainAccessControlStore;
    private LocalDomainAccessController localDomainAccessController;
    private MasterAccessControlEntry masterAce;
    private OwnerAccessControlEntry ownerAce;
    private DomainRoleEntry userDre;

    @Mock
    private ProxyInvocationHandlerFactory proxyInvocationHandlerFactoryMock;
    @Mock
    private StatelessAsyncCallbackDirectory statelessAsyncCallbackDirectoryMock;
    @Mock
    private ProxyInvocationHandler proxyInvocationHandlerMock;
    @Mock
    private LocalDiscoveryAggregator localDiscoveryAggregator;
    @Mock
    private Dispatcher dispatcher;
    @Mock
    private ShutdownNotifier shutdownNotifier;

    @SuppressWarnings("unchecked")
    @Before
    public void setup() {
        cacheManager = CacheManager.create();
        domainAccessControlStore = new DomainAccessControlStoreEhCache(cacheManager,
                                                                       new DefaultDomainAccessControlProvisioning());

        when(proxyInvocationHandlerFactoryMock.create(any(Set.class),
                                                      any(String.class),
                                                      any(String.class),
                                                      any(DiscoveryQos.class),
                                                      any(MessagingQos.class),
                                                      any(ShutdownNotifier.class),
                                                      any())).thenReturn(proxyInvocationHandlerMock);
        GlobalDiscoveryEntry accessControlDomain = mock(GlobalDiscoveryEntry.class);
        when(accessControlDomain.getDomain()).thenReturn("accessControlDomain");
        localDomainAccessController = new LocalDomainAccessControllerImpl(accessControlDomain,
                                                                          domainAccessControlStore,
                                                                          new ProxyBuilderFactoryImpl(localDiscoveryAggregator,
                                                                                                      proxyInvocationHandlerFactoryMock,
                                                                                                      shutdownNotifier,
                                                                                                      statelessAsyncCallbackDirectoryMock,
                                                                                                      MAX_TTL,
                                                                                                      DEFAULT_DISCOVERY_TIMEOUT_MS,
                                                                                                      DEFAULT_RETRY_INTERVAL_MS,
                                                                                                      ARBITRATION_MINIMUMRETRYDELAY),
                                                                          "systemServiceDomain");

        // instantiate some template objects
        userDre = new DomainRoleEntry(UID1, new String[]{ DOMAIN1 }, Role.OWNER);
        masterAce = new MasterAccessControlEntry(UID1,
                                                 DOMAIN1,
                                                 INTERFACE1,
                                                 TrustLevel.LOW,
                                                 new TrustLevel[]{ TrustLevel.MID, TrustLevel.LOW },
                                                 TrustLevel.LOW,
                                                 new TrustLevel[]{ TrustLevel.MID, TrustLevel.LOW },
                                                 OPEARATION1,
                                                 Permission.NO,
                                                 new Permission[]{ Permission.ASK, Permission.NO });
        ownerAce = new OwnerAccessControlEntry(UID1,
                                               DOMAIN1,
                                               INTERFACE1,
                                               TrustLevel.LOW,
                                               TrustLevel.LOW,
                                               OPEARATION1,
                                               Permission.YES);
    }

    @After
    public void tearDown() {
        cacheManager.removeAllCaches();
    }

    @Test
    public void testHasRole() throws Throwable {
        domainAccessControlStore.updateDomainRole(userDre);

        assertTrue("UID1 should have role OWNER in DRT",
                   localDomainAccessController.hasRole(UID1, DOMAIN1, Role.OWNER));
        assertFalse("UID1 should not have role MASTER in DRT",
                    localDomainAccessController.hasRole(UID1, DOMAIN1, Role.MASTER));
    }

    @Test
    public void testConsumerPermission() throws Exception {
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
        domainAccessControlStore.updateOwnerAccessControlEntry(ownerAce);
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
        ownerAce.setRequiredTrustLevel(TrustLevel.MID);
        ownerAce.setConsumerPermission(Permission.ASK);
        domainAccessControlStore.updateOwnerAccessControlEntry(ownerAce);
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
