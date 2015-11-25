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
