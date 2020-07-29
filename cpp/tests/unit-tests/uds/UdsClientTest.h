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
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <smrf/ByteVector.h>

#include "joynr/Settings.h"
#include "joynr/UdsClient.h"
#include "joynr/UdsServer.h"
#include "joynr/UdsSettings.h"
#include "joynr/system/RoutingTypes/UdsClientAddress.h"

#include "tests/mock/MockUdsClientCallbacks.h"

// Fixture creates nominal server for testing client implementation
class UdsClientTest : public ::testing::Test
{
private:
    static constexpr char _settingsFile[] = "./UdsClientTest-does-not-exist.settings";

    joynr::Settings _settingsDb;
    std::unique_ptr<joynr::UdsServer> _server;

protected:
    struct ClientInfo
    {
        joynr::system::RoutingTypes::UdsClientAddress _address;
        std::weak_ptr<joynr::IUdsSender> _sender;
        std::vector<smrf::ByteVector> _receivedMessages;
        ClientInfo(joynr::system::RoutingTypes::UdsClientAddress address,
                   std::shared_ptr<joynr::IUdsSender>& sender)
                : _address{address}, _sender{std::weak_ptr<joynr::IUdsSender>(sender)}
        {
        }
        bool operator==(const joynr::system::RoutingTypes::UdsClientAddress& address)
        {
            return _address == address;
        }
    };

    // Global settings for timeout, when communication between client-server is checked
    static const std::chrono::seconds _waitPeriodForClientServerCommunication;
    static const std::chrono::milliseconds _retryIntervalDuringClientServerCommunication;

    joynr::UdsSettings _udsSettings;
    std::mutex _connectedClientsMutex;
    std::vector<ClientInfo> _connectedClients;

    std::size_t countOpenServerConnections()
    {
        std::size_t sum{0};
        std::lock_guard<std::mutex> lck(_connectedClientsMutex);
        for (const auto& clientInfo : _connectedClients) {
            sum += clientInfo._sender.expired() ? 0u : 1u;
        }
        return sum;
    }

    std::unique_ptr<joynr::UdsClient> createClient(
            std::function<void(const joynr::exceptions::JoynrRuntimeException&)>
                    onFatalRuntimeError = [](const joynr::exceptions::JoynrRuntimeException&) {})
    {
        auto newClientSettings = _udsSettings;
        {
            std::lock_guard<std::mutex> lck(_connectedClientsMutex);
            newClientSettings.setClientId(std::string("Client-ID ") +
                                          std::to_string(_connectedClients.size()));
        }
        auto result = std::make_unique<joynr::UdsClient>(_udsSettings, onFatalRuntimeError);
        return result;
    }

    std::unique_ptr<joynr::UdsClient> createClient(MockUdsClientCallbacks& mock)
    {
        auto result = createClient(std::bind(
                &MockUdsClientCallbacks::fatalRuntimeError, &mock, std::placeholders::_1));
        result->setConnectCallback(std::bind(&MockUdsClientCallbacks::connected, &mock));
        result->setDisconnectCallback(std::bind(&MockUdsClientCallbacks::disconnected, &mock));
        result->setReceiveCallback(
                [&mock](smrf::ByteVector&& val) { mock.received(std::move(val)); });
        return result;
    }

    std::size_t countServerConnections(const std::size_t& expected)
    {
        const auto tbegin = std::chrono::steady_clock::now();
        while (_waitPeriodForClientServerCommunication >
               (std::chrono::steady_clock::now() - tbegin)) {
            if (countOpenServerConnections() == expected) {
                return expected;
            }
            std::this_thread::sleep_for(_retryIntervalDuringClientServerCommunication);
        }
        return countOpenServerConnections();
    }

