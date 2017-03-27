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
#include "joynr/DBusMessageRouterAdapter.h"

namespace joynr
{

DBusMessageRouterAdapter::DBusMessageRouterAdapter(MessageRouter& messageRouter,
                                                   std::string dbusAddress)
        : dbusSkeletonWrapper(
                  new IDbusSkeletonWrapper<DbusMessagingSkeleton, IMessaging>(*this, dbusAddress)),
          messageRouter(messageRouter)
{
}

DBusMessageRouterAdapter::~DBusMessageRouterAdapter()
{
    delete dbusSkeletonWrapper;
}

void DBusMessageRouterAdapter::transmit(JoynrMessage& message, const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure)
{
    dbusSkeletonWrapper->logMethodCall("transmit", "DBusMessageRouterAdapter");
    try {
        messageRouter.route(message);
    } catch (const exceptions::JoynrRuntimeException& e) {
        onFailure(e);
    }
}

} // namespace joynr
