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
#ifndef JOYNRMESSAGESERIALIZER_H
#define JOYNRMESSAGESERIALIZER_H

#include "joynr/ClassDeserializer.h"
#include "joynr/ClassSerializer.h"
#include "joynr/JoynrMessage.h"
#include "IDeserializer.h"

#include <sstream>

namespace joynr
{
// Serializes a request
template <>
void ClassSerializer<JoynrMessage>::serialize(const JoynrMessage& request, std::ostream& o);

// Deserializes a request
template <>
void ClassDeserializer<JoynrMessage>::deserialize(JoynrMessage& t, IObject& o);

} /* namespace joynr */
#endif // JOYNRMESSAGESERIALIZER_H
