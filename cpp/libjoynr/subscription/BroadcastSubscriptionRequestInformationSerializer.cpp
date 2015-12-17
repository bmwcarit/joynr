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
#include "BroadcastSubscriptionRequestInformationSerializer.h"

#include "joynr/SerializerRegistry.h"
#include "joynr/Variant.h"
#include "joynr/JoynrTypeId.h"

namespace joynr
{

// Register the Reply type id and serializer/deserializer
static const bool isBroadcastSubscriptionRequestInformationSerializerRegistered =
        SerializerRegistry::registerType<BroadcastSubscriptionRequestInformation>(
                "joynr.BroadcastSubscriptionRequestInformation");

template <>
void ClassDeserializer<BroadcastSubscriptionRequestInformation>::deserialize(
        BroadcastSubscriptionRequestInformation& info,
        IObject& o)
{
    while (o.hasNextField()) {
        IField& field = o.nextField();
        if (field.name() == "subscriptionId") {
            info.setSubscriptionId(field.value());
        } else if (field.name() == "subscribeToName") {
            info.setSubscribeToName(field.value());
        } else if (field.name() == "qos") {
            Variant qos = convertVariant(field.value());
            info.setQos(qos.get<OnChangeSubscriptionQos>());
        } else if (field.name() == "filterParameters") {
            BroadcastFilterParameters filterParameters;
            ClassDeserializer<BroadcastFilterParameters> filterParametersDeserializer;
            filterParametersDeserializer.deserialize(filterParameters, field.value());
            info.setFilterParameters(filterParameters);
        } else if (field.name() == "proxyId") {
            std::string proxyId = field.value();
            info.setProxyId(proxyId);
        } else if (field.name() == "providerId") {
            std::string providerId = field.value();
            info.setProviderId(providerId);
        }
    }
}

template <>
void ClassSerializer<BroadcastSubscriptionRequestInformation>::serialize(
        const BroadcastSubscriptionRequestInformation& info,
        std::ostream& stream)
{
    stream << R"({)";
    stream << R"("_typeName":")"
           << JoynrTypeId<BroadcastSubscriptionRequestInformation>::getTypeName() << R"(",)";
    stream << R"("subscriptionId": ")" << info.getSubscriptionId() << R"(",)";
    stream << R"("subscribeToName": ")" << info.getSubscribeToName() << R"(",)";
    stream << R"("qos": )";
    ClassSerializer<Variant> variantSerializer;
    variantSerializer.serialize(info.getQos(), stream);
    stream << R"(,"filterParameters": )";
    if (info.getFilterParameters().getFilterParameters().empty()) {
        stream << R"({})";
    } else {
        ClassSerializer<BroadcastFilterParameters> filterParametersSerializer;
        filterParametersSerializer.serialize(info.getFilterParameters(), stream);
    }
    stream << R"(,"proxyId": ")" << info.getProxyId() << R"(",)";
    stream << R"("providerId": ")" << info.getProviderId();
    stream << R"("})";
}
} // namespace joynr
