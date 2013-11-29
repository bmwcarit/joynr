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
#ifndef DBUSMESSAGINGSKELETON_H
#define DBUSMESSAGINGSKELETON_H
#include "joynr/PrivateCopyAssign.h"
#include "joynr/JoynrCommonExport.h"
#include "common-api/joynr/messaging/IMessagingStubDefault.h"
#include "joynr/IMessaging.h"

namespace joynr {

class JOYNRCOMMON_EXPORT DbusMessagingSkeleton : public joynr::messaging::IMessagingStubDefault {
public:

    DbusMessagingSkeleton(IMessaging& callBack);

    virtual void transmit(joynr::messaging::IMessaging::JoynrMessage message, joynr::messaging::types::Types::JoynrMessageQos qos);

private:
    DISALLOW_COPY_AND_ASSIGN(DbusMessagingSkeleton);
    IMessaging& callBack;
};


} // namespace joynr
#endif // DBUSMESSAGINGSKELETON_H
