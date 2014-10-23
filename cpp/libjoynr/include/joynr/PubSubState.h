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
#ifndef PUBSUBSTATE_H
#define PUBSUBSTATE_H

#include <QMutex>

namespace joynr
{

/**
  * \class PubSubState
  * \brief Stores all the runtime Information of a Publication or Subscription,
  *  e.g. lastTimeCalled, isInterrupted.
  *  ThreadSafe
  */
class PubSubState
{
public:
    virtual ~PubSubState()
    {
    }
    PubSubState();
    qint64 getTimeOfLastPublication() const;
    void setTimeOfLastPublication(qint64 timeOfLastPublication = 0);
    bool isInterrupted();
    bool isStopped();
    void interrupt();
    void stop();

protected:
    bool interrupted;
    bool stopped;
    qint64 timeOfLastPublication;
    mutable QMutex
            accesMutex; // QMutex has to be mutable to allow ConstCorrectness of getter-Functions.
    /*A Mutable member variable can be changed in a const method call. Mutable implies, that the
      state of this member
      variable is not related to the logical constness of the object. E.g. a publicationState would
      not change, just because
      the accessMutex has been modified. */

    // this Mutex will prevent two concurrent read-access as well, could be optimised if necessary.
    // E.g. by using a QReadWriteLock
};

} // namespace joynr
#endif // PUBSUBSTATE_H
