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
#include "joynr/SubscriptionUtil.h"

#include "joynr/QtOnChangeSubscriptionQos.h"
#include "joynr/QtOnChangeWithKeepAliveSubscriptionQos.h"
#include "joynr/QtPeriodicSubscriptionQos.h"

namespace joynr
{

bool SubscriptionUtil::isOnChangeSubscription(QtSubscriptionQos* qos)
{
    return qos->inherits(QtOnChangeSubscriptionQos::staticMetaObject.className()) ||
           qos->inherits(QtOnChangeWithKeepAliveSubscriptionQos::staticMetaObject.className());
}

qint64 SubscriptionUtil::getAlertInterval(QtSubscriptionQos* qos)
{
    if (qos->inherits(QtPeriodicSubscriptionQos::staticMetaObject.className())) {
        return (qobject_cast<QtPeriodicSubscriptionQos*>(qos))->getAlertAfterInterval();
    }
    if (qos->inherits(QtOnChangeWithKeepAliveSubscriptionQos::staticMetaObject.className())) {
        return (qobject_cast<QtOnChangeWithKeepAliveSubscriptionQos*>(qos))
                ->getAlertAfterInterval();
    }
    return -1;
}

qint64 SubscriptionUtil::getMinInterval(QtSubscriptionQos* qos)
{
    if (qos->inherits(QtOnChangeSubscriptionQos::staticMetaObject.className())) {
        return (qobject_cast<QtOnChangeSubscriptionQos*>(qos))->getMinInterval();
    }
    if (qos->inherits(QtOnChangeWithKeepAliveSubscriptionQos::staticMetaObject.className())) {
        return (qobject_cast<QtOnChangeWithKeepAliveSubscriptionQos*>(qos))->getMinInterval();
    }
    return -1;
}

qint64 SubscriptionUtil::getPeriodicPublicationInterval(QtSubscriptionQos* qos)
{
    if (qos->inherits(QtOnChangeWithKeepAliveSubscriptionQos::staticMetaObject.className())) {
        return (qobject_cast<QtOnChangeWithKeepAliveSubscriptionQos*>(qos))->getMaxInterval();
    }
    if (qos->inherits(QtPeriodicSubscriptionQos::staticMetaObject.className())) {
        return (qobject_cast<QtPeriodicSubscriptionQos*>(qos))->getPeriod();
    }
    return -1;
}
}
