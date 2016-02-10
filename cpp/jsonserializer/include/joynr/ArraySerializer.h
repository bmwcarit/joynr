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

namespace joynr
{

class Variant;

/**
 * @brief Helper class that serializes arrays
 */
class ArraySerializer
{
public:
    /**
     * @brief serializes a vector
     * @param array the vector to serialize
     * @param stream the stream to write to
     * @tparam T the type of the vector elements
     */
    template <typename T>
    static void serialize(const std::vector<T>& array, std::ostream& stream);
};

} // namespace joynr

// include after ArraySerializer has been declared to handle cyclic dependencies
#include "joynr/ClassSerializer.h"

namespace joynr
{
/**
 * @brief serializes a vector of variants
 * @param array the vector to serialize
 * @param stream the stream to write to
 */
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
        ClassSerializerImpl<T>::serialize(entry, stream);
    }
    stream << "]";
}

} // namespace joynr
#endif // ARRAYSERIALIZER_H

