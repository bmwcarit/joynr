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
#ifndef SUBSCRIPTIONSTOPSERIALIZER_H
#define SUBSCRIPTIONSTOPSERIALIZER_H

#include "joynr/ClassDeserializer.h"
#include "joynr/ClassSerializer.h"
#include "joynr/SubscriptionStop.h"

#include <ostream>

namespace joynr
{

// Serializes a subscriptionStop
template <>
void ClassSerializer<SubscriptionStop>::serialize(const SubscriptionStop& subscriptionStop,
                                                  std::ostream& o);

// Deserializes a subscriptionStop
template <>
void ClassDeserializer<SubscriptionStop>::deserialize(SubscriptionStop& t, IObject& o);

} // namespace joynr

#endif // SUBSCRIPTIONSTOPSERIALIZER_H
