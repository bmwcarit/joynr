/*
 * #%L
 * joynr::C++
 * $Id:$
 * $HeadURL:$
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
#ifndef DBUSSETTINGS_H
#define DBUSSETTINGS_H

#include "joynr/JoynrCommonExport.h"

#include "joynr/joynrlogging.h"

#include <QObject>
#include <QSettings>

namespace joynr {

class JOYNRCOMMON_EXPORT DbusSettings : public QObject {
    Q_OBJECT

public:
    static const QString& SETTING_CC_MESSAGING_ADDRESS();
    static const QString& SETTING_CC_CAPABILITIES_ADDRESS();

    explicit DbusSettings(QSettings& settings, QObject* parent = 0);
    DbusSettings(const DbusSettings& other);

    ~DbusSettings();

    QString getClusterControllerMessagingAddress() const;
    void setClusterControllerMessagingAddress(const QString& address);

    QString getClusterControllerCapabilitiesAddress() const;
    void setClusterControllerCapabilitiesAddress(const QString& address);

    void printSettings() const;

private:
    void operator =(const DbusSettings &other);

    QSettings& settings;
    static joynr_logging::Logger* logger;
    void checkSettings() const;
};


} // namespace joynr
#endif // DBUSSETTINGS_H
