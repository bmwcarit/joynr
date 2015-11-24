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

#ifndef LIBJOYNRRUNTIME_H
#define LIBJOYNRRUNTIME_H

#include <string>
#include <memory>

#include "joynr/PrivateCopyAssign.h"

#include "joynr/JoynrRuntime.h"

#include "joynr/LibjoynrSettings.h"
#include "joynr/SystemServicesSettings.h"
#include "joynr/ProxyBuilder.h"
#include "joynr/IMessaging.h"
#include "joynr/JoynrMessageSender.h"
#include "joynr/CapabilitiesRegistrar.h"
#include "runtimes/libjoynr-runtime/JoynrRuntimeExecutor.h"
#include "joynr/SubscriptionManager.h"
#include "joynr/Semaphore.h"

namespace joynr
{

class IMessaging;
class JoynrMessageSender;
class MessageRouter;
class InProcessMessagingSkeleton;
class IMiddlewareMessagingStubFactory;
class Settings;

class LibJoynrRuntime : public JoynrRuntime
{

public:
    LibJoynrRuntime(Settings* settings);
    virtual ~LibJoynrRuntime();

    static LibJoynrRuntime* create(JoynrRuntimeExecutor* runtimeExecutor);
    void unregisterProvider(const std::string& participantId);

protected:
    ConnectorFactory* connectorFactory;
    SubscriptionManager* subscriptionManager;
    InProcessPublicationSender* inProcessPublicationSender;
    InProcessConnectorFactory* inProcessConnectorFactory;
    JoynrMessagingConnectorFactory* joynrMessagingConnectorFactory;
    std::shared_ptr<IMessaging> joynrMessagingSendStub;
    JoynrMessageSender* joynrMessageSender;
    IDispatcher* joynrDispatcher;
    IDispatcher* inProcessDispatcher;

    // take ownership, so a pointer is used
    Settings* settings;
    // use pointer for settings object to check the configuration before initialization
    LibjoynrSettings* libjoynrSettings;

    std::shared_ptr<InProcessMessagingSkeleton> dispatcherMessagingSkeleton;

    virtual void startLibJoynrMessagingSkeleton(MessageRouter& messageRouter) = 0;

    void init(IMiddlewareMessagingStubFactory* middlewareMessagingStubFactory,
              std::shared_ptr<joynr::system::RoutingTypes::QtAddress> libjoynrMessagingAddress,
              std::shared_ptr<joynr::system::RoutingTypes::QtAddress> ccMessagingAddress);

private:
    DISALLOW_COPY_AND_ASSIGN(LibJoynrRuntime);
    JoynrRuntimeExecutor* runtimeExecutor;
    void setRuntimeExecutor(JoynrRuntimeExecutor* runtimeExecutor);
};

} // namespace joynr
#endif // LIBJOYNRRUNTIME_H
