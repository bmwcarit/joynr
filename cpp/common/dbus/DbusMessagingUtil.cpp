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
#include "common/dbus/DbusMessagingUtil.h"

#include <QVariant>
#include <QVariantMap>

#include "joynr/JoynrMessage.h"

namespace joynr
{

void DbusMessagingUtil::copyDbusMsgToJoynrMsg(
        const joynr::messaging::IMessaging::JoynrMessage& dbusMsg,
        JoynrMessage& joynrMsg)
{
    // type
    joynrMsg.setType(QString::fromStdString(dbusMsg.type));
    QVariantMap headerMap;

    // header
    for (auto it = dbusMsg.header.begin(); it != dbusMsg.header.end(); it++) {
        headerMap.insert(QString::fromStdString(it->first), QString::fromStdString(it->second));
    }
    joynrMsg.setHeader(headerMap);
    // payload
    QByteArray byteArray;
    byteArray.append(QString::fromStdString(dbusMsg.payload));
    joynrMsg.setPayload(byteArray);
}

void DbusMessagingUtil::copyJoynrMsgToDbusMsg(const JoynrMessage& joynrMsg,
                                              joynr::messaging::IMessaging::JoynrMessage& dbusMsg)
{
    // type
    dbusMsg.type = joynrMsg.getType().toStdString();
    // header
    QVariantMap headerMap = joynrMsg.getHeader().toMap();
    for (auto it = headerMap.begin(); it != headerMap.end(); it++) {
        dbusMsg.header.insert(std::make_pair<std::string, std::string>(
                it.key().toStdString(), it.value().toString().toStdString()));
    }
    // payload
    QString payLoad(joynrMsg.getPayload());
    dbusMsg.payload = payLoad.toStdString();
}

} // namespace joynr
