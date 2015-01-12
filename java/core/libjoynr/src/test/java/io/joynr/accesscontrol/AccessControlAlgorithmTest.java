package io.joynr.accesscontrol;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
import joynr.infrastructure.Permission;
import joynr.infrastructure.TrustLevel;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

import java.util.Arrays;
import java.util.List;

// Test the access control Algorithm
public class AccessControlAlgorithmTest {

    private static String UID = "testuid";
    private static String DOMAIN = "testdomain";
    private static String INTERFACE = "testinterface";
    private static List<Permission> allPermissions = Arrays.asList(Permission.NO, Permission.ASK, Permission.YES);
    private static List<TrustLevel> allTrustLevels = Arrays.asList(TrustLevel.LOW, TrustLevel.MID, TrustLevel.HIGH);

    private AccessControlAlgorithm accessControlAlgorithm;
    private MasterAccessControlEntry masterAce;
    private MasterAccessControlEntry mediatorAce;
    private OwnerAccessControlEntry ownerAce;

    @Before
    public void setup() {
        this.accessControlAlgorithm = new AccessControlAlgorithm();
        masterAce = new MasterAccessControlEntry(UID,
                                                 DOMAIN,
                                                 INTERFACE,
                                                 TrustLevel.LOW,
                                                 allTrustLevels,
                                                 TrustLevel.LOW,
                                                 allTrustLevels,
                                                 null,
                                                 Permission.NO,
                                                 allPermissions);

        mediatorAce = new MasterAccessControlEntry(UID,
                                                   DOMAIN,
                                                   INTERFACE,
                                                   TrustLevel.LOW,
                                                   allTrustLevels,
                                                   TrustLevel.LOW,
                                                   allTrustLevels,
                                                   null,
                                                   Permission.NO,
                                                   allPermissions);

        ownerAce = new OwnerAccessControlEntry(UID,
                                               DOMAIN,
                                               INTERFACE,
                                               TrustLevel.LOW,
                                               TrustLevel.LOW,
                                               null,
                                               Permission.NO);
    }

    @Test
    public void testPermissionWithMasterAceOnly() {
        masterAce.setDefaultConsumerPermission(Permission.YES);
        masterAce.setDefaultRequiredTrustLevel(TrustLevel.HIGH);
        Permission consumerPermission = accessControlAlgorithm.getConsumerPermission(masterAce,
                                                                                     null,
                                                                                     null,
                                                                                     TrustLevel.HIGH);
        Assert.assertEquals(Permission.YES, consumerPermission);
    }

    @Test
    public void testPermissionMessageTrustLevelDoesntMatchAce() {
        masterAce.setDefaultConsumerPermission(Permission.YES);
        masterAce.setDefaultRequiredTrustLevel(TrustLevel.MID);
        Permission consumerPermission = accessControlAlgorithm.getConsumerPermission(masterAce,
                                                                                     null,
                                                                                     null,
                                                                                     TrustLevel.LOW);
        Assert.assertEquals(Permission.NO, consumerPermission);
    }

    @Test
    public void testPermissionWithAllAceNull() {
        Permission providerPermission = accessControlAlgorithm.getConsumerPermission(null, null, null, TrustLevel.HIGH);
        Assert.assertEquals(Permission.NO, providerPermission);
    }

    //------ Mediator overrides master with -----------------------------

    @Test
    public void testPermissionWithMasterAndMediatorAce() {
        masterAce.setDefaultConsumerPermission(Permission.YES);
        masterAce.setDefaultRequiredTrustLevel(TrustLevel.HIGH);
        masterAce.setPossibleConsumerPermissions(allPermissions);
        masterAce.setPossibleRequiredTrustLevels(allTrustLevels);

        mediatorAce.setDefaultConsumerPermission(Permission.ASK);
        mediatorAce.setDefaultRequiredTrustLevel(TrustLevel.LOW);

        Permission providerPermission = accessControlAlgorithm.getConsumerPermission(masterAce,
                                                                                     mediatorAce,
                                                                                     null,
                                                                                     TrustLevel.LOW);
        Assert.assertEquals(Permission.ASK, providerPermission);
    }

