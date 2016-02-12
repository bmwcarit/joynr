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
#include "exceptions/JoynrExceptionSerializer.h"

#include <ostream>

#include "joynr/SerializerRegistry.h"
#include "joynr/JoynrTypeId.h"
#include "joynr/IDeserializer.h"
#include "joynr/PrimitiveDeserializer.h"

namespace joynr
{

// Register the JoynrRuntimeException type id and serializer/deserializer
static const bool isJoynrRuntimeExceptionRegistered =
        SerializerRegistry::registerType<exceptions::JoynrRuntimeException>("joynr.exceptions.JoynrRuntimeException");
// Register the ProviderRuntimeException type id and serializer/deserializer
static const bool isProviderRuntimeExceptionRegistered =
        SerializerRegistry::registerType<exceptions::ProviderRuntimeException>("joynr.exceptions.ProviderRuntimeException");
// Register the DiscoveryException type id and serializer/deserializer
static const bool isDiscoveryExceptionRegistered =
        SerializerRegistry::registerType<exceptions::DiscoveryException>("joynr.exceptions.DiscoveryException");
// Register the ApplicationException type id and serializer/deserializer
static const bool isApplicationExceptionRegistered =
        SerializerRegistry::registerType<exceptions::ApplicationException>("joynr.exceptions.ApplicationException");
// Register the JoynrTimeOutException type id and serializer/deserializer
static const bool isJoynrTimeOutExceptionRegistered =
        SerializerRegistry::registerType<exceptions::JoynrTimeOutException>("joynr.exceptions.JoynrTimeOutException");
// Register the PublicationMissedException type id and serializer/deserializer
static const bool isPublicationMissedExceptionRegistered =
        SerializerRegistry::registerType<exceptions::PublicationMissedException>("joynr.exceptions.PublicationMissedException");
// Register the MethodInvocationException type id and serializer/deserializer
static const bool isMethodInvocationExceptionRegistered =
        SerializerRegistry::registerType<exceptions::MethodInvocationException>("joynr.exceptions.MethodInvocationException");

template <>
void ClassDeserializerImpl<exceptions::ApplicationException>::deserialize(exceptions::ApplicationException& t, IObject& o)
{
    while (o.hasNextField()) {
        IField& field = o.nextField();
        if (field.name() == "detailMessage") {
            t.setMessage(field.value());
        } else if (field.name() == "error") {
            IObject& error = field.value();
            std::shared_ptr<IPrimitiveDeserializer> deserializer;
            while(error.hasNextField()){
                IField& errorField = error.nextField();
                if (errorField.name() == "_typeName") {
                    t.setErrorTypeName(errorField.value());
                    deserializer = SerializerRegistry::getPrimitiveDeserializer(t.getErrorTypeName());
                } else if (errorField.name() == "name") {
                    t.setName(errorField.value());
                    //we assume that the _typeName is contained before the name field in the json
                    if (deserializer.get() != nullptr) {
                        t.setError(deserializer->deserializeVariant(errorField.value()));
                    } else {
                        throw joynr::exceptions::JoynrRuntimeException("Received ApplicationException does not contain a valid error enumeration.");
                    }
                }
            }
        }

    }
}
template <>
void ClassDeserializerImpl<exceptions::JoynrRuntimeException>::deserialize(exceptions::JoynrRuntimeException& t, IObject& o)
{
    while (o.hasNextField()) {
        IField& field = o.nextField();
        if (field.name() == "detailMessage") {
            t.setMessage(field.value());
        }
    }
}
template <>
void ClassDeserializerImpl<exceptions::ProviderRuntimeException>::deserialize(exceptions::ProviderRuntimeException& t, IObject& o)
{
    ClassDeserializerImpl<exceptions::JoynrRuntimeException>::deserialize(t, o);
}
template <>
void ClassDeserializerImpl<exceptions::DiscoveryException>::deserialize(exceptions::DiscoveryException& t, IObject& o)
{
    ClassDeserializerImpl<exceptions::JoynrRuntimeException>::deserialize(t, o);
}
template <>
void ClassDeserializerImpl<exceptions::JoynrTimeOutException>::deserialize(exceptions::JoynrTimeOutException& t, IObject& o)
{
    ClassDeserializerImpl<exceptions::JoynrRuntimeException>::deserialize(t, o);
}
template <>
void ClassDeserializerImpl<exceptions::MethodInvocationException>::deserialize(exceptions::MethodInvocationException& t, IObject& o)
{
    ClassDeserializerImpl<exceptions::JoynrRuntimeException>::deserialize(t, o);
}
template <>
void ClassDeserializerImpl<exceptions::PublicationMissedException>::deserialize(exceptions::PublicationMissedException& t, IObject& o)
{
    while (o.hasNextField()) {
        IField& field = o.nextField();
        if (field.name() == "subscriptionId") {
            t.setSubscriptionId(field.value());
        }
    }
}

void initSerialization (const std::string& typeName, std::ostream& stream) {
    stream << R"({)";
    stream << R"("_typeName":")" << typeName << R"(",)";
}

void serializeExceptionWithDetailMessage(const std::string& typeName, const exceptions::JoynrException& exception, std::ostream& stream) {
    initSerialization(typeName, stream);
    stream << R"("detailMessage": ")" << exception.getMessage() << R"(")";
    stream << "}";
}

template <>
void ClassSerializerImpl<exceptions::ApplicationException>::serialize(const exceptions::ApplicationException& exception, std::ostream& stream)
{
    initSerialization(JoynrTypeId<exceptions::ApplicationException>::getTypeName(), stream);
    if (!exception.getMessage().empty()) {
        stream << R"("detailMessage": ")" << exception.getMessage() << R"(",)";
    }
    stream << R"("error": {)";
    stream << R"("_typeName":")" << exception.getErrorTypeName() << R"(",)";
    stream << R"("name": ")" << exception.getName() << R"(")";
    stream << "}"; //error
    stream << "}"; //exception
}

template <>
void ClassSerializerImpl<exceptions::JoynrRuntimeException>::serialize(const exceptions::JoynrRuntimeException& exception, std::ostream& stream)
{
    serializeExceptionWithDetailMessage(JoynrTypeId<exceptions::JoynrRuntimeException>::getTypeName(), exception, stream);
}
template <>
void ClassSerializerImpl<exceptions::ProviderRuntimeException>::serialize(const exceptions::ProviderRuntimeException& exception, std::ostream& stream)
{
    serializeExceptionWithDetailMessage(JoynrTypeId<exceptions::ProviderRuntimeException>::getTypeName(), exception, stream);
}
template <>
void ClassSerializerImpl<exceptions::DiscoveryException>::serialize(const exceptions::DiscoveryException& exception, std::ostream& stream)
{
    serializeExceptionWithDetailMessage(JoynrTypeId<exceptions::DiscoveryException>::getTypeName(), exception, stream);
}
template <>
void ClassSerializerImpl<exceptions::JoynrTimeOutException>::serialize(const exceptions::JoynrTimeOutException& exception, std::ostream& stream)
{
    serializeExceptionWithDetailMessage(JoynrTypeId<exceptions::JoynrTimeOutException>::getTypeName(), exception, stream);
}
template <>
void ClassSerializerImpl<exceptions::MethodInvocationException>::serialize(const exceptions::MethodInvocationException& exception, std::ostream& stream)
{
    serializeExceptionWithDetailMessage(JoynrTypeId<exceptions::MethodInvocationException>::getTypeName(), exception, stream);
}
template <>
void ClassSerializerImpl<exceptions::PublicationMissedException>::serialize(const exceptions::PublicationMissedException& exception, std::ostream& stream)
{
    initSerialization(JoynrTypeId<exceptions::PublicationMissedException>::getTypeName(), stream);
    stream << R"("subscriptionId": ")" << exception.getSubscriptionId() << R"(")";
    stream << "}";
}

} // namespace joynr
