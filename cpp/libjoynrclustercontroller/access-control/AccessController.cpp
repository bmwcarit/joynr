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

#include "AccessController.h"

#include <cassert>
#include <stdexcept>
#include <tuple>

#include "joynr/BroadcastSubscriptionRequest.h"
#include "joynr/ImmutableMessage.h"
#include "joynr/LocalCapabilitiesDirectory.h"
#include "joynr/Message.h"
#include "joynr/MulticastSubscriptionRequest.h"
#include "joynr/OneWayRequest.h"
#include "joynr/Request.h"
#include "joynr/SubscriptionRequest.h"
#include "joynr/Util.h"
#include "joynr/infrastructure/DacTypes/Permission.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/types/DiscoveryEntry.h"
#include "joynr/types/DiscoveryEntryWithMetaInfo.h"
#include "joynr/types/DiscoveryError.h"
#include "joynr/types/DiscoveryScope.h"

#include "ClusterControllerCallContext.h"
#include "ClusterControllerCallContextStorage.h"

#include "LocalDomainAccessStore.h"

namespace joynr
{

using namespace infrastructure;
using namespace infrastructure::DacTypes;
using namespace types;

//--------- InternalConsumerPermissionCallbacks --------------------------------

class AccessController::LdacConsumerPermissionCallback
        : public AccessController::IGetPermissionCallback
{

public:
    LdacConsumerPermissionCallback(
            AccessController& owningAccessController,
            std::shared_ptr<ImmutableMessage> message,
            const std::string& domain,
            const std::string& interfaceName,
            TrustLevel::Enum trustlevel,
            std::shared_ptr<IAccessController::IHasConsumerPermissionCallback> _callback);

    void permission(Permission::Enum permission) override;
    void operationNeeded() override;

private:
    AccessController& _owningAccessController;
    std::shared_ptr<ImmutableMessage> _message;
    std::string _domain;
    std::string _interfaceName;
    TrustLevel::Enum _trustlevel;
    std::shared_ptr<IAccessController::IHasConsumerPermissionCallback> _callback;
};

AccessController::LdacConsumerPermissionCallback::LdacConsumerPermissionCallback(
        AccessController& owningAccessController,
        std::shared_ptr<ImmutableMessage> message,
        const std::string& domain,
        const std::string& interfaceName,
        TrustLevel::Enum trustlevel,
        std::shared_ptr<IAccessController::IHasConsumerPermissionCallback> callback)
        : _owningAccessController(owningAccessController),
          _message(std::move(message)),
          _domain(domain),
          _interfaceName(interfaceName),
          _trustlevel(trustlevel),
          _callback(callback)
{
}

void AccessController::LdacConsumerPermissionCallback::permission(Permission::Enum permission)
{
    assert(permission != Permission::ASK && "Permission.ASK user dialog not yet implemented.");
    // hasPermission set to NO if permission is NO or ASK
    IAccessController::Enum hasPermission = IAccessController::Enum::NO;
    if (permission == Permission::Enum::YES) {
        hasPermission = IAccessController::Enum::YES;
    }

    if (hasPermission == IAccessController::Enum::NO) {
        JOYNR_LOG_ERROR(_owningAccessController.logger(),
                        "Message {} to domain {}, interface {} from creator {} failed ACL check",
                        _message->getId(),
                        _domain,
                        _interfaceName,
                        _message->getCreator());
    }
    _callback->hasConsumerPermission(hasPermission);
}

void AccessController::LdacConsumerPermissionCallback::operationNeeded()
{
    // we only support operation-level ACL for unencrypted messages

    assert(!_message->isEncrypted());
    std::string operation;
    const std::string& messageType = _message->getType();
    if (messageType == Message::VALUE_MESSAGE_TYPE_ONE_WAY()) {
        try {
            OneWayRequest request;
            joynr::serializer::deserializeFromJson(request, _message->getUnencryptedBody());
            operation = request.getMethodName();
        } catch (const std::exception& e) {
            JOYNR_LOG_ERROR(logger(), "could not deserialize OneWayRequest - error {}", e.what());
        }
    } else if (messageType == Message::VALUE_MESSAGE_TYPE_REQUEST()) {
        try {
            Request request;
            joynr::serializer::deserializeFromJson(request, _message->getUnencryptedBody());
            operation = request.getMethodName();
        } catch (const std::exception& e) {
            JOYNR_LOG_ERROR(logger(), "could not deserialize Request - error {}", e.what());
        }
    } else if (messageType == Message::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST()) {
        try {
            SubscriptionRequest request;
            joynr::serializer::deserializeFromJson(request, _message->getUnencryptedBody());
            operation = request.getSubscribeToName();

        } catch (const std::invalid_argument& e) {
            JOYNR_LOG_ERROR(
                    logger(), "could not deserialize SubscriptionRequest - error {}", e.what());
        }
    } else if (messageType == Message::VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST()) {
        try {
            BroadcastSubscriptionRequest request;
            joynr::serializer::deserializeFromJson(request, _message->getUnencryptedBody());
            operation = request.getSubscribeToName();

        } catch (const std::invalid_argument& e) {
            JOYNR_LOG_ERROR(logger(),
                            "could not deserialize BroadcastSubscriptionRequest - error {}",
                            e.what());
        }
    } else if (messageType == Message::VALUE_MESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST()) {
        try {
            MulticastSubscriptionRequest request;
            joynr::serializer::deserializeFromJson(request, _message->getUnencryptedBody());
            operation = request.getSubscribeToName();
        } catch (const std::invalid_argument& e) {
            JOYNR_LOG_ERROR(logger(),
                            "could not deserialize MulticastSubscriptionRequest - error {}",
                            e.what());
        }
    }

    if (operation.empty()) {
        JOYNR_LOG_ERROR(_owningAccessController.logger(), "Could not deserialize request");
        _callback->hasConsumerPermission(IAccessController::Enum::NO);
        return;
    }

    // Get the permission for given operation
    Permission::Enum permission = _owningAccessController.getConsumerPermission(
            _message->getCreator(), _domain, _interfaceName, operation, _trustlevel);
    assert(permission != Permission::ASK && "Permission.ASK user dialog not yet implemented.");

    IAccessController::Enum hasPermission = IAccessController::Enum::NO;
    if (permission == Permission::Enum::YES) {
        hasPermission = IAccessController::Enum::YES;
    } else {
        JOYNR_LOG_ERROR(_owningAccessController.logger(),
                        "Message {} to domain {}, interface/operation {}/{} from creator {} failed "
                        "ACL check",
                        _message->getId(),
                        _domain,
                        _interfaceName,
                        operation,
                        _message->getCreator());
    }

    _callback->hasConsumerPermission(hasPermission);
}

//--------- AccessController ---------------------------------------------------

AccessController::AccessController(
        std::shared_ptr<LocalCapabilitiesDirectory> localCapabilitiesDirectory,
        std::shared_ptr<LocalDomainAccessStore> localDomainAccessStore)
        : _localCapabilitiesDirectory(localCapabilitiesDirectory),
          _localDomainAccessStore(localDomainAccessStore),
          _whitelistParticipantIds(),
          _discoveryQos()
{
    _discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);
    _discoveryQos.setDiscoveryTimeout(60000);
    _discoveryQosWithLocalOnlyScope = _discoveryQos;
    _discoveryQosWithLocalOnlyScope.setDiscoveryScope(types::DiscoveryScope::LOCAL_ONLY);
}