    @Test
    public void testPermissionWithMediatorOnly() {
        mediatorAce.setDefaultConsumerPermission(Permission.YES);
        mediatorAce.setDefaultRequiredTrustLevel(TrustLevel.MID);

        Permission providerPermission = accessControlAlgorithm.getConsumerPermission(null,
                                                                                     mediatorAce,
                                                                                     null,
                                                                                     TrustLevel.HIGH);
        Assert.assertEquals(Permission.YES, providerPermission);
    }

    @Test
    public void testPermissionWithMasterAndInvalidMediatorAce() {
        masterAce.setPossibleConsumerPermissions(Arrays.asList(Permission.NO));

        mediatorAce.setPossibleConsumerPermissions(Arrays.asList(Permission.ASK, Permission.YES));
        mediatorAce.setDefaultConsumerPermission(Permission.YES);
        mediatorAce.setDefaultRequiredTrustLevel(TrustLevel.MID);

        Permission consumerPermission = accessControlAlgorithm.getConsumerPermission(masterAce,
                                                                                     mediatorAce,
                                                                                     null,
                                                                                     TrustLevel.HIGH);
        Assert.assertEquals(Permission.NO, consumerPermission);
    }

    //------ Owner overrides master and mediator ---------------------------------

    @Test
    public void testPermissionWithMasterMediatorAndOwnerAce() {
        masterAce.setDefaultConsumerPermission(Permission.YES);
        masterAce.setDefaultRequiredTrustLevel(TrustLevel.LOW);

        mediatorAce.setDefaultConsumerPermission(Permission.ASK);
        mediatorAce.setDefaultRequiredTrustLevel(TrustLevel.HIGH);

        ownerAce.setConsumerPermission(Permission.YES);
        ownerAce.setRequiredTrustLevel(TrustLevel.MID);

        Permission consumerPermission = accessControlAlgorithm.getConsumerPermission(masterAce,
                                                                                     mediatorAce,
                                                                                     ownerAce,
                                                                                     TrustLevel.MID);
        Assert.assertEquals(Permission.YES, consumerPermission);
    }

    @Test
    public void testPermissionWithMasterAndOwnerAce() {
        masterAce.setDefaultConsumerPermission(Permission.ASK);
        masterAce.setDefaultRequiredTrustLevel(TrustLevel.LOW);

        ownerAce.setConsumerPermission(Permission.YES);
        ownerAce.setRequiredTrustLevel(TrustLevel.HIGH);

        Permission consumerPermission = accessControlAlgorithm.getConsumerPermission(masterAce,
                                                                                     null,
                                                                                     ownerAce,
                                                                                     TrustLevel.HIGH);
        Assert.assertEquals(Permission.YES, consumerPermission);
    }

    @Test
    public void testPermissionWithOwnerAceOnly() {
        ownerAce.setConsumerPermission(Permission.YES);
        ownerAce.setRequiredTrustLevel(TrustLevel.HIGH);

        Permission providerPermission = accessControlAlgorithm.getConsumerPermission(null,
                                                                                     null,
                                                                                     ownerAce,
                                                                                     TrustLevel.HIGH);
        Assert.assertEquals(Permission.YES, providerPermission);
    }

    @Test
    public void testPermissionWithMediatorAndInvalidOwnerAce() {
        mediatorAce.setPossibleConsumerPermissions(Arrays.asList(Permission.NO));

        ownerAce.setConsumerPermission(Permission.ASK);

        Permission consumerPermission = accessControlAlgorithm.getConsumerPermission(null,
                                                                                     mediatorAce,
                                                                                     ownerAce,
                                                                                     TrustLevel.HIGH);
        Assert.assertEquals(Permission.NO, consumerPermission);
    }

    @Test
    public void testPermissionWithMasterAndInvalidOwnerAce() {
        masterAce.setPossibleConsumerPermissions(Arrays.asList(Permission.NO));

        ownerAce.setConsumerPermission(Permission.ASK);

        Permission consumerPermission = accessControlAlgorithm.getConsumerPermission(masterAce,
                                                                                     null,
                                                                                     ownerAce,
                                                                                     TrustLevel.HIGH);
        Assert.assertEquals(Permission.NO, consumerPermission);
    }
}
