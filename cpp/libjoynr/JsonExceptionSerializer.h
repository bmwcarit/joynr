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
#ifndef JSONEXCEPTIONSERIALIZER_H
#define JSONEXCEPTIONSERIALIZER_H

#include "qjson/qobjecthelper.h"
#include "qjson/parser.h"
#include "qjson/serializer.h"
#include "joynr/joynrlogging.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/SerializerRegistry.h"
#include "joynr/Reply.h"
#include "joynr/SubscriptionPublication.h"
#include "joynr/JsonSerializer.h"
#include <QString>
#include <QVariantMap>
#include <QVariant>
#include <QMetaType>
#include <QByteArray>

namespace joynr
{

/**
 * @brief Tool class to provide JSON serialization
 */
class JsonExceptionSerializer
{
public:
    /**
     * @brief Serializes a Reply object into JSON format.
     *
     * @param reply the reply object to serialize.
     * @return QByteArray the serialized byte array in JSON format, UTF-8 encoding.
     */
    static QByteArray serializeReply(const Reply& reply)
    {
        QByteArray json = JsonSerializer::serializeQObject(reply);
        Variant error = reply.getError();
        if (!error.isEmpty()) {
            serializeJoynrException(json, error);
        }
        return json;
    }

    /**
     * @brief Serializes a SubsriptionPublication object into JSON format.
     *
     * @param publication the publication object to serialize.
     * @return QByteArray the serialized byte array in JSON format, UTF-8 encoding.
     */
    static QByteArray serializeSubscriptionPublication(const SubscriptionPublication& publication)
    {
        QByteArray json = JsonSerializer::serializeQObject(publication);
        Variant error = publication.getError();
        if (!error.isEmpty()) {
            serializeJoynrException(json, error);
        }
        return json;
    }

    // TODO This is a workaround which must be removed after the new serializer is introduced
    /**
         * @brief Deserializes a QByteArray in JSON format to a Reply object.
         *
         * The QByteArray must be a valid JSON representation of a Reply message.
         *
         * @param json The JSON representation of the Reply message.
         * @return The deserialized Reply object, or NULL in case of deserialization error
         */
    static Reply* deserializeReply(const QByteArray& json)
    {
        QJson::Parser parser;

        QVariant jsonQVar = parser.parse(json);
        QVariantMap jsonQVarValue = jsonQVar.value<QVariantMap>();

        Reply* reply = (Reply*)JsonSerializer::deserializeQObject<Reply>(json);
        if (reply == Q_NULLPTR) {
            return reply;
        }

        if (jsonQVarValue.contains(JSON_FIELD_NAME_EXCEPTION)) {
            QVariantMap nestedMap = jsonQVarValue.value(JSON_FIELD_NAME_EXCEPTION).toMap();

            const Variant error(deserializeJoynrException(nestedMap, json));
            if (!error.isEmpty()) {
                reply->setError(error);
            } else {
                reply->setError(Variant::make<exceptions::JoynrRuntimeException>(
                        "invalid Reply: unable to deserialize exception."));
            }

        } else {
            reply->setError(Variant::NULL_VARIANT());
        }

        return reply;
    }

