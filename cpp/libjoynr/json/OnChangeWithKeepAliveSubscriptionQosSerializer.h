#ifndef ONCHANGEWITHKEEPALIVESUBSCRIPTIONQOSSERIALIZER_H
#define ONCHANGEWITHKEEPALIVESUBSCRIPTIONQOSSERIALIZER_H

#include "joynr/ClassDeserializer.h"
#include "joynr/ClassSerializer.h"
#include "joynr/OnChangeWithKeepAliveSubscriptionQos.h"
#include "jsonserializer/IDeserializer.h"

#include <ostream>

namespace joynr
{

// Serializes a OnChangeWithKeepAliveSubscriptionQos
template <>
void ClassSerializer<OnChangeWithKeepAliveSubscriptionQos>::serialize(
        const OnChangeWithKeepAliveSubscriptionQos& qos,
        std::ostream& o);

// Deserializes a OnChangeWithKeepAliveSubscriptionQos
template <>
void ClassDeserializer<OnChangeWithKeepAliveSubscriptionQos>::deserialize(
        OnChangeWithKeepAliveSubscriptionQos& qos,
        IObject& o);

} /* namespace joynr */
#endif // ONCHANGEWITHKEEPALIVESUBSCRIPTIONQOSSERIALIZER_H
