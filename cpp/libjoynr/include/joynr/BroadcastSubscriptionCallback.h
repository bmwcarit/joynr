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
#ifndef BROADCASTSUBSCRIPTIONCALLBACK_H
#define BROADCASTSUBSCRIPTIONCALLBACK_H
#include "joynr/PrivateCopyAssign.h"
#include <QSharedPointer>
#include <QMetaType>

#include "joynr/ISubscriptionCallback.h"
#include "joynr/ISubscriptionListener.h"
#include "joynr/TrackableObject.h"
#include "joynr/joynrlogging.h"
#include "joynr/Util.h"

namespace joynr
{

/**
  * \class BroadcastSubscriptionCallback
  * \brief
  */

template <typename T, typename... Ts>
class BroadcastSubscriptionCallback : public ISubscriptionCallback
{
public:
    BroadcastSubscriptionCallback(QSharedPointer<ISubscriptionListener<T, Ts...>> listener)
            : listener(listener)
    {
    }

    virtual ~BroadcastSubscriptionCallback()
    {
        LOG_TRACE(logger, "destructor: entering...");
        LOG_TRACE(logger, "destructor: leaving...");
    }

    void publicationMissed()
    {
        listener->publicationMissed();
    }

    void eventOccurred(const T value, const Ts... values)
    {
        listener->receive(value, values...);
    }

    void timeOut()
    {
        // TODO
    }

    int getTypeId() const
    {
        return Util::getBroadcastTypeId<T, Ts...>();
    }

private:
    DISALLOW_COPY_AND_ASSIGN(BroadcastSubscriptionCallback);
    QSharedPointer<ISubscriptionListener<T, Ts...>> listener;
    static joynr_logging::Logger* logger;
};

template <typename T, typename... Ts>
joynr_logging::Logger* BroadcastSubscriptionCallback<T, Ts...>::logger =
        joynr_logging::Logging::getInstance()->getLogger("MSG", "BroadcastSubscriptionCallback");

} // namespace joynr
#endif // BROADCASTSUBSCRIPTIONCALLBACK_H
