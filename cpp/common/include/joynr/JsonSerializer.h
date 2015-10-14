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
#ifndef JSONSERIALIZER_H
#define JSONSERIALIZER_H

#include "qjson/qobjecthelper.h"
#include "qjson/parser.h"
#include "qjson/serializer.h"
#include "joynr/joynrlogging.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/Reply.h"
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
class JsonSerializer
{
public:
    /**
     * @brief Serializes a QObject into JSON format.
     *
     * @param object the object to serialize.
     * @return QByteArray the serialized byte array in JSON format, UTF-8 encoding.
     */
    static QByteArray serialize(const QObject& object)
    {
        QJson::Serializer serializer;
        const QMetaObject* metaobject = object.metaObject();
        return serializer.serialize(QVariant(QMetaType::type(metaobject->className()), &object));
    }

    // TODO This is a workaround which must be removed after the new serializer is introduced
    /**
     * @brief Serializes a Reply object into JSON format.
     *
     * @param reply the reply object to serialize.
     * @return QByteArray the serialized byte array in JSON format, UTF-8 encoding.
     */
    static QByteArray serializeReply(const Reply& reply)
    {
        QByteArray json = serialize(reply);
        std::shared_ptr<exceptions::JoynrException> error = reply.getError();
        if (error) {
            std::string typeName = error->getTypeName();
            std::string detailMessage = error->what();
            json.chop(1);
            json.append((",\"" + std::string(JSON_FIELD_NAME_EXCEPTION) + "\":{").c_str());
            json.append(
                    ("\"" + std::string(JSON_FIELD_NAME_TYPE) + "\":\"" + typeName + "\"").c_str());
            if (detailMessage.length() > 0) {
                json.append((",\"" + std::string(JSON_FIELD_NAME_DETAIL_MESSAGE) + "\":\"" +
                             detailMessage + "\"").c_str());
            }
            if (typeName == exceptions::PublicationMissedException::TYPE_NAME) {
                std::shared_ptr<exceptions::PublicationMissedException> publicationMissedException =
                        std::dynamic_pointer_cast<exceptions::PublicationMissedException>(error);
                std::string subscriptionId = publicationMissedException->getSubscriptionId();
                json.append((",\"" + std::string(JSON_FIELD_NAME_SUBSCRIPTION_ID) + "\":\"" +
                             subscriptionId + "\"").c_str());
            } else if (typeName == exceptions::ApplicationException::TYPE_NAME) {
                std::shared_ptr<exceptions::ApplicationException> applicationException =
                        std::dynamic_pointer_cast<exceptions::ApplicationException>(error);
                std::string errorTypeName = applicationException->getErrorTypeName();
                std::string name = applicationException->getName();
                json.append((",\"" + std::string(JSON_FIELD_NAME_ERROR_ENUM) + "\":{" + "\"" +
                             std::string(JSON_FIELD_NAME_TYPE) + "\":\"" + errorTypeName + "\",\"" +
                             std::string(JSON_FIELD_NAME_ERROR_ENUM_NAME) + "\":\"" + name +
                             "\"}").c_str());
            }
            json.append("}}");
        }
        return json;
    }

    /**
     * @brief Serializes a variant into JSON format.
     *
     * @param variant the variant to serialize.
     * @return QString the serialized string in JSON format.
     */
    static QByteArray serialize(const QVariant& variant)
    {
        QJson::Serializer serializer;
        return serializer.serialize(variant);
    }

    template <class T>
    /**
     * @brief Deserializes a string in JSON list format to a list of the given
     * template type T.
     *
     * Template type T must inherit from QObject. The JSON String must be a
     * valid JSON list representation of T.
     *
     * @param json The JSON representation of template type T.
     * @return The deserialized list
     */
    static QList<T*> deserializeList(const QByteArray& json)
    {

        // Parse the JSON
        QJson::Parser parser;
        QVariant jsonQVar = parser.parse(json);
        QVariantList jsonQVarList = jsonQVar.value<QVariantList>();

        // Populate the list
        QVariant value;
        QVariantMap valueAsMap;
        QList<T*> ret;
        foreach (value, jsonQVarList) {
            valueAsMap = value.value<QVariantMap>();
            T* item = new T;
            QJson::QObjectHelper::qvariant2qobject(valueAsMap, item);
            ret.append(item);
        }
        return ret;
    }

