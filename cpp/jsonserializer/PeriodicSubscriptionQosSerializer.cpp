#include "PeriodicSubscriptionQosSerializer.h"

#include "joynr/SerializerRegistry.h"
#include "joynr/Variant.h"
#include "joynr/JoynrTypeId.h"

namespace joynr
{

// Register the PeriodicSubscriptionQos type id and serializer/deserializer
static const bool isPeriodicSubscriptionQosSerializerRegistered =
        SerializerRegistry::registerType<PeriodicSubscriptionQos>(
                "joynr.PeriodicSubscriptionQos");

template <>
void ClassDeserializer<PeriodicSubscriptionQos>::deserialize(
        PeriodicSubscriptionQos& qos,
        IObject& o)
{
    while (o.hasNextField()) {
        IField& field = o.nextField();
        if (field.name() == "expiryDate") {
            qos.setExpiryDate(field.value().getIntType<int64_t>());
        } else if (field.name() == "publicationTtl") {
            qos.setPublicationTtl(field.value().getIntType<int64_t>());
        }
    }
}

template <>
void ClassSerializer<PeriodicSubscriptionQos>::serialize(
        const PeriodicSubscriptionQos& qos,
        std::ostream& stream)
{
    stream << R"({)";
    stream << R"("_typeName": ")" << JoynrTypeId<SubscriptionQos>::getTypeName() << R"(",)";
    stream << R"("expiryDate": )" << qos.getExpiryDate() << R"(,)";
    stream << R"("publicationTtl": )" << qos.getPublicationTtl() << R"(,)";
    stream << R"("period": )" << qos.getPeriod() << R"(,)";
    stream << R"("alertAfterInterval": )" << qos.getAlertAfterInterval();
    stream << R"(})";
}
} /* namespace joynr */

