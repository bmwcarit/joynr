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

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import joynr.infrastructure.DacTypes.MasterAccessControlEntry;
import joynr.infrastructure.DacTypes.OwnerAccessControlEntry;
import joynr.infrastructure.DacTypes.Permission;
import joynr.infrastructure.DacTypes.TrustLevel;

import org.junit.Before;
import org.junit.Test;

public class AceValidatorTest {

    private MasterAccessControlEntry masterAce;
    private MasterAccessControlEntry mediatorAce;
    private OwnerAccessControlEntry ownerAce;

    @Before
    public void setup() {
        masterAce = new MasterAccessControlEntry(null,
                                                 null,
                                                 null,
                                                 TrustLevel.LOW,
                                                 new TrustLevel[]{ TrustLevel.MID, TrustLevel.LOW },
                                                 TrustLevel.LOW,
                                                 new TrustLevel[]{ TrustLevel.MID, TrustLevel.HIGH },
                                                 null,
                                                 Permission.NO,
                                                 new Permission[]{ Permission.ASK, Permission.NO });

        mediatorAce = new MasterAccessControlEntry(null,
                                                   null,
                                                   null,
                                                   TrustLevel.LOW,
                                                   new TrustLevel[]{ TrustLevel.MID, TrustLevel.LOW },
                                                   TrustLevel.LOW,
                                                   new TrustLevel[]{ TrustLevel.MID, TrustLevel.HIGH },
                                                   null,
                                                   Permission.NO,
                                                   new Permission[]{ Permission.ASK, Permission.NO });

        ownerAce = new OwnerAccessControlEntry(null, null, null, TrustLevel.MID, TrustLevel.HIGH, null, Permission.ASK);
    }

    @Test
    public void testMediatorInvalidPossiblePermissions() {
        mediatorAce.setPossibleConsumerPermissions(new Permission[]{ Permission.YES, Permission.ASK });
        AceValidator validator = new AceValidator(masterAce, mediatorAce, ownerAce);
        assertFalse("modified mediator is not valid", validator.isMediatorValid());
    }

    @Test
    public void testMediatorInvalidPossibleTrusLevels() {
        mediatorAce.setPossibleRequiredTrustLevels(new TrustLevel[]{ TrustLevel.HIGH, TrustLevel.MID });
        AceValidator validator = new AceValidator(masterAce, mediatorAce, ownerAce);
        assertFalse("modified mediator is not valid", validator.isMediatorValid());
    }

    @Test
    public void testMediatorValid() {
        AceValidator validator = new AceValidator(masterAce, mediatorAce, ownerAce);
        assertTrue("initial mediator is valid", validator.isMediatorValid());
    }

    @Test
    public void testOwnerValid() {
        AceValidator validator = new AceValidator(masterAce, mediatorAce, ownerAce);
        assertTrue("initial owner is valid", validator.isOwnerValid());
    }

    @Test
    public void testOwnerInvalid() {
        ownerAce.setConsumerPermission(Permission.YES);
        AceValidator validator = new AceValidator(masterAce, mediatorAce, ownerAce);
        assertFalse("modified owner is invalid", validator.isOwnerValid());
    }
}
