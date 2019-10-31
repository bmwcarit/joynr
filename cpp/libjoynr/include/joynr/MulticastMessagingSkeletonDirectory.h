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
    void registerSkeleton(std::shared_ptr<IMessagingMulticastSubscriber> skeleton,
                          const std::string& gbid = "")
    {
        std::type_index typeIndex = typeid(T);
        JOYNR_LOG_DEBUG(logger(),
                        "register messaging skeleton for address type {}, gbid: {}",
                        typeIndex.name(),
                        gbid);
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        JOYNR_LOG_DEBUG(logger(),
                        "register messaging skeleton for address type {}, gbid: {}",
                        typeIndex.name(),
                        gbid);
        _multicastSkeletons[std::make_pair(typeIndex, gbid)] = std::move(skeleton);
    }

    template <class T,
              typename = std::enable_if_t<std::is_base_of<system::RoutingTypes::Address, T>::value>>
    void unregisterSkeletons()
    {
        std::type_index typeIndex = typeid(T);
        JOYNR_LOG_DEBUG(
                logger(), "unregister messaging skeletons for address type {}", typeIndex.name());
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        std::vector<std::pair<std::type_index, std::string>> removalList;
        for (const auto& entryOfRegisteredSkeleton : _multicastSkeletons) {
            auto pairKey = entryOfRegisteredSkeleton.first;
            if (pairKey.first == typeIndex) {
                removalList.push_back(std::make_pair(typeIndex, pairKey.second));
            }
        }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunsafe-loop-optimizations"
        for (const auto& entry : removalList) {
            _multicastSkeletons.erase(entry);
        }
#pragma GCC diagnostic pop
    }

    std::shared_ptr<IMessagingMulticastSubscriber> getSkeleton(
            std::shared_ptr<const system::RoutingTypes::Address> address);

    bool contains(std::shared_ptr<const system::RoutingTypes::Address> address);

private:
    DISALLOW_COPY_AND_ASSIGN(MulticastMessagingSkeletonDirectory);
    ADD_LOGGER(MulticastMessagingSkeletonDirectory)
    std::map<std::pair<std::type_index, std::string>,
             std::shared_ptr<IMessagingMulticastSubscriber>> _multicastSkeletons;

    std::recursive_mutex _mutex;
};

} // namespace joynr

#endif // MULTICASTMESSAGINGSKELETONDIRECTORY_H
