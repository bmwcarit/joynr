/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
#ifndef BROADCASTSUBSCRIPTIONREQUESTINFORMATIONSERIALIZER_H
#define BROADCASTSUBSCRIPTIONREQUESTINFORMATIONSERIALIZER_H

#include "joynr/ClassDeserializer.h"
#include "joynr/ClassSerializer.h"
#include "BroadcastSubscriptionRequestInformation.h"
#include "jsonserializer/IDeserializer.h"

#include <ostream>

namespace joynr
{

// Serializes a BroadcastSubscriptionRequestInformation
template <>
void ClassSerializer<BroadcastSubscriptionRequestInformation>::serialize(
        const BroadcastSubscriptionRequestInformation& info,
        std::ostream& o);

// Deserializes a BroadcastSubscriptionRequestInformation
template <>
void ClassDeserializer<BroadcastSubscriptionRequestInformation>::deserialize(
        BroadcastSubscriptionRequestInformation& info,
        IObject& o);

} /* namespace joynr */

#endif // BROADCASTSUBSCRIPTIONREQUESTINFORMATIONSERIALIZER_H
