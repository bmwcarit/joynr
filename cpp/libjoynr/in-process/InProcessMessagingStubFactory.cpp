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

#include "InProcessMessagingStubFactory.h"

#include <cassert>
#include <tuple>

#include "InProcessMessagingStub.h"
#include "joynr/InProcessMessagingAddress.h"
#include "joynr/system/RoutingTypes/Address.h"

namespace joynr
{

InProcessMessagingStubFactory::InProcessMessagingStubFactory()
{
}

bool InProcessMessagingStubFactory::canCreate(
        const joynr::system::RoutingTypes::Address& destAddress)
{
    return dynamic_cast<const InProcessMessagingAddress*>(&destAddress) != nullptr;
}

std::shared_ptr<IMessagingStub> InProcessMessagingStubFactory::create(
        const joynr::system::RoutingTypes::Address& destAddress)
{
    const InProcessMessagingAddress* inprocessAddress =
            dynamic_cast<const InProcessMessagingAddress*>(&destAddress);
    assert(inprocessAddress);
    return std::make_shared<InProcessMessagingStub>(inprocessAddress->getSkeleton());
}

void InProcessMessagingStubFactory::registerOnMessagingStubClosedCallback(
        std::function<void(std::shared_ptr<const joynr::system::RoutingTypes::Address>
                                   destinationAddress)> onMessagingStubClosedCallback)
{
    std::ignore = onMessagingStubClosedCallback;
}

} // namespace joynr
