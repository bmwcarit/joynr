#ifndef SUBSCRIPTIONREQUESTINFORMATIONSERIALIZER_H
#define SUBSCRIPTIONREQUESTINFORMATIONSERIALIZER_H

#include "joynr/ClassDeserializer.h"
#include "joynr/ClassSerializer.h"
#include "subscription/SubscriptionRequestInformation.h"
#include "jsonserializer/IDeserializer.h"

#include <ostream>

namespace joynr
{

// Serializes a SubscriptionRequestInformation
template <>
void ClassSerializer<SubscriptionRequestInformation>::serialize(
        const SubscriptionRequestInformation& info,
        std::ostream& o);

// Deserializes a SubscriptionRequestInformation
template <>
void ClassDeserializer<SubscriptionRequestInformation>::deserialize(
        SubscriptionRequestInformation& info,
        IObject& o);

} /* namespace joynr */

#endif // SUBSCRIPTIONREQUESTINFORMATIONSERIALIZER_H
