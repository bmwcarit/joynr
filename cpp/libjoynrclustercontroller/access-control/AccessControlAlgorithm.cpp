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

#include "AccessControlAlgorithm.h"

#include "joynr/infrastructure/DacTypes/MasterAccessControlEntry.h"
#include "joynr/infrastructure/DacTypes/MasterRegistrationControlEntry.h"
#include "joynr/infrastructure/DacTypes/OwnerAccessControlEntry.h"
#include "joynr/infrastructure/DacTypes/OwnerRegistrationControlEntry.h"

#include "TrustLevelComparator.h"
#include "Validator.h"

namespace joynr
{

using namespace infrastructure::DacTypes;

Permission::Enum AccessControlAlgorithm::getConsumerPermission(
        const boost::optional<MasterAccessControlEntry>& masterOptional,
        const boost::optional<MasterAccessControlEntry>& mediatorOptional,
        const boost::optional<OwnerAccessControlEntry>& ownerOptional,
        TrustLevel::Enum trustLevel)
{
    return getPermission(
            PERMISSION_FOR_CONSUMER, masterOptional, mediatorOptional, ownerOptional, trustLevel);
}

Permission::Enum AccessControlAlgorithm::getProviderPermission(
        const boost::optional<MasterRegistrationControlEntry>& masterOptional,
        const boost::optional<MasterRegistrationControlEntry>& mediatorOptional,
        const boost::optional<OwnerRegistrationControlEntry>& ownerOptional,
        TrustLevel::Enum trustLevel)
{
    return getPermission(
            PERMISSION_FOR_PROVIDER, masterOptional, mediatorOptional, ownerOptional, trustLevel);
}

Permission::Enum AccessControlAlgorithm::getPermission(
        AccessControlAlgorithm::PermissionType permissionType,
        const boost::optional<MasterAccessControlEntry>& masterOptional,
        const boost::optional<MasterAccessControlEntry>& mediatorOptional,
        const boost::optional<OwnerAccessControlEntry>& ownerOptional,
        TrustLevel::Enum trustLevel)
{
    AceValidator validator(masterOptional, mediatorOptional, ownerOptional);
    if (!validator.isValid()) {
        return Permission::NO;
    }

    Permission::Enum permission = Permission::Enum::NO;

    if (ownerOptional) {
        if (TrustLevelComparator::compare(trustLevel, ownerOptional->getRequiredTrustLevel()) >=
            0) {
            if (permissionType == PERMISSION_FOR_CONSUMER) {
                permission = ownerOptional->getConsumerPermission();
            }
        }
    } else if (mediatorOptional) {
        if (TrustLevelComparator::compare(
                    trustLevel, mediatorOptional->getDefaultRequiredTrustLevel()) >= 0) {
            if (permissionType == PERMISSION_FOR_CONSUMER) {
                permission = mediatorOptional->getDefaultConsumerPermission();
            }
        }
    } else if (masterOptional) {
        if (TrustLevelComparator::compare(
                    trustLevel, masterOptional->getDefaultRequiredTrustLevel()) >= 0) {
            if (permissionType == PERMISSION_FOR_CONSUMER) {
                permission = masterOptional->getDefaultConsumerPermission();
            }
        }
    }

    return permission;
}

Permission::Enum AccessControlAlgorithm::getPermission(
        AccessControlAlgorithm::PermissionType permissionType,
        const boost::optional<MasterRegistrationControlEntry>& masterOptional,
        const boost::optional<MasterRegistrationControlEntry>& mediatorOptional,
        const boost::optional<OwnerRegistrationControlEntry>& ownerOptional,
        TrustLevel::Enum trustLevel)
{
    RceValidator validator(masterOptional, mediatorOptional, ownerOptional);
    if (!validator.isValid()) {
        return Permission::NO;
    }

    Permission::Enum permission = Permission::Enum::NO;

    if (ownerOptional) {
        if (TrustLevelComparator::compare(trustLevel, ownerOptional->getRequiredTrustLevel()) >=
            0) {
            if (permissionType == PERMISSION_FOR_PROVIDER) {
                permission = ownerOptional->getProviderPermission();
            }
        }
    } else if (mediatorOptional) {
        if (TrustLevelComparator::compare(
                    trustLevel, mediatorOptional->getDefaultRequiredTrustLevel()) >= 0) {
            if (permissionType == PERMISSION_FOR_PROVIDER) {
                permission = mediatorOptional->getDefaultProviderPermission();
            }
        }
    } else if (masterOptional) {
        if (TrustLevelComparator::compare(
                    trustLevel, masterOptional->getDefaultRequiredTrustLevel()) >= 0) {
            if (permissionType == PERMISSION_FOR_PROVIDER) {
                permission = masterOptional->getDefaultProviderPermission();
            }
        }
    }

    return permission;
}

} // namespace joynr
