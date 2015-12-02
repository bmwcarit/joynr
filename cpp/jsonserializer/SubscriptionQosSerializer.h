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
#ifndef SUBSCRIPTIONQOSSERIALIZER_H
#define SUBSCRIPTIONQOSSERIALIZER_H

#include "joynr/ClassDeserializer.h"
#include "joynr/ClassSerializer.h"
#include "joynr/SubscriptionQos.h"
#include "joynr/IDeserializer.h"

#include <ostream>

namespace joynr
{

// Serializes a SubscriptionQos
template <>
void ClassSerializer<SubscriptionQos>::serialize(const SubscriptionQos& subscription, std::ostream& o);

// Deserializes a SubscriptionQos
template <>
void ClassDeserializer<SubscriptionQos>::deserialize(SubscriptionQos& subscription, IObject& o);

} /* namespace joynr */
#endif // SUBSCRIPTIONQOSSERIALIZER_H
