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
#include "runtimes/libjoynr-runtime/uds/LibJoynrUdsRuntime.h"

#include <tuple>
#include <utility>

#include "joynr/Settings.h"
#include "joynr/UdsClient.h"
#include "joynr/UdsMulticastAddressCalculator.h"
#include "joynr/Util.h"

#include "joynr/exceptions/JoynrException.h"
#include "joynr/system/RoutingTypes/UdsAddress.h"
#include "joynr/system/RoutingTypes/UdsClientAddress.h"

#include "libjoynr/uds/UdsLibJoynrMessagingSkeleton.h"
#include "libjoynr/uds/UdsMessagingStubFactory.h"

namespace joynr
{

LibJoynrUdsRuntime::LibJoynrUdsRuntime(
        std::unique_ptr<Settings> settings,
        std::function<void(const exceptions::JoynrRuntimeException&)>&& onFatalRuntimeError)
        : LibJoynrRuntime(std::move(settings), std::move(onFatalRuntimeError), nullptr),
          _stubFactory(std::make_shared<UdsMessagingStubFactory>()),
          _isShuttingDown(false)
{
    UdsSettings udsSettings(*_settings);
    udsSettings.printSettings();
    _serverAddress =
            std::make_shared<system::RoutingTypes::UdsAddress>(udsSettings.getSocketPath());
    _client = std::make_shared<UdsClient>(udsSettings, _onFatalRuntimeError);
}

LibJoynrUdsRuntime::~LibJoynrUdsRuntime()
{
    shutdown();
}

void LibJoynrUdsRuntime::shutdown()
{
    if (!_isShuttingDown.exchange(true)) {
        /*
         * Assure that LibJoynrRuntime::init (see setConnectCallback) is not executed
         * while executing LibJoynrRuntime::shutdown.
         */
        _client->shutdown();

        LibJoynrRuntime::shutdown();
    }
}

void LibJoynrUdsRuntime::connect(
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onError)
{
    _client->setConnectCallback(
            [this, onSuccess = std::move(onSuccess), onError = std::move(onError)]() mutable {
                // The destructor resolves the circular dependency
                _stubFactory->addServer(*_serverAddress, _client);

                std::unique_ptr<IMulticastAddressCalculator> addressCalculator =
                        std::make_unique<joynr::UdsMulticastAddressCalculator>(_serverAddress);

                auto ownAddress = std::make_shared<system::RoutingTypes::UdsClientAddress>(
                        _client->getAddress());
                this->init(_stubFactory,
                           std::move(ownAddress),
                           _serverAddress,
                           std::move(addressCalculator),
                           std::move(onSuccess),
                           std::move(onError));
            });

    _client->start();
}

void LibJoynrUdsRuntime::startLibJoynrMessagingSkeleton(
        std::shared_ptr<IMessageRouter> messageRouter)
{
    _skeleton = std::make_unique<UdsLibJoynrMessagingSkeleton>(messageRouter);
    /*
     * This method is called from this->init, hence by the client ASIO pool thread,
     * when invoking connect callbak (see above).
     * Therefore any modifications of the client are safe.
     */
    _client->setReceiveCallback(
            [this](smrf::ByteVector&& msg) { _skeleton->onMessageReceived(std::move(msg)); });

    /*
     * The LibJoynrRuntime::init calls registerOnMessagingStubClosedCallback, which implicitly
     * modifies the onMessagingStubClosed behaviour. Hence onMessagingStubClosed must not be called
     * before configuration is complete.
     */
    _client->setDisconnectCallback(
            [this]() { _stubFactory->onMessagingStubClosed(*_serverAddress); });
}

} // namespace joynr
