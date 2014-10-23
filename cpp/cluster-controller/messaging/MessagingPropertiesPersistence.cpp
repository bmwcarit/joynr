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
#include "cluster-controller/messaging/MessagingPropertiesPersistence.h"
#include "joynr/Util.h"

#include <QSettings>

namespace joynr
{

MessagingPropertiesPersistence::MessagingPropertiesPersistence(const QString& filename)
        : filename(filename)
{
}

QString MessagingPropertiesPersistence::getChannelId()
{
    // Read and write to the persistence file using a QSettings object
    QSettings settings(filename, QSettings::IniFormat);

    // Get the persisted channel id if one exists
    QString channelId;
    QVariant value = settings.value(CHANNEL_ID_KEY());

    if (!value.isValid()) {
        // Create and persist a channelId
        channelId = Util::createUuid();
        settings.setValue(CHANNEL_ID_KEY(), channelId);
        settings.sync();
    } else {
        channelId = value.toString();
    }

    return channelId;
}

QString MessagingPropertiesPersistence::getReceiverId()
{
    // Read and write to the persistence file using a QSettings object
    QSettings settings(filename, QSettings::IniFormat);

    // Get the persisted receiver id if one exists
    QString receiverId;
    QVariant value = settings.value(RECEIVER_ID_KEY());

    if (!value.isValid()) {
        // Create and persist a receiverId
        receiverId = Util::createUuid();
        settings.setValue(RECEIVER_ID_KEY(), receiverId);
        settings.sync();
    } else {
        receiverId = value.toString();
    }

    return receiverId;
}

const QString& MessagingPropertiesPersistence::CHANNEL_ID_KEY()
{
    static const QString value("messaging/channelId");
    return value;
}

const QString& MessagingPropertiesPersistence::RECEIVER_ID_KEY()
{
    static const QString value("messaging/receiverId");
    return value;
}

} // namespace joynr
