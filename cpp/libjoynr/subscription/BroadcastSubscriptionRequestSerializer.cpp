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
#include "BroadcastSubscriptionRequestSerializer.h"

#include "joynr/SerializerRegistry.h"
#include "joynr/Variant.h"
#include "joynr/JoynrTypeId.h"

namespace joynr
{

// Register the BroadcastSubscriptionRequest type id and serializer/deserializer
static const bool isBroadcastSubscriptionRequestRegistered =
        SerializerRegistry::registerType<BroadcastSubscriptionRequest>(
                "joynr.BroadcastSubscriptionRequest");

template <>
void ClassDeserializer<BroadcastSubscriptionRequest>::deserialize(
        BroadcastSubscriptionRequest& request,
        IObject& o)
{
    while (o.hasNextField()) {
        IField& field = o.nextField();
        if (field.name() == "subscriptionId") {
            request.setSubscriptionId(field.value());
        } else if (field.name() == "subscribedToName") {
            request.setSubscribeToName(field.value());
        } else if (field.name() == "qos") {
            request.setQos(convertVariant(field.value()).get<OnChangeSubscriptionQos>());
        } else if (field.name() == "filterParameters") {
            ClassDeserializer<BroadcastFilterParameters> filterParametersDeserializer;
            BroadcastFilterParameters filterParameters;
            filterParametersDeserializer.deserialize(filterParameters, field.value());
            request.setFilterParameters(filterParameters);
        }
    }
}

template <>
void ClassSerializer<BroadcastSubscriptionRequest>::serialize(
        const BroadcastSubscriptionRequest& broadcastSubscriptionRequest,
        std::ostream& stream)
{
    stream << R"({)";
    stream << R"("_typeName":")" << JoynrTypeId<BroadcastSubscriptionRequest>::getTypeName()
           << R"(",)";
    stream << R"("subscriptionId": ")" << broadcastSubscriptionRequest.getSubscriptionId()
           << R"(",)";
    stream << R"("subscribedToName": ")" << broadcastSubscriptionRequest.getSubscribeToName()
           << R"(",)";
    stream << R"("qos": )";
    ClassSerializer<Variant> variantSerializer;
    variantSerializer.serialize(broadcastSubscriptionRequest.getQos(), stream);
    stream << R"(,)";
    stream << R"("filterParameters": )";
    ClassSerializer<BroadcastFilterParameters> filterParametersSerializer;
    filterParametersSerializer.serialize(
            broadcastSubscriptionRequest.getFilterParameters(), stream);
    stream << R"(})";
}

} /* namespace joynr */
