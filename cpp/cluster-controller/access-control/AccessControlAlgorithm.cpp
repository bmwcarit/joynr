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

#include "joynr/infrastructure/QtMasterAccessControlEntry.h"
#include "joynr/infrastructure/QtOwnerAccessControlEntry.h"

#include <cassert>

namespace joynr
{

using namespace infrastructure;

AccessControlAlgorithm::AccessControlAlgorithm()
{
}

AccessControlAlgorithm::~AccessControlAlgorithm()
{
}

QtPermission::Enum AccessControlAlgorithm::getConsumerPermission(
        const Optional<QtMasterAccessControlEntry>& masterOptional,
        const Optional<QtMasterAccessControlEntry>& mediatorOptional,
        const Optional<QtOwnerAccessControlEntry>& ownerOptional,
        QtTrustLevel::Enum trustLevel)
{
    return getPermission(
            PERMISSION_FOR_CONSUMER, masterOptional, mediatorOptional, ownerOptional, trustLevel);
}

QtPermission::Enum AccessControlAlgorithm::getProviderPermission(
        const Optional<QtMasterAccessControlEntry>& masterOptional,
        const Optional<QtMasterAccessControlEntry>& mediatorOptional,
        const Optional<QtOwnerAccessControlEntry>& ownerOptional,
        QtTrustLevel::Enum trustLevel)
{
    return getPermission(
            PERMISSION_FOR_PROVIDER, masterOptional, mediatorOptional, ownerOptional, trustLevel);
}

QtPermission::Enum AccessControlAlgorithm::getPermission(
        AccessControlAlgorithm::PermissionType permissionType,
        const Optional<QtMasterAccessControlEntry>& masterOptional,
        const Optional<QtMasterAccessControlEntry>& mediatorOptional,
        const Optional<QtOwnerAccessControlEntry>& ownerOptional,
        QtTrustLevel::Enum trustLevel)
{
    AceValidator validator(masterOptional, mediatorOptional, ownerOptional);
    if (!validator.isValid()) {
        return QtPermission::NO;
    }

    QtPermission::Enum permission = QtPermission::Enum::NO;

    if (ownerOptional) {
        QtOwnerAccessControlEntry ownerAce = ownerOptional.getValue();
        if (TrustLevelComparator::compare(trustLevel, ownerAce.getRequiredTrustLevel()) >= 0) {
            if (permissionType == PERMISSION_FOR_CONSUMER) {
                permission = ownerAce.getConsumerPermission();
            }
        }
    } else if (mediatorOptional) {
        QtMasterAccessControlEntry mediatorAce = mediatorOptional.getValue();
        if (TrustLevelComparator::compare(trustLevel, mediatorAce.getDefaultRequiredTrustLevel()) >=
            0) {
            if (permissionType == PERMISSION_FOR_CONSUMER) {
                permission = mediatorAce.getDefaultConsumerPermission();
            }
        }
    } else if (masterOptional) {
        QtMasterAccessControlEntry masterAce = masterOptional.getValue();
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
