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
#include "joynr/MessageQueue.h"
#include "joynr/DispatcherUtils.h"
#include <QThread>

namespace joynr
{

MessageQueue::MessageQueue() : queue(new QMap<QString, MessageQueueItem*>()), queueMutex()
{
}

MessageQueue::~MessageQueue()
{
    delete queue;
}

qint64 MessageQueue::getQueueLength()
{
    return queue->size();
}

qint64 MessageQueue::queueMessage(const JoynrMessage& message)
{
    QDateTime absTtl = message.getHeaderExpiryDate();
    MessageQueueItem* item = new MessageQueueItem(message, absTtl);
    {
        QMutexLocker locker(&queueMutex);
        queue->insertMulti(message.getHeaderTo(), item);
    }
    return queue->size();
}

MessageQueueItem* MessageQueue::getNextMessageForParticipant(const QString destinationPartId)
{
    QMutexLocker locker(&queueMutex);
    if (queue->contains(destinationPartId)) {
        return queue->take(destinationPartId);
    }
    return NULL;
}

qint64 MessageQueue::removeOutdatedMessages()
{
    qint64 counter = 0;
    if (queue->isEmpty()) {
        return counter;
    }

    QMap<QString, MessageQueueItem*>::iterator i;
    QDateTime now = QDateTime::currentDateTime();
    {
        QMutexLocker locker(&queueMutex);
        for (i = queue->begin(); i != queue->end();) {
            MessageQueueItem* value = i.value();
            if (value->getDecayTime() < now) {
                i = queue->erase(i);
                delete value;
                counter++;
            } else {
                ++i;
            }
        }
    }
    return counter;
}

/**
 * IMPLEMENTATION of MessageQueueCleanerRunnable
 */

MessageQueueCleanerRunnable::MessageQueueCleanerRunnable(MessageQueue& messageQueue,
                                                         qint64 sleepInterval)
        : messageQueue(messageQueue), stopped(false), sleepInterval(sleepInterval)
{
}

void MessageQueueCleanerRunnable::stop()
{
    stopped = true;
}

void MessageQueueCleanerRunnable::run()
{
    while (!stopped) {
        QThread::msleep(sleepInterval);
        messageQueue.removeOutdatedMessages();
    }
}
}