    template <class T>
    /**
     * @brief Deserializes a QByteArray in JSON format to the given template type T.
     *
     * Template type T must inherit from QObject. The QByteArray must be a
     * valid JSON representation of the template type T.
     *
     * @param json The JSON representation of template type T.
     * @return The deserialized object, or NULL in case of deserialization error
     */
    static T* deserialize(const QByteArray& json)
    {
        QJson::Parser parser;

        QVariant jsonQVar = parser.parse(json);
        QVariantMap jsonQVarValue = jsonQVar.value<QVariantMap>();
        if (!jsonQVarValue.contains("_typeName")) {
            LOG_ERROR(logger,
                      QString("_typename not specified in serialized: %1")
                              .arg(QString::fromUtf8(json)));
            return Q_NULLPTR;
        }
        QString typeName = jsonQVarValue.value("_typeName").value<QString>();
        int classId = QJson::QObjectHelper::getClassIdForTransmittedType(typeName);
        if (!QMetaType::isRegistered(classId)) {
            LOG_ERROR(logger, QString("unknown type name: %1").arg(typeName));
            return Q_NULLPTR;
        }
        QObject* object = (QObject*)QMetaType::create(classId);
        QJson::QObjectHelper::qvariant2qobject(jsonQVarValue, object);

        return (T*)object;
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

        Reply* reply = (Reply*)deserialize<Reply>(json);
        if (reply == Q_NULLPTR) {
            return reply;
        }

        if (jsonQVarValue.contains(JSON_FIELD_NAME_EXCEPTION)) {
            QVariantMap nestedMap = jsonQVarValue.value(JSON_FIELD_NAME_EXCEPTION).toMap();

            std::shared_ptr<exceptions::JoynrException> error =
                    deserializeJoynrException(nestedMap, json);
            if (error) {
                reply->setError(error);
            } else {
                reply->setError(std::make_shared<exceptions::JoynrRuntimeException>(
                        "invalid Reply: unable to deserialize exception."));
            }

        } else {
            reply->setError(NULL);
        }

        return reply;
    }

private:
    // TODO This is a workaround which must be removed after the new serializer is introduced
    static constexpr auto JSON_FIELD_NAME_TYPE = "_typeName";
    static constexpr auto JSON_FIELD_NAME_EXCEPTION = "error";
    static constexpr auto JSON_FIELD_NAME_DETAIL_MESSAGE = "detailMessage";
    static constexpr auto JSON_FIELD_NAME_SUBSCRIPTION_ID = "subscriptionId";
    static constexpr auto JSON_FIELD_NAME_ERROR_ENUM = "error";
    static constexpr auto JSON_FIELD_NAME_ERROR_ENUM_NAME = "name";
    // TODO This is a workaround which must be removed after the new serializer is introduced
    static std::shared_ptr<exceptions::JoynrException> deserializeJoynrException(
            const QVariantMap& errorMap,
            const QByteArray& json)
    {
        std::shared_ptr<exceptions::JoynrException> error;

        if (!errorMap.contains(JSON_FIELD_NAME_TYPE)) {
            LOG_ERROR(logger,
                      QString("_typeName not specified for exception in serialized: %1")
                              .arg(QString::fromUtf8(json)));
            return error;
        }
        std::string typeName = errorMap.value(JSON_FIELD_NAME_TYPE).value<QString>().toStdString();

        std::string detailMessage("");
        if (errorMap.contains(JSON_FIELD_NAME_DETAIL_MESSAGE)) {
            detailMessage =
                    errorMap.value(JSON_FIELD_NAME_DETAIL_MESSAGE).value<QString>().toStdString();
        }

        if (typeName == exceptions::JoynrRuntimeException::TYPE_NAME) {
            error.reset(new exceptions::JoynrRuntimeException(detailMessage));
        } else if (typeName == exceptions::JoynrTimeOutException::TYPE_NAME) {
            error.reset(new exceptions::JoynrTimeOutException(detailMessage));
        } else if (typeName == exceptions::DiscoveryException::TYPE_NAME) {
            error.reset(new exceptions::DiscoveryException(detailMessage));
        } else if (typeName == exceptions::MethodInvocationException::TYPE_NAME) {
            error.reset(new exceptions::MethodInvocationException(detailMessage));
        } else if (typeName == exceptions::ProviderRuntimeException::TYPE_NAME) {
            error.reset(new exceptions::ProviderRuntimeException(detailMessage));
        } else if (typeName == exceptions::PublicationMissedException::TYPE_NAME) {
            std::string subscriptionId;
            if (errorMap.contains(JSON_FIELD_NAME_SUBSCRIPTION_ID)) {
                subscriptionId = errorMap.value(JSON_FIELD_NAME_SUBSCRIPTION_ID)
                                         .value<QString>()
                                         .toStdString();
            } else {
                subscriptionId = detailMessage;
            }
            error.reset(new exceptions::PublicationMissedException(subscriptionId));
        } else if (typeName == exceptions::ApplicationException::TYPE_NAME) {
            if (!errorMap.contains(JSON_FIELD_NAME_ERROR_ENUM)) {
                LOG_ERROR(logger,
                          QString("error enum not specified for ApplicationException "
                                  "in serialized: %1").arg(QString::fromUtf8(json)));
                return error;
            }
            QVariantMap errorEnumMap = errorMap.value(JSON_FIELD_NAME_ERROR_ENUM).toMap();

            if (!errorEnumMap.contains(JSON_FIELD_NAME_TYPE)) {
                LOG_ERROR(logger,
                          QString("_typeName not specified for ApplicationException error enum "
                                  "in serialized: %1").arg(QString::fromUtf8(json)));
                return error;
            }
            std::string errorEnumType =
                    errorEnumMap.value(JSON_FIELD_NAME_TYPE).value<QString>().toStdString();
            if (!errorEnumMap.contains(JSON_FIELD_NAME_ERROR_ENUM_NAME)) {
                LOG_ERROR(logger,
                          QString("name not specified for ApplicationException error enum "
                                  "in serialized: %1").arg(QString::fromUtf8(json)));
                return error;
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
            error.reset(new exceptions::ApplicationException(
                    detailMessage, 0, errorEnumName, errorEnumType));
        } else {
            LOG_ERROR(logger,
                      QString("unknown _typeName for exception in serialized: %1")
                              .arg(QString::fromUtf8(json)));
        }
        return error;
    }

    static joynr_logging::Logger* logger;
};

} // namespace joynr
#endif // JSONSERIALIZER_H
