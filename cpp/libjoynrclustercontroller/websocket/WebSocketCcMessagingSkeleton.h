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

#include <websocketpp/server.hpp>

#include <smrf/exceptions.h>

#include "joynr/BoostIoserviceForwardDecl.h"
#include "joynr/IMessageRouter.h"
#include "joynr/ImmutableMessage.h"
#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/Semaphore.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/system/RoutingTypes/WebSocketClientAddress.h"
#include "joynr/Util.h"
#include "libjoynr/websocket/WebSocketMessagingStubFactory.h"
#include "libjoynr/websocket/WebSocketPpReceiver.h"
#include "libjoynr/websocket/WebSocketPpSender.h"

namespace joynr
{

/**
 * @brief This interface provides type erasure for the templatized class below
 */
class IWebsocketCcMessagingSkeleton
{
public:
    virtual ~IWebsocketCcMessagingSkeleton() = default;
    virtual void transmit(
            std::shared_ptr<ImmutableMessage> message,
            const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure) = 0;
    virtual void init() = 0;
    virtual void shutdown() = 0;
};

/**
 * @class WebSocketCcMessagingSkeleton
 * @brief Messaging skeleton for the cluster controller
 */
template <typename Config>
class WebSocketCcMessagingSkeleton
        : public IWebsocketCcMessagingSkeleton,
          public std::enable_shared_from_this<WebSocketCcMessagingSkeleton<Config>>
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
            std::shared_ptr<WebSocketMessagingStubFactory> messagingStubFactory,
            std::uint16_t port)
            : IWebsocketCcMessagingSkeleton(),
              std::enable_shared_from_this<WebSocketCcMessagingSkeleton<Config>>(),
              _ioService(ioService),
              _webSocketPpSingleThreadedIOService(std::make_shared<SingleThreadedIOService>()),
              _endpoint(),
              _clientsMutex(),
              _clients(),
              _receiver(),
              _messageRouter(std::move(messageRouter)),
              _messagingStubFactory(std::move(messagingStubFactory)),
              _port(port),
              _shuttingDown(false)
    {
    }

    virtual void init() override
    {
        _webSocketPpSingleThreadedIOService->start();
        boost::asio::io_service& endpointIoService =
                _webSocketPpSingleThreadedIOService->getIOService();
        websocketpp::lib::error_code initializationError;

        _endpoint.init_asio(&endpointIoService, initializationError);
        if (initializationError) {
            JOYNR_LOG_FATAL(logger(),
                            "error during WebSocketCcMessagingSkeleton initialization: ",
                            initializationError.message());
            return;
        }
        _endpoint.clear_access_channels(websocketpp::log::alevel::all);
        _endpoint.clear_error_channels(websocketpp::log::alevel::all);

        // register handlers
        _endpoint.set_close_handler([thisWeakPtr = joynr::util::as_weak_ptr(
                                             this->shared_from_this())](ConnectionHandle hdl) {
            if (auto thisSharedPtr = thisWeakPtr.lock()) {
                thisSharedPtr->onConnectionClosed(hdl);
            }
        });

        // new connections are handled in onInitMessageReceived; if initialization was successful,
        // any further messages for this connection are handled in onMessageReceived
        _endpoint.set_message_handler([thisWeakPtr =
                                               joynr::util::as_weak_ptr(this->shared_from_this())](
                ConnectionHandle hdl, MessagePtr message) {
            if (auto thisSharedPtr = thisWeakPtr.lock()) {
                thisSharedPtr->onInitMessageReceived(hdl, message);
            }
        });

        _receiver.registerReceiveCallback([thisWeakPtr = joynr::util::as_weak_ptr(
                                                   this->shared_from_this())](
                ConnectionHandle && hdl, smrf::ByteVector && msg) {
            if (auto thisSharedPtr = thisWeakPtr.lock()) {
                thisSharedPtr->onMessageReceived(std::move(hdl), std::move(msg));
            }
        });
    }

    /**
     * @brief Destructor
     */
    ~WebSocketCcMessagingSkeleton() override
    {
        // make sure shutdown() has been invoked earlier
        assert(_shuttingDown);
    }

    void shutdown() override
    {
        // make sure shutdown() is called only once
        {
            std::lock_guard<std::mutex> lock1(_clientsMutex);
            assert(!_shuttingDown);
            _shuttingDown = true;
        }

        websocketpp::lib::error_code shutdownError;
        _endpoint.stop_listening(shutdownError);
        if (shutdownError) {
            JOYNR_LOG_ERROR(logger(),
                            "error during WebSocketCcMessagingSkeleton shutdown: ",
                            shutdownError.message());
        }

        for (const auto& elem : _clients) {
            websocketpp::lib::error_code websocketError;
            _endpoint.close(elem.first, websocketpp::close::status::normal, "", websocketError);
            if (websocketError) {
                if (websocketError != websocketpp::error::bad_connection) {
                    JOYNR_LOG_ERROR(logger(),
                                    "Unable to close websocket connection. Error: {}",
                                    websocketError.message());
                }
            }
        }

        // prior to destruction of the endpoint, the background thread
        // under direct control of the webSocketPpSingleThreadedIOService
        // must have finished its work, thus wait for it here;
        // however do not destruct the ioService since it is still
        // referenced within the endpoint by an internally created
        // thread from tcp::resolver which is joined by the endpoint
        // destructor
        _webSocketPpSingleThreadedIOService->stop();
    }

    void transmit(
            std::shared_ptr<ImmutableMessage> message,
            const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure) override
    {
        try {
            _messageRouter->route(std::move(message));
        } catch (exceptions::JoynrRuntimeException& e) {
            onFailure(e);
        }
    }

