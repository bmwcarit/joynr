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
#include "joynr/ParticipantIdStorage.h"

#include <algorithm>
#include <cassert>
#include <exception>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/format.hpp>

#include "joynr/Util.h"

namespace joynr
{

ParticipantIdStorage::ParticipantIdStorage(const std::string& filename)
        : fileMutex(), storageMutex(), storage(), entriesWrittenToDisk(0), fileName(filename)
{
    assert(!fileName.empty());
    loadEntriesFromFile();
}

const std::string& ParticipantIdStorage::STORAGE_FORMAT_STRING()
{
    static const std::string value("joynr.participant.%1%.%2%.v%3%");
    return value;
}

void ParticipantIdStorage::loadEntriesFromFile()
{
    if (!joynr::util::fileExists(fileName)) {
        return;
    }

    JOYNR_LOG_TRACE(logger(), "Attempting to load ParticipantIdStorage from: {}", fileName);

    boost::property_tree::ptree pt;
    WriteLocker lockAccessToStorage(storageMutex);
    try {
        boost::property_tree::ini_parser::read_ini(fileName, pt);
        for (boost::property_tree::ptree::const_iterator it = pt.begin(); it != pt.end(); ++it) {
            StorageItem item{it->first, it->second.data()};
            auto retVal = storage.insert(std::move(item));
            assert(retVal.second);
            std::ignore = retVal;
        }
    } catch (const std::exception& ex) {
        JOYNR_LOG_WARN(logger(),
                       "Cannot read participantId storage file {}. Removing file. Exception: {}",
                       fileName,
                       ex.what());
        std::remove(fileName.c_str());
    }

    // set entriesWrittenToDisk to the size of the storage
    entriesWrittenToDisk = storage.get<participantIdStorageTags::write>().size();
    JOYNR_LOG_TRACE(logger(), "Loaded {} entries.", entriesWrittenToDisk);
}

void ParticipantIdStorage::setProviderParticipantId(const std::string& domain,
                                                    const std::string& interfaceName,
                                                    std::int32_t majorVersion,
                                                    const std::string& participantId)
{
    assert(!domain.empty());
    assert(!interfaceName.empty());
    assert(!participantId.empty());

    bool fileNeedsUpdate = false;
    std::string providerKey = createProviderKey(domain, interfaceName, majorVersion);
    StorageItem item{providerKey, participantId};
    {
        WriteLocker lockAccessToStorage(storageMutex);
        auto retVal = storage.insert(std::move(item));
        fileNeedsUpdate = retVal.second;
    }

    if (fileNeedsUpdate) {
        writeStoreToFile();
    }
}

std::string ParticipantIdStorage::getProviderParticipantId(const std::string& domain,
                                                           const std::string& interfaceName,
                                                           std::int32_t majorVersion)
{
    return getProviderParticipantId(domain, interfaceName, majorVersion, "");
}

std::string ParticipantIdStorage::getProviderParticipantId(const std::string& domain,
                                                           const std::string& interfaceName,
                                                           std::int32_t majorVersion,
                                                           const std::string& defaultValue)
{
    assert(!domain.empty());
    assert(!interfaceName.empty());

    const std::string providerKey = createProviderKey(domain, interfaceName, majorVersion);
    MultiIndexContainer::const_iterator value;

    {
        ReadLocker lockAccessToStorage(storageMutex);
        value = storage.template get<participantIdStorageTags::read>().find(providerKey);
    }

    if (value != storage.cend()) {
        return value->participantId;
    } else {
        return (!defaultValue.empty()) ? defaultValue : util::createUuid();
    }
}

void ParticipantIdStorage::writeStoreToFile()
{
    std::lock_guard<std::mutex> lockAccessToFile(fileMutex);
    WriteLocker lockAccessToStorage(storageMutex);

    auto& writeIndex = storage.get<participantIdStorageTags::write>();
    const size_t entries = writeIndex.size();

    // The storage in memory is supposed to contain at least one entry at this point.
    if (entries == 0) {
        assert(entriesWrittenToDisk == 0);
        return;
    }

    if (entries > entriesWrittenToDisk) {
        JOYNR_LOG_TRACE(
                logger(), "Writing {} new entries to file.", entries - entriesWrittenToDisk);

        // write not present entries to File
        size_t writtenToDisk = 0;
        for (size_t i = entriesWrittenToDisk; i < entries; ++i, ++writtenToDisk) {
            auto entry = writeIndex[i];
            try {
                joynr::util::appendStringToFile(fileName, entry.toIniForm());
            } catch (const std::runtime_error& ex) {
                JOYNR_LOG_ERROR(logger(),
                                "Cannot save ParticipantId to file. Next application lifecycle "
                                "might not function correctly. Exception: ",
                                ex.what());
                entriesWrittenToDisk += writtenToDisk;
                return;
            }
        }
        assert(entries == entriesWrittenToDisk + writtenToDisk);
        entriesWrittenToDisk = entries;
        JOYNR_LOG_TRACE(logger(), "Storage on file contains now {} entries.", entriesWrittenToDisk);
    } else if (entries < entriesWrittenToDisk) {
        // This actually means that someone modified the file and inserted other entries.
        // Do nothing.
    } else {
        // In this case the number of entries written to disk matches those in the store.
        // Do nothing.
    }
}

std::string ParticipantIdStorage::createProviderKey(const std::string& domain,
                                                    const std::string& interfaceName,
                                                    std::int32_t majorVersion)
{
    std::string key = (boost::format(STORAGE_FORMAT_STRING()) % domain % interfaceName %
                       std::to_string(majorVersion)).str();
    std::replace(key.begin(), key.end(), '/', '.');
    return key;
}

} // namespace joynr
