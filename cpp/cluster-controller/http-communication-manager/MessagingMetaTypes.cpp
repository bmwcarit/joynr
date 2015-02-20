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
#include "cluster-controller/http-communication-manager/MessagingMetaTypes.h"
#include "joynr/JoynrMessage.h"
#include "joynr/MessagingQos.h"
#include "joynr/OnChangeSubscriptionQos.h"
#include "joynr/OnChangeWithKeepAliveSubscriptionQos.h"
#include "joynr/PeriodicSubscriptionQos.h"

#include <QRunnable>

namespace joynr
{

void registerMessagingMetaTypes()
{

    qRegisterMetaType<JoynrMessage>("joynr.JoynrMessage");
    qRegisterMetaType<QRunnable*>("QRunnableStar");
    qRegisterMetaType<OnChangeSubscriptionQos>("OnChangeSubscriptionQos");
    qRegisterMetaType<OnChangeWithKeepAliveSubscriptionQos>("OnChangeWithKeepAliveSubscriptionQos");
    qRegisterMetaType<PeriodicSubscriptionQos>("PeriodicSubscriptionQos");
}

} // namespace joynr
