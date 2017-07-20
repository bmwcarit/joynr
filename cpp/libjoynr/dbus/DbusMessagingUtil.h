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
#ifndef DBUSMESSAGINGUTIL_H
#define DBUSMESSAGINGUTIL_H

#include "joynr/PrivateCopyAssign.h"

namespace joynr
{
class JoynrMessage;
}

// save the GCC diagnostic state
#pragma GCC diagnostic push
// Disable compiler warnings in this CommonAPI generated includes.
#pragma GCC diagnostic ignored "-Wunused-parameter"
// include CommonAPI stuff here:
#include "common-api/joynr/messaging/IMessaging.h"
#include "common-api/joynr/messaging/types/Types.h"
// restore the old GCC diagnostic state
#pragma GCC diagnostic pop

namespace joynr
{

class DbusMessagingUtil
{
public:
    static void copyDbusMsgToJoynrMsg(const joynr::messaging::IMessaging::JoynrMessage& DbusMsg,
                                      JoynrMessage& joynrMsg);

    static void copyJoynrMsgToDbusMsg(const JoynrMessage& joynrMsg,
                                      joynr::messaging::IMessaging::JoynrMessage& dbusMsg);

private:
    DISALLOW_COPY_AND_ASSIGN(DbusMessagingUtil);
};

} // namespace joynr
#endif // DBUSMESSAGINGUTIL_H
