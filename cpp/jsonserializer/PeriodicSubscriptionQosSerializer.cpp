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
#include "PeriodicSubscriptionQosSerializer.h"

#include <ostream>

#include "joynr/SerializerRegistry.h"
#include "joynr/JoynrTypeId.h"
#include "joynr/IDeserializer.h"
#include "joynr/SubscriptionQos.h"

namespace joynr
{

// Register the PeriodicSubscriptionQos type id and serializer/deserializer
static const bool isPeriodicSubscriptionQosSerializerRegistered =
        SerializerRegistry::registerType<PeriodicSubscriptionQos>(
                "joynr.PeriodicSubscriptionQos");

template <>
void ClassDeserializerImpl<PeriodicSubscriptionQos>::deserialize(
        PeriodicSubscriptionQos& qos,
        IObject& o)
{
    while (o.hasNextField()) {
        IField& field = o.nextField();
        if (field.name() == "expiryDate") {
            qos.setExpiryDate(field.value().getIntType<std::int64_t>());
        } else if (field.name() == "publicationTtl") {
            qos.setPublicationTtl(field.value().getIntType<std::int64_t>());
        }
    }
}

template <>
void ClassSerializerImpl<PeriodicSubscriptionQos>::serialize(
        const PeriodicSubscriptionQos& qos,
        std::ostream& stream)
{
    stream << R"({)";
    stream << R"("_typeName":")" << JoynrTypeId<SubscriptionQos>::getTypeName() << R"(",)";
    stream << R"("expiryDate": )" << qos.getExpiryDate() << R"(,)";
    stream << R"("publicationTtl": )" << qos.getPublicationTtl() << R"(,)";
    stream << R"("period": )" << qos.getPeriod() << R"(,)";
    stream << R"("alertAfterInterval": )" << qos.getAlertAfterInterval();
    stream << R"(})";
}
} // namespace joynr

