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
#ifndef MESSAGEQUEUE_H
#define MESSAGEQUEUE_H

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "joynr/ContentWithDecayTime.h"
#include "joynr/JoynrExport.h"
#include "joynr/PrivateCopyAssign.h"

namespace joynr
{

class ImmutableMessage;

using MessageQueueItem = ContentWithDecayTime<std::shared_ptr<ImmutableMessage>>;

class JOYNR_EXPORT MessageQueue
{
public:
    MessageQueue();

    std::size_t getQueueLength() const;

    std::size_t queueMessage(std::shared_ptr<ImmutableMessage> message);

    std::unique_ptr<MessageQueueItem> getNextMessageForParticipant(
            const std::string& destinationPartId);

    std::int64_t removeOutdatedMessages();

private:
    DISALLOW_COPY_AND_ASSIGN(MessageQueue);

    // TODO should we replace by std::unordered_multimap?
    // or a boost multi_index table because we need to store the TTL
    std::multimap<std::string, std::unique_ptr<MessageQueueItem>> queue;
    mutable std::mutex queueMutex;
};
} // namespace joynr

#endif // MESSAGEQUEUE_H
