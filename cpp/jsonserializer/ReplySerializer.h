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
#ifndef REPLYSERIALIZER_H
#define REPLYSERIALIZER_H

#include "joynr/ClassDeserializer.h"
#include "joynr/ClassSerializer.h"
#include "joynr/Reply.h"
#include "joynr/IDeserializer.h"

#include <ostream>

namespace joynr
{

// Serializes a Reply
template <>
void ClassSerializer<Reply>::serialize(const Reply& reply, std::ostream& o);

// Deserializes a Reply
template <>
void ClassDeserializer<Reply>::deserialize(Reply& t, IObject& o);

} /* namespace joynr */
#endif // REPLYSERIALIZER_H
