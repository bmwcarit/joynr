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

import java.util.Arrays;
import java.util.List;

import joynr.infrastructure.DacTypes.MasterRegistrationControlEntry;
import joynr.infrastructure.DacTypes.OwnerRegistrationControlEntry;
import joynr.infrastructure.DacTypes.Permission;
import joynr.infrastructure.DacTypes.TrustLevel;

public class RceValidator {

    private MasterRegistrationControlEntry masterRce;
    private MasterRegistrationControlEntry mediatorRce;
    private OwnerRegistrationControlEntry ownerRce;

    public RceValidator(MasterRegistrationControlEntry masterRce,
                        MasterRegistrationControlEntry mediatorRce,
                        OwnerRegistrationControlEntry ownerRce) {
        this.masterRce = masterRce;
        this.mediatorRce = mediatorRce;
        this.ownerRce = ownerRce;
    }

    public boolean isValid() {
        return isOwnerValid();
    }

    public boolean isOwnerValid() {
        boolean isOwnerValid = true;
        if (mediatorRce != null) {
            isOwnerValid = isMediatorValid() && validateOwner(mediatorRce);
        } else {
            if (masterRce != null) {
                isOwnerValid = validateOwner(masterRce);
            }
        }

        return isOwnerValid;
    }

    private boolean validateOwner(MasterRegistrationControlEntry targetMasterRce) {
        if (ownerRce == null) {
            return true;
        }

        boolean isValid = true;
        List<Permission> masterRcePossibleProviderPermissions = Arrays.asList(targetMasterRce.getPossibleProviderPermissions());
        List<TrustLevel> masterRcePossibleRequiredTrustLevels = Arrays.asList(targetMasterRce.getPossibleRequiredTrustLevels());
        if (!masterRcePossibleProviderPermissions.contains(ownerRce.getProviderPermission())) {
            isValid = false;
        } else if (!masterRcePossibleRequiredTrustLevels.contains(ownerRce.getRequiredTrustLevel())) {
            isValid = false;
        }

        return isValid;
    }

    public boolean isMediatorValid() {
        if (mediatorRce == null) {
            return true;
        }

        // if mediator RCE is not null and master RCE is null, mediator is valid
        if (masterRce == null) {
            return true;
        }

        boolean isMediatorValid = true;
        List<Permission> masterRcePossibleProviderPermissions = Arrays.asList(masterRce.getPossibleProviderPermissions());
        List<Permission> mediatorRcePossibleProviderPermissions = Arrays.asList(mediatorRce.getPossibleProviderPermissions());
        List<TrustLevel> masterRcePossibleRequiredTrustLevels = Arrays.asList(masterRce.getPossibleRequiredTrustLevels());
        List<TrustLevel> mediatorRcePossibleRequiredTrustLevels = Arrays.asList(mediatorRce.getPossibleRequiredTrustLevels());

        if (!masterRcePossibleProviderPermissions.contains(mediatorRce.getDefaultProviderPermission())) {
            isMediatorValid = false;
        } else if (!masterRcePossibleProviderPermissions.containsAll(mediatorRcePossibleProviderPermissions)) {
            isMediatorValid = false;
        } else if (!masterRcePossibleRequiredTrustLevels.contains(mediatorRce.getDefaultRequiredTrustLevel())) {
            isMediatorValid = false;
        } else if (!masterRcePossibleRequiredTrustLevels.containsAll(mediatorRcePossibleRequiredTrustLevels)) {
            isMediatorValid = false;
        }

        return isMediatorValid;
    }
}
