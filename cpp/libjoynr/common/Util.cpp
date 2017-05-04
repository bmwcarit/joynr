/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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

#include <cctype>
#include <fstream>
#include <iterator>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "joynr/Logger.h"

namespace joynr
{

namespace util
{

bool fileExists(const std::string& fileName)
{
    std::ifstream fileToTest(fileName);
    return fileToTest.good();
}

void saveStringToFile(const std::string& fileName, const std::string& strToSave)
{
    std::fstream file;
    file.open(fileName, std::ios::out);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file " + fileName + " for writing: " +
                                 std::strerror(errno));
    }

    // save input string to file
    file << strToSave;
}

std::string loadStringFromFile(const std::string& fileName)
{
    // read from file
    std::ifstream inStream(fileName.c_str(), std::ios::in | std::ios::binary);
    if (!inStream.is_open()) {
        throw std::runtime_error("Could not open file " + fileName + " for reading: " +
                                 std::strerror(errno));
    }

    std::string fileContents;
    inStream.seekg(0, std::ios::end);
    fileContents.resize(inStream.tellg());
    inStream.seekg(0, std::ios::beg);
    inStream.read(&fileContents[0], fileContents.size());
    inStream.close();
    return fileContents;
}

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
    std::lock_guard<std::mutex> uuidLock(uuidMutex);
    return boost::uuids::to_string(uuidGenerator());
}

std::string createMulticastId(const std::string& providerParticipantId,
                              const std::string& multicastName,
                              const std::vector<std::string>& partitions)
{
    std::stringstream multicastId;
    multicastId << providerParticipantId << MULTICAST_PARTITION_SEPARATOR << multicastName;
    for (const auto& partition : partitions) {
        multicastId << MULTICAST_PARTITION_SEPARATOR << partition;
    }
    return multicastId.str();
}

std::string extractParticipantIdFromMulticastId(const std::string& multicastId)
{
    auto separatorIt = multicastId.find(MULTICAST_PARTITION_SEPARATOR);

    if (separatorIt == std::string::npos) {
        throw std::invalid_argument("Cannot extract provider participant Id from multicast Id: " +
                                    multicastId);
    }

    return multicastId.substr(0, separatorIt);
}

void validatePartitions(const std::vector<std::string>& partitions, bool allowWildcards)
{
    static const std::regex patternRegex("^[a-zA-Z0-9]+$");
    const std::vector<std::string>::const_iterator lastPartition = --partitions.cend();
    for (auto partition = partitions.cbegin(); partition != partitions.cend(); ++partition) {
        if (!partition->empty()) {
            if (allowWildcards) {
                if (*partition == SINGLE_LEVEL_WILDCARD) {
                    continue;
                } else if (*partition == MULTI_LEVEL_WILDCARD) {
                    if (partition == lastPartition) {
                        return;
                    }
                }
            }
            if (std::regex_search(*partition, patternRegex)) {
                continue;
            }
        }
        throw std::invalid_argument("Partition " + *partition +
                                    " contains invalid characters.\nMust only contain a-z A-Z 0-9, "
                                    "or be a single level wildcard (" +
                                    SINGLE_LEVEL_WILDCARD +
                                    "),\nor the last partition may be a multi level wildcard (" +
                                    MULTI_LEVEL_WILDCARD + ").");
    }
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

std::string toDateString(const std::chrono::system_clock::time_point& timePoint)
{
    std::time_t time = std::chrono::system_clock::to_time_t(timePoint);
    return std::ctime(&time);
}

std::uint64_t toMilliseconds(const std::chrono::system_clock::time_point& timePoint)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(timePoint.time_since_epoch())
            .count();
}

} // namespace util

} // namespace joynr
