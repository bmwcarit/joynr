/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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
#ifndef TESTS_MOCK_MOCKLOCALDOMAINACCESSCONTROLLER_H
#define TESTS_MOCK_MOCKLOCALDOMAINACCESSCONTROLLER_H

#include "tests/utils/Gmock.h"

#include "libjoynrclustercontroller/access-control/LocalDomainAccessController.h"

class MockLocalDomainAccessController : public joynr::LocalDomainAccessController {
public:
    using joynr::LocalDomainAccessController::LocalDomainAccessController;

    MOCK_METHOD5(getConsumerPermission,
                 void(
                     const std::string& userId,
                     const std::string& domain,
                     const std::string& interfaceName,
                     joynr::infrastructure::DacTypes::TrustLevel::Enum trustLevel,
                     std::shared_ptr<joynr::LocalDomainAccessController::IGetPermissionCallback> callback));

    MOCK_METHOD5(getConsumerPermission,
                 joynr::infrastructure::DacTypes::Permission::Enum(
                     const std::string& userId,
                     const std::string& domain,
                     const std::string& interfaceName,
                     const std::string& operation,
                     joynr::infrastructure::DacTypes::TrustLevel::Enum trustLevel));
    MOCK_METHOD5(getProviderPermission,
                 void(
                     const std::string& userId,
                     const std::string& domain,
                     const std::string& interfaceName,
                     joynr::infrastructure::DacTypes::TrustLevel::Enum trustLevel,
                     std::shared_ptr<joynr::LocalDomainAccessController::IGetPermissionCallback> callback));

    MOCK_METHOD4(getProviderPermission,
                 joynr::infrastructure::DacTypes::Permission::Enum(
                     const std::string& userId,
                     const std::string& domain,
                     const std::string& interfaceName,
                     joynr::infrastructure::DacTypes::TrustLevel::Enum trustLevel));

    MOCK_METHOD3(hasRole,
                 bool(
                     const std::string& userId,
                     const std::string& domain,
                     joynr::infrastructure::DacTypes::Role::Enum role));
};

#endif // TESTS_MOCK_MOCKLOCALDOMAINACCESSCONTROLLER_H
