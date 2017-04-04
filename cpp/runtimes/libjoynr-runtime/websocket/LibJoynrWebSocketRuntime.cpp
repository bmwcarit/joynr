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
#include <cassert>
#include "runtimes/libjoynr-runtime/websocket/LibJoynrWebSocketRuntime.h"
#include "libjoynr/websocket/WebSocketMessagingStubFactory.h"
#include "joynr/system/RoutingTypes/WebSocketClientAddress.h"
#include "libjoynr/websocket/WebSocketLibJoynrMessagingSkeleton.h"
#include "joynr/Util.h"
#include "libjoynr/websocket/WebSocketPpClientTLS.h"
#include "libjoynr/websocket/WebSocketPpClientNonTLS.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/WebSocketMulticastAddressCalculator.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/SingleThreadedIOService.h"

namespace joynr
{

INIT_LOGGER(LibJoynrWebSocketRuntime);

LibJoynrWebSocketRuntime::LibJoynrWebSocketRuntime(std::unique_ptr<Settings> settings)
        : LibJoynrRuntime(std::move(settings)),
          wsSettings(*this->settings),
          websocket(nullptr),
          initializationMsg()
{
    createWebsocketClient();
}

LibJoynrWebSocketRuntime::~LibJoynrWebSocketRuntime()
{
    assert(websocket);
    websocket->close();

    // synchronously stop the underlying boost::asio::io_service
    // this ensures all asynchronous operations are stopped now
    // which allows a safe shutdown
    assert(singleThreadIOService);
    singleThreadIOService->stop();
}

void LibJoynrWebSocketRuntime::connect(
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onError)
{
    std::string uuid = util::createUuid();
    // remove dashes
    uuid.erase(std::remove(uuid.begin(), uuid.end(), '-'), uuid.end());
    std::string libjoynrMessagingId = "libjoynr.messaging.participantid_" + uuid;
    auto libjoynrMessagingAddress =
            std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>(
                    libjoynrMessagingId);

    // send initialization message containing libjoynr messaging address
    initializationMsg = joynr::serializer::serializeToJson(*libjoynrMessagingAddress);
    JOYNR_LOG_TRACE(logger,
                    "OUTGOING sending websocket intialization message\nmessage: {}\nto: {}",
                    initializationMsg,
                    libjoynrMessagingAddress->toString());

    // create connection to parent routing service
    auto ccMessagingAddress = std::make_shared<const joynr::system::RoutingTypes::WebSocketAddress>(
            wsSettings.createClusterControllerMessagingAddress());

    auto factory = std::make_shared<WebSocketMessagingStubFactory>();
    factory->addServer(*ccMessagingAddress, websocket->getSender());

    std::weak_ptr<WebSocketMessagingStubFactory> weakFactoryRef(factory);
    websocket->registerDisconnectCallback([weakFactoryRef, ccMessagingAddress]() {
        if (auto factory = weakFactoryRef.lock()) {
            factory->onMessagingStubClosed(*ccMessagingAddress);
        }
    });

    auto connectCallback = [
        this,
        onSuccess = std::move(onSuccess),
        onError = std::move(onError),
        factory,
        libjoynrMessagingAddress,
        ccMessagingAddress
    ]()
    {
        sendInitializationMsg();

        std::unique_ptr<IMulticastAddressCalculator> addressCalculator =
                std::make_unique<joynr::WebSocketMulticastAddressCalculator>(ccMessagingAddress);
        init(factory,
             libjoynrMessagingAddress,
             ccMessagingAddress,
             std::move(addressCalculator),
             std::move(onSuccess),
             std::move(onError));
    };

    auto reconnectCallback = [this]() { sendInitializationMsg(); };

    websocket->registerConnectCallback(connectCallback);
    websocket->registerReconnectCallback(reconnectCallback);
    websocket->connect(*ccMessagingAddress);
}

void LibJoynrWebSocketRuntime::sendInitializationMsg()
{
    auto onFailure = [this](const exceptions::JoynrRuntimeException& e) {
        // initialization message will be sent after reconnect
        JOYNR_LOG_ERROR(logger,
                        "Sending websocket initialization message failed. Error: {}",
                        e.getMessage());
    };
    websocket->send(initializationMsg, onFailure);
}

void LibJoynrWebSocketRuntime::createWebsocketClient()
{
    system::RoutingTypes::WebSocketAddress webSocketAddress =
            wsSettings.createClusterControllerMessagingAddress();

    std::string certificateAuthorityPemFilename = wsSettings.getCertificateAuthorityPemFilename();
    std::string certificatePemFilename = wsSettings.getCertificatePemFilename();
    std::string privateKeyPemFilename = wsSettings.getPrivateKeyPemFilename();

    if (webSocketAddress.getProtocol() == system::RoutingTypes::WebSocketProtocol::WSS) {
        if (checkAndLogCryptoFileExistence(certificateAuthorityPemFilename,
                                           certificatePemFilename,
                                           privateKeyPemFilename,
                                           logger)) {
            JOYNR_LOG_INFO(logger, "Using TLS connection");
            websocket =
                    std::make_shared<WebSocketPpClientTLS>(wsSettings,
                                                           singleThreadIOService->getIOService(),
                                                           certificateAuthorityPemFilename,
                                                           certificatePemFilename,
                                                           privateKeyPemFilename);
        } else {
            throw exceptions::JoynrRuntimeException(
                    "Settings property 'cluster-controller-messaging-url' uses TLS "
                    "but not all TLS properties were configured");
        }
    } else if (webSocketAddress.getProtocol() == system::RoutingTypes::WebSocketProtocol::WS) {
        JOYNR_LOG_INFO(logger, "Using non-TLS connection");
        websocket = std::make_shared<WebSocketPpClientNonTLS>(
                wsSettings, singleThreadIOService->getIOService());
    } else {
        throw exceptions::JoynrRuntimeException(
                "Unknown protocol used for settings property 'cluster-controller-messaging-url'");
    }
}

void LibJoynrWebSocketRuntime::startLibJoynrMessagingSkeleton(
        std::shared_ptr<IMessageRouter> messageRouter)
{
    auto wsLibJoynrMessagingSkeleton =
            std::make_shared<WebSocketLibJoynrMessagingSkeleton>(util::as_weak_ptr(messageRouter));
    websocket->registerReceiveCallback([wsLibJoynrMessagingSkeleton](const std::string& msg) {
        wsLibJoynrMessagingSkeleton->onMessageReceived(msg);
    });
}

} // namespace joynr
