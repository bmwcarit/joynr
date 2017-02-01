/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

#include "joynr/MulticastMessagingSkeletonDirectory.h"

#include "joynr/IMessagingMulticastSubscriber.h"
#include "joynr/system/RoutingTypes/Address.h"

namespace joynr
{

INIT_LOGGER(MulticastMessagingSkeletonDirectory);

std::shared_ptr<IMessagingMulticastSubscriber> MulticastMessagingSkeletonDirectory::getSkeleton(
        std::shared_ptr<const system::RoutingTypes::Address> address)
{
    const system::RoutingTypes::Address& addressRef = *address;
    std::type_index typeIndex(typeid(addressRef));
    JOYNR_LOG_TRACE(logger, "get messaging skeleton for address type {}", typeIndex.name());
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (contains(address)) {
        return multicastSkeletons[typeIndex];
    }
    JOYNR_LOG_WARN(
            logger, "No messaging skeleton registered for address type {}", typeIndex.name());
    return std::shared_ptr<IMessagingMulticastSubscriber>();
}

bool MulticastMessagingSkeletonDirectory::contains(
        std::shared_ptr<const system::RoutingTypes::Address> address)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);
    const system::RoutingTypes::Address& addressRef = *address;
    std::type_index typeIndex(typeid(addressRef));
    return multicastSkeletons.find(typeIndex) != multicastSkeletons.cend();
}

} // namespace joynr
