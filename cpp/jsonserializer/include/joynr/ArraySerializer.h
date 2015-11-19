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
#ifndef ARRAYSERIALIZER_H
#define ARRAYSERIALIZER_H

#include <ostream>
#include <vector>
#include "joynr/Variant.h"
#include "joynr/ClassSerializer.h"

namespace joynr
{

/**
 * @brief Helper class that serializes arrays
 */
class ArraySerializer
{
public:
    /**
     * @brief Serialize array to stream
     */
    template <typename T>
    static void serialize(const std::vector<T>& array, std::ostream& stream);
};


template <>
void ArraySerializer::serialize(const std::vector<Variant>& array, std::ostream& stream);

template <typename T>
void ArraySerializer::serialize(const std::vector<T>& array,
                                std::ostream& stream)
{
    stream << "[";
    bool needsComma = false;

    for (const T& entry : array) {
        if (needsComma) {
            stream << ",";
        } else {
            needsComma = true;
        }
        ClassSerializer<T> serializer;
        serializer.serialize(entry, stream);
    }
    stream << "]";
}

} /* namespace joynr */
#endif // ARRAYSERIALIZER_H

