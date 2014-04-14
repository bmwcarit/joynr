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
#ifndef ICAPABILITIESCLIENT_H
#define ICAPABILITIESCLIENT_H

#include "joynr/types/CapabilityInformation.h"

#include <QString>
#include <QList>
#include <QSharedPointer>

namespace joynr {

class IGlobalCapabilitiesCallback;

class ICapabilitiesClient {
public:
    virtual ~ICapabilitiesClient(){ }
    virtual void registerCapabilities(QList<types::CapabilityInformation> capabilitiesInformationList) =0;
    virtual void removeCapabilities(QList<types::CapabilityInformation> capabilitiesInformationList) =0;
    virtual QList<types::CapabilityInformation> getCapabilitiesForInterfaceAddress(const QString& domain, const QString& interfaceName) =0;
    virtual void getCapabilitiesForInterfaceAddress(const QString& domain, const QString& interfaceName, QSharedPointer<IGlobalCapabilitiesCallback> callback) =0;
    virtual void getCapabilitiesForParticipantId(const QString& participantId, QSharedPointer<IGlobalCapabilitiesCallback> callback) =0;
    virtual QString getLocalChannelId() =0;

};


} // namespace joynr
#endif //ICAPABILITIESCLIENT_H
