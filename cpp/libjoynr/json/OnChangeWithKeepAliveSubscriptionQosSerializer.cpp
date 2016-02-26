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
#include "OnChangeWithKeepAliveSubscriptionQosSerializer.h"

#include "joynr/SerializerRegistry.h"
#include "joynr/Variant.h"
#include "joynr/JoynrTypeId.h"
#include "joynr/ArraySerializer.h"

namespace joynr
{

// Register the Reply type id and serializer/deserializer
static const bool isOnChangeWithKeepAliveSubscriptionQosSerializerRegistered =
        SerializerRegistry::registerType<OnChangeWithKeepAliveSubscriptionQos>(
                "joynr.OnChangeWithKeepAliveSubscriptionQos");

template <>
void ClassDeserializerImpl<OnChangeWithKeepAliveSubscriptionQos>::deserialize(
        OnChangeWithKeepAliveSubscriptionQos& qos,
        IObject& o)
{
    while (o.hasNextField()) {
        IField& field = o.nextField();
        if (field.name() == "expiryDateMs") {
            qos.setExpiryDateMs(field.value().getIntType<std::int64_t>());
        } else if (field.name() == "publicationTtlMs") {
            qos.setPublicationTtlMs(field.value().getIntType<std::int64_t>());
        } else if (field.name() == "minIntervalMs") {
            qos.setMinIntervalMs(field.value().getIntType<std::int64_t>());
        } else if (field.name() == "maxInterval") {
            qos.setMaxInterval(field.value().getIntType<std::int64_t>());
        } else if (field.name() == "alertAfterInterval") {
            qos.setAlertAfterInterval(field.value().getIntType<std::int64_t>());
        }
    }
}

template <>
void ClassSerializerImpl<OnChangeWithKeepAliveSubscriptionQos>::serialize(
        const OnChangeWithKeepAliveSubscriptionQos& qos,
        std::ostream& stream)
{
    stream << R"({)";
    stream << R"("_typeName":")" << JoynrTypeId<OnChangeWithKeepAliveSubscriptionQos>::getTypeName()
           << R"(",)";
    stream << R"("expiryDateMs": )" << qos.getExpiryDateMs() << R"(,)";
    stream << R"("publicationTtlMs": )" << qos.getPublicationTtlMs() << R"(,)";
    stream << R"("minIntervalMs": )" << qos.getMinIntervalMs() << R"(,)";
    stream << R"("maxInterval": )" << qos.getMaxInterval() << R"(,)";
    stream << R"("alertAfterInterval": )" << qos.getAlertAfterInterval();
    stream << R"(})";
}

} // namespace joynr
