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
#include <atomic>
#include <chrono>
#include <cstdio>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/Settings.h"
#include "joynr/UdsClient.h"
#include "joynr/UdsServer.h"
#include "joynr/UdsSettings.h"
#include "joynr/system/RoutingTypes/UdsClientAddress.h"

#include "tests/mock/MockUdsServerCallbacks.h"

// Fixture creates nominal client for testing server implementation interaction
class UdsServerTest : public ::testing::Test
{
private:
    static constexpr char _settingsFile[] = "./UdsServerTest-does-not-exist.settings";
    joynr::Settings _settingsDb;
    std::unique_ptr<joynr::UdsClient> _client;
    std::atomic_bool _clientConnected;

protected:
    joynr::UdsSettings _settings;
    std::vector<smrf::ByteVector> _messagesReceivedByClient;
    std::mutex _syncAllMutex;

    // Global settings for timeout, when communication between client-server is checked
    static const std::chrono::seconds _waitPeriodForClientServerCommunication;
    static const std::chrono::seconds _retryIntervalDuringClientServerCommunication;

    class ErroneousClient
    {
        boost::asio::local::stream_protocol::endpoint _endpoint;
        boost::asio::io_service _ioContext;
        boost::asio::local::stream_protocol::socket _socket;

    public:
        ErroneousClient(const joynr::UdsSettings& settings)
                : _endpoint(settings.getSocketPath()), _socket(_ioContext)
        {
            _socket.connect(_endpoint);
        }

        template <typename CONTAINER>
        bool write(const CONTAINER& bytes)
        {
            boost::system::error_code error;
            boost::asio::write(_socket, boost::asio::buffer(bytes.data(), bytes.size()), error);
            return !error;
        }

        bool waitTillClose()
        {
            const auto tbegin = std::chrono::steady_clock::now();
            while (_waitPeriodForClientServerCommunication >
                   (std::chrono::steady_clock::now() - tbegin)) {
                std::this_thread::sleep_for(_retryIntervalDuringClientServerCommunication);
                static smrf::ByteVector dummyByte(1, 0x00);
                boost::system::error_code error;
                // Test connection
                boost::asio::write(
                        _socket, boost::asio::buffer(dummyByte.data(), dummyByte.size()), error);
                if (error) {
                    return true;
                }
            }
            return false;
        }
    };

    std::unique_ptr<joynr::UdsServer> createServer()
    {
        return std::make_unique<joynr::UdsServer>(_settings);
    }

    std::unique_ptr<joynr::UdsServer> createServer(MockUdsServerCallbacks& mock)
    {
        auto udsServerTest = createServer();
        udsServerTest->setConnectCallback([&mock](
                const joynr::system::RoutingTypes::UdsClientAddress& id,
                std::shared_ptr<joynr::IUdsSender> connection) { mock.connected(id, connection); });
        udsServerTest->setDisconnectCallback(
                [&mock](const joynr::system::RoutingTypes::UdsClientAddress& id) {
                    mock.disconnected(id);
                });
        udsServerTest->setReceiveCallback(
                [&mock](const joynr::system::RoutingTypes::UdsClientAddress& id,
                        smrf::ByteVector&& val, const std::string& creator) { mock.received(id, std::move(val), creator); });
        return udsServerTest;
    }

    void sendFromClient(smrf::Byte message)
    {
        smrf::ByteVector messageVector(1, message);
        smrf::ByteArrayView messageView(messageVector);
        _client->send(messageView, [](const joynr::exceptions::JoynrRuntimeException&) {});
    }

    template <typename V>
    std::size_t waitFor(V& member, const std::size_t& expected)
    { // pass expected by value
        const auto tbegin = std::chrono::steady_clock::now();
        while (_waitPeriodForClientServerCommunication >
               (std::chrono::steady_clock::now() - tbegin)) {
            {
                std::lock_guard<std::mutex> lck(_syncAllMutex);
                if (member.size() >= expected) {
                    return member.size();
                }
            }
            std::this_thread::sleep_for(_retryIntervalDuringClientServerCommunication);
        }
        std::lock_guard<std::mutex> lck(_syncAllMutex);
        return member.size();
    }

    bool waitClientConnected(const bool& expected)
    { // pass expected by value
        const auto tbegin = std::chrono::steady_clock::now();
        while (_waitPeriodForClientServerCommunication >
               (std::chrono::steady_clock::now() - tbegin)) {
            {
                if (_clientConnected.load() == expected) {
                    return expected;
                }
            }
            std::this_thread::sleep_for(_retryIntervalDuringClientServerCommunication);
        }
        return _clientConnected.load();
    }

    static void sendToClient(std::shared_ptr<joynr::IUdsSender>& user,
                             const smrf::ByteVector& msg,
                             const joynr::IUdsSender::SendFailed& callback =
                                     [](const joynr::exceptions::JoynrRuntimeException&) {})
    {
        user->send(smrf::ByteArrayView(msg), callback);
    }

    static void sendToClient(std::shared_ptr<joynr::IUdsSender>& user,
                             const smrf::ByteVector& msg,
                             MockUdsServerCallbacks& mock)
    {
        sendToClient(user, msg, [&mock](const joynr::exceptions::JoynrRuntimeException& exception) {
            mock.sendFailed(exception);
        });
    }

public:
    UdsServerTest() : _settingsDb(_settingsFile), _clientConnected{false}, _settings(_settingsDb)
    {
        _settings.setSocketPath("./UdsServerTest.sock");
        restartClient();
    }

    ~UdsServerTest()
    {
        _client.reset();
    }

    void restartClient()
    {
        // This test is for the server, not for the client. The latter one just acts as a nominal
        // counter part.
        static std::function<void(const joynr::exceptions::JoynrRuntimeException&)>
                ignoreClientFatalRuntimeErrors =
                        [](const joynr::exceptions::JoynrRuntimeException&) {};

        _clientConnected.store(false);
        _client = std::make_unique<joynr::UdsClient>(_settings, ignoreClientFatalRuntimeErrors);
        _client->setConnectCallback([this]() { _clientConnected.store(true); });
        _client->setDisconnectCallback([this]() { _clientConnected.store(false); });
        _client->setReceiveCallback([this](smrf::ByteVector&& message) mutable {
            std::lock_guard<std::mutex> lck(_syncAllMutex);
            _messagesReceivedByClient.push_back(std::move(message));
        });
        _client->start();
    }

    void TearDown() override
    {
        remove(_settingsFile);
    }

    void stopClient()
    {
        _client.reset();
    }
};
