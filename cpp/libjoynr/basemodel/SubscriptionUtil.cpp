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
#include "joynr/SubscriptionUtil.h"

#include <cassert>
#include <typeinfo>

#include "joynr/OnChangeSubscriptionQos.h"
#include "joynr/OnChangeWithKeepAliveSubscriptionQos.h"
#include "joynr/PeriodicSubscriptionQos.h"
#include "joynr/SubscriptionQos.h"
#include "joynr/exceptions/JoynrException.h"

namespace joynr
{

bool SubscriptionUtil::isOnChangeSubscription(std::shared_ptr<SubscriptionQos> qos)
{
    static const std::type_info& onChangeSubscriptionQosTypeId = typeid(OnChangeSubscriptionQos);
    static const std::type_info& onChangeWithKeepAliveSubscriptionQosTypeId =
            typeid(OnChangeWithKeepAliveSubscriptionQos);

    const SubscriptionQos* const ptr = qos.get();
    const std::type_info& qosTypeId = typeid(*ptr);
    return qosTypeId == onChangeSubscriptionQosTypeId ||
           qosTypeId == onChangeWithKeepAliveSubscriptionQosTypeId;
}

std::int64_t SubscriptionUtil::getAlertInterval(std::shared_ptr<SubscriptionQos> qos)
{
    if (auto typedOnChangeWithKeepAliveSubscriptionQos =
                std::dynamic_pointer_cast<OnChangeWithKeepAliveSubscriptionQos>(qos)) {
        return typedOnChangeWithKeepAliveSubscriptionQos->getAlertAfterIntervalMs();
    } else if (auto typedPeriodicSubscriptionQos =
                       std::dynamic_pointer_cast<PeriodicSubscriptionQos>(qos)) {
        return typedPeriodicSubscriptionQos->getAlertAfterIntervalMs();
    }
    return -1;
}

std::int64_t SubscriptionUtil::getMinInterval(std::shared_ptr<SubscriptionQos> qos)
{
    if (auto typedQos = std::dynamic_pointer_cast<OnChangeSubscriptionQos>(qos)) {
        return typedQos->getMinIntervalMs();
    }
    return -1;
}

std::int64_t SubscriptionUtil::getPeriodicPublicationInterval(std::shared_ptr<SubscriptionQos> qos)
{
    if (auto typedOnChangeWithKeepAliveSubscriptionQos =
                std::dynamic_pointer_cast<OnChangeWithKeepAliveSubscriptionQos>(qos)) {
        return typedOnChangeWithKeepAliveSubscriptionQos->getMaxIntervalMs();
    } else if (auto typedPeriodicSubscriptionQos =
                       std::dynamic_pointer_cast<PeriodicSubscriptionQos>(qos)) {
        return typedPeriodicSubscriptionQos->getPeriodMs();
    }
    return -1;
}

} // namespace joynr
