#ifndef MAPSERIALIZER
#define MAPSERIALIZER

#include <ostream>
#include <string>
#include <vector>
#include <map>

namespace joynr
{

/**
 * @brief Helper class that serializes maps with std::string keys
 */
class MapSerializer
{
public:
    /**
     * @brief Serialize array to stream
     */
    template <typename T>
    static void serialize(const std::map<std::string, T>& map, std::ostream& stream);
};

template <typename T>
void MapSerializer::serialize(const std::map<std::string, T>& map,
                                std::ostream& stream)
{
    stream << "{";
    bool needsComma = false;

    for (const auto& entry : map) {
        if (needsComma) {
            stream << ",";
        } else {
            needsComma = true;
        }
        ClassSerializer<std::string> stringSerializer;
        stringSerializer.serialize(entry.first, stream);
        stream << R"(: )";
        ClassSerializer<T> serializer;
        serializer.serialize(entry.second, stream);
    }
    stream << "}";
}

} /* namespace joynr */
#endif // MAPSERIALIZER

