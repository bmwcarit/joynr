/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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

#ifndef LIBJOYNRUDSRUNTIME_H
#define LIBJOYNRUDSRUNTIME_H

#include <atomic>
#include <functional>
#include <memory>
#include <string>

#include "joynr/Logger.h"
#include "runtimes/libjoynr-runtime/LibJoynrRuntime.h"

namespace joynr
{

class IMessageRouter;
class UdsMessagingStubFactory;
class UdsLibJoynrMessagingSkeleton;
class UdsClient;
class Settings;

namespace exceptions
{
class JoynrRuntimeException;
}

namespace system
{
namespace RoutingTypes
{
class UdsAddress;
}
}
class LibJoynrUdsRuntime : public LibJoynrRuntime
{
public:
    LibJoynrUdsRuntime(
            std::unique_ptr<Settings> settings,
            std::function<void(const exceptions::JoynrRuntimeException&)>&& onFatalRuntimeError);
    ~LibJoynrUdsRuntime() override;

    DISALLOW_COPY_AND_ASSIGN(LibJoynrUdsRuntime);

    void shutdown() override;

protected:
    void startLibJoynrMessagingSkeleton(std::shared_ptr<IMessageRouter> messageRouter) override;
    void connect(std::function<void()> onSuccess,
                 std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onError);

private:
    // Server address is registered to client callback
    std::shared_ptr<system::RoutingTypes::UdsAddress> _serverAddress;

    // Callbacks to skeleton and factory are registered to the client
    std::shared_ptr<UdsMessagingStubFactory> _stubFactory;
    std::unique_ptr<UdsLibJoynrMessagingSkeleton> _skeleton;
    std::shared_ptr<UdsClient> _client;

    std::atomic<bool> _isShuttingDown;
    ADD_LOGGER(LibJoynrUdsRuntime)

    friend class JoynrRuntime;
};

} // namespace joynr
#endif // LIBJOYNRUDSRUNTIME_H
