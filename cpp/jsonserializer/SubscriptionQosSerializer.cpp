#include "SubscriptionQosSerializer.h"
#include "joynr/SerializerRegistry.h"

namespace joynr
{
// Register the Reply type id and serializer/deserializer
static const bool isSubscriptionQosSerializerRegistered = SerializerRegistry::registerType<SubscriptionQos>("joynr.SubscriptionQos");

template <>
void ClassDeserializer<SubscriptionQos>::deserialize(SubscriptionQos& subscription, IObject& o)
{
    while (o.hasNextField()) {
        IField& field = o.nextField();
        if (field.name() == "expiryDate") {
            subscription.setExpiryDate(field.value().getIntType<int64_t>());
        } else if (field.name() == "publicationTtl") {
            subscription.setPublicationTtl(field.value().getIntType<int64_t>());
        }
    }
}

template <>
void ClassSerializer<SubscriptionQos>::serialize(const SubscriptionQos& subscription, std::ostream& stream)
{
    stream << R"({)";
    stream << R"("_typeName": ")" << JoynrTypeId<SubscriptionQos>::getTypeName() << R"(",)";
    stream << R"("expiryDate": )" << subscription.getExpiryDate() << R"(,)";
    stream << R"("publicationTtl": )" << subscription.getPublicationTtl();
    stream << R"(})";
}
}
