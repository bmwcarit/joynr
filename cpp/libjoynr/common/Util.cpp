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
#include <cstring>
#include <fstream>
#include <iterator>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>

#include <boost/filesystem.hpp>
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

void writeToFile(const std::string& fileName,
                 const std::string& strToSave,
                 std::ios_base::openmode mode)
{
    std::ofstream file;
    file.open(fileName, mode);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file " + fileName + " for writing: " +
                                 std::strerror(errno));
    }

    // append input string to file
    file << strToSave;
}

void saveStringToFile(const std::string& fileName, const std::string& strToSave)
{
    writeToFile(fileName, strToSave, std::ios::out);
}

void appendStringToFile(const std::string& fileName, const std::string& strToSave)
{
    writeToFile(fileName, strToSave, std::ios::app);
}

std::string loadStringFromFile(const std::string& fileName)
{
    // read from file
    std::ifstream inStream(fileName.c_str(), std::ios::in | std::ios::binary);
    if (!inStream.is_open()) {
        throw std::runtime_error("Could not open file " + fileName + " for reading: " +
                                 std::strerror(errno));
    }

    // validate the file size
    constexpr std::uint64_t MAX_FILE_SIZE = 2 * 1024 * 1024 * 1024UL;
    std::uint64_t fileSize = boost::filesystem::file_size(fileName);
    if (fileSize > MAX_FILE_SIZE) {
        throw std::runtime_error("File " + fileName + " is too big: " + std::strerror(errno));
    }

    std::string fileContents;
    inStream.seekg(0, std::ios::end);
    fileContents.resize(inStream.tellg());
    inStream.seekg(0, std::ios::beg);
    inStream.read(&fileContents[0], fileContents.size());
    inStream.close();
    return fileContents;
}

std::string attributeGetterFromName(const std::string& attributeName)
{
    assert(!attributeName.empty());
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

std::string truncateSerializedMessage(const std::string& message)
{
    constexpr std::size_t maxSize = 2048;
    const std::size_t messageSize = message.size();
    if (messageSize > maxSize) {
        return message.substr(0, maxSize) + std::string("<**truncated, length=") +
               std::to_string(messageSize);
    }
    return message;
}

bool isAdditionOnPointerSafe(std::uintptr_t address, int payloadLength)
{
    return address + payloadLength < address;
}

std::string getErrorString(int errorNumber)
{
    // MT-safe retrieval of string describing errorNumber
    char buf[256];
    char* resultBuf = buf;
    int storedErrno = errorNumber;
#if defined(__GLIBC__) && defined(_GNU_SOURCE)
    // POSIX compliant check for conversion errors,
    // see 'man strerror_r'
    errno = 0;
    resultBuf = strerror_r(storedErrno, buf, sizeof(buf));
    if (errno) {
        return "failed to convert errno";
    }
#else
    int rc = strerror_r(storedErrno, buf, sizeof(buf));
    if (rc) {
        return "failed to convert errno";
    }
#endif
    return std::string(resultBuf);
}

} // namespace util

} // namespace joynr
