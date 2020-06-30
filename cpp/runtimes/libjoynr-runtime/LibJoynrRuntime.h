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

#ifndef LIBJOYNRRUNTIME_H
#define LIBJOYNRRUNTIME_H

#include <atomic>
#include <functional>
#include <memory>

#include "joynr/JoynrRuntimeImpl.h"
#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"

namespace joynr
{

class IKeychain;
class IMessageRouter;
class IMessageSender;
class IMiddlewareMessagingStubFactory;
class IMulticastAddressCalculator;
class InProcessMessagingSkeleton;
class JoynrMessagingConnectorFactory;
class LibJoynrMessageRouter;
class LibjoynrSettings;
class Settings;
class SubscriptionManager;

namespace system
{
class RoutingProxy;
} // namespace system

class LibJoynrRuntime : public JoynrRuntimeImpl
{

public:
    explicit LibJoynrRuntime(
            std::unique_ptr<Settings> settings,
            std::function<void(const exceptions::JoynrRuntimeException&)>&& onFatalRuntimeError,
            std::shared_ptr<IKeychain> keyChain = nullptr);
    ~LibJoynrRuntime() override;

    void shutdown() override;

protected:
    void buildInternalProxies(
            std::shared_ptr<JoynrMessagingConnectorFactory> connectorFactory,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError);

    std::shared_ptr<IMessageRouter> getMessageRouter() final;

    std::shared_ptr<SubscriptionManager> _subscriptionManager;
    std::shared_ptr<IMessageSender> _messageSender;
    std::shared_ptr<IDispatcher> _joynrDispatcher;
    std::shared_ptr<joynr::system::RoutingProxy> _ccRoutingProxy;

    // take ownership, so a pointer is used
    std::unique_ptr<Settings> _settings;
    // use pointer for settings object to check the configuration before initialization
    LibjoynrSettings* _libjoynrSettings;

    std::shared_ptr<InProcessMessagingSkeleton> _dispatcherMessagingSkeleton;

    virtual void startLibJoynrMessagingSkeleton(std::shared_ptr<IMessageRouter> messageRouter) = 0;

    void init(std::shared_ptr<IMiddlewareMessagingStubFactory> middlewareMessagingStubFactory,
              std::shared_ptr<const joynr::system::RoutingTypes::Address> libjoynrMessagingAddress,
              std::shared_ptr<const joynr::system::RoutingTypes::Address> ccMessagingAddress,
              std::unique_ptr<IMulticastAddressCalculator> addressCalculator,
              std::function<void()> onSuccess,
              std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onError);

private:
    DISALLOW_COPY_AND_ASSIGN(LibJoynrRuntime);
    std::shared_ptr<LibJoynrMessageRouter> _libJoynrMessageRouter;
    std::atomic<bool> _libJoynrRuntimeIsShuttingDown;
    ADD_LOGGER(LibJoynrRuntime)
};

} // namespace joynr
#endif // LIBJOYNRRUNTIME_H
