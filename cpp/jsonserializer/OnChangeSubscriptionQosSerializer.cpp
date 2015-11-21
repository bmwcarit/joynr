#include "OnChangeSubscriptionQosSerializer.h"
#include "joynr/SerializerRegistry.h"

namespace joynr
{
// Register the OnChangeSubscriptionQos type id and serializer/deserializer
static const bool isOnChangeSubscriptionQosSerializerRegistered = SerializerRegistry::registerType<OnChangeSubscriptionQos>("joynr.OnChangeSubscriptionQos");

template <>
void ClassDeserializer<OnChangeSubscriptionQos>::deserialize(OnChangeSubscriptionQos& subscription, IObject& o)
{
    while (o.hasNextField()) {
        IField& field = o.nextField();
        if (field.name() == "expiryDate") {
            subscription.setExpiryDate(field.value().getIntType<int64_t>());
        } else if (field.name() == "publicationTtl") {
            subscription.setPublicationTtl(field.value().getIntType<int64_t>());
        } else if (field.name() == "minInterval") {
            subscription.setMinInterval(field.value().getIntType<int64_t>());
        }
    }
}

template <>
void ClassSerializer<OnChangeSubscriptionQos>::serialize(const OnChangeSubscriptionQos& subscription, std::ostream& stream)
{
    stream << R"({)";
    stream << R"("_typeName": ")" << JoynrTypeId<OnChangeSubscriptionQos>::getTypeName() << R"(",)";
    stream << R"("expiryDate": )" << subscription.getExpiryDate() << R"(,)";
    stream << R"("publicationTtl": )" << subscription.getPublicationTtl()<< R"(,)";
    stream << R"("minInterval": )" << subscription.getMinInterval();
    stream << R"(})";
}
}


