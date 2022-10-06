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

#include "joynr/MulticastReceiverDirectory.h"

namespace joynr
{

MulticastReceiverDirectory::~MulticastReceiverDirectory()
{
    JOYNR_LOG_TRACE(logger(), "destructor: number of entries = {}", _multicastReceivers.size());
    _multicastReceivers.clear();
}

void MulticastReceiverDirectory::registerMulticastReceiver(const std::string& multicastId,
                                                           const std::string& receiverId)
{
    JOYNR_LOG_TRACE(logger(),
                    "register multicast receiver: multicastId={}, receiverId={}",
                    multicastId,
                    receiverId);
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    MulticastMatcher matcher(multicastId);
    _multicastReceivers[matcher].insert(receiverId);
}

bool MulticastReceiverDirectory::unregisterMulticastReceiver(const std::string& multicastId,
                                                             const std::string& receiverId)
{
    JOYNR_LOG_TRACE(logger(),
                    "unregister multicast receiver: multicastId={}, receiverId={}",
                    multicastId,
                    receiverId);
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (contains(multicastId)) {
        MulticastMatcher matcher(multicastId);
        std::unordered_set<std::string>& receivers = _multicastReceivers[matcher];
        receivers.erase(receiverId);
        JOYNR_LOG_TRACE(logger(),
                        "removed multicast receiver: multicastId={}, receiverId={}",
                        multicastId,
                        receiverId);
        if (receivers.empty()) {
            JOYNR_LOG_TRACE(
                    logger(), "removed last multicast receiver: multicastId={}", multicastId);
            _multicastReceivers.erase(matcher);
        }
        return true;
    }
    return false;
}

std::unordered_set<std::string> MulticastReceiverDirectory::getReceivers(
        const std::string& multicastId)
{
    JOYNR_LOG_TRACE(logger(), "get multicast receivers: multicastId={}", multicastId);
    std::lock_guard<std::recursive_mutex> lock(_mutex);

    std::unordered_set<std::string> foundReceivers;
    for (const auto& entry : _multicastReceivers) {
        if (entry.first.doesMatch(multicastId)) {
            foundReceivers.insert(entry.second.cbegin(), entry.second.cend());
        }
    }
    return foundReceivers;
}

std::vector<std::string> MulticastReceiverDirectory::getMulticastIds() const
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    std::vector<std::string> multicastIds;

    for (const auto& multicastReceiver : _multicastReceivers) {
        multicastIds.push_back(multicastReceiver.first._multicastId);
    }

    return multicastIds;
}

bool MulticastReceiverDirectory::contains(const std::string& multicastId)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    MulticastMatcher matcher(multicastId);
    return _multicastReceivers.find(matcher) != _multicastReceivers.cend();
}

bool MulticastReceiverDirectory::contains(const std::string& multicastId,
                                          const std::string& receiverId)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    const auto& receivers = getReceivers(multicastId);
    return receivers.find(receiverId) != receivers.cend();
}

} // namespace joynr
