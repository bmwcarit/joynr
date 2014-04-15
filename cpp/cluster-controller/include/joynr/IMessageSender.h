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
#ifndef IMESSAGESENDER_H
#define IMESSAGESENDER_H

#include "joynr/MessagingSettings.h"
#include "joynr/ILocalChannelUrlDirectory.h"
#include "joynr/JoynrMessage.h"

#include <QString>
#include <QSharedPointer>
#include <QDateTime>

namespace joynr {

class IMessageSender {
public:
    virtual ~IMessageSender() {}
    /**
    * @brief Sends the message to the messaging endpoint associated with the channelId.
    */
    virtual void sendMessage(
            const QString& channelId,
            const QDateTime& decayTime,
            const JoynrMessage& message) = 0;
    /**
    * @brief The MessageSender needs the localChannelUrlDirectory to obtain Url's for
    * the channelIds.
    */
    virtual void init(
            QSharedPointer<ILocalChannelUrlDirectory> channelUrlDirectory,
            const MessagingSettings& settings) = 0;
};

}

#endif // IMESSAGESENDER_H
