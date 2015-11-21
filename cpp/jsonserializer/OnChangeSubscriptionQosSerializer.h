#ifndef ONCHANGESUBSCRIPTIONQOSSERIALIZER_H
#define ONCHANGESUBSCRIPTIONQOSSERIALIZER_H

#include "joynr/ClassDeserializer.h"
#include "joynr/ClassSerializer.h"
#include "joynr/OnChangeSubscriptionQos.h"
#include "IDeserializer.h"

#include <ostream>

namespace joynr
{

// Serializes a OnChangeSubscriptionQos
template <>
void ClassSerializer<OnChangeSubscriptionQos>::serialize(const OnChangeSubscriptionQos& subscription, std::ostream& o);

// Deserializes a OnChangeSubscriptionQos
template <>
void ClassDeserializer<OnChangeSubscriptionQos>::deserialize(OnChangeSubscriptionQos& subscription, IObject& o);

} /* namespace joynr */

#endif // ONCHANGESUBSCRIPTIONQOSSERIALIZER_H
