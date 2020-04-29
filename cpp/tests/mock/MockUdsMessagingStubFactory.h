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
#ifndef TESTS_MOCKUDSMESSAGINGSTUBFACTORY_H
#define TESTS_MOCKUDSMESSAGINGSTUBFACTORY_H

#include <functional>
#include <memory>

#include <gmock/gmock.h>

#include "joynr/exceptions/JoynrException.h"
#include "libjoynr/uds/UdsMessagingStubFactory.h"

namespace joynr
{
    class IMessagingStub;
    class ImmutableMessage;
    class IUdsSender;

namespace system
{
namespace RoutingTypes
{
class Address;
} // namespace RoutingTypes
} // namespace system
} // namespace joynr

class MockUdsMessagingStubFactory : public joynr::UdsMessagingStubFactory
{
public:
    MOCK_METHOD0(dtorCalled, void());
    ~MockUdsMessagingStubFactory() override
    {
        dtorCalled();
    }

    MOCK_METHOD1(create, std::shared_ptr<joynr::IMessagingStub>(const joynr::system::RoutingTypes::Address& destAddress));
    MOCK_METHOD1(canCreate, bool(const joynr::system::RoutingTypes::Address& destAddress));
    MOCK_METHOD2(addClient, void(const joynr::system::RoutingTypes::UdsClientAddress& clientAddress, std::shared_ptr<joynr::IUdsSender> udsSender));
    MOCK_METHOD2(addServer, void(const joynr::system::RoutingTypes::UdsAddress& serverAddress, std::shared_ptr<joynr::IUdsSender> udsSender));
    MOCK_METHOD1(onMessagingStubClosed, void(const joynr::system::RoutingTypes::Address& address));
    MOCK_METHOD1(registerOnMessagingStubClosedCallback, void(std::function<void(std::shared_ptr<const joynr::system::RoutingTypes::Address> destinationAddress)> _onMessagingStubClosedCallback));
};

#endif // TESTS_MOCKUDSMESSAGINGSTUBFACTORY_H