    /**
         * @brief Deserializes a QByteArray in JSON format to a SubscriptionPublication object.
         *
         * The QByteArray must be a valid JSON representation of a SubscriptionPublication message.
         *
         * @param json The JSON representation of the publication message.
         * @return The deserialized SubscriptionPublication object, or NULL in case of
      *deserialization error
         */
    static SubscriptionPublication* deserializeSubscriptionPublication(const QByteArray& json)
    {
        QJson::Parser parser;

        QVariant jsonQVar = parser.parse(json);
        QVariantMap jsonQVarValue = jsonQVar.value<QVariantMap>();

        SubscriptionPublication* publication = (SubscriptionPublication*)
                JsonSerializer::deserializeQObject<SubscriptionPublication>(json);
        if (publication == Q_NULLPTR) {
            return publication;
        }

        if (jsonQVarValue.contains(JSON_FIELD_NAME_EXCEPTION)) {
            QVariantMap nestedMap = jsonQVarValue.value(JSON_FIELD_NAME_EXCEPTION).toMap();

            const Variant error(deserializeJoynrException(nestedMap, json));
            if (!error.isEmpty()) {
                publication->setError(error);
            } else {
                publication->setError(Variant::make<exceptions::JoynrRuntimeException>(
                        "invalid Reply: unable to deserialize exception."));
            }

        } else {
            publication->setError(Variant::NULL_VARIANT());
        }

        return publication;
    }

private:
    // TODO This is a workaround which must be removed after the new serializer is introduced
    static constexpr auto JSON_FIELD_NAME_TYPE = "_typeName";
    static constexpr auto JSON_FIELD_NAME_EXCEPTION = "error";
    static constexpr auto JSON_FIELD_NAME_DETAIL_MESSAGE = "detailMessage";
    static constexpr auto JSON_FIELD_NAME_SUBSCRIPTION_ID = "subscriptionId";
    static constexpr auto JSON_FIELD_NAME_ERROR_ENUM = "error";
    static constexpr auto JSON_FIELD_NAME_ERROR_ENUM_NAME = "name";

    static void serializeDetailMessage(QByteArray& json, const std::string& detailMessage)
    {
        if (detailMessage.length() > 0) {
            json.append((",\"" + std::string(JSON_FIELD_NAME_DETAIL_MESSAGE) + "\":\"" +
                         detailMessage + "\"").c_str());
        }
    }

    // TODO This is a workaround which must be removed after the new serializer is introduced
    static void serializeJoynrException(QByteArray& json, Variant error)
    {
        std::string typeName = error.getTypeName();
        json.chop(1);
        json.append((",\"" + std::string(JSON_FIELD_NAME_EXCEPTION) + "\":{").c_str());
        json.append(("\"" + std::string(JSON_FIELD_NAME_TYPE) + "\":\"" + typeName + "\"").c_str());

        if (typeName == exceptions::PublicationMissedException::TYPE_NAME) {
            const exceptions::PublicationMissedException& publicationMissedException =
                    error.get<exceptions::PublicationMissedException>();
            serializeDetailMessage(json, publicationMissedException.getMessage());
            json.append((",\"" + std::string(JSON_FIELD_NAME_SUBSCRIPTION_ID) + "\":\"" +
                         publicationMissedException.getSubscriptionId() + "\"").c_str());
        } else if (typeName == exceptions::ApplicationException::TYPE_NAME) {
            const exceptions::ApplicationException& applicationException =
                    error.get<exceptions::ApplicationException>();
            serializeDetailMessage(json, applicationException.getMessage());
            std::string errorTypeName = applicationException.getErrorTypeName();
            std::string name = applicationException.getName();
            json.append((",\"" + std::string(JSON_FIELD_NAME_ERROR_ENUM) + "\":{" + "\"" +
                         std::string(JSON_FIELD_NAME_TYPE) + "\":\"" + errorTypeName + "\",\"" +
                         std::string(JSON_FIELD_NAME_ERROR_ENUM_NAME) + "\":\"" + name +
                         "\"}").c_str());
        }
        json.append("}}");
    }