protected:
    using MessagePtr = typename Config::message_type::ptr;
    using Server = websocketpp::server<Config>;
    using ConnectionHandle = websocketpp::connection_hdl;

    ADD_LOGGER(WebSocketCcMessagingSkeleton)
    boost::asio::io_service& _ioService;
    std::shared_ptr<SingleThreadedIOService> _webSocketPpSingleThreadedIOService;
    Server _endpoint;

    virtual bool validateIncomingMessage(const ConnectionHandle& hdl,
                                         std::shared_ptr<ImmutableMessage> message) = 0;
    virtual bool preprocessIncomingMessage(std::shared_ptr<ImmutableMessage> message) = 0;

    void startAccept()
    {
        try {
            _endpoint.set_reuse_addr(true);
            _endpoint.listen(_port);
            _endpoint.start_accept();
        } catch (const std::exception& e) {
            JOYNR_LOG_FATAL(logger(), "WebSocket server could not be started: \"{}\"", e.what());
        }
    }

    // List of client connections
    struct CertEntry
    {
        CertEntry() : _webSocketClientAddress(), _ownerId()
        {
        }
        explicit CertEntry(
                const joynr::system::RoutingTypes::WebSocketClientAddress& webSocketClientAddress,
                std::string ownerId)
                : _webSocketClientAddress(webSocketClientAddress), _ownerId(std::move(ownerId))
        {
        }
        CertEntry(CertEntry&&) = default;
        CertEntry& operator=(CertEntry&&) = default;
        joynr::system::RoutingTypes::WebSocketClientAddress _webSocketClientAddress;
        std::string _ownerId;
    };

    std::mutex _clientsMutex;
    std::map<ConnectionHandle, CertEntry, std::owner_less<ConnectionHandle>> _clients;

