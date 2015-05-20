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

#include "AccessController.h"

#include "LocalDomainAccessController.h"
#include "joynr/LocalCapabilitiesDirectory.h"

#include "joynr/JsonSerializer.h"
#include "joynr/JoynrMessage.h"
#include "joynr/system/DiscoveryEntry.h"
#include "joynr/RequestStatus.h"
#include "joynr/Request.h"
#include "joynr/SubscriptionRequest.h"
#include "joynr/BroadcastSubscriptionRequest.h"
#include "joynr/system/Address.h"
#include "joynr/joynrlogging.h"

#include <QByteArray>
#include <QScopedPointer>

#include <cassert>
#include "joynr/JsonSerializer.h"

namespace joynr
{

using namespace infrastructure;
using namespace system;
using namespace joynr_logging;

Logger* AccessController::logger = Logging::getInstance()->getLogger("MSG", "AccessController");

//--------- InternalConsumerPermissionCallbacks --------------------------------

class AccessController::LdacConsumerPermissionCallback
        : public LocalDomainAccessController::IGetConsumerPermissionCallback
{

public:
    LdacConsumerPermissionCallback(
            AccessController& owningAccessController,
            const JoynrMessage& message,
            const QString& domain,
            const QString& interfaceName,
            TrustLevel::Enum trustlevel,
            QSharedPointer<IAccessController::IHasConsumerPermissionCallback> callback);

    // Callbacks made from the LocalDomainAccessController
    void consumerPermission(infrastructure::Permission::Enum permission);
    void operationNeeded();

private:
    AccessController& owningAccessController;
    JoynrMessage message;
    QString domain;
    QString interfaceName;
    TrustLevel::Enum trustlevel;
    QSharedPointer<IAccessController::IHasConsumerPermissionCallback> callback;

    bool convertToBool(Permission::Enum permission);
};

AccessController::LdacConsumerPermissionCallback::LdacConsumerPermissionCallback(
        AccessController& parent,
        const JoynrMessage& message,
        const QString& domain,
        const QString& interfaceName,
        TrustLevel::Enum trustlevel,
        QSharedPointer<IAccessController::IHasConsumerPermissionCallback> callback)
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

    if (hasPermission == false) {
        LOG_ERROR(logger,
                  QString("Message %1 to domain %2, interface %3 failed ACL check")
                          .arg(message.getHeaderMessageId())
                          .arg(domain)
                          .arg(interfaceName));
    }
    callback->hasConsumerPermission(hasPermission);
}

void AccessController::LdacConsumerPermissionCallback::operationNeeded()
{

    QString operation;
    QString messageType = message.getType();

    // Deserialize the message to get the operation
    QByteArray jsonRequest = message.getPayload();

    if (messageType == JoynrMessage::VALUE_MESSAGE_TYPE_REQUEST) {

        QScopedPointer<Request> request(JsonSerializer::deserialize<Request>(jsonRequest));
        if (!request.isNull()) {
            operation = request->getMethodName();
        }
    } else if (messageType == JoynrMessage::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST) {

        QScopedPointer<SubscriptionRequest> request(
                JsonSerializer::deserialize<SubscriptionRequest>(jsonRequest));
        if (!request.isNull()) {
            operation = request->getSubscribeToName();
        }
    } else if (messageType == JoynrMessage::VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST) {

        QScopedPointer<BroadcastSubscriptionRequest> request(
                JsonSerializer::deserialize<BroadcastSubscriptionRequest>(jsonRequest));
        if (!request.isNull()) {
            operation = request->getSubscribeToName();
        }
    }

    if (operation.isEmpty()) {
        LOG_ERROR(logger, "Could not deserialize request");
        callback->hasConsumerPermission(false);
        return;
    }

    // Get the permission for given operation
    Permission::Enum permission =
            owningAccessController.localDomainAccessController.getConsumerPermission(
                    message.getHeaderCreatorUserId(), domain, interfaceName, operation, trustlevel);

    bool hasPermission = convertToBool(permission);

    if (hasPermission == false) {
        LOG_ERROR(logger,
                  QString("Message %1 to domain %2, interface/operation %3/%4 failed ACL check")
                          .arg(message.getHeaderMessageId())
                          .arg(domain)
                          .arg(interfaceName)
                          .arg(operation));
    }

    callback->hasConsumerPermission(hasPermission);
}

bool AccessController::LdacConsumerPermissionCallback::convertToBool(Permission::Enum permission)
{
    switch (permission) {
    case Permission::YES:
        return true;
    case Permission::ASK:
        Q_ASSERT_X(
                false, "hasConsumerPermission", "Permission.ASK user dialog not yet implemented.");
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
    ProviderRegistrationObserver(LocalDomainAccessController& localDomainAccessController)
            : localDomainAccessController(localDomainAccessController)
    {
    }
    virtual void onProviderAdd(const DiscoveryEntry& discoveryEntry)
    {
        Q_UNUSED(discoveryEntry)
        // Ignored
    }

    virtual void onProviderRemove(const DiscoveryEntry& discoveryEntry)
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
                  new ProviderRegistrationObserver(localDomainAccessController)),
          whitelistParticipantIds()
{
    localCapabilitiesDirectory.addProviderRegistrationObserver(providerRegistrationObserver);
}

AccessController::~AccessController()
{
    localCapabilitiesDirectory.removeProviderRegistrationObserver(providerRegistrationObserver);
}

void AccessController::addParticipantToWhitelist(const QString& participantId)
{
    whitelistParticipantIds.append(participantId);
}

bool AccessController::needsPermissionCheck(const JoynrMessage& message)
{
    if (whitelistParticipantIds.contains(message.getHeaderTo())) {
        return false;
    }

    QString messageType = message.getType();
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
        QSharedPointer<IAccessController::IHasConsumerPermissionCallback> callback)
{
    if (!needsPermissionCheck(message)) {
        callback->hasConsumerPermission(true);
        return;
    }

    // Get the domain and interface of the message destination
    DiscoveryEntry discoveryEntry;
    RequestStatus status;
    QString participantId = message.getHeaderTo();
    localCapabilitiesDirectory.lookup(status, discoveryEntry, participantId);
    if (!status.successful()) {
        LOG_ERROR(logger,
                  QString("Failed to get capabilities for participantId %1").arg(participantId));
        callback->hasConsumerPermission(false);
        return;
    }

    QString domain = discoveryEntry.getDomain();
    QString interfaceName = discoveryEntry.getInterfaceName();

    // TODO: remove this shortcut used for system integration tests
    if (interfaceName == "tests/test") {
        callback->hasConsumerPermission(true);
        return;
    }

    // Create a callback object
    QSharedPointer<LocalDomainAccessController::IGetConsumerPermissionCallback> ldacCallback(
            new LdacConsumerPermissionCallback(
                    *this, message, domain, interfaceName, TrustLevel::HIGH, callback));

    // Try to determine permission without expensive message deserialization
    // For now TrustLevel::HIGH is assumed.
    QString msgCreatorUid = message.getHeaderCreatorUserId();
    localDomainAccessController.getConsumerPermission(
            msgCreatorUid, domain, interfaceName, TrustLevel::HIGH, ldacCallback);
}

bool AccessController::hasProviderPermission(const QString& userId,
                                             TrustLevel::Enum trustLevel,
                                             const QString& domain,
                                             const QString& interfaceName)
{
    Q_UNUSED(userId)
    Q_UNUSED(trustLevel)
    Q_UNUSED(domain)
    Q_UNUSED(interfaceName)

    Q_ASSERT_X(false, "hasProviderPermission", "Not yet implemented.");
    return true;
}

} // namespace joynr
