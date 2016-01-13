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
#ifndef SUBSCRIPTIONREQUESTINFORMATIONSERIALIZER_H
#define SUBSCRIPTIONREQUESTINFORMATIONSERIALIZER_H

#include "joynr/ClassDeserializer.h"
#include "joynr/ClassSerializer.h"
#include "subscription/SubscriptionRequestInformation.h"

#include <ostream>

namespace joynr
{

// Serializes a SubscriptionRequestInformation
template <>
void ClassSerializerImpl<SubscriptionRequestInformation>::serialize(
        const SubscriptionRequestInformation& info,
        std::ostream& o);

// Deserializes a SubscriptionRequestInformation
template <>
void ClassDeserializerImpl<SubscriptionRequestInformation>::deserialize(
        SubscriptionRequestInformation& info,
        IObject& o);

} // namespace joynr

#endif // SUBSCRIPTIONREQUESTINFORMATIONSERIALIZER_H
