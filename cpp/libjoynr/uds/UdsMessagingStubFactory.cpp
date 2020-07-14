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
#include "UdsMessagingStubFactory.h"

#include "joynr/IMessagingStub.h"
#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/system/RoutingTypes/UdsAddress.h"
#include "joynr/system/RoutingTypes/UdsClientAddress.h"

#include "UdsMessagingStub.h"

namespace joynr
{

UdsMessagingStubFactory::UdsMessagingStubFactory()
        : IMiddlewareMessagingStubFactory(),
          _onMessagingStubClosedCallback(nullptr),
          _serverStubMap(),
          _serverStubMapMutex(),
          _clientStubMap(),
          _clientStubMapMutex()
{
}

UdsMessagingStubFactory::~UdsMessagingStubFactory()
{
    {
        std::lock_guard<std::mutex> lock(_clientStubMapMutex);
        _clientStubMap.clear();
    }
    {
        std::lock_guard<std::mutex> lock(_serverStubMapMutex);
        _serverStubMap.clear();
    }
}

bool UdsMessagingStubFactory::canCreate(const joynr::system::RoutingTypes::Address& destAddress)
{
    return dynamic_cast<const system::RoutingTypes::UdsAddress*>(&destAddress) != nullptr ||
           dynamic_cast<const system::RoutingTypes::UdsClientAddress*>(&destAddress) != nullptr;
}

std::shared_ptr<IMessagingStub> UdsMessagingStubFactory::create(
        const joynr::system::RoutingTypes::Address& destAddress)
{
    // if destination is a uds client address
    if (auto udsClientAddress =
                dynamic_cast<const system::RoutingTypes::UdsClientAddress*>(&destAddress)) {
        std::lock_guard<std::mutex> lock(_clientStubMapMutex);
        const auto stub = _clientStubMap.find(*udsClientAddress);
        if (stub == _clientStubMap.cend()) {
            JOYNR_LOG_ERROR(logger(),
                            "No client messaging stub found for address {}",
                            udsClientAddress->toString());
            return std::shared_ptr<IMessagingStub>();
        }
        return stub->second;
    }
    // if destination is a uds server address
    if (const system::RoutingTypes::UdsAddress* udsServerAddress =
                dynamic_cast<const system::RoutingTypes::UdsAddress*>(&destAddress)) {
        std::lock_guard<std::mutex> lock(_serverStubMapMutex);
        const auto stub = _serverStubMap.find(*udsServerAddress);
        if (stub == _serverStubMap.cend()) {
            JOYNR_LOG_ERROR(logger(),
                            "No server messaging stub found for address {}",
                            udsServerAddress->toString());
            return std::shared_ptr<IMessagingStub>();
        }
        return stub->second;
    }

    return std::shared_ptr<IMessagingStub>();
}

void UdsMessagingStubFactory::addClient(const system::RoutingTypes::UdsClientAddress& clientAddress,
                                        std::shared_ptr<IUdsSender> udsSender)
{
    bool inserted;
    {
        auto udsClientStub = std::make_shared<UdsMessagingStub>(std::move(udsSender));
        std::lock_guard<std::mutex> lock(_clientStubMapMutex);
        inserted = _clientStubMap.insert(std::pair<joynr::system::RoutingTypes::UdsClientAddress,
                                                   std::shared_ptr<IMessagingStub>>(
                                                 clientAddress, std::move(udsClientStub))).second;
    }
    if (inserted) {
        JOYNR_LOG_INFO(logger(), "Added messaging stub for address: {}", clientAddress.toString());
    } else {
        JOYNR_LOG_ERROR(logger(),
                        "Client with address {} already exists in the clientStubMap",
                        clientAddress.toString());
    }
}

void UdsMessagingStubFactory::addServer(
        const joynr::system::RoutingTypes::UdsAddress& serverAddress,
        std::shared_ptr<IUdsSender> udsSender)
{
    bool inserted;
    {
        auto udsServerStub = std::make_shared<UdsMessagingStub>(std::move(udsSender));
        std::lock_guard<std::mutex> lock(_serverStubMapMutex);
        inserted = _serverStubMap.insert(std::pair<joynr::system::RoutingTypes::UdsAddress,
                                                   std::shared_ptr<IMessagingStub>>(
                                                 serverAddress, std::move(udsServerStub))).second;
    }
    if (inserted) {
        JOYNR_LOG_INFO(logger(), "Added messaging stub for address: {}", serverAddress.toString());
    } else {
        JOYNR_LOG_ERROR(logger(),
                        "Server with address {} already exists in the serverStubMap",
                        serverAddress.toString());
    }
}

void UdsMessagingStubFactory::onMessagingStubClosed(const system::RoutingTypes::Address& address)
{
    std::shared_ptr<const system::RoutingTypes::Address> addressPtr = nullptr;
    if (auto udsClientAddress =
                dynamic_cast<const system::RoutingTypes::UdsClientAddress*>(&address)) {
        JOYNR_LOG_INFO(logger(), "removing messaging stub for address: {}", address.toString());
        std::unique_lock<std::mutex> lock(_clientStubMapMutex);
        _clientStubMap.erase(*udsClientAddress);
        lock.unlock();
        addressPtr =
                std::make_shared<const system::RoutingTypes::UdsClientAddress>(*udsClientAddress);
    } else if (auto udsServerAddress =
                       dynamic_cast<const system::RoutingTypes::UdsAddress*>(&address)) {
        JOYNR_LOG_INFO(logger(), "removing messaging stub for address: {}", address.toString());
        std::unique_lock<std::mutex> lock(_serverStubMapMutex);
        _serverStubMap.erase(*udsServerAddress);
        lock.unlock();
        addressPtr = std::make_shared<const system::RoutingTypes::UdsAddress>(*udsServerAddress);
    } else {
        JOYNR_LOG_ERROR(logger(),
                        "cannot remove messaging stub for non UDS address {}",
                        address.toString());
        return;
    }
    if (_onMessagingStubClosedCallback) {
        _onMessagingStubClosedCallback(addressPtr);
    }
}

void UdsMessagingStubFactory::registerOnMessagingStubClosedCallback(
        std::function<void(std::shared_ptr<const joynr::system::RoutingTypes::Address>
                                   destinationAddress)> onMessagingStubClosedCallback)
{
    this->_onMessagingStubClosedCallback = std::move(onMessagingStubClosedCallback);
}

} // namespace joynr
