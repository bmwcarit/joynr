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
#ifndef WEBSOCKETCCMESSAGINGSKELETON_H
#define WEBSOCKETCCMESSAGINGSKELETON_H

#include <map>
#include <memory>
#include <mutex>
#include <string>

#include <boost/asio/io_service.hpp>
#include <websocketpp/config/asio.hpp>
#include <websocketpp/server.hpp>

#include "libjoynr/websocket/WebSocketPpReceiver.h"
#include "joynr/system/RoutingTypes/WebSocketClientAddress.h"

#include "joynr/JoynrClusterControllerRuntimeExport.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/Logger.h"

#include "joynr/IMessaging.h"

namespace joynr
{

class JoynrMessage;
class IMessageRouter;
class WebSocketMessagingStubFactory;

namespace system
{
namespace RoutingTypes
{
class WebSocketAddress;
} // namespace RoutingTypes
} // namespace system

/**
 * @class WebSocketCcMessagingSkeleton
 * @brief Messaging skeleton for the cluster controller
 */
class JOYNRCLUSTERCONTROLLERRUNTIME_EXPORT WebSocketCcMessagingSkeleton : public IMessaging
{
    using Config = websocketpp::config::asio;
    using MessagePtr = Config::message_type::ptr;
    using Server = websocketpp::server<Config>;
    using ConnectionHandle = websocketpp::connection_hdl;

public:
    /**
     * @brief Constructor
     * @param messageRouter Router
     * @param messagingStubFactory Factory
     * @param serverAddress Address of the server
     */
    WebSocketCcMessagingSkeleton(
            boost::asio::io_service& ioService,
            std::shared_ptr<IMessageRouter> messageRouter,
            std::shared_ptr<WebSocketMessagingStubFactory> messagingStubFactory,
            const system::RoutingTypes::WebSocketAddress& serverAddress);

    /**
     * @brief Destructor
     */
    ~WebSocketCcMessagingSkeleton() override;

    void transmit(JoynrMessage& message,
                  const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure)
            override;

private:
    void onConnectionClosed(ConnectionHandle hdl);
    void onInitMessageReceived(ConnectionHandle hdl, MessagePtr message);
    void onTextMessageReceived(const std::string& message);

    Server endpoint;
    WebSocketPpReceiver<Server> receiver;
    /*! List of client connections */
    std::map<ConnectionHandle,
             joynr::system::RoutingTypes::WebSocketClientAddress,
             std::owner_less<ConnectionHandle>> clients;
    /*! Router for incoming messages */
    std::mutex clientsMutex;
    std::shared_ptr<IMessageRouter> messageRouter;
    /*! Factory to build outgoing messaging stubs */
    std::shared_ptr<WebSocketMessagingStubFactory> messagingStubFactory;

    ADD_LOGGER(WebSocketCcMessagingSkeleton);
    DISALLOW_COPY_AND_ASSIGN(WebSocketCcMessagingSkeleton);
    bool isInitializationMessage(const std::string& message);
};

} // namespace joynr
#endif // WEBSOCKETCCMESSAGINGSKELETON_H
