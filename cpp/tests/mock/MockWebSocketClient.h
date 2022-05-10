/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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
#ifndef TESTS_MOCK_MOCKWEBSOCKETCLIENT_H
#define TESTS_MOCK_MOCKWEBSOCKETCLIENT_H

#include "tests/utils/Gmock.h"

#include "libjoynr/websocket/IWebSocketPpClient.h"

class MockWebSocketClient : public joynr::IWebSocketPpClient
{
public:
    MOCK_METHOD0(dtorCalled, void());
    ~MockWebSocketClient() override
    {
        dtorCalled();
    }

    using ConnectionHandle = websocketpp::connection_hdl;
    MOCK_METHOD1(registerConnectCallback, void(std::function<void()>));
    MOCK_METHOD1(registerReconnectCallback, void(std::function<void()>));
    MOCK_METHOD1(registerReceiveCallback, void(std::function<void(ConnectionHandle&&, smrf::ByteVector&&)>));

    void registerDisconnectCallback(std::function<void()> callback) override
    {
        onConnectionClosedCallback = callback;
    }

    void signalDisconnect()
    {
        onConnectionClosedCallback();
    }

    MOCK_METHOD1(connect, void(const joynr::system::RoutingTypes::WebSocketAddress&));
    MOCK_METHOD0(close, void());
    MOCK_METHOD0(stop, void());

    MOCK_CONST_METHOD0(isConnected, bool());

    MOCK_METHOD2(send, void(
            const smrf::ByteArrayView&,
            const std::function<void(const joynr::exceptions::JoynrRuntimeException&)>&));

    MOCK_CONST_METHOD0(getSender, std::shared_ptr<joynr::IWebSocketSendInterface>());

private:
    std::function<void()> onConnectionClosedCallback;
};

#endif // TESTS_MOCK_MOCKWEBSOCKETCLIENT_H
