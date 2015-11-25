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
#include "OnChangeSubscriptionQosSerializer.h"
#include "joynr/SerializerRegistry.h"

namespace joynr
{
// Register the OnChangeSubscriptionQos type id and serializer/deserializer
static const bool isOnChangeSubscriptionQosSerializerRegistered = SerializerRegistry::registerType<OnChangeSubscriptionQos>("joynr.OnChangeSubscriptionQos");

template <>
void ClassDeserializer<OnChangeSubscriptionQos>::deserialize(OnChangeSubscriptionQos& subscription, IObject& o)
{
    while (o.hasNextField()) {
        IField& field = o.nextField();
        if (field.name() == "expiryDate") {
            subscription.setExpiryDate(field.value().getIntType<int64_t>());
        } else if (field.name() == "publicationTtl") {
            subscription.setPublicationTtl(field.value().getIntType<int64_t>());
        } else if (field.name() == "minInterval") {
            subscription.setMinInterval(field.value().getIntType<int64_t>());
        }
    }
}

template <>
void ClassSerializer<OnChangeSubscriptionQos>::serialize(const OnChangeSubscriptionQos& subscription, std::ostream& stream)
{
    stream << R"({)";
    stream << R"("_typeName": ")" << JoynrTypeId<OnChangeSubscriptionQos>::getTypeName() << R"(",)";
    stream << R"("expiryDate": )" << subscription.getExpiryDate() << R"(,)";
    stream << R"("publicationTtl": )" << subscription.getPublicationTtl()<< R"(,)";
    stream << R"("minInterval": )" << subscription.getMinInterval();
    stream << R"(})";
}
}


