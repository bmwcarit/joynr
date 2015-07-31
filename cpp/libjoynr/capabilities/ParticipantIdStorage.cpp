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

namespace joynr
{

ParticipantIdStorage::ParticipantIdStorage(const std::string& filename) : filename(filename)
{
}

const QString& ParticipantIdStorage::STORAGE_FORMAT_STRING()
{
    static const QString value("joynr.participant.%1.%2.%3");
    return value;
}

void ParticipantIdStorage::setProviderParticipantId(const std::string& domain,
                                                    const std::string& interfaceName,
                                                    const std::string& participantId,
                                                    const std::string& authenticationToken)
{
    // Access the persistence file through a threadsafe QSettings object
    QSettings settings(QString::fromStdString(filename), QSettings::IniFormat);

    std::string providerKey = createProviderKey(domain, interfaceName, authenticationToken);
    settings.setValue(QString::fromStdString(providerKey), QString::fromStdString(participantId));
    settings.sync();
}

std::string ParticipantIdStorage::getProviderParticipantId(const std::string& domain,
                                                           const std::string& interfaceName,
                                                           const std::string& authenticationToken)
{
    return getProviderParticipantId(domain, interfaceName, "", authenticationToken);
}

std::string ParticipantIdStorage::getProviderParticipantId(const std::string& domain,
                                                           const std::string& interfaceName,
                                                           const std::string& defaultValue,
                                                           const std::string& authenticationToken)
{
    // Access the persistence file through a threadsafe QSettings object
    QSettings settings(QString::fromStdString(filename), QSettings::IniFormat);

    // Arrange the provider ids by authentication token
    std::string authToken =
            (!authenticationToken.empty()) ? authenticationToken : std::string("default");
    std::string providerKey = createProviderKey(domain, interfaceName, authToken);

    // Lookup the participant id
    std::string participantId;
    QVariant value = settings.value(QString::fromStdString(providerKey));

    if (!value.isValid()) {
        // Persist a new participant Id, using the defaultValue if possible
        participantId = (!defaultValue.empty()) ? defaultValue : Util::createUuid().toStdString();
        settings.setValue(
                QString::fromStdString(providerKey), QString::fromStdString(participantId));
        settings.sync();
    } else {
        participantId = value.toString().toStdString();
    }

    return participantId;
}

std::string ParticipantIdStorage::createProviderKey(const std::string& domain,
                                                    const std::string& interfaceName,
                                                    const std::string& authenticationToken)
{
    QString key = STORAGE_FORMAT_STRING()
                          .arg(QString::fromStdString(domain))
                          .arg(QString::fromStdString(interfaceName))
                          .arg(QString::fromStdString(authenticationToken));

    return key.replace("/", ".").toStdString();
}

} // namespace joynr
