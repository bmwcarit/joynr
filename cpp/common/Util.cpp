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

#include <regex>
#include <cctype>
#include <iterator>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "joynr/Logger.h"
#include "joynr/exceptions/JoynrException.h"

namespace joynr
{

namespace util
{

std::vector<std::string> splitIntoJsonObjects(const std::string& jsonStream)
{
    // This code relies assumes jsonStream is a valid JSON string
    std::vector<std::string> jsonObjects;
    int parenthesisCount = 0;
    int currentObjectStart = -1;
    bool isInsideString = false;
    /*A string starts with an unescaped " and ends with an unescaped "
     * } or { within a string must be ignored.
    */
    for (std::size_t i = 0; i < jsonStream.size(); i++) {
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
            jsonObjects.push_back(
                    jsonStream.substr(currentObjectStart, i - currentObjectStart + 1));

            currentObjectStart = -1;
        }
    }
    return jsonObjects;
}

std::string attributeGetterFromName(const std::string& attributeName)
{
    std::string result = attributeName;
    result[0] = std::toupper(result[0]);
    result.insert(0, "get");
    return result;
}

std::string createUuid()
{
    // instantiation of random generator is expensive,
    // therefore we use a static one:
    static boost::uuids::random_generator uuidGenerator;
    // uuid generator is not threadsafe
    static std::mutex uuidMutex;
    std::unique_lock<std::mutex> uuidLock(uuidMutex);
    return boost::uuids::to_string(uuidGenerator());
}

void throwJoynrException(const exceptions::JoynrException& error)
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
    } else if (typeName == exceptions::JoynrMessageNotSentException::TYPE_NAME) {
        throw dynamic_cast<exceptions::JoynrMessageNotSentException&>(
                const_cast<exceptions::JoynrException&>(error));
    } else if (typeName == exceptions::JoynrDelayMessageException::TYPE_NAME) {
        throw dynamic_cast<exceptions::JoynrDelayMessageException&>(
                const_cast<exceptions::JoynrException&>(error));
    } else {
        std::string message = error.getMessage();
        throw exceptions::JoynrRuntimeException("Unknown exception: " + error.getTypeName() + ": " +
                                                message);
    }
}

std::string removeEscapeFromSpecialChars(const std::string& inputStr)
{
    std::string unEscapedString;
    std::regex expr(R"((\\)(\\|"))");
    std::regex_replace(std::back_inserter(unEscapedString),
                       inputStr.begin(),
                       inputStr.end(),
                       expr,
                       std::string(R"($2)"));

    return unEscapedString;
}

void logSerializedMessage(Logger& logger,
                          const std::string& explanation,
                          const std::string& message)
{
    if (message.size() > 2048) {
        JOYNR_LOG_DEBUG(logger,
                        "{} {}<**truncated, length {}",
                        explanation,
                        message.substr(0, 2048),
                        message.length());
    } else {
        JOYNR_LOG_DEBUG(logger, "{} {}, length {}", explanation, message, message.length());
    }
}

} // namespace util

} // namespace joynr
