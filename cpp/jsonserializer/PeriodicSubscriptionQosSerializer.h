#ifndef PERIODICSUBSCRIPTIONQOSSERIALIZER_H
#define PERIODICSUBSCRIPTIONQOSSERIALIZER_H

#include "joynr/ClassDeserializer.h"
#include "joynr/ClassSerializer.h"
#include "joynr/PeriodicSubscriptionQos.h"
#include "jsonserializer/IDeserializer.h"

#include <ostream>

namespace joynr
{

// Serializes a PeriodicSubscriptionQos
template <>
void ClassSerializer<PeriodicSubscriptionQos>::serialize(
        const PeriodicSubscriptionQos& qos,
        std::ostream& o);

// Deserializes a PeriodicSubscriptionQos
template <>
void ClassDeserializer<PeriodicSubscriptionQos>::deserialize(
        PeriodicSubscriptionQos& qos,
        IObject& o);

} /* namespace joynr */
#endif // PERIODICSUBSCRIPTIONQOSSERIALIZER_H
