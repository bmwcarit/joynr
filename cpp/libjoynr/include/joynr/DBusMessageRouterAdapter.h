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
#ifndef DBUSMESSAGEROUTERADAPTER_H
#define DBUSMESSAGEROUTERADAPTER_H

#include "joynr/PrivateCopyAssign.h"

#include "joynr/MessageRouter.h"
#include "joynr/IDbusSkeletonWrapper.h"
#include "joynr/DbusMessagingSkeleton.h"
#include "joynr/IMessaging.h"

#include "joynr/JoynrExport.h"

namespace joynr
{

class JOYNR_EXPORT DBusMessageRouterAdapter : public IMessaging
{
public:
    DBusMessageRouterAdapter(MessageRouter& messageRouter, std::string dbusAddress);

    ~DBusMessageRouterAdapter();

    virtual void transmit(
            JoynrMessage& message,
            const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure);

private:
    DISALLOW_COPY_AND_ASSIGN(DBusMessageRouterAdapter);
    IDbusSkeletonWrapper<DbusMessagingSkeleton, IMessaging>* dbusSkeletonWrapper;
    MessageRouter& messageRouter;
};

} // namespace joynr
#endif // DBUSMESSAGEROUTERADAPTER_H
