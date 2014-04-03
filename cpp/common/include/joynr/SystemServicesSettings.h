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
#ifndef SYSTEMSERVICESSETTINGS_H
#define SYSTEMSERVICESSETTINGS_H

#include "joynr/JoynrCommonExport.h"
#include "joynr/joynrlogging.h"
#include <QObject>
#include <QSettings>

namespace joynr {

class JOYNRCOMMON_EXPORT SystemServicesSettings : public QObject {
    Q_OBJECT

public:
    explicit SystemServicesSettings(QSettings& settings, QObject* parent = 0);
    SystemServicesSettings(const SystemServicesSettings& other);

    ~SystemServicesSettings();

    static const QString& SETTING_DOMAIN();
    static const QString& SETTING_CC_ROUTINGPROVIDER_AUTHENTICATIONTOKEN();
    static const QString& SETTING_CC_ROUTINGPROVIDER_PARTICIPANTID();

    static const QString& DEFAULT_SYSTEM_SERVICES_SETTINGS_FILENAME();

    QString getDomain() const;
    void setJoynrSystemServicesDomain(const QString& systemServicesDomain);
    QString getCcRoutingProviderAuthenticationToken() const;
    void setCcRoutingProviderAuthenticationToken(const QString& authenticationToken);
    QString getCcRoutingProviderParticipantId() const;
    void setCcRoutingProviderParticipantId(const QString& participantId);

    bool contains(const QString& key) const;
    QVariant value(const QString& key) const;

public slots:
    void printSettings() const;

private:
    void operator=(const SystemServicesSettings& other);

    QSettings& settings;
    static joynr_logging::Logger* logger;
    void checkSettings() const;
};


} // namespace joynr
#endif // SYSTEMSERVICESSETTINGS_H
