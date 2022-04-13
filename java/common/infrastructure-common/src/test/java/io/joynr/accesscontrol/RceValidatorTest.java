/*
 * #%L
 * %%
 * Copyright (C) 2022 BMW Car IT GmbH
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

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Before;
import org.junit.Test;

import joynr.infrastructure.DacTypes.MasterRegistrationControlEntry;
import joynr.infrastructure.DacTypes.OwnerRegistrationControlEntry;
import joynr.infrastructure.DacTypes.Permission;
import joynr.infrastructure.DacTypes.TrustLevel;

public class RceValidatorTest {

    private MasterRegistrationControlEntry masterRce;
    private MasterRegistrationControlEntry mediatorRce;
    private OwnerRegistrationControlEntry ownerRce;

    @Before
    public void setup() {
        masterRce = new MasterRegistrationControlEntry(null,
                                                       null,
                                                       null,
                                                       TrustLevel.LOW,
                                                       new TrustLevel[]{ TrustLevel.MID, TrustLevel.LOW },
                                                       TrustLevel.LOW,
                                                       new TrustLevel[]{ TrustLevel.MID, TrustLevel.HIGH },
                                                       Permission.NO,
                                                       new Permission[]{ Permission.ASK, Permission.NO });

        mediatorRce = new MasterRegistrationControlEntry(null,
                                                         null,
                                                         null,
                                                         TrustLevel.LOW,
                                                         new TrustLevel[]{ TrustLevel.MID, TrustLevel.LOW },
                                                         TrustLevel.LOW,
                                                         new TrustLevel[]{ TrustLevel.MID, TrustLevel.HIGH },
                                                         Permission.NO,
                                                         new Permission[]{ Permission.ASK, Permission.NO });

        ownerRce = new OwnerRegistrationControlEntry(null, null, null, TrustLevel.MID, TrustLevel.HIGH, Permission.ASK);
    }

    @Test
    public void testMediatorInvalidPossiblePermissions() {
        mediatorRce.setPossibleProviderPermissions(new Permission[]{ Permission.YES, Permission.ASK });
        RceValidator validator = new RceValidator(masterRce, mediatorRce, ownerRce);
        assertFalse("modified mediator is not valid", validator.isMediatorValid());
    }

    @Test
    public void testMediatorInvalidPossibleTrusLevels() {
        mediatorRce.setPossibleRequiredTrustLevels(new TrustLevel[]{ TrustLevel.HIGH, TrustLevel.MID });
        RceValidator validator = new RceValidator(masterRce, mediatorRce, ownerRce);
        assertFalse("modified mediator is not valid", validator.isMediatorValid());
    }

    @Test
    public void testMediatorValid() {
        RceValidator validator = new RceValidator(masterRce, mediatorRce, ownerRce);
        assertTrue("initial mediator is valid", validator.isMediatorValid());
    }

    @Test
    public void testOwnerValid() {
        RceValidator validator = new RceValidator(masterRce, mediatorRce, ownerRce);
        assertTrue("initial owner is valid", validator.isOwnerValid());
    }

    @Test
    public void testOwnerInvalid() {
        ownerRce.setProviderPermission(Permission.YES);
        RceValidator validator = new RceValidator(masterRce, mediatorRce, ownerRce);
        assertFalse("modified owner is invalid", validator.isOwnerValid());
    }
}
