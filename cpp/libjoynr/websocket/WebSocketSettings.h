/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2014 BMW Car IT GmbH
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
#ifndef WEBSOCKETSETTINGS_H
#define WEBSOCKETSETTINGS_H

#include "joynr/joynrlogging.h"

#include <QObject>
#include <QSettings>

#include "joynr/system/routingtypes/QtWebSocketAddress.h"

namespace joynr
{

class WebSocketSettings : public QObject
{
    Q_OBJECT

public:
    static const QString& SETTING_CC_MESSAGING_URL();

    static const QString& DEFAULT_WEBSOCKET_SETTINGS_FILENAME();

    explicit WebSocketSettings(QSettings& settings, QObject* parent = 0);
    WebSocketSettings(const WebSocketSettings& other);

    ~WebSocketSettings();

    QString getClusterControllerMessagingUrl() const;
    void setClusterControllerMessagingUrl(const QString& url);
    system::routingtypes::QtWebSocketAddress createClusterControllerMessagingAddress() const;

    void printSettings() const;

    bool contains(const QString& key) const;
    QVariant value(const QString& key) const;

private:
    void operator=(const WebSocketSettings& other);

    QSettings& settings;
    static joynr_logging::Logger* logger;
    void checkSettings() const;
};

} // namespace joynr
#endif // WEBSOCKETSETTINGS_H
