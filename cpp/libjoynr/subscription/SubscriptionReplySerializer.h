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
#ifndef SUBSCRIPTIONREPLYSERIALIZER_H
#define SUBSCRIPTIONREPLYSERIALIZER_H

#include "joynr/ClassDeserializer.h"
#include "joynr/ClassSerializer.h"
#include "joynr/SubscriptionReply.h"

#include <ostream>

namespace joynr
{

// Serializes a SubscriptionReply
template <>
void ClassSerializer<SubscriptionReply>::serialize(const SubscriptionReply& subscriptionReply,
                                                   std::ostream& o);

// Deserializes a SubscriptionReply
template <>
void ClassDeserializer<SubscriptionReply>::deserialize(SubscriptionReply& t, IObject& o);

} // namespace joynr

#endif // SUBSCRIPTIONREPLYSERIALIZER_H
