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
#ifndef DBUSMESSAGINSTUBADAPTER_H
#define DBUSMESSAGINSTUBADAPTER_H

#include "joynr/PrivateCopyAssign.h"
#include "joynr/JoynrExport.h"

#include "joynr/IMessaging.h"
#include "joynr/Logger.h"

// save the GCC diagnostic state
#pragma GCC diagnostic push
// Disable compiler warnings in this CommonAPI generated includes.
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Weffc++"
// include CommonAPI stuff here:
#include "common/dbus/IDbusStubWrapper.h"
#include "joynr/messaging/IMessagingProxy.h"
// restore the old GCC diagnostic state
#pragma GCC diagnostic pop

namespace joynr
{

class JOYNR_EXPORT DbusMessagingStubAdapter
        : public IDbusStubWrapper<joynr::messaging::IMessagingProxy>,
          public IMessaging
{
public:
    DbusMessagingStubAdapter(std::string serviceAddress);
    virtual void transmit(JoynrMessage& message, const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure);

private:
    ADD_LOGGER(DbusMessagingStubAdapter)
    DISALLOW_COPY_AND_ASSIGN(DbusMessagingStubAdapter);
};

} // namespace joynr
#endif // DBUSMESSAGINSTUBADAPTER_H
