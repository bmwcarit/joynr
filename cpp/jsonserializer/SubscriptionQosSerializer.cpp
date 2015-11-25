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
#include "SubscriptionQosSerializer.h"
#include "joynr/SerializerRegistry.h"

namespace joynr
{
// Register the Reply type id and serializer/deserializer
static const bool isSubscriptionQosSerializerRegistered = SerializerRegistry::registerType<SubscriptionQos>("joynr.SubscriptionQos");

template <>
void ClassDeserializer<SubscriptionQos>::deserialize(SubscriptionQos& subscription, IObject& o)
{
    while (o.hasNextField()) {
        IField& field = o.nextField();
        if (field.name() == "expiryDate") {
            subscription.setExpiryDate(field.value().getIntType<int64_t>());
        } else if (field.name() == "publicationTtl") {
            subscription.setPublicationTtl(field.value().getIntType<int64_t>());
        }
    }
}

template <>
void ClassSerializer<SubscriptionQos>::serialize(const SubscriptionQos& subscription, std::ostream& stream)
{
    stream << R"({)";
    stream << R"("_typeName": ")" << JoynrTypeId<SubscriptionQos>::getTypeName() << R"(",)";
    stream << R"("expiryDate": )" << subscription.getExpiryDate() << R"(,)";
    stream << R"("publicationTtl": )" << subscription.getPublicationTtl();
    stream << R"(})";
}
}
