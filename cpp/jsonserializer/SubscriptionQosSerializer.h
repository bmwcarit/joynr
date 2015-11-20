#ifndef SUBSCRIPTIONQOSSERIALIZER_H
#define SUBSCRIPTIONQOSSERIALIZER_H

#include "joynr/ClassDeserializer.h"
#include "joynr/ClassSerializer.h"
#include "joynr/SubscriptionQos.h"
#include "IDeserializer.h"

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
