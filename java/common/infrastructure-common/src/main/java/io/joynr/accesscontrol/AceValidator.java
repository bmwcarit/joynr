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

import java.util.Arrays;
import java.util.List;

import joynr.infrastructure.DacTypes.MasterAccessControlEntry;
import joynr.infrastructure.DacTypes.OwnerAccessControlEntry;
import joynr.infrastructure.DacTypes.Permission;
import joynr.infrastructure.DacTypes.TrustLevel;

public class AceValidator {

    private MasterAccessControlEntry masterAce;
    private MasterAccessControlEntry mediatorAce;
    private OwnerAccessControlEntry ownerAce;

    public AceValidator(MasterAccessControlEntry masterAce,
                        MasterAccessControlEntry mediatorAce,
                        OwnerAccessControlEntry ownerAce) {
        this.masterAce = (masterAce != null) ? new MasterAccessControlEntry(masterAce) : null;
        this.mediatorAce = (mediatorAce != null) ? new MasterAccessControlEntry(mediatorAce) : null;
        this.ownerAce = (ownerAce != null) ? new OwnerAccessControlEntry(ownerAce) : null;
    }

    public boolean isValid() {
        return isOwnerValid();
    }

    public boolean isOwnerValid() {
        boolean isOwnerValid = true;
        if (mediatorAce != null) {
            isOwnerValid = isMediatorValid() && validateOwner(mediatorAce);
        } else {
            if (masterAce != null) {
                isOwnerValid = validateOwner(masterAce);
            }
        }

        return isOwnerValid;
    }

    private boolean validateOwner(MasterAccessControlEntry targetMasterAce) {
        if (ownerAce == null) {
            return true;
        }

        boolean isValid = true;
        List<Permission> masterAcePossibleConsumerPermissions = Arrays.asList(targetMasterAce.getPossibleConsumerPermissions());
        List<TrustLevel> masterAcePossibleRequiredTrustLevels = Arrays.asList(targetMasterAce.getPossibleRequiredTrustLevels());
        if (!masterAcePossibleConsumerPermissions.contains(ownerAce.getConsumerPermission())) {
            isValid = false;
        } else if (!masterAcePossibleRequiredTrustLevels.contains(ownerAce.getRequiredTrustLevel())) {
            isValid = false;
        }

        return isValid;
    }

    public boolean isMediatorValid() {
        if (mediatorAce == null) {
            return true;
        }

        // if mediator ACE is not null and master ACE is null, mediator is valid
        if (masterAce == null) {
            return true;
        }

        boolean isMediatorValid = true;
        List<Permission> masterAcePossibleConsumerPermissions = Arrays.asList(masterAce.getPossibleConsumerPermissions());
        List<Permission> mediatorAcePossibleConsumerPermissions = Arrays.asList(mediatorAce.getPossibleConsumerPermissions());
        List<TrustLevel> masterAcePossibleRequiredTrustLevels = Arrays.asList(masterAce.getPossibleRequiredTrustLevels());
        List<TrustLevel> mediatorAcePossibleRequiredTrustLevels = Arrays.asList(mediatorAce.getPossibleRequiredTrustLevels());

        if (!masterAcePossibleConsumerPermissions.contains(mediatorAce.getDefaultConsumerPermission())) {
            isMediatorValid = false;
        } else if (!masterAcePossibleConsumerPermissions.containsAll(mediatorAcePossibleConsumerPermissions)) {
            isMediatorValid = false;
        } else if (!masterAcePossibleRequiredTrustLevels.contains(mediatorAce.getDefaultRequiredTrustLevel())) {
            isMediatorValid = false;
        } else if (!masterAcePossibleRequiredTrustLevels.containsAll(mediatorAcePossibleRequiredTrustLevels)) {
            isMediatorValid = false;
        }

        return isMediatorValid;
    }
}
