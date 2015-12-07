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
#include "joynr/Util.h"

#include <QtCore/QDebug>
#include <QByteArray>
#include <QUuid>
#include <cstring>

namespace joynr
{

using namespace joynr_logging;

Logger* Util::logger = Logging::getInstance()->getLogger("MSG", "Util");

QList<QByteArray> Util::splitIntoJsonObjects(const QByteArray& jsonStream)
{
    // This code relies assumes jsonStream is a valid JSON string
    QList<QByteArray> jsonObjects;
    int parenthesisCount = 0;
    int currentObjectStart = -1;
    bool isInsideString = false;
    /*A string starts with an unescaped " and ends with an unescaped "
     * } or { within a string must be ignored.
    */
    for (int i = 0; i < jsonStream.size(); i++) {
        if (jsonStream.at(i) == '"' && (i > 0) && jsonStream.at(i - 1) != '\\') {
            // only switch insideString if " is not escaped
            isInsideString = !isInsideString;
        } else if (!isInsideString && jsonStream.at(i) == '{') {
            parenthesisCount++;
        } else if (!isInsideString && jsonStream.at(i) == '}') {
            parenthesisCount--;
        }

        if (parenthesisCount == 1 && currentObjectStart < 0) {
            // found start of object
            currentObjectStart = i;
        }
        if (parenthesisCount == 0 && currentObjectStart >= 0) {
            // found end of object
            jsonObjects += jsonStream.mid(currentObjectStart, i - currentObjectStart + 1);

            currentObjectStart = -1;
        }
    }
    return jsonObjects;
}

QString Util::attributeGetterFromName(const QString& attributeName)
{
    QString result = attributeName;
    result[0] = result[0].toUpper();
    result.prepend("get");
    return result;
}

QString Util::createUuid()
{
    QString baseUuid = QUuid::createUuid().toString();
    return baseUuid.mid(1, baseUuid.length() - 2);
}

void Util::logSerializedMessage(joynr_logging::Logger* logger,
                                const QString& explanation,
                                const QString& message)
{
    if (message.length() > 2048) {
        LOG_DEBUG(logger,
                  QString("%1 %2<**truncated, length %3")
                          .arg(explanation)
                          .arg(message.left(2048))
                          .arg(message.length()));
    } else {
        LOG_DEBUG(logger,
                  QString("%1 %2, length %3").arg(explanation).arg(message).arg(message.length()));
    }
}

QString Util::removeNamespace(const QString& className)
{
    static QString doubleColon = QString::fromLatin1("::");

    int namespaceEnd = className.indexOf(doubleColon);
    return (namespaceEnd == -1) ? className : className.mid(namespaceEnd + 2);
}

void Util::throwJoynrException(const exceptions::JoynrException& error)
{
    std::string typeName = error.getTypeName();
    if (typeName == exceptions::JoynrRuntimeException::TYPE_NAME) {
        throw dynamic_cast<exceptions::JoynrRuntimeException&>(
                const_cast<exceptions::JoynrException&>(error));
    } else if (typeName == exceptions::JoynrTimeOutException::TYPE_NAME) {
        throw dynamic_cast<exceptions::JoynrTimeOutException&>(
                const_cast<exceptions::JoynrException&>(error));
    } else if (typeName == exceptions::DiscoveryException::TYPE_NAME) {
        throw dynamic_cast<exceptions::DiscoveryException&>(
                const_cast<exceptions::JoynrException&>(error));
    } else if (typeName == exceptions::MethodInvocationException::TYPE_NAME) {
        throw dynamic_cast<exceptions::MethodInvocationException&>(
                const_cast<exceptions::JoynrException&>(error));
    } else if (typeName == exceptions::ProviderRuntimeException::TYPE_NAME) {
        throw dynamic_cast<exceptions::ProviderRuntimeException&>(
                const_cast<exceptions::JoynrException&>(error));
    } else if (typeName == exceptions::PublicationMissedException::TYPE_NAME) {
        throw dynamic_cast<exceptions::PublicationMissedException&>(
                const_cast<exceptions::JoynrException&>(error));
    } else if (typeName == exceptions::ApplicationException::TYPE_NAME) {
        throw dynamic_cast<exceptions::ApplicationException&>(
                const_cast<exceptions::JoynrException&>(error));
    } else {
        std::string message = error.getMessage();
        throw exceptions::JoynrRuntimeException("Unknown exception: " + error.getTypeName() + ": " +
                                                message);
    }
}

} // namespace joynr
