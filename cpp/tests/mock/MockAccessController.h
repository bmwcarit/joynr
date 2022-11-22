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
#ifndef TESTS_MOCK_MOCKACCESSCONTROLLER_H
#define TESTS_MOCK_MOCKACCESSCONTROLLER_H

#include "tests/utils/Gmock.h"

#include "libjoynr/basemodel/generated/include/joynr/infrastructure/DacTypes/Permission.h"
#include "libjoynr/basemodel/generated/include/joynr/infrastructure/DacTypes/TrustLevel.h"
#include "libjoynrclustercontroller/access-control/AccessController.h"

class MockAccessController : public joynr::AccessController
{
public:
    MockAccessController(
            std::shared_ptr<joynr::LocalCapabilitiesDirectory> localCapabilitiesDirectory = nullptr,
            std::shared_ptr<joynr::LocalDomainAccessStore> localDomainAccessStore = nullptr)
            : AccessController(localCapabilitiesDirectory, localDomainAccessStore)
    {
    }
    MockAccessController(std::shared_ptr<joynr::LocalDomainAccessStore> localDomainAccessStore)
            : AccessController(nullptr, localDomainAccessStore)
    {
    }

    MOCK_METHOD3(
            hasConsumerPermission,
            void(std::shared_ptr<joynr::ImmutableMessage> message,
                 std::shared_ptr<joynr::IAccessController::IHasConsumerPermissionCallback> callback,
                 bool isLocalRecipient));

    MOCK_METHOD4(hasProviderPermission,
                 bool(const std::string& userId,
                      joynr::infrastructure::DacTypes::TrustLevel::Enum trustLevel,
                      const std::string& domain,
                      const std::string& interfaceName));

    MOCK_METHOD1(addParticipantToWhitelist, void(const std::string& participantId));

    MOCK_METHOD3(hasRole,
                 bool(const std::string& userId,
                      const std::string& domain,
                      joynr::infrastructure::DacTypes::Role::Enum role));
};

#endif // TESTS_MOCK_MOCKACCESSCONTROLLER_H
