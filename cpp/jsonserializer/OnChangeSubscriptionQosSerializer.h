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
#ifndef ONCHANGESUBSCRIPTIONQOSSERIALIZER_H
#define ONCHANGESUBSCRIPTIONQOSSERIALIZER_H

#include "joynr/ClassDeserializer.h"
#include "joynr/ClassSerializer.h"
#include "joynr/OnChangeSubscriptionQos.h"
#include "IDeserializer.h"

#include <ostream>

namespace joynr
{

/**
 * @brief Specialized serialize method for OnChangeSubscriptionQos.
 * @param subscription the object to serialize
 * @param o the stream to write the serialized content to
 */
template <>
void ClassSerializer<OnChangeSubscriptionQos>::serialize(const OnChangeSubscriptionQos& subscription, std::ostream& o);

/**
 * @brief Specialized deserialize method for OnChangeSubscriptionQos.
 * @param subscription the object to fill during deserialization
 * @param o object containing the parsed json tokens
 */
template <>
void ClassDeserializer<OnChangeSubscriptionQos>::deserialize(OnChangeSubscriptionQos& subscription, IObject& o);

} /* namespace joynr */

#endif // ONCHANGESUBSCRIPTIONQOSSERIALIZER_H
