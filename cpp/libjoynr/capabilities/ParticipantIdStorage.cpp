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
#include "joynr/ParticipantIdStorage.h"
#include "joynr/Util.h"

#include <QSettings>

namespace joynr {

ParticipantIdStorage::ParticipantIdStorage(const QString& filename) :
    filename(filename)
{
}

QString ParticipantIdStorage::getProviderParticipantId(const QString& domain,
                                                       const QString& interfaceName,
                                                       const QString& authenticationToken)
{
    return getProviderParticipantId(domain, interfaceName, authenticationToken, QString());
}

QString ParticipantIdStorage::getProviderParticipantId(const QString& domain,
                                                       const QString& interfaceName,
                                                       const QString& authenticationToken,
                                                       const QString& defaultValue)
{
    // Access the persistence file through a threadsafe QSettings object
    QSettings settings(filename, QSettings::IniFormat);

    // Arrange the provider ids by authentication token
    QString authToken = (!authenticationToken.isEmpty()) ? authenticationToken :
                                                           QString("default");
    QString token = QString("joynr.participant/%1|%2|%3").arg(domain).arg(interfaceName).arg(authToken);

    // Lookup the participant id
    QString participantId;
    QVariant value = settings.value(token);

    if (!value.isValid()) {
        // Persist a new participant Id, using the defaultValue if possible
        participantId = (!defaultValue.isEmpty()) ? defaultValue : Util::createUuid();
        settings.setValue(token, participantId);
        settings.sync();
    } else {
        participantId = value.toString();
    }

    return participantId;
}

} // namespace joynr
