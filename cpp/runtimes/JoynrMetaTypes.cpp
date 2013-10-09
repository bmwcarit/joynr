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

#include "runtimes/JoynrMetaTypes.h"
#include "joynr/JoynrMessage.h"
#include "joynr/Request.h"
#include "joynr/Reply.h"
#include "joynr/SubscriptionPublication.h"
#include "joynr/SubscriptionRequest.h"
#include "joynr/SubscriptionReply.h"
#include "joynr/SubscriptionStop.h"

#include <QMetaType>

namespace joynr {

void registerJoynrMetaTypes(){
    qRegisterMetaType<JoynrMessage>("joynr.JoynrMessage");
    qRegisterMetaType<Request>("joynr.Request");
    qRegisterMetaType<Reply>("joynr.Reply");
    qRegisterMetaType<SubscriptionPublication>("joynr.SubscriptionPublication");
    qRegisterMetaType<SubscriptionRequest>("joynr.SubscriptionRequest");
    qRegisterMetaType<SubscriptionReply>("joynr.SubscriptionReply");
    qRegisterMetaType<SubscriptionStop>("joynr.SubscriptionStop");
}

} // namespace joynr
