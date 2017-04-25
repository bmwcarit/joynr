/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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
#include "joynr/DbusMessagingSkeleton.h"
#include <CommonAPI/CommonAPI.h>

#include "joynr/JoynrMessage.h"
#include "common/dbus/DbusMessagingUtil.h"

namespace joynr
{

INIT_LOGGER(DbusMessagingSkeleton);

DbusMessagingSkeleton::DbusMessagingSkeleton(IMessaging& callBack) : callBack(callBack)
{
}

void DbusMessagingSkeleton::transmit(joynr::messaging::IMessaging::JoynrMessage message)
{
    // convert joynr message
    JoynrMessage joynrMessage;
    DbusMessagingUtil::copyDbusMsgToJoynrMsg(message, joynrMessage);
    // callback
    JOYNR_LOG_INFO(logger, "transmit incoming message: {}", joynrMessage.getHeaderMessageId());
    callBack.transmit(joynrMessage);
}

} // namespace joynr
