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

import java.util.Optional;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

import joynr.infrastructure.DacTypes.MasterAccessControlEntry;
import joynr.infrastructure.DacTypes.OwnerAccessControlEntry;
import joynr.infrastructure.DacTypes.Permission;
import joynr.infrastructure.DacTypes.TrustLevel;

// Test the access control Algorithm
public class AccessControlAlgorithmTest {

    private static String UID = "testuid";
    private static String DOMAIN = "testdomain";
    private static String INTERFACE = "testinterface";
    private static Permission[] allPermissions = { Permission.NO, Permission.ASK, Permission.YES };
    private static TrustLevel[] allTrustLevels = { TrustLevel.LOW, TrustLevel.MID, TrustLevel.HIGH };

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
        Permission consumerPermission = accessControlAlgorithm.getConsumerPermission(Optional.of(masterAce),
                                                                                     Optional.empty(),
                                                                                     Optional.empty(),
                                                                                     TrustLevel.HIGH);
        Assert.assertEquals(Permission.YES, consumerPermission);
    }

    @Test
    public void testPermissionMessageTrustLevelDoesntMatchAce() {
        masterAce.setDefaultConsumerPermission(Permission.YES);
        masterAce.setDefaultRequiredTrustLevel(TrustLevel.MID);
        Permission consumerPermission = accessControlAlgorithm.getConsumerPermission(Optional.of(masterAce),
                                                                                     Optional.empty(),
                                                                                     Optional.empty(),
                                                                                     TrustLevel.LOW);
        Assert.assertEquals(Permission.NO, consumerPermission);
    }

    @Test
    public void testPermissionWithAllAceNull() {
        Permission providerPermission = accessControlAlgorithm.getConsumerPermission(Optional.empty(),
                                                                                     Optional.empty(),
                                                                                     Optional.empty(),
                                                                                     TrustLevel.HIGH);
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

        Permission providerPermission = accessControlAlgorithm.getConsumerPermission(Optional.of(masterAce),
                                                                                     Optional.of(mediatorAce),
                                                                                     Optional.empty(),
                                                                                     TrustLevel.LOW);
        Assert.assertEquals(Permission.ASK, providerPermission);
    }

    @Test
    public void testPermissionWithMediatorOnly() {
        mediatorAce.setDefaultConsumerPermission(Permission.YES);
        mediatorAce.setDefaultRequiredTrustLevel(TrustLevel.MID);

        Permission providerPermission = accessControlAlgorithm.getConsumerPermission(Optional.empty(),
                                                                                     Optional.of(mediatorAce),
                                                                                     Optional.empty(),
                                                                                     TrustLevel.HIGH);
        Assert.assertEquals(Permission.YES, providerPermission);
    }

    @Test
    public void testPermissionWithMasterAndInvalidMediatorAce() {
        masterAce.setPossibleConsumerPermissions(new Permission[]{ Permission.NO });

        mediatorAce.setPossibleConsumerPermissions(new Permission[]{ Permission.ASK, Permission.YES });
        mediatorAce.setDefaultConsumerPermission(Permission.YES);
        mediatorAce.setDefaultRequiredTrustLevel(TrustLevel.MID);

        Permission consumerPermission = accessControlAlgorithm.getConsumerPermission(Optional.of(masterAce),
                                                                                     Optional.of(mediatorAce),
                                                                                     Optional.empty(),
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

        Permission consumerPermission = accessControlAlgorithm.getConsumerPermission(Optional.of(masterAce),
                                                                                     Optional.of(mediatorAce),
                                                                                     Optional.of(ownerAce),
                                                                                     TrustLevel.MID);
        Assert.assertEquals(Permission.YES, consumerPermission);
    }

    @Test
    public void testPermissionWithMasterAndOwnerAce() {
        masterAce.setDefaultConsumerPermission(Permission.ASK);
        masterAce.setDefaultRequiredTrustLevel(TrustLevel.LOW);

        ownerAce.setConsumerPermission(Permission.YES);
        ownerAce.setRequiredTrustLevel(TrustLevel.HIGH);

        Permission consumerPermission = accessControlAlgorithm.getConsumerPermission(Optional.of(masterAce),
                                                                                     Optional.empty(),
                                                                                     Optional.of(ownerAce),
                                                                                     TrustLevel.HIGH);
        Assert.assertEquals(Permission.YES, consumerPermission);
    }

    @Test
    public void testPermissionWithOwnerAceOnly() {
        ownerAce.setConsumerPermission(Permission.YES);
        ownerAce.setRequiredTrustLevel(TrustLevel.HIGH);

        Permission providerPermission = accessControlAlgorithm.getConsumerPermission(Optional.empty(),
                                                                                     Optional.empty(),
                                                                                     Optional.of(ownerAce),
                                                                                     TrustLevel.HIGH);
        Assert.assertEquals(Permission.YES, providerPermission);
    }

    @Test
    public void testPermissionWithMediatorAndInvalidOwnerAce() {
        mediatorAce.setPossibleConsumerPermissions(new Permission[]{ Permission.NO });

        ownerAce.setConsumerPermission(Permission.ASK);

        Permission consumerPermission = accessControlAlgorithm.getConsumerPermission(Optional.empty(),
                                                                                     Optional.of(mediatorAce),
                                                                                     Optional.of(ownerAce),
                                                                                     TrustLevel.HIGH);
        Assert.assertEquals(Permission.NO, consumerPermission);
    }

    @Test
    public void testPermissionWithMasterAndInvalidOwnerAce() {
        masterAce.setPossibleConsumerPermissions(new Permission[]{ Permission.NO });

        ownerAce.setConsumerPermission(Permission.ASK);

        Permission consumerPermission = accessControlAlgorithm.getConsumerPermission(Optional.of(masterAce),
                                                                                     Optional.empty(),
                                                                                     Optional.of(ownerAce),
                                                                                     TrustLevel.HIGH);
        Assert.assertEquals(Permission.NO, consumerPermission);
    }
}