void AccessController::addParticipantToWhitelist(const std::string& participantId)
{
    _whitelistParticipantIds.push_back(participantId);
}

bool AccessController::needsHasConsumerPermissionCheck(const ImmutableMessage& message) const
{
    if (util::vectorContains(_whitelistParticipantIds, message.getRecipient())) {
        return false;
    }

    const std::string& messageType = message.getType();
    if (messageType == Message::VALUE_MESSAGE_TYPE_MULTICAST() ||
        messageType == Message::VALUE_MESSAGE_TYPE_PUBLICATION() ||
        messageType == Message::VALUE_MESSAGE_TYPE_REPLY() ||
        messageType == Message::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY()) {
        // reply messages don't need permission check
        // they are filtered by request reply ID or subscritpion ID
        return false;
    }

    // If this point is reached, checking is required
    return true;
}

bool AccessController::needsHasProviderPermissionCheck() const
{
    const ClusterControllerCallContext& callContext = ClusterControllerCallContextStorage::get();

    if (callContext.getIsValid()) {
        return !callContext.getIsInternalProviderRegistration();
    }

    return true;
}

void AccessController::hasConsumerPermission(
        std::shared_ptr<ImmutableMessage> message,
        std::shared_ptr<IAccessController::IHasConsumerPermissionCallback> callback,
        bool isLocalRecipient)
{
    if (!needsHasConsumerPermissionCheck(*message)) {
        callback->hasConsumerPermission(IAccessController::Enum::YES);
        return;
    }

    // Get the domain and interface of the message destination
    auto lookupSuccessCallback = [message,
                                  thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this()),
                                  callback](
                                         const types::DiscoveryEntryWithMetaInfo& discoveryEntry) {
        if (auto thisSharedPtr = thisWeakPtr.lock()) {
            const std::string& participantId = message->getRecipient();
            if (discoveryEntry.getParticipantId() != participantId) {
                JOYNR_LOG_ERROR(thisSharedPtr->logger(),
                                "Failed to get capabilities for participantId {}",
                                participantId);
                callback->hasConsumerPermission(IAccessController::Enum::NO);
                return;
            }

            std::string domain = discoveryEntry.getDomain();
            std::string interfaceName = discoveryEntry.getInterfaceName();

            // Create a callback object
            auto ldacCallback = std::make_shared<LdacConsumerPermissionCallback>(
                    *thisSharedPtr, message, domain, interfaceName, TrustLevel::HIGH, callback);

            // Try to determine permission without expensive message deserialization
            // For now TrustLevel::HIGH is assumed.

            const std::string& msgCreatorUid = message->getCreator();
            thisSharedPtr->getConsumerPermission(
                    msgCreatorUid, domain, interfaceName, TrustLevel::HIGH, ldacCallback);
        }
    };

    std::function<void(const types::DiscoveryError::Enum&)> lookupErrorCallback =
            [callback](const types::DiscoveryError::Enum& error) {
                std::ignore = error;
                callback->hasConsumerPermission(IAccessController::Enum::RETRY);
            };

    // Lookup participantId in the local Capabilities Directory
    _localCapabilitiesDirectory->lookup(
            message->getRecipient(),
            (isLocalRecipient ? _discoveryQosWithLocalOnlyScope : _discoveryQos),
            std::vector<std::string>(),
            std::move(lookupSuccessCallback),
            std::move(lookupErrorCallback));
}

