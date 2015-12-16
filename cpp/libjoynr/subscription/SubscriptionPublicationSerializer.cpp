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
#include "SubscriptionPublicationSerializer.h"

#include "joynr/SerializerRegistry.h"
#include "joynr/Variant.h"
#include "joynr/JoynrTypeId.h"
#include "joynr/ArraySerializer.h"

namespace joynr
{

// Register the SubscriptionPublication type id and serializer/deserializer
static const bool isSubscriptionPublicationRegistered =
        SerializerRegistry::registerType<SubscriptionPublication>("joynr.SubscriptionPublication");

template <>
void ClassDeserializer<SubscriptionPublication>::deserialize(SubscriptionPublication& subscription,
                                                             IObject& o)
{
    while (o.hasNextField()) {
        IField& field = o.nextField();
        if (field.name() == "subscriptionId") {
            subscription.setSubscriptionId(field.value());
        } else if (field.name() == "response") {
            IArray& array = field.value();
            auto&& converted = convertArray<Variant>(array, convertVariant);
            subscription.setResponse(std::forward<std::vector<Variant>>(converted));
        } else if (field.name() == "error") {
            subscription.setError(convertVariant(field.value()));
        }
    }
}

template <>
void ClassSerializer<SubscriptionPublication>::serialize(
        const SubscriptionPublication& subscriptionPublication,
        std::ostream& stream)
{
    stream << R"({)";
    stream << R"("_typeName":")" << JoynrTypeId<SubscriptionPublication>::getTypeName() << R"(",)";
    stream << R"("subscriptionId": ")" << subscriptionPublication.getSubscriptionId() << R"(",)";
    if (!subscriptionPublication.getError().isEmpty()) {
        stream << R"("error": )";
        ClassSerializer<Variant> variantSerializer;
        variantSerializer.serializeVariant(subscriptionPublication.getError(), stream);
    } else {
        stream << R"("response": )";
        ArraySerializer::serialize<Variant>(subscriptionPublication.getResponse(), stream);
    }
    stream << R"(})";
}

} /* namespace joynr */
