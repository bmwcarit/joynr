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
#ifndef PARTICIPANTIDSTORAGE_H
#define PARTICIPANTIDSTORAGE_H

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/indexed_by.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/tag.hpp>

#include "joynr/JoynrExport.h"
#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/ReadWriteLock.h"

namespace joynr
{

namespace participantIdStorageTags
{
struct write;
struct read;
}

/**
 * Creates and persists participant ids.
 *
 * This class is thread safe.
 */
class JOYNR_EXPORT ParticipantIdStorage
{
public:
    /**
     * Persist participant ids using the given file
     */
    explicit ParticipantIdStorage(const std::string& filename);
    virtual ~ParticipantIdStorage() = default;

    static const std::string& STORAGE_FORMAT_STRING();

    /**
     * @brief setProviderParticipantId Sets a participant ID for a specific
     * provider. This is useful for provisioning of provider participant IDs.
     * @param domain the domain of the provider.
     * @param interfaceName the interface name of the provider.
     * @param majorVersion the major version pf the providerr
     * @param participantId the participantId to set.
     */
    virtual void setProviderParticipantId(const std::string& domain,
                                          const std::string& interfaceName,
                                          int32_t majorVersion,
                                          const std::string& participantId);

    /**
     * Get a provider participant id
     */
    virtual std::string getProviderParticipantId(const std::string& domain,
                                                 const std::string& interfaceName,
                                                 std::int32_t majorVersion);

    /**
     * Get a provider participant id or use a default
     */
    virtual std::string getProviderParticipantId(const std::string& domain,
                                                 const std::string& interfaceName,
                                                 std::int32_t majorVersion,
                                                 const std::string& defaultValue);

private:
    struct StorageItem
    {
        const std::string key;
        const std::string participantId;

        std::string toIniForm() const
        {
            std::stringstream iniForm;
            iniForm << key << "=" << participantId << std::endl;
            return iniForm.str();
        }
    };

    // Use one view for writing and one for reading:
    //  - reading is ordered alphabetically and hence optimized for lookups
    //  - writing is ordered sequentially so that we only write the diff to disk
    using MultiIndexContainer = boost::multi_index_container<
            StorageItem,
            boost::multi_index::indexed_by<
                    boost::multi_index::ordered_unique<
                            boost::multi_index::tag<participantIdStorageTags::read>,
                            BOOST_MULTI_INDEX_MEMBER(StorageItem, const std::string, key)>,
                    boost::multi_index::random_access<
                            boost::multi_index::tag<participantIdStorageTags::write>>>>;

    DISALLOW_COPY_AND_ASSIGN(ParticipantIdStorage);
    ADD_LOGGER(ParticipantIdStorage)

    std::string createProviderKey(const std::string& domain,
                                  const std::string& interfaceName,
                                  std::int32_t majorVersion);
    void loadEntriesFromFile();
    void writeStoreToFile();

    std::mutex fileMutex;
    ReadWriteLock storageMutex;

    MultiIndexContainer storage;
    size_t entriesWrittenToDisk;
    std::string fileName;
};

} // namespace joynr
#endif // PARTICIPANTIDSTORAGE_H
