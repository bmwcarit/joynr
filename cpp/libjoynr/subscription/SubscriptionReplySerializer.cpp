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
#include "SubscriptionReplySerializer.h"

#include "joynr/ArraySerializer.h"
#include "joynr/SerializerRegistry.h"
#include "joynr/JoynrTypeId.h"

#include <string>
#include <utility>

namespace joynr
{

// Register the SubscriptionStop type id and serializer/deserializer
static const bool isSubscriptionReplyRegistered =
        SerializerRegistry::registerType<SubscriptionReply>("joynr.SubscriptionReply");

template <>
void ClassDeserializerImpl<SubscriptionReply>::deserialize(SubscriptionReply& t, IObject& o)
{
    while (o.hasNextField()) {
        IField& field = o.nextField();
        if (field.name() == "subscriptionId") {
            t.setSubscriptionId(field.value());
        }
    }
}

template <>
void ClassSerializerImpl<SubscriptionReply>::serialize(const SubscriptionReply& subscriptionReply,
                                                       std::ostream& stream)
{
    stream << "{";
    stream << "\"_typeName\": \"" << JoynrTypeId<SubscriptionReply>::getTypeName() << "\",";
    stream << "\"subscriptionId\": \"" << subscriptionReply.getSubscriptionId() << "\"";
    stream << "}";
}

} // namespace joynr
