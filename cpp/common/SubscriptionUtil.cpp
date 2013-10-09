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
#include "joynr/SubscriptionUtil.h"

#include "joynr/OnChangeSubscriptionQos.h"
#include "joynr/OnChangeWithKeepAliveSubscriptionQos.h"
#include "joynr/PeriodicSubscriptionQos.h"

namespace joynr {

bool SubscriptionUtil::isOnChangeSubscription(SubscriptionQos* qos) {
    return qos->inherits(OnChangeSubscriptionQos::staticMetaObject.className()) ||
            qos->inherits(OnChangeWithKeepAliveSubscriptionQos::staticMetaObject.className());
}

qint64 SubscriptionUtil::getAlertInterval(SubscriptionQos* qos) {
    if(qos->inherits(PeriodicSubscriptionQos::staticMetaObject.className())) {
        return (qobject_cast<PeriodicSubscriptionQos*> (qos))->getAlertAfterInterval();
    }
    if(qos->inherits(OnChangeWithKeepAliveSubscriptionQos::staticMetaObject.className())) {
        return (qobject_cast<OnChangeWithKeepAliveSubscriptionQos*> (qos))->getAlertAfterInterval();
    }
    return -1;
}

qint64 SubscriptionUtil::getMinInterval(SubscriptionQos* qos) {
    if(qos->inherits(OnChangeSubscriptionQos::staticMetaObject.className())) {
        return (qobject_cast<OnChangeSubscriptionQos*> (qos))->getMinInterval();
    }
    if(qos->inherits(OnChangeWithKeepAliveSubscriptionQos::staticMetaObject.className())) {
        return (qobject_cast<OnChangeWithKeepAliveSubscriptionQos*> (qos))->getMinInterval();
    }
    return -1;
}

qint64 SubscriptionUtil::getPeriodicPublicationInterval(SubscriptionQos* qos) {
    if(qos->inherits(OnChangeWithKeepAliveSubscriptionQos::staticMetaObject.className())) {
        return (qobject_cast<OnChangeWithKeepAliveSubscriptionQos*> (qos))->getMaxInterval();
    }
    if(qos->inherits(PeriodicSubscriptionQos::staticMetaObject.className())) {
        return (qobject_cast<PeriodicSubscriptionQos*> (qos))->getPeriod();
    }
    return -1;
}
}
