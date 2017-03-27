package io.joynr.accesscontrol;

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

import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.verify;
import io.joynr.accesscontrol.broadcastlistener.LdacDomainRoleEntryChangedBroadcastListener;
import io.joynr.accesscontrol.broadcastlistener.LdacMasterAccessControlEntryChangedBroadcastListener;
import io.joynr.accesscontrol.broadcastlistener.LdacMediatorAccessControlEntryChangedBroadcastListener;
import io.joynr.accesscontrol.broadcastlistener.LdacOwnerAccessControlEntryChangedBroadcastListener;
import joynr.infrastructure.DacTypes.ChangeType;
import joynr.infrastructure.DacTypes.DomainRoleEntry;
import joynr.infrastructure.DacTypes.MasterAccessControlEntry;
import joynr.infrastructure.DacTypes.OwnerAccessControlEntry;
import joynr.infrastructure.DacTypes.Permission;
import joynr.infrastructure.DacTypes.Role;
import joynr.infrastructure.DacTypes.TrustLevel;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
public class GdacBroadcastListenerTest {
    private static final String UID1 = "uid1";
    private static final String DOMAIN1 = "domain1";
    private static final String INTERFACE1 = "interface1";
    private static final String OPEARATION1 = "operation1";

    private MasterAccessControlEntry masterAce;
    private OwnerAccessControlEntry ownerAce;
    private DomainRoleEntry userDre;

    @Mock
    private DomainAccessControlStore domainAccessControlStore;

    @Before
    public void setup() {
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

    @Test
    public void testDreChangedListenerForUpdateDre() {
        LdacDomainRoleEntryChangedBroadcastListener dreChangedListener = new LdacDomainRoleEntryChangedBroadcastListener(domainAccessControlStore);
        dreChangedListener.onReceive(ChangeType.UPDATE, userDre);
        verify(domainAccessControlStore).updateDomainRole(eq(userDre));
    }

    @Test
    public void testDreChangedListenerForRemoveDre() {
        userDre.setDomains(null);
        LdacDomainRoleEntryChangedBroadcastListener dreChangedListener = new LdacDomainRoleEntryChangedBroadcastListener(domainAccessControlStore);
        dreChangedListener.onReceive(ChangeType.REMOVE, userDre);
        verify(domainAccessControlStore).removeDomainRole(eq(userDre.getUid()), eq(userDre.getRole()));
    }

    @Test
    public void testMasterAceChangedListenerForUpdateAce() {
        LdacMasterAccessControlEntryChangedBroadcastListener dreChangedListener = new LdacMasterAccessControlEntryChangedBroadcastListener(domainAccessControlStore);
        dreChangedListener.onReceive(ChangeType.UPDATE, masterAce);
        verify(domainAccessControlStore).updateMasterAccessControlEntry(eq(masterAce));
    }

    @Test
    public void testMasterAceChangedListenerForRemoveAce() {
        masterAce.setDefaultConsumerPermission(null);
        masterAce.setPossibleConsumerPermissions(null);
        masterAce.setDefaultRequiredTrustLevel(null);
        masterAce.setPossibleRequiredTrustLevels(null);
        masterAce.setDefaultRequiredControlEntryChangeTrustLevel(null);
        masterAce.setPossibleRequiredControlEntryChangeTrustLevels(null);
        LdacMasterAccessControlEntryChangedBroadcastListener dreChangedListener = new LdacMasterAccessControlEntryChangedBroadcastListener(domainAccessControlStore);
        dreChangedListener.onReceive(ChangeType.REMOVE, masterAce);
        verify(domainAccessControlStore).removeMasterAccessControlEntry(masterAce.getUid(),
                                                                        masterAce.getDomain(),
                                                                        masterAce.getInterfaceName(),
                                                                        masterAce.getOperation());
    }

    @Test
    public void testMediatorAceChangedListenerForUpdateAce() {
        LdacMediatorAccessControlEntryChangedBroadcastListener dreChangedListener = new LdacMediatorAccessControlEntryChangedBroadcastListener(domainAccessControlStore);
        dreChangedListener.onReceive(ChangeType.UPDATE, masterAce);
        verify(domainAccessControlStore).updateMediatorAccessControlEntry(eq(masterAce));
    }

    @Test
    public void testMediatorAceChangedListenerForRemoveAce() {
        masterAce.setDefaultConsumerPermission(null);
        masterAce.setPossibleConsumerPermissions(null);
        masterAce.setDefaultRequiredTrustLevel(null);
        masterAce.setPossibleRequiredTrustLevels(null);
        masterAce.setDefaultRequiredControlEntryChangeTrustLevel(null);
        masterAce.setPossibleRequiredControlEntryChangeTrustLevels(null);
        LdacMediatorAccessControlEntryChangedBroadcastListener dreChangedListener = new LdacMediatorAccessControlEntryChangedBroadcastListener(domainAccessControlStore);
        dreChangedListener.onReceive(ChangeType.REMOVE, masterAce);
        verify(domainAccessControlStore).removeMediatorAccessControlEntry(masterAce.getUid(),
                                                                          masterAce.getDomain(),
                                                                          masterAce.getInterfaceName(),
                                                                          masterAce.getOperation());
    }

    @Test
    public void testOnwerAceChangedListenerForUpdateAce() {
        LdacOwnerAccessControlEntryChangedBroadcastListener dreChangedListener = new LdacOwnerAccessControlEntryChangedBroadcastListener(domainAccessControlStore);
        dreChangedListener.onReceive(ChangeType.UPDATE, ownerAce);
        verify(domainAccessControlStore).updateOwnerAccessControlEntry(eq(ownerAce));
    }

    @Test
    public void testOnwerAceChangedListenerForRemoveAce() {
        ownerAce.setConsumerPermission(null);
        ownerAce.setRequiredAceChangeTrustLevel(null);
        ownerAce.setRequiredTrustLevel(null);
        LdacOwnerAccessControlEntryChangedBroadcastListener dreChangedListener = new LdacOwnerAccessControlEntryChangedBroadcastListener(domainAccessControlStore);
        dreChangedListener.onReceive(ChangeType.REMOVE, ownerAce);
        verify(domainAccessControlStore).removeOwnerAccessControlEntry(ownerAce.getUid(),
                                                                       ownerAce.getDomain(),
                                                                       ownerAce.getInterfaceName(),
                                                                       ownerAce.getOperation());
    }
}
