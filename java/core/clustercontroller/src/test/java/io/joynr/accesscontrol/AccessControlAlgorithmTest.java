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
import joynr.infrastructure.DacTypes.MasterRegistrationControlEntry;
import joynr.infrastructure.DacTypes.OwnerRegistrationControlEntry;
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
    private MasterRegistrationControlEntry masterRce;
    private MasterRegistrationControlEntry mediatorRce;
    private OwnerRegistrationControlEntry ownerRce;

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

        masterRce = new MasterRegistrationControlEntry(UID,
                                                       DOMAIN,
                                                       INTERFACE,
                                                       TrustLevel.LOW,
                                                       allTrustLevels,
                                                       TrustLevel.LOW,
                                                       allTrustLevels,
                                                       Permission.NO,
                                                       allPermissions);

        mediatorRce = new MasterRegistrationControlEntry(UID,
                                                         DOMAIN,
                                                         INTERFACE,
                                                         TrustLevel.LOW,
                                                         allTrustLevels,
                                                         TrustLevel.LOW,
                                                         allTrustLevels,
                                                         Permission.NO,
                                                         allPermissions);

        ownerRce = new OwnerRegistrationControlEntry(UID,
                                                     DOMAIN,
                                                     INTERFACE,
                                                     TrustLevel.LOW,
                                                     TrustLevel.LOW,
                                                     Permission.NO);

    }

    // getConsumerPermission

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

    // getProviderPermission

    @Test
    public void testPermissionWithMasterRceOnly() {
        masterRce.setDefaultProviderPermission(Permission.YES);
        masterRce.setDefaultRequiredTrustLevel(TrustLevel.HIGH);
        Permission providerPermission = accessControlAlgorithm.getProviderPermission(Optional.of(masterRce),
                                                                                     Optional.empty(),
                                                                                     Optional.empty(),
                                                                                     TrustLevel.HIGH);
        Assert.assertEquals(Permission.YES, providerPermission);
    }

    @Test
    public void testPermissionMessageTrustLevelDoesntMatchRce() {
        masterRce.setDefaultProviderPermission(Permission.YES);
        masterRce.setDefaultRequiredTrustLevel(TrustLevel.MID);
        Permission providerPermission = accessControlAlgorithm.getProviderPermission(Optional.of(masterRce),
                                                                                     Optional.empty(),
                                                                                     Optional.empty(),
                                                                                     TrustLevel.LOW);
        Assert.assertEquals(Permission.NO, providerPermission);
    }

    @Test
    public void testPermissionWithAllRceNull() {
        Permission providerPermission = accessControlAlgorithm.getProviderPermission(Optional.empty(),
                                                                                     Optional.empty(),
                                                                                     Optional.empty(),
                                                                                     TrustLevel.HIGH);
        Assert.assertEquals(Permission.NO, providerPermission);
    }

    //------ Mediator overrides master with -----------------------------

    @Test
    public void testPermissionWithMasterAndMediatorRce() {
        masterRce.setDefaultProviderPermission(Permission.YES);
        masterRce.setDefaultRequiredTrustLevel(TrustLevel.HIGH);
        masterRce.setPossibleProviderPermissions(allPermissions);
        masterRce.setPossibleRequiredTrustLevels(allTrustLevels);

        mediatorRce.setDefaultProviderPermission(Permission.ASK);
        mediatorRce.setDefaultRequiredTrustLevel(TrustLevel.LOW);

        Permission providerPermission = accessControlAlgorithm.getProviderPermission(Optional.of(masterRce),
                                                                                     Optional.of(mediatorRce),
                                                                                     Optional.empty(),
                                                                                     TrustLevel.LOW);
        Assert.assertEquals(Permission.ASK, providerPermission);
    }

    @Test
    public void testPermissionWithMediatorRceOnly() {
        mediatorRce.setDefaultProviderPermission(Permission.YES);
        mediatorRce.setDefaultRequiredTrustLevel(TrustLevel.MID);

        Permission providerPermission = accessControlAlgorithm.getProviderPermission(Optional.empty(),
                                                                                     Optional.of(mediatorRce),
                                                                                     Optional.empty(),
                                                                                     TrustLevel.HIGH);
        Assert.assertEquals(Permission.YES, providerPermission);
    }

    @Test
    public void testPermissionWithMasterAndInvalidMediatorRce() {
        masterRce.setPossibleProviderPermissions(new Permission[]{ Permission.NO });

        mediatorRce.setPossibleProviderPermissions(new Permission[]{ Permission.ASK, Permission.YES });
        mediatorRce.setDefaultProviderPermission(Permission.YES);
        mediatorRce.setDefaultRequiredTrustLevel(TrustLevel.MID);

        Permission providerPermission = accessControlAlgorithm.getProviderPermission(Optional.of(masterRce),
                                                                                     Optional.of(mediatorRce),
                                                                                     Optional.empty(),
                                                                                     TrustLevel.HIGH);
        Assert.assertEquals(Permission.NO, providerPermission);
    }

    //------ Owner overrides master and mediator ---------------------------------

    @Test
    public void testPermissionWithMasterMediatorAndOwnerRce() {
        masterRce.setDefaultProviderPermission(Permission.YES);
        masterRce.setDefaultRequiredTrustLevel(TrustLevel.LOW);

        mediatorRce.setDefaultProviderPermission(Permission.ASK);
        mediatorRce.setDefaultRequiredTrustLevel(TrustLevel.HIGH);

        ownerRce.setProviderPermission(Permission.YES);
        ownerRce.setRequiredTrustLevel(TrustLevel.MID);

        Permission providerPermission = accessControlAlgorithm.getProviderPermission(Optional.of(masterRce),
                                                                                     Optional.of(mediatorRce),
                                                                                     Optional.of(ownerRce),
                                                                                     TrustLevel.MID);
        Assert.assertEquals(Permission.YES, providerPermission);
    }

    @Test
    public void testPermissionWithMasterAndOwnerRce() {
        masterRce.setDefaultProviderPermission(Permission.ASK);
        masterRce.setDefaultRequiredTrustLevel(TrustLevel.LOW);

        ownerRce.setProviderPermission(Permission.YES);
        ownerRce.setRequiredTrustLevel(TrustLevel.HIGH);

        Permission providerPermission = accessControlAlgorithm.getProviderPermission(Optional.of(masterRce),
                                                                                     Optional.empty(),
                                                                                     Optional.of(ownerRce),
                                                                                     TrustLevel.HIGH);
        Assert.assertEquals(Permission.YES, providerPermission);
    }

    @Test
    public void testPermissionWithOwnerRceOnly() {
        ownerRce.setProviderPermission(Permission.YES);
        ownerRce.setRequiredTrustLevel(TrustLevel.HIGH);

        Permission providerPermission = accessControlAlgorithm.getProviderPermission(Optional.empty(),
                                                                                     Optional.empty(),
                                                                                     Optional.of(ownerRce),
                                                                                     TrustLevel.HIGH);
        Assert.assertEquals(Permission.YES, providerPermission);
    }

    @Test
    public void testPermissionWithMediatorAndInvalidOwnerRce() {
        mediatorRce.setPossibleProviderPermissions(new Permission[]{ Permission.NO });

        ownerRce.setProviderPermission(Permission.ASK);

        Permission providerPermission = accessControlAlgorithm.getProviderPermission(Optional.empty(),
                                                                                     Optional.of(mediatorRce),
                                                                                     Optional.of(ownerRce),
                                                                                     TrustLevel.HIGH);
        Assert.assertEquals(Permission.NO, providerPermission);
    }

    @Test
    public void testPermissionWithMasterAndInvalidOwnerRce() {
        masterRce.setPossibleProviderPermissions(new Permission[]{ Permission.NO });

        ownerRce.setProviderPermission(Permission.ASK);

        Permission providerPermission = accessControlAlgorithm.getProviderPermission(Optional.of(masterRce),
                                                                                     Optional.empty(),
                                                                                     Optional.of(ownerRce),
                                                                                     TrustLevel.HIGH);
        Assert.assertEquals(Permission.NO, providerPermission);
    }
}