    // TODO This is a workaround which must be removed after the new serializer is introduced
    static Variant deserializeJoynrException(const QVariantMap& errorMap, const QByteArray& json)
    {
        if (!errorMap.contains(JSON_FIELD_NAME_TYPE)) {
            LOG_ERROR(logger,
                      QString("_typeName not specified for exception in serialized: %1")
                              .arg(QString::fromUtf8(json)));
            return Variant::NULL_VARIANT();
        }
        std::string typeName = errorMap.value(JSON_FIELD_NAME_TYPE).value<QString>().toStdString();

        std::string detailMessage("");
        if (errorMap.contains(JSON_FIELD_NAME_DETAIL_MESSAGE)) {
            detailMessage =
                    errorMap.value(JSON_FIELD_NAME_DETAIL_MESSAGE).value<QString>().toStdString();
        }

        if (typeName == exceptions::JoynrRuntimeException::TYPE_NAME) {
            return Variant::make<exceptions::JoynrRuntimeException>(
                    exceptions::JoynrRuntimeException(detailMessage));
        } else if (typeName == exceptions::JoynrTimeOutException::TYPE_NAME) {
            return Variant::make<exceptions::JoynrTimeOutException>(detailMessage);
        } else if (typeName == exceptions::DiscoveryException::TYPE_NAME) {
            return Variant::make<exceptions::DiscoveryException>(detailMessage);
        } else if (typeName == exceptions::MethodInvocationException::TYPE_NAME) {
            return Variant::make<exceptions::MethodInvocationException>(detailMessage);
        } else if (typeName == exceptions::ProviderRuntimeException::TYPE_NAME) {
            return Variant::make<exceptions::ProviderRuntimeException>(detailMessage);
        } else if (typeName == exceptions::PublicationMissedException::TYPE_NAME) {
            std::string subscriptionId;
            if (errorMap.contains(JSON_FIELD_NAME_SUBSCRIPTION_ID)) {
                subscriptionId = errorMap.value(JSON_FIELD_NAME_SUBSCRIPTION_ID)
                                         .value<QString>()
                                         .toStdString();
            } else {
                subscriptionId = detailMessage;
            }
            return Variant::make<exceptions::PublicationMissedException>(subscriptionId);
        } else if (typeName == exceptions::ApplicationException::TYPE_NAME) {
            if (!errorMap.contains(JSON_FIELD_NAME_ERROR_ENUM)) {
                LOG_ERROR(logger,
                          QString("error enum not specified for ApplicationException "
                                  "in serialized: %1").arg(QString::fromUtf8(json)));
                return Variant::NULL_VARIANT();
            }
            QVariantMap errorEnumMap = errorMap.value(JSON_FIELD_NAME_ERROR_ENUM).toMap();

            if (!errorEnumMap.contains(JSON_FIELD_NAME_TYPE)) {
                LOG_ERROR(logger,
                          QString("_typeName not specified for ApplicationException error enum "
                                  "in serialized: %1").arg(QString::fromUtf8(json)));
                return Variant::NULL_VARIANT();
            }
            std::string errorEnumType =
                    errorEnumMap.value(JSON_FIELD_NAME_TYPE).value<QString>().toStdString();
            if (!errorEnumMap.contains(JSON_FIELD_NAME_ERROR_ENUM_NAME)) {
                LOG_ERROR(logger,
                          QString("name not specified for ApplicationException error enum "
                                  "in serialized: %1").arg(QString::fromUtf8(json)));
                return Variant::NULL_VARIANT();
            }
            std::string errorEnumName(errorEnumMap.value(JSON_FIELD_NAME_ERROR_ENUM_NAME)
                                              .value<QString>()
                                              .toStdString());
            if (detailMessage.empty()) {
                detailMessage = errorEnumName;
            }
            // The dummy enumeration value (0) has to be replaced by the actual value corresponding
            // to name in the enumeration class specified by typeName.
            // This has to be done in the generated code of the calling MessagingConnector before
            // returning the exception to the consumer.
            std::shared_ptr<IEnumDeserializer> deserializer =
                    SerializerRegistry::getEnumDeserializer(errorEnumType);
            JsonTokenizer tokenizer(errorEnumName);
            JsonValue value(tokenizer);
            return Variant::make<exceptions::ApplicationException>(
                    detailMessage,
                    deserializer->deserializeVariant(value),
                    errorEnumName,
                    errorEnumType);
        } else {
            LOG_ERROR(logger,
                      QString("unknown _typeName for exception in serialized: %1")
                              .arg(QString::fromUtf8(json)));
            return Variant::NULL_VARIANT();
        }
    }

    static joynr_logging::Logger* logger;
};

} // namespace joynr
#endif // JSONEXCEPTIONSERIALIZER_H
