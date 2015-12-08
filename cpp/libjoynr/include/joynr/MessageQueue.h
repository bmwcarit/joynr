/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2014 BMW Car IT GmbH
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

#include "joynr/PrivateCopyAssign.h"
#include "joynr/JoynrExport.h"

#include "joynr/JoynrMessage.h"
#include "joynr/ContentWithDecayTime.h"

#include <QMutex>
#include <QRunnable>
#include <string>
#include <map>
#include <cstdint>

namespace joynr
{

typedef ContentWithDecayTime<JoynrMessage> MessageQueueItem;

class JOYNR_EXPORT MessageQueue
{
public:
    MessageQueue();

    ~MessageQueue();

    std::size_t getQueueLength();

    std::size_t queueMessage(const JoynrMessage& message);

    MessageQueueItem* getNextMessageForParticipant(const std::string destinationPartId);

    int64_t removeOutdatedMessages();

private:
    DISALLOW_COPY_AND_ASSIGN(MessageQueue);

    std::multimap<std::string, MessageQueueItem*>* queue;
    mutable QMutex queueMutex;
};

/**
 * Runnable to remove outdated message from message queue
 */
class JOYNR_EXPORT MessageQueueCleanerRunnable : public QRunnable
{
public:
    MessageQueueCleanerRunnable(MessageQueue& messageQueue, int64_t sleepInterval = 1000);
    void run();
    void stop();

private:
    MessageQueue& messageQueue;
    bool stopped;
    int64_t sleepInterval;
};
}

#endif // MESSAGEQUEUE_H
