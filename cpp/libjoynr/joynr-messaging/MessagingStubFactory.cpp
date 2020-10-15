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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunsafe-loop-optimizations"

#include "joynr/MessagingStubFactory.h"

#include <utility>

#include "joynr/IMiddlewareMessagingStubFactory.h"

namespace joynr
{

MessagingStubFactory::MessagingStubFactory() : _address2MessagingStubMap(), _factoryList(), mutex()
{
}

std::shared_ptr<IMessagingStub> MessagingStubFactory::create(
        const std::shared_ptr<const joynr::system::RoutingTypes::Address>& destinationAddress)
{
    std::shared_ptr<IMessagingStub> stub = _address2MessagingStubMap.value(destinationAddress);
    if (!stub) {
        std::unique_lock<std::mutex> lock(mutex);
        for (auto const& factory : _factoryList) {
            if (factory->canCreate(*destinationAddress)) {
                auto createStub = factory->create(*destinationAddress);
                if (createStub) {
                    _address2MessagingStubMap.insert(destinationAddress, createStub);
                }
                return createStub;
            }
        }
    }
    return stub;
}

void MessagingStubFactory::remove(
        const std::shared_ptr<const joynr::system::RoutingTypes::Address>& destinationAddress)
{
    std::unique_lock<std::mutex> lock(mutex);
    if (_address2MessagingStubMap.contains(destinationAddress)) {
        _address2MessagingStubMap.remove(destinationAddress);
    }
}

bool MessagingStubFactory::contains(
        const std::shared_ptr<const joynr::system::RoutingTypes::Address>& destinationAddress)
{
    return _address2MessagingStubMap.contains(destinationAddress);
}

void MessagingStubFactory::registerStubFactory(
        std::shared_ptr<IMiddlewareMessagingStubFactory> factory)
{
    std::unique_lock<std::mutex> lock(mutex);
    _factoryList.push_back(std::move(factory));
}

void MessagingStubFactory::shutdown()
{
    std::unique_lock<std::mutex> lock(mutex);
    _factoryList.clear();
}

} // namespace joynr

#pragma GCC diagnostic pop
