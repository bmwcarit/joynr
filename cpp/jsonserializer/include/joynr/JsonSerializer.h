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
#include <QByteArray>
#include <QObject>
#include <vector>
#include <sstream>
#include "joynr/Variant.h"
#include "joynr/SerializerRegistry.h"
#include "jsonserializer/JsonTokenizer.h"
#include <QList>
#include <QVariant>

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
     * @param variant the object to serialize.
     * @return std::string the serialized variant in JSON format, UTF-8 encoding.
     */
    static std::string serialize(const Variant& variant)
    {
        std::stringstream stream;
        auto serializer = SerializerRegistry::getSerializer(variant.getTypeName());
        serializer->serializeVariant(variant, stream);
        return stream.str();
    }

    static QByteArray serializeQObject(const QObject& object)
    {
        QJson::Serializer serializer;
        const QMetaObject* metaobject = object.metaObject();
        return serializer.serialize(QVariant(QMetaType::type(metaobject->className()), &object));
    }

    /**
     * @brief Serializes a object into JSON format.
     *
     * @param object to serialize.
     * @return std::string the string in JSON format.
     */
    static QByteArray serializeQObject(const QVariant& variant)
    {
        QJson::Serializer serializer;
        return serializer.serialize(variant);
    }

    /**
     * @brief Serializes a object into JSON format.
     *
     * @param object to serialize.
     * @return std::string the string in JSON format.
     */
    template <typename T>
    static std::string serialize(const T& object)
    {
        std::stringstream stream;
        auto serializer = ClassSerializer<T>{};
        serializer.serialize(object, stream);
        return stream.str();
    }

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
    template <class T>
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
    static T* deserializeQObject(const QByteArray& json)
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
    static T* deserialize(const std::string& json)
    {
        JsonTokenizer tokenizer(json);

        T* object = new T();
        if (tokenizer.hasNextObject()) {
            ClassDeserializer<T>::deserialize(*object, tokenizer.nextObject());
        }

        return object;
    }

private:
    static joynr_logging::Logger* logger;
};

} // namespace joynr
#endif // JSONSERIALIZER_H
