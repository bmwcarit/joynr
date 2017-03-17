/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

#include <boost/format.hpp>

#include "joynr/Util.h"
#include "joynr/Settings.h"

namespace joynr
{

ParticipantIdStorage::ParticipantIdStorage(const std::string& filename) : filename(filename)
{
}

const std::string& ParticipantIdStorage::STORAGE_FORMAT_STRING()
{
    static const std::string value("joynr.participant.%1%.%2%");
    return value;
}

void ParticipantIdStorage::setProviderParticipantId(const std::string& domain,
                                                    const std::string& interfaceName,
                                                    const std::string& participantId)
{
    // Access the persistence file through a threadsafe Settings object
    Settings settings(filename);

    std::string providerKey = createProviderKey(domain, interfaceName);
    settings.set(providerKey, participantId);
    settings.sync();
}

std::string ParticipantIdStorage::getProviderParticipantId(const std::string& domain,
                                                           const std::string& interfaceName)
{
    return getProviderParticipantId(domain, interfaceName, "");
}

std::string ParticipantIdStorage::getProviderParticipantId(const std::string& domain,
                                                           const std::string& interfaceName,
                                                           const std::string& defaultValue)
{
    // Access the persistence file through a threadsafe Settings object
    Settings settings(filename);

    std::string providerKey = createProviderKey(domain, interfaceName);

    std::string participantId;
    // Lookup the participant id
    if (!settings.contains(providerKey)) {
        // Persist a new participant Id, using the defaultValue if possible
        participantId = (!defaultValue.empty()) ? defaultValue : util::createUuid();
        settings.set(providerKey, participantId);
        settings.sync();
    } else {
        participantId = settings.get<std::string>(providerKey);
    }
    return participantId;
}

std::string ParticipantIdStorage::createProviderKey(const std::string& domain,
                                                    const std::string& interfaceName)
{
    std::string key = (boost::format(STORAGE_FORMAT_STRING()) % domain % interfaceName).str();
    std::replace(key.begin(), key.end(), '/', '.');
    return key;
}

} // namespace joynr
