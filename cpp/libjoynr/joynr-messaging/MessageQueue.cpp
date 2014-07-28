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

namespace joynr {


MessageQueue::MessageQueue():
    queue(new QMap<QString, MessageQueueItem*>()),
    queueMutex()
{
}

MessageQueue::~MessageQueue() {
    delete queue;
}

qint64 MessageQueue::getQueueLength(){
    return queue->size();
}

qint64 MessageQueue::queueMessage(const JoynrMessage &message,
                                const MessagingQos &qos) {
    QDateTime absTtl = DispatcherUtils::convertTtlToAbsoluteTime(qos.getTtl());
    MessageQueueItem* item = new MessageQueueItem(QPair<JoynrMessage, MessagingQos>(message, qos), absTtl);
    {
        QMutexLocker locker(&queueMutex);
        queue->insertMulti(message.getHeaderTo(), item);
    }
    return queue->size();
}

MessageQueueItem* MessageQueue::getNextMessageForParticipant(const QString destinationPartId) {
    QMutexLocker locker(&queueMutex);
    if(queue->contains(destinationPartId)) {
        return queue->take(destinationPartId);
    }
    return NULL;
}

}
