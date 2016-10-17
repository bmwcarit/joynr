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
    multicastReceivers[multicastId].insert(receiverId);
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
        std::unordered_set<std::string>& receivers = multicastReceivers[multicastId];
        receivers.erase(receiverId);
        JOYNR_LOG_DEBUG(logger,
                        "removed multicast receiver: multicastId={}, receiverId={}",
                        multicastId,
                        receiverId);
        if (receivers.empty()) {
            JOYNR_LOG_DEBUG(logger, "removed last multicast receiver: multicastId={}", multicastId);
            multicastReceivers.erase(multicastId);
        }
        return true;
    }
    return false;
}

std::unordered_set<std::string> MulticastReceiverDirectory::getReceivers(
        const std::string& multicastId)
{
    JOYNR_LOG_DEBUG(logger, "get multicast receivers: multicastId={}", multicastId);
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (contains(multicastId)) {
        return multicastReceivers[multicastId];
    }
    return std::unordered_set<std::string>();
}

bool MulticastReceiverDirectory::contains(const std::string& multicastId)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);
    return multicastReceivers.find(multicastId) != multicastReceivers.cend();
}

bool MulticastReceiverDirectory::contains(const std::string& multicastId,
                                          const std::string& receiverId)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);
    auto receivers = getReceivers(multicastId);
    return receivers.find(receiverId) != receivers.cend();
}

} // namespace joynr
