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
#include "common/dbus/DbusMessagingStubAdapter.h"
#include "common/dbus/DbusMessagingUtil.h"
#include "joynr/JoynrMessage.h"
#include <CommonAPI/CommonAPI.h>
#include "joynr/FormatString.h"

namespace joynr
{

INIT_LOGGER(DbusMessagingStubAdapter);

DbusMessagingStubAdapter::DbusMessagingStubAdapter(std::string serviceAddress)
        : IDbusStubWrapper(serviceAddress)
{
    JOYNR_LOG_INFO(logger, "Get dbus proxy on address: {}", serviceAddress);

    // init the stub
    init();
}

void DbusMessagingStubAdapter::transmit(JoynrMessage& message, const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure)
{
    std::ignore = onFailure;
    logMethodCall(FormatString("transmit message with ID: %1 and payload: %2")
                          .arg(message.getHeaderMessageId())
                          .arg(message.getPayload())
                          .str());
    // copy joynr message
    joynr::messaging::IMessaging::JoynrMessage dbusMsg;
    DbusMessagingUtil::copyJoynrMsgToDbusMsg(message, dbusMsg);
    // call
    CommonAPI::CallStatus status;
    proxy->transmit(dbusMsg, status);
    // print the status
    printCallStatus(status, "transmit");
}

} // namespace joynr
