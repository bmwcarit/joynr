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

#ifndef ACCESSCONTROLALGORITHM_H
#define ACCESSCONTROLALGORITHM_H

#include <boost/optional.hpp>

#include "joynr/JoynrClusterControllerExport.h"
#include "joynr/infrastructure/DacTypes/Permission.h"
#include "joynr/infrastructure/DacTypes/TrustLevel.h"

namespace joynr
{
namespace infrastructure
{

namespace DacTypes
{
class MasterAccessControlEntry;
class MasterRegistrationControlEntry;
class OwnerAccessControlEntry;
class OwnerRegistrationControlEntry;
} // namespace DacTypes
} // namespace infrastructure

/**
 * Algorithm that decides if a provider/consumer has access to a service
 */
class JOYNRCLUSTERCONTROLLER_EXPORT AccessControlAlgorithm
{
public:
    AccessControlAlgorithm() = default;
    virtual ~AccessControlAlgorithm() = default;

    /**
     * Get the consumer permission for given combination of control entries and with the given trust
     *level.
     *
     * @param master The master access control entry
     * @param mediator The mediator access control entry
     * @param owner The owner access control entry
     * @param trustLevel The trust level of the user sending the message
     * @return The permission
     */
    virtual infrastructure::DacTypes::Permission::Enum getConsumerPermission(
            const boost::optional<infrastructure::DacTypes::MasterAccessControlEntry>&
                    masterOptional,
            const boost::optional<infrastructure::DacTypes::MasterAccessControlEntry>&
                    mediatorOptional,
            const boost::optional<infrastructure::DacTypes::OwnerAccessControlEntry>& ownerOptional,
            infrastructure::DacTypes::TrustLevel::Enum trustLevel);

    /**
     * Get the permission for a Provider.
     *
     * @param master A list containing a single matching master entry or an empty list
     * @param mediator A list containing a single mediator entry or an empty list
     * @param owner A list containing a single owner entry or an empty list
     * @param trustLevel The trust level of the user sending the message
     * @return Always Permission::YES
     */
    virtual infrastructure::DacTypes::Permission::Enum getProviderPermission(
            const boost::optional<infrastructure::DacTypes::MasterRegistrationControlEntry>&
                    masterOptional,
            const boost::optional<infrastructure::DacTypes::MasterRegistrationControlEntry>&
                    mediatorOptional,
            const boost::optional<infrastructure::DacTypes::OwnerRegistrationControlEntry>&
                    ownerOptional,
            infrastructure::DacTypes::TrustLevel::Enum trustLevel);

private:
    enum PermissionType { PERMISSION_FOR_CONSUMER, PERMISSION_FOR_PROVIDER };

    infrastructure::DacTypes::Permission::Enum getPermission(
            PermissionType permissionType,
            const boost::optional<infrastructure::DacTypes::MasterAccessControlEntry>&
                    masterOptional,
            const boost::optional<infrastructure::DacTypes::MasterAccessControlEntry>&
                    mediatorOptional,
            const boost::optional<infrastructure::DacTypes::OwnerAccessControlEntry>& ownerOptional,
            infrastructure::DacTypes::TrustLevel::Enum trustLevel);

    infrastructure::DacTypes::Permission::Enum getPermission(
            PermissionType permissionType,
            const boost::optional<infrastructure::DacTypes::MasterRegistrationControlEntry>&
                    masterOptional,
            const boost::optional<infrastructure::DacTypes::MasterRegistrationControlEntry>&
                    mediatorOptional,
            const boost::optional<infrastructure::DacTypes::OwnerRegistrationControlEntry>&
                    ownerOptional,
            infrastructure::DacTypes::TrustLevel::Enum trustLevel);
};

} // namespace joynr
#endif // ACCESSCONTROLALGORITHM_H
