/*
 * #%L
 * joynr::C++
 * $Id:$
 * $HeadURL:$
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
#ifndef SUBSCRIPTIONCALLBACK_H
#define SUBSCRIPTIONCALLBACK_H
#include "joynr/PrivateCopyAssign.h"
#include <QSharedPointer>
#include <QMetaType>

#include "joynr/ISubscriptionCallback.h"
#include "joynr/ISubscriptionListener.h"
#include "joynr/TrackableObject.h"
#include "joynr/joynrlogging.h"

namespace joynr {

/**
  * \class SubscriptionCallback
  * \brief
  */

template<class T>
class SubscriptionCallback : public ISubscriptionCallback{
public:

    SubscriptionCallback(QSharedPointer<ISubscriptionListener<T> > listener)
    : listener(listener){

    }

    virtual ~SubscriptionCallback() {
        LOG_TRACE(logger, "destructor: entering...");
        LOG_TRACE(logger, "destructor: leaving...");
    }

    void publicationMissed(){
        listener->publicationMissed();
    }

    void attributeChanged(const T &value){
        listener->receive(value);
    }

    void timeOut(){
        //TODO
    }

    int getTypeId() const {
        return qMetaTypeId<T>();
    }

private:
    DISALLOW_COPY_AND_ASSIGN(SubscriptionCallback);
    QSharedPointer<ISubscriptionListener<T> > listener;
    static joynr_logging::Logger* logger;
};

template<typename T>
joynr_logging::Logger* SubscriptionCallback<T>::logger = joynr_logging::Logging::getInstance()->getLogger("MSG", "SubscriptionCallback");


} // namespace joynr
#endif // SUBSCRIPTIONCALLBACK_H
