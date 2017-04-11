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
#include "joynr/MessagingStubFactory.h"

#include "joynr/IMiddlewareMessagingStubFactory.h"

namespace joynr
{

MessagingStubFactory::MessagingStubFactory() : address2MessagingStubMap(), factoryList(), mutex()
{
}

std::shared_ptr<IMessaging> MessagingStubFactory::create(
        const std::shared_ptr<const joynr::system::RoutingTypes::Address>& destinationAddress)
{
    std::shared_ptr<IMessaging> stub = address2MessagingStubMap.value(destinationAddress);
    if (!stub) {
        for (std::vector<std::shared_ptr<IMiddlewareMessagingStubFactory>>::iterator it =
                     this->factoryList.begin();
             it != factoryList.end();
             ++it) {
            if ((*it)->canCreate(*destinationAddress)) {
                std::shared_ptr<IMessaging> stub = (*it)->create(*destinationAddress);
                if (stub != nullptr) {
                    address2MessagingStubMap.insert(destinationAddress, stub);
                }
                return stub;
            }
        }
    }
    return stub;
}

void MessagingStubFactory::remove(
        const std::shared_ptr<const joynr::system::RoutingTypes::Address>& destinationAddress)
{
    if (contains(destinationAddress)) {
        address2MessagingStubMap.remove(destinationAddress);
    }
}

bool MessagingStubFactory::contains(
        const std::shared_ptr<const joynr::system::RoutingTypes::Address>& destinationAddress)
{
    return address2MessagingStubMap.contains(destinationAddress);
}

void MessagingStubFactory::registerStubFactory(
        std::shared_ptr<IMiddlewareMessagingStubFactory> factory)
{
    this->factoryList.push_back(std::move(factory));
}

} // namespace joynr
