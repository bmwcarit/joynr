/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
#ifndef CAPABILITYENTRY_SERIALIZER_H
#define CAPABILITYENTRY_SERIALIZER_H

#include <ostream>

#include "joynr/ClassDeserializer.h"
#include "joynr/ClassSerializer.h"
#include "joynr/CapabilityEntry.h"

namespace joynr
{

/**
 * @brief Specialized serialize method for CapabilityEntry.
 * @param capabilityEntry the object to serialization
 * @param stringstream the stream to write the serialized content to
 */
template <>
void ClassSerializerImpl<CapabilityEntry>::serialize(const CapabilityEntry& capabilityEntry,
                                                     std::ostream& stringstream);

/**
 * @brief Specialized deserialize method for CapabilityEntry.
 * @param capabilityEntry the object to fill during deserialization
 * @param object object containing the parsed json tokens
 */
template <>
void ClassDeserializerImpl<CapabilityEntry>::deserialize(CapabilityEntry& capabilityEntry,
                                                         IObject& object);

} // namespace joynr

#endif // CAPABILITYENTRY_SERIALIZER_H