bool AccessController::hasProviderPermission(const std::string& userId,
                                             TrustLevel::Enum trustLevel,
                                             const std::string& domain,
                                             const std::string& interfaceName)
{
    if (!needsHasProviderPermissionCheck()) {
        return true;
    }

    return getProviderPermission(userId, domain, interfaceName, trustLevel) ==
           Permission::Enum::YES;
}

bool AccessController::hasRole(const std::string& userId,
                               const std::string& domain,
                               Role::Enum role)
{
    JOYNR_LOG_TRACE(logger(), "execute: entering hasRole");

    // See if the user has the given role
    bool hasRole = false;
    boost::optional<DomainRoleEntry> dre = _localDomainAccessStore->getDomainRole(userId, role);
    if (dre) {
        std::vector<std::string> domains = dre->getDomains();
        const std::string wildcard = "*";
        if (util::vectorContains(domains, domain) || util::vectorContains(domains, wildcard)) {
            hasRole = true;
        }
    }

    return hasRole;
}

void AccessController::getConsumerPermission(
        const std::string& userId,
        const std::string& domain,
        const std::string& interfaceName,
        TrustLevel::Enum trustLevel,
        std::shared_ptr<AccessController::IGetPermissionCallback> callback)
{
    JOYNR_LOG_TRACE(logger(), "Entering getConsumerPermission with unknown operation");
    // The operations of the ACEs should only contain wildcards, if not
    // getConsumerPermission should be called with an operation
    if (!_localDomainAccessStore->onlyWildcardOperations(userId, domain, interfaceName)) {
        JOYNR_LOG_INFO(logger(), "Operation needed for ACL check.");
        callback->operationNeeded();
    } else {
        // The operations are all wildcards
        Permission::Enum permission = getConsumerPermission(
                userId, domain, interfaceName, access_control::WILDCARD, trustLevel);
        callback->permission(permission);
    }
}

