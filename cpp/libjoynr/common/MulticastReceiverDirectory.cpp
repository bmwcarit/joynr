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

#include "joynr/MulticastReceiverDirectory.h"

namespace joynr
{

INIT_LOGGER(MulticastReceiverDirectory);

MulticastReceiverDirectory::~MulticastReceiverDirectory()
{
    JOYNR_LOG_TRACE(logger, "destructor: number of entries = {}", multicastReceivers.size());
    multicastReceivers.clear();
}

void MulticastReceiverDirectory::registerMulticastReceiver(const std::string& multicastId,
                                                           const std::string& receiverId)
{
    JOYNR_LOG_DEBUG(logger,
                    "register multicast receiver: multicastId={}, receiverId={}",
                    multicastId,
                    receiverId);
    std::lock_guard<std::recursive_mutex> lock(mutex);
    MulticastMatcher matcher(multicastId);
    multicastReceivers[matcher].insert(receiverId);
}

bool MulticastReceiverDirectory::unregisterMulticastReceiver(const std::string& multicastId,
                                                             const std::string& receiverId)
{
    JOYNR_LOG_DEBUG(logger,
                    "unregister multicast receiver: multicastId={}, receiverId={}",
                    multicastId,
                    receiverId);
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (contains(multicastId)) {
        MulticastMatcher matcher(multicastId);
        std::unordered_set<std::string>& receivers = multicastReceivers[matcher];
        receivers.erase(receiverId);
        JOYNR_LOG_DEBUG(logger,
                        "removed multicast receiver: multicastId={}, receiverId={}",
                        multicastId,
                        receiverId);
        if (receivers.empty()) {
            JOYNR_LOG_DEBUG(logger, "removed last multicast receiver: multicastId={}", multicastId);
            multicastReceivers.erase(matcher);
        }
        return true;
    }
    return false;
}

std::unordered_set<std::string> MulticastReceiverDirectory::getReceivers(
        const std::string& multicastId)
{
    JOYNR_LOG_TRACE(logger, "get multicast receivers: multicastId={}", multicastId);
    std::lock_guard<std::recursive_mutex> lock(mutex);

    std::unordered_set<std::string> foundReceivers;
    for (const std::pair<MulticastMatcher, std::unordered_set<std::string>>& pair :
         multicastReceivers) {
        if (pair.first.doesMatch(multicastId)) {
            foundReceivers.insert(pair.second.cbegin(), pair.second.cend());
        }
    }
    return foundReceivers;
}

std::vector<std::string> MulticastReceiverDirectory::getMulticastIds() const
{
    std::vector<std::string> multicastIds;

    for (const auto& multicastReceiver : multicastReceivers) {
        multicastIds.push_back(multicastReceiver.first.multicastId);
    }

    return multicastIds;
}

bool MulticastReceiverDirectory::contains(const std::string& multicastId)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);
    MulticastMatcher matcher(multicastId);
    return multicastReceivers.find(matcher) != multicastReceivers.cend();
}

bool MulticastReceiverDirectory::contains(const std::string& multicastId,
                                          const std::string& receiverId)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);
    auto receivers = getReceivers(multicastId);
    return receivers.find(receiverId) != receivers.cend();
}

} // namespace joynr
