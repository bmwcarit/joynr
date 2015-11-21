#ifndef BROADCASTFILTERPARAMETERSSERIALIZER_H
#define BROADCASTFILTERPARAMETERSSERIALIZER_H

#include "joynr/ClassDeserializer.h"
#include "joynr/ClassSerializer.h"
#include "joynr/BroadcastFilterParameters.h"
#include "jsonserializer/IDeserializer.h"

#include <ostream>

namespace joynr
{

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
