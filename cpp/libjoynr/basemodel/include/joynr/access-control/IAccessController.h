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

#ifndef IACCESSCONTROLLER_H
#define IACCESSCONTROLLER_H

#include <memory>
#include <string>

#include "joynr/JoynrExport.h"
#include "joynr/infrastructure/DacTypes/Role.h"
#include "joynr/infrastructure/DacTypes/TrustLevel.h"

namespace joynr
{

class ImmutableMessage;

/**
 * Interface for objects that control access to providers
 */
class JOYNR_EXPORT IAccessController
{
public:
    /**
     * Enum to be used as an argument in hasConsumerPermission callback for three cases
     */
    enum class Enum { YES = 0, NO = 1, RETRY = 2 };
    /**
     * Callback interface for hasConsumerPermission
     *
     * Calls to hasConsumerPermission are asynchronous and a callback object must
     * be supplied.
     */
    class IHasConsumerPermissionCallback
    {
    public:
        virtual ~IHasConsumerPermissionCallback() = default;

        /**
         * Called with the result of hasConsumerPermission
         */
        virtual void hasConsumerPermission(IAccessController::Enum hasPermission) = 0;
    };

    virtual ~IAccessController() = default;

    /**
     * Does the given request message have permission to reach the provider?
     *
     * @param message The message to check
     * @param callback An object that will be called back with the result
     * @param isLocalRecipient true in case of local destination address, false otherwise
     */
    virtual void hasConsumerPermission(std::shared_ptr<ImmutableMessage> message,
                                       std::shared_ptr<IHasConsumerPermissionCallback> callback,
                                       bool isLocalRecipient) = 0;

    /**
     * Does the provider with given userId and given trust level, have permission to expose given
     *domain interface?
     *
     * @param userId The provider userId
     * @param trustLevel The trustLevel for given userId
     * @param domain The domain where provider interface belongs to
     * @param interfaceName The interface provider wants to register
     * @return true if the message has permission, false otherwise
     */
    virtual bool hasProviderPermission(const std::string& userId,
                                       infrastructure::DacTypes::TrustLevel::Enum trustLevel,
                                       const std::string& domain,
                                       const std::string& interfaceName) = 0;
    /**
     * @brief addParticipantToWhitelist Adds a participant to the internal
     * whitelist. Access control to participants on the whitelist is always
     * granted.
     *
     * @param participantId the participant ID to add to the whitelist.
     */
    virtual void addParticipantToWhitelist(const std::string& participantId) = 0;
};

} // namespace joynr
#endif // IACCESSCONTROLLER_H
