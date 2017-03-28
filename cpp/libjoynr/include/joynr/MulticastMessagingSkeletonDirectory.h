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
#ifndef MULTICASTMESSAGINGSKELETONDIRECTORY_H
#define MULTICASTMESSAGINGSKELETONDIRECTORY_H

#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

#include "joynr/IMessagingMulticastSubscriber.h"
#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/system/RoutingTypes/Address.h"

namespace joynr
{

class MulticastMessagingSkeletonDirectory
{
public:
    MulticastMessagingSkeletonDirectory() = default;

    virtual ~MulticastMessagingSkeletonDirectory() = default;

    template <class T,
              typename = std::enable_if_t<std::is_base_of<system::RoutingTypes::Address, T>::value>>
    void registerSkeleton(std::shared_ptr<IMessagingMulticastSubscriber> skeleton)
    {
        std::type_index typeIndex = typeid(T);
        JOYNR_LOG_DEBUG(
                logger, "register messaging skeleton for address type {}", typeIndex.name());
        std::lock_guard<std::recursive_mutex> lock(mutex);
        multicastSkeletons[typeIndex] = skeleton;
    }

    template <class T,
              typename = std::enable_if_t<std::is_base_of<system::RoutingTypes::Address, T>::value>>
    void unregisterSkeleton()
    {
        std::type_index typeIndex = typeid(T);
        JOYNR_LOG_DEBUG(
                logger, "unregister messaging skeleton for address type {}", typeIndex.name());
        std::lock_guard<std::recursive_mutex> lock(mutex);
        multicastSkeletons.erase(typeIndex);
    }

    std::shared_ptr<IMessagingMulticastSubscriber> getSkeleton(
            std::shared_ptr<const system::RoutingTypes::Address> address);

    bool contains(std::shared_ptr<const system::RoutingTypes::Address> address);

private:
    DISALLOW_COPY_AND_ASSIGN(MulticastMessagingSkeletonDirectory);
    ADD_LOGGER(MulticastMessagingSkeletonDirectory);
    std::unordered_map<std::type_index, std::shared_ptr<IMessagingMulticastSubscriber>>
            multicastSkeletons;
    std::recursive_mutex mutex;
};

} // namespace joynr

#endif // MULTICASTMESSAGINGSKELETONDIRECTORY_H
