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
#include "joynr/JoynrRuntime.h"

#include "joynr/SingleThreadedIOService.h"
#include "joynr/Util.h"
#include "joynr/system/IRouting.h"
#include "joynr/types/DiscoveryEntryWithMetaInfo.h"

namespace joynr
{

JoynrRuntime::JoynrRuntime(Settings& settings)
        : singleThreadIOService(std::make_unique<SingleThreadedIOService>()),
          proxyFactory(nullptr),
          requestCallerDirectory(nullptr),
          participantIdStorage(nullptr),
          capabilitiesRegistrar(nullptr),
          messagingSettings(settings),
          systemServicesSettings(settings),
          dispatcherAddress(nullptr),
          discoveryProxy(nullptr),
          publicationManager(nullptr)
{
    messagingSettings.printSettings();
    systemServicesSettings.printSettings();
}

JoynrRuntime::~JoynrRuntime()
{
}

bool JoynrRuntime::checkAndLogCryptoFileExistence(const std::string& caPemFile,
                                                  const std::string& certPemFile,
                                                  const std::string& privateKeyPemFile,
                                                  Logger& logger)
{
    if (!util::fileExists(caPemFile)) {
        JOYNR_LOG_ERROR(logger, "CA PEM file {} does not exist", caPemFile);
        return false;
    }

    if (!util::fileExists(certPemFile)) {
        JOYNR_LOG_ERROR(logger, "Cert PEM file {} does not exist", certPemFile);
        return false;
    }

    if (!util::fileExists(privateKeyPemFile)) {
        JOYNR_LOG_ERROR(logger, "Private key file {} does not exist", privateKeyPemFile);
        return false;
    }

    return true;
}

std::map<std::string, joynr::types::DiscoveryEntryWithMetaInfo> JoynrRuntime::
        getProvisionedEntries() const
{
    std::int64_t lastSeenDateMs = 0;
    std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
    std::string defaultPublicKeyId("");
    std::map<std::string, joynr::types::DiscoveryEntryWithMetaInfo> provisionedDiscoveryEntries;

    joynr::types::Version routingProviderVersion(
            joynr::system::IRouting::MAJOR_VERSION, joynr::system::IRouting::MINOR_VERSION);
    joynr::types::Version discoveryProviderVersion(
            joynr::system::IDiscovery::MAJOR_VERSION, joynr::system::IDiscovery::MINOR_VERSION);
    joynr::types::DiscoveryEntryWithMetaInfo routingProviderDiscoveryEntry(
            routingProviderVersion,
            systemServicesSettings.getDomain(),
            joynr::system::IRouting::INTERFACE_NAME(),
            systemServicesSettings.getCcRoutingProviderParticipantId(),
            joynr::types::ProviderQos(),
            lastSeenDateMs,
            expiryDateMs,
            defaultPublicKeyId,
            true);
    provisionedDiscoveryEntries.insert(std::make_pair(
            routingProviderDiscoveryEntry.getParticipantId(), routingProviderDiscoveryEntry));
    joynr::types::DiscoveryEntryWithMetaInfo discoveryProviderDiscoveryEntry(
            discoveryProviderVersion,
            systemServicesSettings.getDomain(),
            joynr::system::IDiscovery::INTERFACE_NAME(),
            systemServicesSettings.getCcDiscoveryProviderParticipantId(),
            joynr::types::ProviderQos(),
            lastSeenDateMs,
            expiryDateMs,
            defaultPublicKeyId,
            true);
    provisionedDiscoveryEntries.insert(std::make_pair(
            discoveryProviderDiscoveryEntry.getParticipantId(), discoveryProviderDiscoveryEntry));

    return provisionedDiscoveryEntries;
}

} // namespace joynr