Permission::Enum AccessController::getConsumerPermission(const std::string& userId,
                                                         const std::string& domain,
                                                         const std::string& interfaceName,
                                                         const std::string& operation,
                                                         TrustLevel::Enum trustLevel)
{
    JOYNR_LOG_TRACE(logger(),
                    "Entering getConsumerPermission with userID={} domain={} interfaceName={}",
                    userId,
                    domain,
                    interfaceName);

    boost::optional<MasterAccessControlEntry> masterAceOptional =
            _localDomainAccessStore->getMasterAccessControlEntry(
                    userId, domain, interfaceName, operation);
    boost::optional<MasterAccessControlEntry> mediatorAceOptional =
            _localDomainAccessStore->getMediatorAccessControlEntry(
                    userId, domain, interfaceName, operation);
    boost::optional<OwnerAccessControlEntry> ownerAceOptional =
            _localDomainAccessStore->getOwnerAccessControlEntry(
                    userId, domain, interfaceName, operation);

    return _accessControlAlgorithm.getConsumerPermission(
            masterAceOptional, mediatorAceOptional, ownerAceOptional, trustLevel);
}

void AccessController::getProviderPermission(
        const std::string& userId,
        const std::string& domain,
        const std::string& interfaceName,
        TrustLevel::Enum trustLevel,
        std::shared_ptr<AccessController::IGetPermissionCallback> callback)
{
    JOYNR_LOG_TRACE(logger(), "Entering getProviderPermission with callback");
    Permission::Enum permission = getProviderPermission(userId, domain, interfaceName, trustLevel);
    callback->permission(permission);
}

Permission::Enum AccessController::getProviderPermission(const std::string& uid,
                                                         const std::string& domain,
                                                         const std::string& interfaceName,
                                                         TrustLevel::Enum trustLevel)
{
    JOYNR_LOG_TRACE(logger(),
                    "Entering getProviderPermission with userID={} domain={} interfaceName={}",
                    uid,
                    domain,
                    interfaceName);

    boost::optional<MasterRegistrationControlEntry> masterRceOptional =
            _localDomainAccessStore->getMasterRegistrationControlEntry(uid, domain, interfaceName);
    boost::optional<MasterRegistrationControlEntry> mediatorRceOptional =
            _localDomainAccessStore->getMediatorRegistrationControlEntry(
                    uid, domain, interfaceName);
    boost::optional<OwnerRegistrationControlEntry> ownerRceOptional =
            _localDomainAccessStore->getOwnerRegistrationControlEntry(uid, domain, interfaceName);

    return _accessControlAlgorithm.getProviderPermission(
            masterRceOptional, mediatorRceOptional, ownerRceOptional, trustLevel);
}

} // namespace joynr
