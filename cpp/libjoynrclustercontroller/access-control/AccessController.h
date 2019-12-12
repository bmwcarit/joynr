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

#ifndef ACCESSCONTROLLER_H
#define ACCESSCONTROLLER_H

#include <memory>
#include <string>
#include <vector>

#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/access-control/IAccessController.h"
#include "joynr/infrastructure/DacTypes/TrustLevel.h"
#include "joynr/types/DiscoveryQos.h"

namespace joynr
{

class ImmutableMessage;
class LocalCapabilitiesDirectory;
class LocalDomainAccessController;

/**
 * Object that controls access to providers
 */
class AccessController : public IAccessController,
                         public std::enable_shared_from_this<AccessController>
{
public:
    AccessController(std::shared_ptr<LocalCapabilitiesDirectory> localCapabilitiesDirectory,
                     std::shared_ptr<LocalDomainAccessController> localDomainAccessController);

    ~AccessController() override;

    //---IAccessController interface -------------------------------------------

    void hasConsumerPermission(std::shared_ptr<ImmutableMessage> message,
                               std::shared_ptr<IHasConsumerPermissionCallback> callback) override;

    bool hasProviderPermission(const std::string& userId,
                               infrastructure::DacTypes::TrustLevel::Enum trustLevel,
                               const std::string& domain,
                               const std::string& interfaceName) override;

    void addParticipantToWhitelist(const std::string& participantId) override;

private:
    class LdacConsumerPermissionCallback;
    class ProviderRegistrationObserver;

    DISALLOW_COPY_AND_ASSIGN(AccessController);
    bool needsHasConsumerPermissionCheck(const ImmutableMessage& message) const;
    bool needsHasProviderPermissionCheck() const;

    std::shared_ptr<LocalCapabilitiesDirectory> _localCapabilitiesDirectory;
    std::shared_ptr<LocalDomainAccessController> _localDomainAccessController;
    std::shared_ptr<ProviderRegistrationObserver> _providerRegistrationObserver;
    std::vector<std::string> _whitelistParticipantIds;
    types::DiscoveryQos _discoveryQos;

    ADD_LOGGER(AccessController)
};

} // namespace joynr
#endif // IACCESSCONTROLLER_H
