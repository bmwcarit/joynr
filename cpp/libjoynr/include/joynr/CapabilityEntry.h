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
#ifndef CAPABILITYENTRY_H
#define CAPABILITYENTRY_H

/*
 * This is an internal datatype for the storage of capability details
 * for the capability directory.  This class contains attributes that
 * are necessary for determining whether to use the cache or not.
 */

#include <string>
#include <vector>

#include "joynr/JoynrExport.h"
#include "joynr/types/ProviderQos.h"
#include "joynr/types/Version.h"
#include "joynr/serializer/Serializer.h"

namespace joynr
{

class JOYNR_EXPORT CapabilityEntry
{
public:
    CapabilityEntry();

    CapabilityEntry(const CapabilityEntry& other) = default;

    CapabilityEntry(joynr::types::Version providerVersion,
                    const std::string& domain,
                    const std::string& interfaceName,
                    joynr::types::ProviderQos qos,
                    const std::string& participantId,
                    const std::string& publicKeyId,
                    bool isGlobal);

    CapabilityEntry& operator=(const CapabilityEntry& other);

    bool operator==(const CapabilityEntry& other) const;

    std::string getInterfaceName() const;
    std::string getDomain() const;
    void setInterfaceName(std::string interfaceName);
    void setDomain(std::string domain);

    types::ProviderQos getQos() const;
    void setQos(joynr::types::ProviderQos qos);

    types::Version getProviderVersion() const;
    void setProviderVersion(joynr::types::Version providerVersion);

    std::string getParticipantId() const;
    void setParticipantId(std::string participantId);

    const std::string& getPublicKeyId() const;
    void setPublicKeyId(const std::string& publicKeyId);

    bool isGlobal() const;
    void setGlobal(bool global);

    std::string toString() const;

    template <typename Archive>
    void serialize(Archive& archive)
    {
        archive(MUESLI_NVP(providerVersion),
                MUESLI_NVP(domain),
                MUESLI_NVP(interfaceName),
                MUESLI_NVP(qos),
                MUESLI_NVP(participantId),
                MUESLI_NVP(publicKeyId),
                MUESLI_NVP(global));
    }

private:
    types::Version providerVersion;
    std::string domain;
    std::string interfaceName;
    types::ProviderQos qos;
    std::string participantId;
    std::string publicKeyId;
    bool global;
};

} // namespace joynr

#endif // CAPABILITYENTRY_H
