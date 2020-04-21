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
#ifndef UDSMESSAGINGSTUBFACTORY_H
#define UDSMESSAGINGSTUBFACTORY_H

#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "joynr/IMiddlewareMessagingStubFactory.h"
#include "joynr/Logger.h"
#include "joynr/system/RoutingTypes/UdsAddress.h"
#include "joynr/system/RoutingTypes/UdsClientAddress.h"

namespace joynr
{

class IUdsSender;
class IMessagingStub;

namespace system
{
namespace RoutingTypes
{
class Address;
} // namespace RoutingTypes
} // namespace system

class UdsMessagingStubFactory : public IMiddlewareMessagingStubFactory
{

public:
    UdsMessagingStubFactory();
    std::shared_ptr<IMessagingStub> create(
            const joynr::system::RoutingTypes::Address& destAddress) override;
    bool canCreate(const joynr::system::RoutingTypes::Address& destAddress) override;
    void addClient(const joynr::system::RoutingTypes::UdsClientAddress& clientAddress,
                   std::shared_ptr<IUdsSender> udsSender);
    void addServer(const joynr::system::RoutingTypes::UdsAddress& serverAddress,
                   std::shared_ptr<IUdsSender> udsSender);
    void onMessagingStubClosed(const joynr::system::RoutingTypes::Address& address);
    void registerOnMessagingStubClosedCallback(std::function<
            void(std::shared_ptr<const joynr::system::RoutingTypes::Address> destinationAddress)>
                                                       _onMessagingStubClosedCallback) override;

private:
    std::unordered_map<joynr::system::RoutingTypes::UdsAddress, std::shared_ptr<IMessagingStub>>
            _serverStubMap;
    std::mutex _serverStubMapMutex;
    std::unordered_map<joynr::system::RoutingTypes::UdsClientAddress,
                       std::shared_ptr<IMessagingStub>> _clientStubMap;
    std::mutex _clientStubMapMutex;
    std::function<void(std::shared_ptr<const joynr::system::RoutingTypes::Address>
                               destinationAddress)> _onMessagingStubClosedCallback;

    ADD_LOGGER(UdsMessagingStubFactory)
};

} // namespace joynr
#endif // UDSMESSAGINGSTUBFACTORY_H
