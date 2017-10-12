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

#include <string>

#include "joynr/JoynrExport.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/Settings.h"

namespace joynr
{

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
     * @param participantId the participantId to set.
     */
    virtual void setProviderParticipantId(const std::string& domain,
                                          const std::string& interfaceName,
                                          const std::string& participantId);

    /**
     * Get a provider participant id
     */
    virtual std::string getProviderParticipantId(const std::string& domain,
                                                 const std::string& interfaceName);

    /**
     * Get a provider participant id or use a default
     */
    virtual std::string getProviderParticipantId(const std::string& domain,
                                                 const std::string& interfaceName,
                                                 const std::string& defaultValue);

    /**
     * Sync/Persist changes in the storage to file.
     */
    void sync();

private:
    DISALLOW_COPY_AND_ASSIGN(ParticipantIdStorage);
    std::string createProviderKey(const std::string& domain, const std::string& interfaceName);

    joynr::Settings storage;
};

} // namespace joynr
#endif // PARTICIPANTIDSTORAGE_H
