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
#ifndef JOYNREXCEPTIONSERIALIZER_H
#define JOYNREXCEPTIONSERIALIZER_H

#include "joynr/ClassDeserializer.h"
#include "joynr/ClassSerializer.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/IDeserializer.h"

#include <sstream>

namespace joynr
{
// Serializes a JoynrRuntimeException
template <>
void ClassSerializer<exceptions::JoynrRuntimeException>::serialize(const exceptions::JoynrRuntimeException& exception, std::ostream& o);

// Deserializes a JoynrRuntimeException
template <>
void ClassDeserializer<exceptions::JoynrRuntimeException>::deserialize(exceptions::JoynrRuntimeException& t, IObject& o);

// Serializes a JoynrTimeOutException
template <>
void ClassSerializer<exceptions::JoynrTimeOutException>::serialize(const exceptions::JoynrTimeOutException& exception, std::ostream& o);

// Deserializes a JoynrTimeOutException
template <>
void ClassDeserializer<exceptions::JoynrTimeOutException>::deserialize(exceptions::JoynrTimeOutException& t, IObject& o);

} /* namespace joynr */
#endif // JOYNREXCEPTIONSERIALIZER_H
