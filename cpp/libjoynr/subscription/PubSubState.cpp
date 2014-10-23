/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
#include "joynr/PubSubState.h"

#include <QDateTime>

namespace joynr
{

PubSubState::PubSubState()
        : interrupted(false), stopped(false), timeOfLastPublication(0), accesMutex()
{
}

qint64 PubSubState::getTimeOfLastPublication() const
{
    QMutexLocker locker(&accesMutex);
    return this->timeOfLastPublication;
}

void PubSubState::setTimeOfLastPublication(qint64 timeOfLastPublication /* = 0 */)
{
    QMutexLocker locker(&accesMutex);
    if (timeOfLastPublication == 0) {
        this->timeOfLastPublication = QDateTime::currentMSecsSinceEpoch();
    } else {
        this->timeOfLastPublication = timeOfLastPublication;
    }
}

bool PubSubState::isInterrupted()
{
    QMutexLocker locker(&accesMutex);
    return this->interrupted;
}

bool PubSubState::isStopped()
{
    QMutexLocker locker(&accesMutex);
    return this->stopped;
}

void PubSubState::interrupt()
{
    QMutexLocker locker(&accesMutex);
    this->interrupted = true;
}

void PubSubState::stop()
{
    QMutexLocker locker(&accesMutex);
    this->stopped = true;
}

} // namespace joynr