private:
    void onConnectionClosed(ConnectionHandle hdl)
    {
        std::lock_guard<std::mutex> lock2(_clientsMutex);
        if (_shuttingDown) {
            return;
        }
        auto it = _clients.find(hdl);
        if (it != _clients.cend()) {
            JOYNR_LOG_INFO(logger(),
                           "Closed connection for websocket client id: {}",
                           it->second._webSocketClientAddress.getId());
            _messagingStubFactory->onMessagingStubClosed(it->second._webSocketClientAddress);
            _clients.erase(it);
        }
    }

    void onInitMessageReceived(ConnectionHandle hdl, MessagePtr message)
    {
        using websocketpp::frame::opcode::value;
        const value mode = message->get_opcode();
        if (mode != value::binary) {
            JOYNR_LOG_ERROR(
                    logger(),
                    "received an initial message of unsupported message type {}, dropping message",
                    mode);
            return;
        }
        const std::string& initMessage = message->get_payload();
        if (isInitializationMessage(initMessage)) {
            JOYNR_LOG_DEBUG(logger(),
                            "received initialization message from websocket client: {}",
                            initMessage);
            // register client with messaging stub factory
            std::shared_ptr<joynr::system::RoutingTypes::WebSocketClientAddress> clientAddress;
            try {
                joynr::serializer::deserializeFromJson(clientAddress, initMessage);
            } catch (const std::invalid_argument& e) {
                JOYNR_LOG_FATAL(
                        logger(),
                        "client address must be valid, otherwise libjoynr and CC are deployed "
                        "in different versions - raw: {} - error: {}",
                        initMessage,
                        e.what());
                return;
            }

            JOYNR_LOG_INFO(logger(),
                           "Init connection for websocket client id: {}",
                           clientAddress->getId());

            auto sender = std::make_shared<WebSocketPpSender<Server>>(_endpoint);
            sender->setConnectionHandle(hdl);

            _messagingStubFactory->addClient(*clientAddress, std::move(sender));

            typename Server::connection_ptr connection = _endpoint.get_con_from_hdl(hdl);
            connection->set_message_handler(
                    std::bind(&WebSocketPpReceiver<Server>::onMessageReceived,
                              &_receiver,
                              std::placeholders::_1,
                              std::placeholders::_2));
            {
                std::lock_guard<std::mutex> lock3(_clientsMutex);
                // search whether this connection handler has been mapped to a cert. (secure
                // connection)
                // if so, then move the client address to it. Otherwise, make a new entry with an
                // empty ownerId
                auto it = _clients.find(hdl);
                if (it != _clients.cend()) {
                    it->second._webSocketClientAddress = *clientAddress;
                } else {
                    // insecure connection, no CN exists
                    auto certEntry = CertEntry(*clientAddress, std::string());
                    _clients[hdl] = std::move(certEntry);
                }
            }

            _messageRouter->sendQueuedMessages(std::move(clientAddress));
        } else {
            JOYNR_LOG_ERROR(
                    logger(), "received an initial message with wrong format: \"{}\"", initMessage);
        }
    }

    void onMessageReceived(ConnectionHandle&& hdl, smrf::ByteVector&& message)
    {
        // deserialize message and transmit
        std::shared_ptr<ImmutableMessage> immutableMessage;
        try {
            immutableMessage = std::make_shared<ImmutableMessage>(std::move(message));
        } catch (const smrf::EncodingException& e) {
            JOYNR_LOG_ERROR(logger(), "Unable to deserialize message - error: {}", e.what());
            return;
        } catch (const std::invalid_argument& e) {
            JOYNR_LOG_ERROR(logger(), "deserialized message is not valid - error: {}", e.what());
            return;
        }

        if (logger().getLogLevel() == LogLevel::Debug) {
            JOYNR_LOG_DEBUG(logger(), "<<< INCOMING <<< {}", immutableMessage->getTrackingInfo());
        } else {
            JOYNR_LOG_TRACE(logger(), "<<< INCOMING <<< {}", immutableMessage->toLogMessage());
        }

        if (!preprocessIncomingMessage(immutableMessage)) {
            JOYNR_LOG_ERROR(logger(), "Dropping message {}", immutableMessage->getTrackingInfo());
            return;
        }

        if (!validateIncomingMessage(hdl, immutableMessage)) {
            JOYNR_LOG_ERROR(logger(), "Dropping message {}", immutableMessage->getTrackingInfo());
            return;
        }

        auto onFailure = [trackingInfo = immutableMessage->getTrackingInfo()](
                const exceptions::JoynrRuntimeException& e)
        {
            JOYNR_LOG_ERROR(logger(),
                            "Incoming Message {} could not be sent! reason: {}",
                            trackingInfo,
                            e.getMessage());
        };
        transmit(std::move(immutableMessage), std::move(onFailure));
    }

    bool isInitializationMessage(const std::string& message)
    {
        return boost::starts_with(
                message, "{\"_typeName\":\"joynr.system.RoutingTypes.WebSocketClientAddress\"");
    }

    std::shared_ptr<Semaphore> _webSocketPpSingleThreadedIOServiceDestructed;
    WebSocketPpReceiver<Server> _receiver;

    /*! Router for incoming messages */
    std::shared_ptr<IMessageRouter> _messageRouter;
    /*! Factory to build outgoing messaging stubs */
    std::shared_ptr<WebSocketMessagingStubFactory> _messagingStubFactory;
    std::uint16_t _port;
    std::atomic<bool> _shuttingDown;

    DISALLOW_COPY_AND_ASSIGN(WebSocketCcMessagingSkeleton);
};

} // namespace joynr
#endif // WEBSOCKETCCMESSAGINGSKELETON_H
