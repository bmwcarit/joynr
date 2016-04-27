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
#include "joynr/CapabilityEntrySerializer.h"

#include <string>
#include <utility>
#include <algorithm>

#include "joynr/ClassDeserializer.h"
#include "joynr/JoynrTypeId.h"
#include "joynr/PrimitiveDeserializer.h"
#include "joynr/SerializerRegistry.h"
#include "joynr/Variant.h"

namespace joynr
{

// Register the CapabilityEntry type id (_typeName value) and serializer/deserializer
static const bool isCapabilityEntrySerializerRegistered =
        SerializerRegistry::registerType<joynr::CapabilityEntry>("joynr.CapabilityEntry");

template <>
void ClassDeserializerImpl<CapabilityEntry>::deserialize(CapabilityEntry& capabilityEntryVar,
                                                         IObject& object)
{
    while (object.hasNextField()) {
        IField& field = object.nextField();
        if (field.name() == "domain") {
            std::string stringValue;
            PrimitiveDeserializer<std::string>::deserialize(stringValue, field.value());
            capabilityEntryVar.setDomain(stringValue);
        } else if (field.name() == "interfaceName") {
            std::string stringValue;
            PrimitiveDeserializer<std::string>::deserialize(stringValue, field.value());
            capabilityEntryVar.setInterfaceName(stringValue);
        } else if (field.name() == "providerQos") {
            types::ProviderQos providerQosContainer;
            ClassDeserializer<types::ProviderQos>::deserialize(providerQosContainer, field.value());
            capabilityEntryVar.setQos(providerQosContainer);
        } else if (field.name() == "participantId") {
            std::string stringValue;
            PrimitiveDeserializer<std::string>::deserialize(stringValue, field.value());
            capabilityEntryVar.setParticipantId(stringValue);
        } else if (field.name() == "isGlobal") {
            bool isGlobal = TypeConverter<bool>::convert(field.value());
            capabilityEntryVar.setGlobal(isGlobal);
        }
    }
}

template <>
void ClassSerializerImpl<CapabilityEntry>::serialize(const CapabilityEntry& capabilityEntryVar,
                                                     std::ostream& stream)
{
    stream << "{";
    stream << "\"_typeName\":\"" << JoynrTypeId<CapabilityEntry>::getTypeName() << "\",";
    stream << "\"domain\": ";
    ClassSerializerImpl<std::string>::serialize(capabilityEntryVar.getDomain(), stream);
    stream << ",";
    stream << "\"interfaceName\": ";
    ClassSerializerImpl<std::string>::serialize(capabilityEntryVar.getInterfaceName(), stream);
    stream << ",";
    stream << "\"providerQos\": ";
    ClassSerializerImpl<types::ProviderQos> providerQosSerializer;
    providerQosSerializer.serialize(capabilityEntryVar.getQos(), stream);
    stream << ",";
    stream << "\"participantId\": ";
    ClassSerializerImpl<std::string>::serialize(capabilityEntryVar.getParticipantId(), stream);
    stream << ",";
    stream << "\"isGlobal\": ";
    ClassSerializerImpl<bool>::serialize(capabilityEntryVar.isGlobal(), stream);
    stream << "}";
}

} // namespace joynr
