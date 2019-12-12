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

#ifndef LIBJOYNRWEBSOCKETRUNTIME_H
#define LIBJOYNRWEBSOCKETRUNTIME_H

#include <atomic>
#include <functional>
#include <memory>
#include <string>

#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/WebSocketSettings.h"
#include "runtimes/libjoynr-runtime/LibJoynrRuntime.h"

namespace joynr
{

class IKeychain;
class IMessageRouter;
class IWebSocketPpClient;
class Settings;

namespace exceptions
{
class JoynrRuntimeException;
}

class LibJoynrWebSocketRuntime : public LibJoynrRuntime
{
public:
    LibJoynrWebSocketRuntime(std::unique_ptr<Settings> settings,
                             std::shared_ptr<IKeychain> keyChain = nullptr);
    ~LibJoynrWebSocketRuntime() override;

    void shutdown() override;

protected:
    void startLibJoynrMessagingSkeleton(std::shared_ptr<IMessageRouter> messageRouter) override;
    void connect(std::function<void()> onSuccess,
                 std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onError);

private:
    DISALLOW_COPY_AND_ASSIGN(LibJoynrWebSocketRuntime);

    void sendInitializationMsg();
    void createWebsocketClient();

    WebSocketSettings _wsSettings;
    std::shared_ptr<IWebSocketPpClient> _websocket;
    std::string _initializationMsg;
    std::atomic<bool> _isShuttingDown;
    ADD_LOGGER(LibJoynrWebSocketRuntime)

    friend class JoynrRuntime;
};

} // namespace joynr
#endif // LIBJOYNRWEBSOCKETRUNTIME_H
