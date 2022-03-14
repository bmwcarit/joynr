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

#include <cassert>
#include <cctype>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <mutex>
#include <regex>
#include <stdexcept>

#include <boost/filesystem.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/ostream_iterator.hpp>

#pragma GCC diagnostic ignored "-Wsign-conversion"

namespace joynr
{

namespace util
{

class FileBufWithFileno : public std::filebuf
{
public:
    int fileno()
    {
        return _M_file.fd();
    }
};

bool fileExists(const std::string& fileName)
{
    std::ifstream fileToTest(fileName);
    return fileToTest.good();
}

void writeToFile(const std::string& fileName,
                 const std::string& strToSave,
                 std::ios_base::openmode mode,
                 bool syncFile)
{
    std::ofstream file;
    file.open(fileName, mode);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file " + fileName + " for writing: " +
                                 std::strerror(errno));
    }

    // append input string to file
    file << strToSave;
    if (syncFile) {
        file.flush();
        int fd = static_cast<FileBufWithFileno*>(file.rdbuf())->fileno();
        if (fsync(fd) == -1) {
            throw std::runtime_error("Could not fsync file " + fileName + ": " +
                                     std::strerror(errno));
        }
    }
}

void saveStringToFile(const std::string& fileName, const std::string& strToSave, bool syncFile)
{
    writeToFile(fileName, strToSave, std::ios::out, syncFile);
}

void appendStringToFile(const std::string& fileName, const std::string& strToSave, bool syncFile)
{
    writeToFile(fileName, strToSave, std::ios::app, syncFile);
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
    std::size_t length = 0;
    if (inStream.tellg() > 0) {
        length = static_cast<std::size_t>(inStream.tellg());
    }
    fileContents.resize(length);
    inStream.seekg(0, std::ios::beg);
    inStream.read(&fileContents[0], static_cast<std::streamsize>(fileContents.size()));
    inStream.close();
    return fileContents;
}

std::string attributeGetterFromName(const std::string& attributeName)
{
    assert(!attributeName.empty());
    std::string result = attributeName;
    result[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(result[0])));
    result.insert(0, "get");
    return result;
}

template <class CharType>
struct base64url_from_6_bit
{
    typedef CharType result_type;
    CharType operator()(CharType t) const
    {
        static const char* lookup_table = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                          "abcdefghijklmnopqrstuvwxyz"
                                          "0123456789"
                                          "-_";
        return lookup_table[static_cast<size_t>(t)];
    }
};

template <class Base, class CharType = typename boost::iterator_value<Base>::type>
class base64url_from_binary
        : public boost::transform_iterator<joynr::util::base64url_from_6_bit<CharType>, Base>
{
    friend class boost::iterator_core_access;
    typedef boost::transform_iterator<typename joynr::util::base64url_from_6_bit<CharType>, Base>
            super_t;

public:
    // make composible buy using templated constructor
    template <class T>
    base64url_from_binary(T start)
            : super_t(Base(static_cast<T>(start)), base64url_from_6_bit<CharType>())
    {
    }
};

std::string createUuid()
{
    // instantiation of random generator is expensive,
    // therefore we use a static one:
    static boost::uuids::random_generator uuidGenerator;
    // uuid generator is not threadsafe
    static std::mutex uuidMutex;
    std::unique_lock<std::mutex> uuidLock(uuidMutex);
    auto uuid = uuidGenerator();
    uuidLock.unlock();

    typedef base64url_from_binary<                      // convert binary values to base64
                                                        // characters
            boost::archive::iterators::transform_width< // retrieve 6 bit integers from a sequence
                                                        // of 8 bit bytes
                    const char*,
                    6,
                    8>> base64_text; // compose all the above operations in to a new iterator

    std::stringstream result;
    std::copy(base64_text(uuid.begin()),
              base64_text(uuid.end()),
              boost::archive::iterators::ostream_iterator<char>(result));
    return result.str();
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

bool isAdditionOnPointerCausesOverflow(std::uintptr_t address, int payloadLength)
{
    if (payloadLength <= 0) {
        return false;
    }
    return address + static_cast<std::size_t>(payloadLength) < address;
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
