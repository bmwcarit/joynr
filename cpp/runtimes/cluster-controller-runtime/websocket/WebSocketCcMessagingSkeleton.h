/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
#include <boost/algorithm/string/predicate.hpp>
#include <websocketpp/config/asio.hpp>
#include <websocketpp/server.hpp>

#include "libjoynr/websocket/WebSocketPpReceiver.h"
#include "joynr/system/RoutingTypes/WebSocketClientAddress.h"

#include "joynr/JoynrClusterControllerRuntimeExport.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/Logger.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/JoynrMessage.h"
#include "joynr/Util.h"
#include "joynr/MessageRouter.h"
#include "joynr/IWebSocketSendInterface.h"
#include "libjoynr/websocket/WebSocketMessagingStubFactory.h"
#include "libjoynr/websocket/WebSocketPpSender.h"
#include "joynr/system/RoutingTypes/WebSocketProtocol.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"

#include "joynr/IMessaging.h"

namespace joynr
{

class JoynrMessage;
class MessageRouter;
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
            std::shared_ptr<MessageRouter> messageRouter,
            std::shared_ptr<WebSocketMessagingStubFactory> messagingStubFactory,
            const system::RoutingTypes::WebSocketAddress& serverAddress)
            : endpoint(),
              receiver(),
              clients(),
              clientsMutex(),
              messageRouter(messageRouter),
              messagingStubFactory(messagingStubFactory)
    {

        websocketpp::lib::error_code initializationError;
        endpoint.init_asio(&ioService, initializationError);
        if (initializationError) {
            JOYNR_LOG_FATAL(logger,
                            "error during WebSocketCcMessagingSkeleton initialization: ",
                            initializationError.message());
            return;
        }

        endpoint.clear_access_channels(websocketpp::log::alevel::all);
        endpoint.clear_error_channels(websocketpp::log::alevel::all);

        // register handlers
        using namespace std::placeholders;
        endpoint.set_close_handler(
                std::bind(&WebSocketCcMessagingSkeleton::onConnectionClosed, this, _1));

        // new connections are handled in onInitMessageReceived; if initialization was successful,
        // any further messages for this connection are handled in onTextMessageReceived
        endpoint.set_message_handler(
                std::bind(&WebSocketCcMessagingSkeleton::onInitMessageReceived, this, _1, _2));

        receiver.registerReceiveCallback(
                [this](const std::string& msg) { onTextMessageReceived(msg); });

        try {
            endpoint.set_reuse_addr(true);
            endpoint.listen(serverAddress.getPort());
            endpoint.start_accept();
        } catch (const std::exception& e) {
            JOYNR_LOG_ERROR(logger, "WebSocket server could not be started: \"{}\"", e.what());
        }
    }

    /**
     * @brief Destructor
     */
    ~WebSocketCcMessagingSkeleton() override
    {
        websocketpp::lib::error_code shutdownError;
        endpoint.stop_listening(shutdownError);
        if (shutdownError) {
            JOYNR_LOG_ERROR(logger,
                            "error during WebSocketCcMessagingSkeleton shutdown: ",
                            shutdownError.message());
        }
    }

    void transmit(
            JoynrMessage& message,
            const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure) override
    {
        try {
            messageRouter->route(message);
        } catch (exceptions::JoynrRuntimeException& e) {
            onFailure(e);
        }
    }

private:
    void onConnectionClosed(ConnectionHandle hdl)
    {
        std::unique_lock<std::mutex> lock(clientsMutex);
        auto it = clients.find(hdl);
        if (it != clients.cend()) {
            messagingStubFactory->onMessagingStubClosed(it->second);
            clients.erase(it);
        }
    }

    void onInitMessageReceived(ConnectionHandle hdl, MessagePtr message)
    {
        std::string textMessage = message->get_payload();
        if (isInitializationMessage(textMessage)) {

            JOYNR_LOG_DEBUG(logger,
                            "received initialization message from websocket client: {}",
                            message->get_payload());
            // register client with messaging stub factory
            try {
                joynr::system::RoutingTypes::WebSocketClientAddress clientAddress;
                joynr::serializer::deserializeFromJson(clientAddress, textMessage);

                auto sender = std::make_shared<WebSocketPpSender<Server>>(endpoint);
                sender->setConnectionHandle(hdl);

                messagingStubFactory->addClient(clientAddress, sender);

                Server::connection_ptr connection = endpoint.get_con_from_hdl(hdl);
                using namespace std::placeholders;
                connection->set_message_handler(std::bind(
                        &WebSocketPpReceiver<Server>::onMessageReceived, &receiver, _1, _2));
                {
                    std::unique_lock<std::mutex> lock(clientsMutex);
                    clients[hdl] = clientAddress;
                }

            } catch (const std::invalid_argument& e) {
                JOYNR_LOG_FATAL(
                        logger,
                        "client address must be valid, otherwise libjoynr and CC are deployed "
                        "in different versions - raw: {} - error: {}",
                        textMessage,
                        e.what());
            }
        } else {
            JOYNR_LOG_ERROR(
                    logger, "received an initial message with wrong format: \"{}\"", textMessage);
        }
    }

    void onTextMessageReceived(const std::string& message)
    {
        // deserialize message and transmit
        try {
            JoynrMessage joynrMsg;
            joynr::serializer::deserializeFromJson(joynrMsg, message);
            if (joynrMsg.getType().empty()) {
                JOYNR_LOG_ERROR(logger, "Message type is empty : {}", message);
                return;
            }
            if (joynrMsg.getPayload().empty()) {
                JOYNR_LOG_ERROR(logger, "joynr message payload is empty: {}", message);
                return;
            }
            if (!joynrMsg.containsHeaderExpiryDate()) {
                JOYNR_LOG_ERROR(
                        logger,
                        "received message [msgId=[{}] without decay time - dropping message",
                        joynrMsg.getHeaderMessageId());
                return;
            }

            JOYNR_LOG_TRACE(logger, "<<<< INCOMING <<<< {}", message);

            auto onFailure = [joynrMsg](const exceptions::JoynrRuntimeException& e) {
                JOYNR_LOG_ERROR(logger,
                                "Incoming Message with ID {} could not be sent! reason: {}",
                                joynrMsg.getHeaderMessageId(),
                                e.getMessage());
            };
            transmit(joynrMsg, onFailure);
        } catch (const std::invalid_argument& e) {
            JOYNR_LOG_ERROR(logger,
                            "Unable to deserialize joynr message object from: {} - error: {}",
                            message,
                            e.what());
            return;
        }
    }

    bool isInitializationMessage(const std::string& message)
    {
        return boost::starts_with(
                message, "{\"_typeName\":\"joynr.system.RoutingTypes.WebSocketClientAddress\"");
    }

    Server endpoint;
    WebSocketPpReceiver<Server> receiver;
    /*! List of client connections */
    std::map<ConnectionHandle,
             joynr::system::RoutingTypes::WebSocketClientAddress,
             std::owner_less<ConnectionHandle>> clients;
    /*! Router for incoming messages */
    std::mutex clientsMutex;
    std::shared_ptr<MessageRouter> messageRouter;
    /*! Factory to build outgoing messaging stubs */
    std::shared_ptr<WebSocketMessagingStubFactory> messagingStubFactory;

    ADD_LOGGER(WebSocketCcMessagingSkeleton);
    DISALLOW_COPY_AND_ASSIGN(WebSocketCcMessagingSkeleton);
};

} // namespace joynr
#endif // WEBSOCKETCCMESSAGINGSKELETON_H
