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

#include "AccessControlAlgorithm.h"
#include "AceValidator.h"
#include "TrustLevelComparator.h"

#include "joynr/infrastructure/DacTypes/MasterAccessControlEntry.h"
#include "joynr/infrastructure/DacTypes/OwnerAccessControlEntry.h"

namespace joynr
{

using namespace infrastructure::DacTypes;

Permission::Enum AccessControlAlgorithm::getConsumerPermission(
        const Optional<MasterAccessControlEntry>& masterOptional,
        const Optional<MasterAccessControlEntry>& mediatorOptional,
        const Optional<OwnerAccessControlEntry>& ownerOptional,
        TrustLevel::Enum trustLevel)
{
    return getPermission(
            PERMISSION_FOR_CONSUMER, masterOptional, mediatorOptional, ownerOptional, trustLevel);
}

Permission::Enum AccessControlAlgorithm::getProviderPermission(
        const Optional<MasterAccessControlEntry>& masterOptional,
        const Optional<MasterAccessControlEntry>& mediatorOptional,
        const Optional<OwnerAccessControlEntry>& ownerOptional,
        TrustLevel::Enum trustLevel)
{
    return getPermission(
            PERMISSION_FOR_PROVIDER, masterOptional, mediatorOptional, ownerOptional, trustLevel);
}

Permission::Enum AccessControlAlgorithm::getPermission(
        AccessControlAlgorithm::PermissionType permissionType,
        const Optional<MasterAccessControlEntry>& masterOptional,
        const Optional<MasterAccessControlEntry>& mediatorOptional,
        const Optional<OwnerAccessControlEntry>& ownerOptional,
        TrustLevel::Enum trustLevel)
{
    AceValidator validator(masterOptional, mediatorOptional, ownerOptional);
    if (!validator.isValid()) {
        return Permission::NO;
    }

    Permission::Enum permission = Permission::Enum::NO;

    if (ownerOptional) {
        OwnerAccessControlEntry ownerAce = ownerOptional.getValue();
        if (TrustLevelComparator::compare(trustLevel, ownerAce.getRequiredTrustLevel()) >= 0) {
            if (permissionType == PERMISSION_FOR_CONSUMER) {
                permission = ownerAce.getConsumerPermission();
            }
        }
    } else if (mediatorOptional) {
        MasterAccessControlEntry mediatorAce = mediatorOptional.getValue();
        if (TrustLevelComparator::compare(trustLevel, mediatorAce.getDefaultRequiredTrustLevel()) >=
            0) {
            if (permissionType == PERMISSION_FOR_CONSUMER) {
                permission = mediatorAce.getDefaultConsumerPermission();
            }
        }
    } else if (masterOptional) {
        MasterAccessControlEntry masterAce = masterOptional.getValue();
        if (TrustLevelComparator::compare(trustLevel, masterAce.getDefaultRequiredTrustLevel()) >=
            0) {
            if (permissionType == PERMISSION_FOR_CONSUMER) {
                permission = masterAce.getDefaultConsumerPermission();
            }
        }
    }

    return permission;
}

} // namespace joynr
