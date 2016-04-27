/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

#include "joynr/ClassSerializer.h"

namespace joynr
{

/*
 * MapSerializer class used to serialize std::map and std::unordered_map to JSON.
 */
class MapSerializer
{
public:
    template <typename Map>
    static void serialize(const std::string& typeName, const Map& map, std::ostream& stream);

    template <typename Map>
    static void serialize(const Map& map, std::ostream& stream);

private:
    template <typename Map>
    static void serializeEntries(const Map& map, std::ostream& stream, bool needsComma);
};

template <typename Map>
void MapSerializer::serializeEntries(const Map& map,
                                     std::ostream& stream,
                                     bool needsInitialComma)
{
    using Key = typename Map::key_type;
    using Value = typename Map::mapped_type;

    for (const auto& entry : map) {
        if (needsInitialComma) {
            stream << ",";
        } else {
            needsInitialComma = true;
        }
        ClassSerializerImpl<Key>::serialize(entry.first, stream);
        stream << R"(: )";
        ClassSerializerImpl<Value>::serialize(entry.second, stream);
    }
}

template <typename Map>
void MapSerializer::serialize(const std::string& typeName,
                              const Map& map,
                              std::ostream& stream)
{
    stream << "{";
    stream << "\"_typeName\":\"" << typeName << "\"";
    serializeEntries(map, stream, true);
    stream << "}";
}

template <typename Map>
void MapSerializer::serialize(const Map& map,
                              std::ostream& stream)
{
    stream << "{";
    serializeEntries(map, stream, false);
    stream << "}";
}

} // namespace joynr
#endif // MAPSERIALIZER_H

