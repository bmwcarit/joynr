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

import joynr.infrastructure.MasterAccessControlEntry;
import joynr.infrastructure.OwnerAccessControlEntry;
import joynr.infrastructure.Role;
import joynr.infrastructure.DomainRoleEntry;
import joynr.infrastructure.Permission;
import joynr.infrastructure.TrustLevel;
import net.sf.ehcache.CacheManager;
import org.junit.After;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

import java.util.Arrays;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.assertFalse;

public class LocalDomainAccessControllerTest {
    private static final String WILDCARD = "*";
    private static final String UID1 = "uid1";
    private static final String DOMAIN1 = "domain1";
    private static final String INTERFACE1 = "interface1";
    private static final String OPEARATION1 = "operation1";

    private static CacheManager cacheManager;
    private static DomainAccessControlStore store;
    private static AccessControlAlgorithm accessControlAlgorithm;
    private static LocalDomainAccessController localDomainAccessController;
    private MasterAccessControlEntry masterAce;
    private OwnerAccessControlEntry ownerAce;
    private DomainRoleEntry userDre;
    private DomainRoleEntry dummyUserDre;

    @BeforeClass
    public static void initialize() {
        cacheManager = CacheManager.create();
        store = new DomainAccessControlStoreEhCache(cacheManager);
        accessControlAlgorithm = new AccessControlAlgorithm();
        localDomainAccessController = new LocalDomainAccessControllerImpl(store, accessControlAlgorithm);
    }

    @Before
    public void setup() {
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
    public void testHasRole() throws Exception {
        store.updateDomainRole(userDre);

        assertTrue("UID1 should have role OWNER in DRT", localDomainAccessController.hasRole(UID1, DOMAIN1, Role.OWNER));
        assertFalse("UID1 should not have role MASTER in DRT", localDomainAccessController.hasRole(UID1,
                                                                                                   DOMAIN1,
                                                                                                   Role.MASTER));
    }

    @Test
    public void testConsumerPermission() throws Exception {
        store.updateDomainRole(dummyUserDre);
        store.updateOwnerAccessControlEntry(ownerAce);

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
        store.updateDomainRole(dummyUserDre);
        store.updateOwnerAccessControlEntry(ownerAce);
        DomainRoleEntry dummyUserDomainRoleEntryMaster = new DomainRoleEntry(DomainAccessControlStoreEhCache.DUMMY_USERID,
                                                                             Arrays.asList(DOMAIN1),
                                                                             Role.MASTER);
        store.updateDomainRole(dummyUserDomainRoleEntryMaster);
        store.updateMasterAccessControlEntry(masterAce);

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
        store.updateDomainRole(dummyUserDre);
        ownerAce.setRequiredTrustLevel(TrustLevel.MID);
        ownerAce.setConsumerPermission(Permission.ASK);
        store.updateOwnerAccessControlEntry(ownerAce);
        DomainRoleEntry dummyUserDomainRoleEntryMaster = new DomainRoleEntry(DomainAccessControlStoreEhCache.DUMMY_USERID,
                                                                             Arrays.asList(DOMAIN1),
                                                                             Role.MASTER);
        store.updateDomainRole(dummyUserDomainRoleEntryMaster);
        store.updateMasterAccessControlEntry(masterAce);

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
        store.updateDomainRole(dummyUserDre);
        ownerAce.setOperation(WILDCARD);
        store.updateOwnerAccessControlEntry(ownerAce);
        assertEquals("UID1 should have Permission YES",
                     Permission.YES,
                     localDomainAccessController.getConsumerPermission(UID1,
                                                                       DOMAIN1,
                                                                       INTERFACE1,
                                                                       OPEARATION1,
                                                                       TrustLevel.HIGH));
    }
}
