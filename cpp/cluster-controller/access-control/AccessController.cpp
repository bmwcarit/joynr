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

#include "joynr/JsonSerializer.h"
#include "joynr/JoynrMessage.h"
#include "joynr/LocalCapabilitiesDirectory.h"
#include "joynr/system/DiscoveryEntry.h"
#include "joynr/RequestStatus.h"
#include "joynr/Request.h"
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
    // Deserialize the message to get the operation
    QByteArray jsonRequest = message.getPayload();
    QScopedPointer<Request> request(JsonSerializer::deserialize<Request>(jsonRequest));

    if (request.isNull()) {
        LOG_ERROR(logger, "Could not deserialize request");
        callback->hasConsumerPermission(false);
        return;
    }

    QString operation = request->getMethodName();

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

AccessController::AccessController(LocalCapabilitiesDirectory& localCapabilitiesDirectory,
                                   LocalDomainAccessController& localDomainAccessController)
        : localCapabilitiesDirectory(localCapabilitiesDirectory),
          localDomainAccessController(localDomainAccessController)
{
}

AccessController::~AccessController()
{
}

void AccessController::hasConsumerPermission(
        const JoynrMessage& message,
        QSharedPointer<IAccessController::IHasConsumerPermissionCallback> callback)
{
    assert(message.getType() == JoynrMessage::VALUE_MESSAGE_TYPE_REQUEST);

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

    // Create a callback object
    QSharedPointer<LocalDomainAccessController::IGetConsumerPermissionCallback> internalCallback(
            new LdacConsumerPermissionCallback(*this,
                                               message,
                                               discoveryEntry.getDomain(),
                                               discoveryEntry.getInterfaceName(),
                                               TrustLevel::HIGH,
                                               callback));

    // Try to determine permission without expensive message deserialization
    // For now TrustLevel::HIGH is assumed.
    QString msgCreatorUid = message.getHeaderCreatorUserId();
    localDomainAccessController.getConsumerPermission(msgCreatorUid,
                                                      discoveryEntry.getDomain(),
                                                      discoveryEntry.getInterfaceName(),
                                                      TrustLevel::HIGH,
                                                      internalCallback);
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
