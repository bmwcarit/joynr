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
#ifndef REQUESTSERIALIZER_H
#define REQUESTSERIALIZER_H

#include "joynr/ClassDeserializer.h"
#include "joynr/ClassSerializer.h"

#include <ostream>

namespace joynr
{

class Request;

// Serializes a request
template <>
void ClassSerializer<Request>::serialize(const Request& request, std::ostream& o);

// Deserializes a request
template <>
void ClassDeserializer<Request>::deserialize(Request& t, IObject& o);

} // namespace joynr
#endif // REQUESTSERIALIZER_H
