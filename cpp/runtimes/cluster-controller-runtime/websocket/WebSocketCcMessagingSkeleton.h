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

#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio/io_service.hpp>
#include <websocketpp/server.hpp>

#include "joynr/IMessageRouter.h"
#include "joynr/IMessaging.h"
#include "joynr/JoynrMessage.h"
#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/system/RoutingTypes/WebSocketClientAddress.h"
#include "libjoynr/websocket/WebSocketMessagingStubFactory.h"
#include "libjoynr/websocket/WebSocketPpReceiver.h"
#include "libjoynr/websocket/WebSocketPpSender.h"

namespace joynr
{

/**
 * @class WebSocketCcMessagingSkeleton
 * @brief Messaging skeleton for the cluster controller
 */
template <typename Config>
class WebSocketCcMessagingSkeleton : public IMessaging
{
public:
    /**
     * @brief Constructor
     * @param messageRouter Router
     * @param messagingStubFactory Factory
     */
    WebSocketCcMessagingSkeleton(
            boost::asio::io_service& ioService,
            std::shared_ptr<IMessageRouter> messageRouter,
            std::shared_ptr<WebSocketMessagingStubFactory> messagingStubFactory)
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
        endpoint.set_close_handler(std::bind(
                &WebSocketCcMessagingSkeleton::onConnectionClosed, this, std::placeholders::_1));

        // new connections are handled in onInitMessageReceived; if initialization was successful,
        // any further messages for this connection are handled in onMessageReceived
        endpoint.set_message_handler(std::bind(&WebSocketCcMessagingSkeleton::onInitMessageReceived,
                                               this,
                                               std::placeholders::_1,
                                               std::placeholders::_2));

        receiver.registerReceiveCallback(
                [this](const std::string& msg) { onMessageReceived(msg); });
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

protected:
    using MessagePtr = typename Config::message_type::ptr;
    using Server = websocketpp::server<Config>;
    using ConnectionHandle = websocketpp::connection_hdl;

    ADD_LOGGER(WebSocketCcMessagingSkeleton);
    Server endpoint;

    void startAccept(std::uint16_t port)
    {
        try {
            endpoint.set_reuse_addr(true);
            endpoint.listen(port);
            endpoint.start_accept();
        } catch (const std::exception& e) {
            JOYNR_LOG_FATAL(logger, "WebSocket server could not be started: \"{}\"", e.what());
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
        using websocketpp::frame::opcode::value;
        const value mode = message->get_opcode();
        if (mode != value::binary) {
            JOYNR_LOG_ERROR(
                    logger,
                    "received an initial message of unsupported message type {}, dropping message",
                    mode);
            return;
        }
        std::string textMessage = message->get_payload();
        if (isInitializationMessage(textMessage)) {
            JOYNR_LOG_DEBUG(logger,
                            "received initialization message from websocket client: {}",
                            message->get_payload());
            // register client with messaging stub factory
            joynr::system::RoutingTypes::WebSocketClientAddress clientAddress;
            try {
                joynr::serializer::deserializeFromJson(clientAddress, textMessage);
            } catch (const std::invalid_argument& e) {
                JOYNR_LOG_FATAL(
                        logger,
                        "client address must be valid, otherwise libjoynr and CC are deployed "
                        "in different versions - raw: {} - error: {}",
                        textMessage,
                        e.what());
                return;
            }

            auto sender = std::make_shared<WebSocketPpSender<Server>>(endpoint);
            sender->setConnectionHandle(hdl);

            messagingStubFactory->addClient(clientAddress, sender);

            typename Server::connection_ptr connection = endpoint.get_con_from_hdl(hdl);
            connection->set_message_handler(
                    std::bind(&WebSocketPpReceiver<Server>::onMessageReceived,
                              &receiver,
                              std::placeholders::_1,
                              std::placeholders::_2));
            {
                std::unique_lock<std::mutex> lock(clientsMutex);
                clients[hdl] = clientAddress;
            }
        } else {
            JOYNR_LOG_ERROR(
                    logger, "received an initial message with wrong format: \"{}\"", textMessage);
        }
    }

    void onMessageReceived(const std::string& message)
    {
        // deserialize message and transmit
        JoynrMessage joynrMsg;
        try {
            joynr::serializer::deserializeFromJson(joynrMsg, message);
        } catch (const std::invalid_argument& e) {
            JOYNR_LOG_ERROR(logger,
                            "Unable to deserialize joynr message object from: {} - error: {}",
                            message,
                            e.what());
            return;
        }

        if (joynrMsg.getType().empty()) {
            JOYNR_LOG_ERROR(logger, "Message type is empty : {}", message);
            return;
        }
        if (joynrMsg.getPayload().empty()) {
            JOYNR_LOG_ERROR(logger, "joynr message payload is empty: {}", message);
            return;
        }
        if (!joynrMsg.containsHeaderExpiryDate()) {
            JOYNR_LOG_ERROR(logger,
                            "received message [msgId=[{}] without decay time - dropping message",
                            joynrMsg.getHeaderMessageId());
            return;
        }

        JOYNR_LOG_DEBUG(logger, "<<<< INCOMING <<<< {}", joynrMsg.toLogMessage());

        auto onFailure = [joynrMsg](const exceptions::JoynrRuntimeException& e) {
            JOYNR_LOG_ERROR(logger,
                            "Incoming Message with ID {} could not be sent! reason: {}",
                            joynrMsg.getHeaderMessageId(),
                            e.getMessage());
        };
        transmit(joynrMsg, onFailure);
    }

    bool isInitializationMessage(const std::string& message)
    {
        return boost::starts_with(
                message, "{\"_typeName\":\"joynr.system.RoutingTypes.WebSocketClientAddress\"");
    }

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

    DISALLOW_COPY_AND_ASSIGN(WebSocketCcMessagingSkeleton);
};

template <typename Config>
INIT_LOGGER(WebSocketCcMessagingSkeleton<Config>);

} // namespace joynr
#endif // WEBSOCKETCCMESSAGINGSKELETON_H
