#include "joynr/ArraySerializer.h"

namespace joynr {

template <>
void ArraySerializer::serialize(const std::vector<Variant>& array, std::ostream& stream)
{
    stream << "[";
    bool needsComma = false;

    for (const Variant& entry : array) {
        if (needsComma) {
            stream << ",";
        } else {
            needsComma = true;
        }
        if (entry.is<std::vector<Variant>>()){
            ArraySerializer::serialize<Variant>(entry.get<std::vector<Variant>>(), stream);
        } else {
            ClassSerializer<Variant> serializer;
            serializer.serialize(entry, stream);
        }
    }
    stream << "]";
}

}
