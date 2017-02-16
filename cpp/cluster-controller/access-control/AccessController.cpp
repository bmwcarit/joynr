/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

#include "AccessController.h"

#include <tuple>

#include "LocalDomainAccessController.h"
#include "joynr/LocalCapabilitiesDirectory.h"
#include "joynr/JoynrMessage.h"
#include "joynr/types/DiscoveryEntry.h"
#include "joynr/Request.h"
#include "joynr/SubscriptionRequest.h"
#include "joynr/BroadcastSubscriptionRequest.h"
#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/serializer/Serializer.h"

namespace joynr
{

using namespace infrastructure;
using namespace infrastructure::DacTypes;
using namespace types;

INIT_LOGGER(AccessController);

//--------- InternalConsumerPermissionCallbacks --------------------------------

class AccessController::LdacConsumerPermissionCallback
        : public LocalDomainAccessController::IGetConsumerPermissionCallback
{

public:
    LdacConsumerPermissionCallback(
            AccessController& owningAccessController,
            const JoynrMessage& message,
            const std::string& domain,
            const std::string& interfaceName,
            TrustLevel::Enum trustlevel,
            std::shared_ptr<IAccessController::IHasConsumerPermissionCallback> callback);

    // Callbacks made from the LocalDomainAccessController
    void consumerPermission(Permission::Enum permission) override;
    void operationNeeded() override;

private:
    AccessController& owningAccessController;
    JoynrMessage message;
    std::string domain;
    std::string interfaceName;
    TrustLevel::Enum trustlevel;
    std::shared_ptr<IAccessController::IHasConsumerPermissionCallback> callback;

    bool convertToBool(Permission::Enum permission);
};

AccessController::LdacConsumerPermissionCallback::LdacConsumerPermissionCallback(
        AccessController& parent,
        const JoynrMessage& message,
        const std::string& domain,
        const std::string& interfaceName,
        TrustLevel::Enum trustlevel,
        std::shared_ptr<IAccessController::IHasConsumerPermissionCallback> callback)
        : owningAccessController(parent),
          message(message),
          domain(domain),
          interfaceName(interfaceName),
          trustlevel(trustlevel),
          callback(callback)
{
}

void AccessController::LdacConsumerPermissionCallback::consumerPermission(
        Permission::Enum permission)
{
    bool hasPermission = convertToBool(permission);

    if (!hasPermission) {
        JOYNR_LOG_ERROR(owningAccessController.logger,
                        "Message {} to domain {}, interface {} failed ACL check",
                        message.getHeaderMessageId(),
                        domain,
                        interfaceName);
    }
    callback->hasConsumerPermission(hasPermission);
}

void AccessController::LdacConsumerPermissionCallback::operationNeeded()
{
    std::string operation;
    const std::string messageType = message.getType();
    if (messageType == JoynrMessage::VALUE_MESSAGE_TYPE_ONE_WAY) {
        try {
            OneWayRequest request;
            joynr::serializer::deserializeFromJson(request, message.getPayload());
            operation = request.getMethodName();
        } catch (const std::exception& e) {
            JOYNR_LOG_ERROR(logger, "could not deserialize OneWayRequest - error {}", e.what());
        }
    } else if (messageType == JoynrMessage::VALUE_MESSAGE_TYPE_REQUEST) {
        try {
            Request request;
            joynr::serializer::deserializeFromJson(request, message.getPayload());
            operation = request.getMethodName();
        } catch (const std::exception& e) {
            JOYNR_LOG_ERROR(logger, "could not deserialize Request - error {}", e.what());
        }
    } else if (messageType == JoynrMessage::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST) {
        try {
            SubscriptionRequest request;
            joynr::serializer::deserializeFromJson(request, message.getPayload());
            operation = request.getSubscribeToName();

        } catch (const std::invalid_argument& e) {
            JOYNR_LOG_ERROR(
                    logger, "could not deserialize SubscriptionRequest - error {}", e.what());
        }
    } else if (messageType == JoynrMessage::VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST) {
        try {
            BroadcastSubscriptionRequest request;
            joynr::serializer::deserializeFromJson(request, message.getPayload());
            operation = request.getSubscribeToName();

        } catch (const std::invalid_argument& e) {
            JOYNR_LOG_ERROR(logger,
                            "could not deserialize BroadcastSubscriptionRequest - error {}",
                            e.what());
        }
    }

    if (operation.empty()) {
        JOYNR_LOG_ERROR(owningAccessController.logger, "Could not deserialize request");
        callback->hasConsumerPermission(false);
        return;
    }

    // Get the permission for given operation
    Permission::Enum permission =
            owningAccessController.localDomainAccessController.getConsumerPermission(
                    message.getHeaderCreatorUserId(), domain, interfaceName, operation, trustlevel);

    bool hasPermission = convertToBool(permission);

    if (!hasPermission) {
        JOYNR_LOG_ERROR(owningAccessController.logger,
                        "Message {} to domain {}, interface/operation {}/{} failed ACL check",
                        message.getHeaderMessageId(),
                        domain,
                        interfaceName,
                        operation);
    }

    callback->hasConsumerPermission(hasPermission);
}

bool AccessController::LdacConsumerPermissionCallback::convertToBool(Permission::Enum permission)
{
    switch (permission) {
    case Permission::YES:
        return true;
    case Permission::ASK:
        assert(false && "Permission.ASK user dialog not yet implemented.");
        return false;
    case Permission::NO:
        return false;
    default:
        return false;
    }
}

//--------- AccessController ---------------------------------------------------

class AccessController::ProviderRegistrationObserver
        : public LocalCapabilitiesDirectory::IProviderRegistrationObserver
{
public:
    explicit ProviderRegistrationObserver(LocalDomainAccessController& localDomainAccessController)
            : localDomainAccessController(localDomainAccessController)
    {
    }
    void onProviderAdd(const DiscoveryEntry& discoveryEntry) override
    {
        std::ignore = discoveryEntry;
        // Ignored
    }

    void onProviderRemove(const DiscoveryEntry& discoveryEntry) override
    {
        localDomainAccessController.unregisterProvider(
                discoveryEntry.getDomain(), discoveryEntry.getInterfaceName());
    }

private:
    LocalDomainAccessController& localDomainAccessController;
};

AccessController::AccessController(LocalCapabilitiesDirectory& localCapabilitiesDirectory,
                                   LocalDomainAccessController& localDomainAccessController)
        : localCapabilitiesDirectory(localCapabilitiesDirectory),
          localDomainAccessController(localDomainAccessController),
          providerRegistrationObserver(
                  std::make_shared<ProviderRegistrationObserver>(localDomainAccessController)),
          whitelistParticipantIds()
{
    localCapabilitiesDirectory.addProviderRegistrationObserver(providerRegistrationObserver);
}

AccessController::~AccessController()
{
    localCapabilitiesDirectory.removeProviderRegistrationObserver(providerRegistrationObserver);
}

void AccessController::addParticipantToWhitelist(const std::string& participantId)
{
    whitelistParticipantIds.push_back(participantId);
}

bool AccessController::needsPermissionCheck(const JoynrMessage& message)
{
    if (util::vectorContains(whitelistParticipantIds, message.getHeaderTo())) {
        return false;
    }

    std::string messageType = message.getType();
    if (messageType == JoynrMessage::VALUE_MESSAGE_TYPE_REPLY ||
        messageType == JoynrMessage::VALUE_MESSAGE_TYPE_PUBLICATION) {
        // reply messages don't need permission check
        // they are filtered by request reply ID or subscritpion ID
        return false;
    }

    // If this point is reached, checking is required
    return true;
}

void AccessController::hasConsumerPermission(
        const JoynrMessage& message,
        std::shared_ptr<IAccessController::IHasConsumerPermissionCallback> callback)
{
    if (!needsPermissionCheck(message)) {
        callback->hasConsumerPermission(true);
        return;
    }

    // Get the domain and interface of the message destination
    std::string participantId = message.getHeaderTo();
    std::function<void(const types::DiscoveryEntry&)> lookupSuccessCallback =
            [this, message, callback, participantId](const types::DiscoveryEntry& discoveryEntry) {
        if (discoveryEntry.getParticipantId() != participantId) {
            JOYNR_LOG_ERROR(
                    logger, "Failed to get capabilities for participantId {}", participantId);
            callback->hasConsumerPermission(false);
            return;
        }

        std::string domain = discoveryEntry.getDomain();
        std::string interfaceName = discoveryEntry.getInterfaceName();

        // Create a callback object
        auto ldacCallback = std::make_shared<LdacConsumerPermissionCallback>(
                *this, message, domain, interfaceName, TrustLevel::HIGH, callback);

        // Try to determine permission without expensive message deserialization
        // For now TrustLevel::HIGH is assumed.
        std::string msgCreatorUid = message.getHeaderCreatorUserId();
        localDomainAccessController.getConsumerPermission(
                msgCreatorUid, domain, interfaceName, TrustLevel::HIGH, ldacCallback);
    };

    std::function<void(const joynr::exceptions::ProviderRuntimeException&)> lookupErrorCallback =
            [callback](const joynr::exceptions::ProviderRuntimeException& exception) {
        std::ignore = exception;
        callback->hasConsumerPermission(false);
    };
    localCapabilitiesDirectory.lookup(participantId, lookupSuccessCallback, lookupErrorCallback);
}

bool AccessController::hasProviderPermission(const std::string& userId,
                                             TrustLevel::Enum trustLevel,
                                             const std::string& domain,
                                             const std::string& interfaceName)
{
    std::ignore = userId;
    std::ignore = trustLevel;
    std::ignore = domain;
    std::ignore = interfaceName;

    assert(false && "Not yet implemented.");
    return true;
}

} // namespace joynr
