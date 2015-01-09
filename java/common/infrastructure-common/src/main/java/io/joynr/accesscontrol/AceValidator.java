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

public class AceValidator {

    private MasterAccessControlEntry masterAce;
    private MasterAccessControlEntry mediatorAce;
    private OwnerAccessControlEntry ownerAce;

    public AceValidator(MasterAccessControlEntry masterAce,
                        MasterAccessControlEntry mediatorAce,
                        OwnerAccessControlEntry ownerAce) {
        this.masterAce = masterAce;
        this.mediatorAce = mediatorAce;
        this.ownerAce = ownerAce;
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
        if (!targetMasterAce.getPossibleConsumerPermissions().contains(ownerAce.getConsumerPermission())) {
            isValid = false;
        } else if (!targetMasterAce.getPossibleRequiredTrustLevels().contains(ownerAce.getRequiredTrustLevel())) {
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
        if (!masterAce.getPossibleConsumerPermissions().contains(mediatorAce.getDefaultConsumerPermission())) {
            isMediatorValid = false;
        } else if (!masterAce.getPossibleConsumerPermissions()
                             .containsAll(mediatorAce.getPossibleConsumerPermissions())) {
            isMediatorValid = false;
        } else if (!masterAce.getPossibleRequiredTrustLevels().contains(mediatorAce.getDefaultRequiredTrustLevel())) {
            isMediatorValid = false;
        } else if (!masterAce.getPossibleRequiredTrustLevels()
                             .containsAll(mediatorAce.getPossibleRequiredTrustLevels())) {
            isMediatorValid = false;
        }

        return isMediatorValid;
    }
}