    template <typename V>
    std::size_t waitFor(V& member, const std::size_t& expected)
    {
        const auto tbegin = std::chrono::steady_clock::now();
        while (_waitPeriodForClientServerCommunication >
               (std::chrono::steady_clock::now() - tbegin)) {
            {
                std::lock_guard<std::mutex> lck(_connectedClientsMutex);
                if (member.size() == expected) {
                    return expected;
                }
            }
            std::this_thread::sleep_for(_retryIntervalDuringClientServerCommunication);
        }
        std::lock_guard<std::mutex> lck(_connectedClientsMutex);
        return member.size();
    }

    static std::size_t waitForGreaterThan(std::atomic_uint32_t& value,
                                          const std::uint32_t& expected)
    {
        const auto tbegin = std::chrono::steady_clock::now();
        while (_waitPeriodForClientServerCommunication >
               (std::chrono::steady_clock::now() - tbegin)) {
            {
                if (value.load() >= expected) {
                    return expected;
                }
            }
            std::this_thread::sleep_for(_retryIntervalDuringClientServerCommunication);
        }
        return value.load();
    }

    void sendFromServer(smrf::Byte value)
    {
        std::lock_guard<std::mutex> lck(_connectedClientsMutex);
        smrf::ByteVector array(1, value);
        smrf::ByteArrayView view(array);
        for (auto& clientInfo : _connectedClients) {
            auto senderLock = clientInfo._sender.lock();
            if (senderLock) {
                senderLock->send(view, [](const joynr::exceptions::JoynrRuntimeException&) {});
            }
        }
    }

    static void sendFromClient(std::unique_ptr<joynr::UdsClient>& client,
                               const smrf::ByteVector& msg,
                               const joynr::IUdsSender::SendFailed& callback =
                                       [](const joynr::exceptions::JoynrRuntimeException&) {})
    {
        client->send(smrf::ByteArrayView(msg), callback);
    }

    static void sendFromClient(std::unique_ptr<joynr::UdsClient>& client,
                               const smrf::ByteVector& msg,
                               MockUdsClientCallbacks& mock)
    {
        sendFromClient(
                client, msg, [&mock](const joynr::exceptions::JoynrRuntimeException& exception) {
                    mock.sendFailed(exception);
                });
    }

    void stopServer()
    {
        _server.reset();
    }

public:
    UdsClientTest() : _settingsDb(_settingsFile), _udsSettings(_settingsDb)
    {
        _udsSettings.setSocketPath("./UdsClientTest.sock");
    }

    ~UdsClientTest()
    {
        // Assure that server is stopped before deleting memory accessed by the registered callbacks
        _server.reset();
    }

    void SetUp() override
    {
        _server = std::make_unique<joynr::UdsServer>(_udsSettings);
        _server->setConnectCallback(
                [this](const joynr::system::RoutingTypes::UdsClientAddress& address,
                       std::shared_ptr<joynr::IUdsSender> sender) {
                    std::lock_guard<std::mutex> lck(_connectedClientsMutex);
                    _connectedClients.push_back(ClientInfo(address, sender));
                });
        _server->setDisconnectCallback([this](
                const joynr::system::RoutingTypes::UdsClientAddress& address) {
            std::lock_guard<std::mutex> lck(_connectedClientsMutex);
            if (_connectedClients.end() ==
                std::find(_connectedClients.begin(), _connectedClients.end(), address)) {
                FAIL() << "Client with ID " << address.getId() << " has not connected (yet).";
            }
        });
        _server->setReceiveCallback([this](
                const joynr::system::RoutingTypes::UdsClientAddress& address,
                smrf::ByteVector&& message, const std::string& creator) {
            std::ignore = creator;
            std::lock_guard<std::mutex> lck(_connectedClientsMutex);
            auto clientInfo =
                    std::find(_connectedClients.begin(), _connectedClients.end(), address);
            if (_connectedClients.end() == clientInfo) {
                FAIL() << "Client with ID " << address.getId() << " has not connected (yet).";
            } else {
                clientInfo->_receivedMessages.push_back(std::move(message));
            }
        });
        _server->start();
    }

    void TearDown() override
    {
        remove(_settingsFile);
    }
};
