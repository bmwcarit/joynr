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

#ifndef SELECTEDDESERIALIZERSHAREDPTR_H
#define SELECTEDDESERIALIZERSHAREDPTR_H

#include <memory>

#include "joynr/Logger.h"
#include "joynr/SerializerRegistry.h"
#include "joynr/Variant.h"

namespace joynr
{

/*
 * Deserialize std::shared_ptr<T>
 *
 * The deserialize is being used to deserialize maps of std::shared_ptr<T> used for example
 * to deserialize routing tables.
 *
 */
template <typename T>
struct SelectedDeserializer<std::shared_ptr<T>>
{
    /**
     * @brief Deserialize std::shared_ptr<T>. A deserializer for T must already exists.
     * and serialization must include typeName information.
     * @param value reference to concrete IValue
     * @return typeReference is a reference to the deserialized std::shared_ptr<T>
     */
    static void deserialize(std::shared_ptr<T>& typeReference, IValue& value)
    {
        if(!value.isObject()) {
            JOYNR_LOG_FATAL(logger, "input value is not an object");
            return;
        }

        IObject& object = value;
        if(!object.hasNextField()) {
            JOYNR_LOG_FATAL(logger, "input object does not have any fields");
            return;
        }

        IField& field = object.nextField();
        if(field.name() != "_typeName") {
            JOYNR_LOG_FATAL(logger, "first element of input object must be _typeName");
            return;
        }

        std::unique_ptr<IClassDeserializer> deserializer = SerializerRegistry::getDeserializer(field.value());
        if (!deserializer) {
            JOYNR_LOG_FATAL(logger, "no deserializer found for {}", std::string(field.value()));
            return;
        }

        Variant variant = deserializer->deserializeVariant(object);
        T& type = variant.get<T>();
        std::shared_ptr<T> typePtr(type.clone().release());
        typeReference =  typePtr;
    }

    ADD_LOGGER(SelectedDeserializer<std::shared_ptr<T>>);
};

template <typename T>
INIT_LOGGER(SelectedDeserializer<std::shared_ptr<T>>);

} // namespace joynr
#endif // SELECTEDDESERIALIZERSHAREDPTR_H

