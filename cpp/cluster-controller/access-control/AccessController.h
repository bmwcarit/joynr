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

#ifndef ACCESSCONTROLLER_H
#define ACCESSCONTROLLER_H

#include "IAccessController.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/infrastructure/DacTypes_QtTrustLevel.h"
#include <memory>
#include <QList>
#include <QString>

namespace joynr
{
class LocalCapabilitiesDirectory;
class LocalDomainAccessController;

namespace joynr_logging
{
class Logger;
}

/**
 * Object that controls access to providers
 */
class AccessController : public IAccessController
{
public:
    AccessController(LocalCapabilitiesDirectory& localCapabilitiesDirectory,
                     LocalDomainAccessController& localDomainAccessController);

    virtual ~AccessController();

    //---IAccessController interface -------------------------------------------

    virtual void hasConsumerPermission(const JoynrMessage& message,
                                       std::shared_ptr<IHasConsumerPermissionCallback> callback);

    virtual bool hasProviderPermission(const QString& userId,
                                       infrastructure::DacTypes::QtTrustLevel::Enum trustLevel,
                                       const QString& domain,
                                       const QString& interfaceName);

    virtual void addParticipantToWhitelist(const QString& participantId);

private:
    class LdacConsumerPermissionCallback;
    class ProviderRegistrationObserver;

    DISALLOW_COPY_AND_ASSIGN(AccessController);
    bool needsPermissionCheck(const JoynrMessage& message);

    LocalCapabilitiesDirectory& localCapabilitiesDirectory;
    LocalDomainAccessController& localDomainAccessController;
    std::shared_ptr<ProviderRegistrationObserver> providerRegistrationObserver;
    QList<QString> whitelistParticipantIds;

    static joynr_logging::Logger* logger;
};

} // namespace joynr
#endif // IACCESSCONTROLLER_H
