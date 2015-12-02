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
#ifndef BROADCASTFILTERPARAMETERSSERIALIZER_H
#define BROADCASTFILTERPARAMETERSSERIALIZER_H

#include "joynr/ClassSerializer.h"
#include "joynr/ClassDeserializer.h"
#include "joynr/BroadcastFilterParameters.h"

#include <ostream>

namespace joynr
{

class IObject;

// Serializes a BroadcastFilterParameters
template <>
void ClassSerializer<BroadcastFilterParameters>::serialize(
        const BroadcastFilterParameters& parameters,
        std::ostream& o);

// Deserializes a BroadcastFilterParameters
template <>
void ClassDeserializer<BroadcastFilterParameters>::deserialize(
        BroadcastFilterParameters& parameters,
        IObject& o);

} /* namespace joynr */

#endif // BROADCASTFILTERPARAMETERSSERIALIZER_H
