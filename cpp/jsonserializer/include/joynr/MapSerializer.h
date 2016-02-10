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
#ifndef MAPSERIALIZER_H
#define MAPSERIALIZER_H

#include <ostream>
#include <string>
#include <map>
#include "joynr/ClassSerializer.h"

namespace joynr
{

/**
 * @brief Helper class that serializes maps with std::string keys
 */
class MapSerializer
{
public:
    /**
     * @brief Serialize map to stream
     */
    template <typename T, typename S>
    static void serialize(const std::string& typeName, const std::map<T, S>& map, std::ostream& stream);

    template <typename T, typename S>
    static void serialize(const std::map<T, S>& map, std::ostream& stream);
private:
    template <typename T, typename S>
    static void serializeEntries(const std::map<T, S>& map, std::ostream& stream, bool needsComma);
};


template <typename T, typename S>
void MapSerializer::serializeEntries(const std::map<T, S>& map,
                                     std::ostream& stream,
                                     bool needsInitialComma)
{
    for (const auto& entry : map) {
        if (needsInitialComma) {
            stream << ",";
        } else {
            needsInitialComma = true;
        }
        ClassSerializerImpl<T>::serialize(entry.first, stream);
        stream << R"(: )";
        ClassSerializerImpl<S>::serialize(entry.second, stream);
    }
}

template <typename T, typename S>
void MapSerializer::serialize(const std::string& typeName,
                              const std::map<T, S>& map,
                              std::ostream& stream)
{
    stream << "{";
    stream << "\"_typeName\":\"" << typeName << "\"";
    serializeEntries(map, stream, true);
    stream << "}";
}

template <typename T, typename S>
void MapSerializer::serialize(const std::map<T, S>& map,
                              std::ostream& stream)
{
    stream << "{";
    serializeEntries(map, stream, false);
    stream << "}";
}

} // namespace joynr
#endif // MAPSERIALIZER_H

