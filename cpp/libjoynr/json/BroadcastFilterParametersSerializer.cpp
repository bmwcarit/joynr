#include "BroadcastFilterParametersSerializer.h"
#include "joynr/SerializerRegistry.h"
#include "joynr/MapSerializer.h"
#include <string>

namespace joynr
{
// Register the BroadcastFilterParameters type id and serializer/deserializer
static const bool isBroadcastFilterParametersSerializerRegistered =
        SerializerRegistry::registerType<BroadcastFilterParameters>(
                "joynr.BroadcastFilterParameters");

template <>
void ClassDeserializer<BroadcastFilterParameters>::deserialize(
        BroadcastFilterParameters& parameters,
        IObject& o)
{
    while (o.hasNextField()) {
        IField& field = o.nextField();
        if (field.name() == "filterParameters") {
            auto&& converted = convertMap<std::string>(field.value(), convertString);
            parameters.setFilterParameters(converted);
        }
    }
}

template <>
void ClassSerializer<BroadcastFilterParameters>::serialize(
        const BroadcastFilterParameters& parameters,
        std::ostream& stream)
{
    stream << R"({)";
    stream << R"("_typeName": ")" << JoynrTypeId<BroadcastFilterParameters>::getTypeName()
           << R"(",)";
    stream << R"("filterParameters": )";
    MapSerializer::serialize<std::string>(parameters.getFilterParameters(), stream);
    stream << R"(})";
}
}
