#include "OnChangeWithKeepAliveSubscriptionQosSerializer.h"

#include "joynr/SerializerRegistry.h"
#include "joynr/Variant.h"
#include "joynr/JoynrTypeId.h"
#include "joynr/ArraySerializer.h"

namespace joynr
{

// Register the Reply type id and serializer/deserializer
static const bool isOnChangeWithKeepAliveSubscriptionQosSerializerRegistered =
        SerializerRegistry::registerType<OnChangeWithKeepAliveSubscriptionQos>(
                "joynr.OnChangeWithKeepAliveSubscriptionQos");

template <>
void ClassDeserializer<OnChangeWithKeepAliveSubscriptionQos>::deserialize(
        OnChangeWithKeepAliveSubscriptionQos& qos,
        IObject& o)
{
    while (o.hasNextField()) {
        IField& field = o.nextField();
        if (field.name() == "expiryDate") {
            qos.setExpiryDate(field.value().getIntType<int64_t>());
        } else if (field.name() == "publicationTtl") {
            qos.setPublicationTtl(field.value().getIntType<int64_t>());
        } else if (field.name() == "minInterval") {
            qos.setMinInterval(field.value().getIntType<int64_t>());
        } else if (field.name() == "maxInterval") {
            qos.setMaxInterval(field.value().getIntType<int64_t>());
        } else if (field.name() == "alertAfterInterval") {
            qos.setAlertAfterInterval(field.value().getIntType<int64_t>());
        }
    }
}

template <>
void ClassSerializer<OnChangeWithKeepAliveSubscriptionQos>::serialize(
        const OnChangeWithKeepAliveSubscriptionQos& qos,
        std::ostream& stream)
{
    stream << R"({)";
    stream << R"("_typeName": ")"
           << JoynrTypeId<OnChangeWithKeepAliveSubscriptionQos>::getTypeName() << R"(",)";
    stream << R"("expiryDate": )" << qos.getExpiryDate() << R"(,)";
    stream << R"("publicationTtl": )" << qos.getPublicationTtl() << R"(,)";
    stream << R"("minInterval": )" << qos.getMinInterval() << R"(,)";
    stream << R"("maxInterval": )" << qos.getMaxInterval() << R"(,)";
    stream << R"("alertAfterInterval": )" << qos.getAlertAfterInterval();
    stream << R"(})";
}

} /* namespace joynr */
