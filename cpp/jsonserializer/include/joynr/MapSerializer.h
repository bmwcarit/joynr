/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
 * %%
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * #L%
 */
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

