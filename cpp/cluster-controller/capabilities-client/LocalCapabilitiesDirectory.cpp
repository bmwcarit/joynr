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

#include "joynr/LocalCapabilitiesDirectory.h"

#include <boost/algorithm/string/predicate.hpp>

#include "cluster-controller/capabilities-client/ICapabilitiesClient.h"
#include "joynr/infrastructure/IGlobalCapabilitiesDirectory.h"
#include "cluster-controller/capabilities-client/ICapabilitiesClient.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "joynr/CapabilityEntry.h"
#include "joynr/DiscoveryQos.h"
#include "joynr/ILocalCapabilitiesCallback.h"
#include "joynr/JsonSerializer.h"
#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/MessageRouter.h"
#include "common/InterfaceAddress.h"

#include "joynr/Util.h"
#include "joynr/JsonSerializer.h"
#include "joynr/CapabilityEntrySerializer.h"

namespace joynr
{

INIT_LOGGER(LocalCapabilitiesDirectory);

LocalCapabilitiesDirectory::LocalCapabilitiesDirectory(
        MessagingSettings& messagingSettings,
        std::shared_ptr<ICapabilitiesClient> capabilitiesClientPtr,
        const std::string& localAddress,
        MessageRouter& messageRouter)
        : joynr::system::DiscoveryAbstractProvider(),
          messagingSettings(messagingSettings),
          capabilitiesClient(capabilitiesClientPtr),
          localAddress(localAddress),
          cacheLock(),
          interfaceAddress2GlobalCapabilities(),
          participantId2GlobalCapabilities(),
          interfaceAddress2LocalCapabilities(),
          participantId2LocalCapability(),
          registeredGlobalCapabilities(),
          messageRouter(messageRouter),
          observers(),
          mqttSettings(),
          localCapabilitiesDirectoryFileName()
{
    // setting up the provisioned values for GlobalCapabilitiesClient
    // The GlobalCapabilitiesServer is also provisioned in MessageRouter
    types::ProviderQos providerQos;
    providerQos.setPriority(1);
    std::int64_t lastSeenDateMs = 0;
    std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
    types::Version providerVersion;
    this->insertInCache(joynr::types::DiscoveryEntry(
                                providerVersion,
                                messagingSettings.getDiscoveryDirectoriesDomain(),
                                infrastructure::IGlobalCapabilitiesDirectory::INTERFACE_NAME(),
                                messagingSettings.getCapabilitiesDirectoryParticipantId(),
                                providerQos,
                                lastSeenDateMs,
                                expiryDateMs),
                        false,
                        true,
                        false);
}

LocalCapabilitiesDirectory::~LocalCapabilitiesDirectory()
{
    cleanCaches();
}

void LocalCapabilitiesDirectory::cleanCaches()
{
    const auto zero = std::chrono::milliseconds::zero();
    interfaceAddress2GlobalCapabilities.cleanup(zero);
    participantId2GlobalCapabilities.cleanup(zero);

    interfaceAddress2LocalCapabilities.cleanup(zero);
    participantId2LocalCapability.cleanup(zero);
}

void LocalCapabilitiesDirectory::add(const joynr::types::DiscoveryEntry& discoveryEntry)
{
    bool isGlobal = discoveryEntry.getQos().getScope() == types::ProviderScope::GLOBAL;

    // register locally
    this->insertInCache(discoveryEntry, isGlobal, true, isGlobal);

    // Inform observers
    informObserversOnAdd(discoveryEntry);

    // register globally
    if (isGlobal) {
        types::GlobalDiscoveryEntry globalDiscoveryEntry(discoveryEntry.getProviderVersion(),
                                                         discoveryEntry.getDomain(),
                                                         discoveryEntry.getInterfaceName(),
                                                         discoveryEntry.getParticipantId(),
                                                         discoveryEntry.getQos(),
                                                         discoveryEntry.getLastSeenDateMs(),
                                                         discoveryEntry.getExpiryDateMs(),
                                                         localAddress);
        if (std::find(registeredGlobalCapabilities.begin(),
                      registeredGlobalCapabilities.end(),
                      globalDiscoveryEntry) == registeredGlobalCapabilities.end()) {
            registeredGlobalCapabilities.push_back(globalDiscoveryEntry);
        }
        this->capabilitiesClient->add(registeredGlobalCapabilities);
    }

    saveToFile();
}

void LocalCapabilitiesDirectory::remove(const std::string& domain,
                                        const std::string& interfaceName,
                                        const types::ProviderQos& qos)
{
    // TODO does it make sense to remove any capability for a domain/interfaceName
    // without knowing which provider registered the capability
    std::lock_guard<std::mutex> lock(cacheLock);
    std::vector<CapabilityEntry> entries =
            interfaceAddress2GlobalCapabilities.lookUpAll(InterfaceAddress(domain, interfaceName));
    std::vector<std::string> participantIdsToRemove;

    types::DiscoveryEntry discoveryEntry;

    for (std::size_t i = 0; i < entries.size(); ++i) {
        CapabilityEntry entry = entries.at(i);
        if (entry.isGlobal()) {
            auto compareFunc =
                    [&entry, &domain, &interfaceName, &qos](const types::GlobalDiscoveryEntry& it) {
                return it.getProviderVersion() == entry.getProviderVersion() &&
                       it.getDomain() == domain && it.getInterfaceName() == interfaceName &&
                       it.getQos() == qos && it.getParticipantId() == entry.getParticipantId();
            };

            while (registeredGlobalCapabilities.erase(
                           std::remove_if(registeredGlobalCapabilities.begin(),
                                          registeredGlobalCapabilities.end(),
                                          compareFunc),
                           registeredGlobalCapabilities.end()) !=
                   registeredGlobalCapabilities.end()) {
            }
            participantIdsToRemove.push_back(entry.getParticipantId());
            participantId2GlobalCapabilities.remove(entry.getParticipantId(), entry);
            interfaceAddress2GlobalCapabilities.remove(
                    InterfaceAddress(entry.getDomain(), entry.getInterfaceName()), entry);
        }
        participantId2LocalCapability.remove(entry.getParticipantId(), entry);
        interfaceAddress2LocalCapabilities.remove(InterfaceAddress(domain, interfaceName), entry);

        convertCapabilityEntryIntoDiscoveryEntry(entry, discoveryEntry);
        informObserversOnRemove(discoveryEntry);
    }
    if (!participantIdsToRemove.empty()) {
        capabilitiesClient->remove(participantIdsToRemove);
    }

    saveToFile();
}

void LocalCapabilitiesDirectory::remove(const std::string& participantId)
{
    std::lock_guard<std::mutex> lock(cacheLock);
    CapabilityEntry entry = participantId2LocalCapability.take(participantId);
    interfaceAddress2LocalCapabilities.remove(
            InterfaceAddress(entry.getDomain(), entry.getInterfaceName()), entry);
    if (entry.isGlobal()) {
        participantId2GlobalCapabilities.remove(participantId, entry);
        interfaceAddress2GlobalCapabilities.remove(
                InterfaceAddress(entry.getDomain(), entry.getInterfaceName()), entry);
        capabilitiesClient->remove(participantId);
    }

    types::DiscoveryEntry discoveryEntry;
    convertCapabilityEntryIntoDiscoveryEntry(entry, discoveryEntry);
    informObserversOnRemove(discoveryEntry);

    saveToFile();
}

bool LocalCapabilitiesDirectory::getLocalAndCachedCapabilities(
        const InterfaceAddress& interfaceAddress,
        const joynr::types::DiscoveryQos& discoveryQos,
        std::shared_ptr<ILocalCapabilitiesCallback> callback)
{
    joynr::types::DiscoveryScope::Enum scope = discoveryQos.getDiscoveryScope();

    std::vector<CapabilityEntry> localCapabilities =
            searchCache(interfaceAddress, std::chrono::milliseconds(-1), true);
    std::vector<CapabilityEntry> globalCapabilities = searchCache(
            interfaceAddress, std::chrono::milliseconds(discoveryQos.getCacheMaxAge()), false);

    return callReceiverIfPossible(scope, localCapabilities, globalCapabilities, callback);
}

bool LocalCapabilitiesDirectory::getLocalAndCachedCapabilities(
        const std::string& participantId,
        const joynr::types::DiscoveryQos& discoveryQos,
        std::shared_ptr<ILocalCapabilitiesCallback> callback)
{
    joynr::types::DiscoveryScope::Enum scope = discoveryQos.getDiscoveryScope();

    std::vector<CapabilityEntry> localCapabilities =
            searchCache(participantId, std::chrono::milliseconds(-1), true);
    std::vector<CapabilityEntry> globalCapabilities = searchCache(
            participantId, std::chrono::milliseconds(discoveryQos.getCacheMaxAge()), false);

    return callReceiverIfPossible(scope, localCapabilities, globalCapabilities, callback);
}

bool LocalCapabilitiesDirectory::callReceiverIfPossible(
        joynr::types::DiscoveryScope::Enum& scope,
        std::vector<CapabilityEntry>& localCapabilities,
        std::vector<CapabilityEntry>& globalCapabilities,
        std::shared_ptr<ILocalCapabilitiesCallback> callback)
{
    // return only local capabilities
    if (scope == joynr::types::DiscoveryScope::LOCAL_ONLY) {
        callback->capabilitiesReceived(localCapabilities);
        return true;
    }

    // return local then global capabilities
    if (scope == joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL) {
        if (!localCapabilities.empty()) {
            callback->capabilitiesReceived(localCapabilities);
            return true;
        }
        if (!globalCapabilities.empty()) {
            callback->capabilitiesReceived(globalCapabilities);
            return true;
        }
    }

    // return local and global capabilities
    if (scope == joynr::types::DiscoveryScope::LOCAL_AND_GLOBAL) {
        // return if global entries
        if (!globalCapabilities.empty()) {
            // remove duplicates
            std::vector<CapabilityEntry> result;
            for (CapabilityEntry entry : localCapabilities) {
                if (std::find(result.begin(), result.end(), entry) == result.end()) {
                    result.push_back(entry);
                }
            }
            for (CapabilityEntry entry : globalCapabilities) {
                if (std::find(result.begin(), result.end(), entry) == result.end()) {
                    result.push_back(entry);
                }
            }
            callback->capabilitiesReceived(result);
            return true;
        }
    }

    // return the global cached entries
    if (scope == joynr::types::DiscoveryScope::GLOBAL_ONLY) {
        if (!globalCapabilities.empty()) {
            callback->capabilitiesReceived(globalCapabilities);
            return true;
        }
    }
    return false;
}

void LocalCapabilitiesDirectory::capabilitiesReceived(
        const std::vector<types::GlobalDiscoveryEntry>& results,
        std::vector<CapabilityEntry> cachedLocalCapabilies,
        std::shared_ptr<ILocalCapabilitiesCallback> callback,
        joynr::types::DiscoveryScope::Enum discoveryScope)
{
    QMap<std::string, CapabilityEntry> capabilitiesMap;
    std::vector<CapabilityEntry> mergedEntries;

    for (types::GlobalDiscoveryEntry globalDiscoveryEntry : results) {
        CapabilityEntry capEntry(globalDiscoveryEntry.getProviderVersion(),
                                 globalDiscoveryEntry.getDomain(),
                                 globalDiscoveryEntry.getInterfaceName(),
                                 globalDiscoveryEntry.getQos(),
                                 globalDiscoveryEntry.getParticipantId(),
                                 true);
        capabilitiesMap.insertMulti(globalDiscoveryEntry.getAddress(), capEntry);
        mergedEntries.push_back(capEntry);
    }
    registerReceivedCapabilities(capabilitiesMap);

    if (discoveryScope == joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL ||
        discoveryScope == joynr::types::DiscoveryScope::LOCAL_AND_GLOBAL) {
        // look if in the meantime there are some local providers registered
        // lookup in the local directory to get local providers which were registered in the
        // meantime.
        for (CapabilityEntry entry : cachedLocalCapabilies) {
            mergedEntries.push_back(entry);
        }
    }
    callback->capabilitiesReceived(mergedEntries);
}

void LocalCapabilitiesDirectory::lookup(const std::string& participantId,
                                        std::shared_ptr<ILocalCapabilitiesCallback> callback)
{
    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL);
    // get the local and cached entries
    bool receiverCalled = getLocalAndCachedCapabilities(participantId, discoveryQos, callback);

    // if no receiver is called, use the global capabilities directory
    if (!receiverCalled) {
        // search for global entires in the global capabilities directory
        auto onSuccess = [this, participantId, callback](
                const std::vector<joynr::types::GlobalDiscoveryEntry>& result) {
            this->capabilitiesReceived(result,
                                       getCachedLocalCapabilities(participantId),
                                       callback,
                                       joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL);
        };
        this->capabilitiesClient->lookup(participantId, onSuccess);
    }
}

void LocalCapabilitiesDirectory::lookup(const std::string& domain,
                                        const std::string& interfaceName,
                                        std::shared_ptr<ILocalCapabilitiesCallback> callback,
                                        const joynr::types::DiscoveryQos& discoveryQos)
{
    InterfaceAddress interfaceAddress(domain, interfaceName);

    // get the local and cached entries
    bool receiverCalled = getLocalAndCachedCapabilities(interfaceAddress, discoveryQos, callback);

    // if no receiver is called, use the global capabilities directory
    if (!receiverCalled) {
        // search for global entires in the global capabilities directory
        auto onSuccess = [this, interfaceAddress, callback, discoveryQos](
                std::vector<joynr::types::GlobalDiscoveryEntry> capabilities) {
            this->capabilitiesReceived(capabilities,
                                       getCachedLocalCapabilities(interfaceAddress),
                                       callback,
                                       discoveryQos.getDiscoveryScope());
        };
        this->capabilitiesClient->lookup(
                domain, interfaceName, discoveryQos.getDiscoveryTimeout(), onSuccess);
    }
}

std::vector<CapabilityEntry> LocalCapabilitiesDirectory::getCachedLocalCapabilities(
        const std::string& participantId)
{
    return searchCache(participantId, std::chrono::milliseconds(-1), true);
}

std::vector<CapabilityEntry> LocalCapabilitiesDirectory::getCachedLocalCapabilities(
        const InterfaceAddress& interfaceAddress)
{
    return searchCache(interfaceAddress, std::chrono::milliseconds(-1), true);
}

void LocalCapabilitiesDirectory::cleanCache(std::chrono::milliseconds maxAge)
{
    std::lock_guard<std::mutex> lock(cacheLock);
    interfaceAddress2GlobalCapabilities.cleanup(maxAge);
    participantId2GlobalCapabilities.cleanup(maxAge);
    interfaceAddress2LocalCapabilities.cleanup(maxAge);
    participantId2LocalCapability.cleanup(maxAge);
}

void LocalCapabilitiesDirectory::registerReceivedCapabilities(
        QMap<std::string, CapabilityEntry> capabilityEntries)
{
    QMapIterator<std::string, CapabilityEntry> entryIterator(capabilityEntries);
    while (entryIterator.hasNext()) {
        entryIterator.next();
        CapabilityEntry currentEntry = entryIterator.value();

        std::string serializedAddress = entryIterator.key();

        // TODO: check joynrAddress for nullptr instead of string.find after the deserialization
        // works as expected.
        // Currently, JsonDeserializer.deserialize<T> always returns an instance of T
        std::size_t foundTypeNameKey = serializedAddress.find("\"_typeName\"");
        std::size_t foundTypeNameValue =
                serializedAddress.find("\"joynr.system.RoutingTypes.MqttAddress\"");
        if (boost::starts_with(serializedAddress, "{") && foundTypeNameKey != std::string::npos &&
            foundTypeNameValue != std::string::npos && foundTypeNameKey < foundTypeNameValue) {
            try {
                using system::RoutingTypes::MqttAddress;
                MqttAddress joynrAddress =
                        JsonSerializer::deserialize<MqttAddress>(serializedAddress);
                auto addressPtr = std::make_shared<MqttAddress>(joynrAddress);
                messageRouter.addNextHop(currentEntry.getParticipantId(), addressPtr);
            } catch (const std::invalid_argument& e) {
                JOYNR_LOG_FATAL(logger,
                                "could not deserialize MqttAddress from {} - error: {}",
                                serializedAddress,
                                e.what());
            }
        } else {
            try {
                using system::RoutingTypes::ChannelAddress;

                ChannelAddress channelAddress =
                        JsonSerializer::deserialize<ChannelAddress>(serializedAddress);
                auto channelAddressPtr = std::make_shared<const ChannelAddress>(channelAddress);

                messageRouter.addNextHop(currentEntry.getParticipantId(), channelAddressPtr);
            } catch (const std::invalid_argument& e) {
                JOYNR_LOG_FATAL(logger,
                                "could not deserialize ChannelAddress from {} - error: {}",
                                serializedAddress,
                                e.what());
            }
        }
        this->insertInCache(currentEntry, false, true);
    }
}

// inherited method from joynr::system::DiscoveryProvider
void LocalCapabilitiesDirectory::add(
        const types::DiscoveryEntry& discoveryEntry,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::ignore = onError;
    add(discoveryEntry);
    onSuccess();
}

// inherited method from joynr::system::DiscoveryProvider
void LocalCapabilitiesDirectory::lookup(
        const std::string& domain,
        const std::string& interfaceName,
        const types::DiscoveryQos& discoveryQos,
        std::function<void(const std::vector<joynr::types::DiscoveryEntry>& result)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::ignore = onError;
    auto future = std::make_shared<LocalCapabilitiesFuture>();
    lookup(domain, interfaceName, future, discoveryQos);
    std::vector<CapabilityEntry> capabilities = future->get();
    std::vector<types::DiscoveryEntry> result;
    convertCapabilityEntriesIntoDiscoveryEntries(capabilities, result);
    onSuccess(result);
}

// inherited method from joynr::system::DiscoveryProvider
void LocalCapabilitiesDirectory::lookup(
        const std::string& participantId,
        std::function<void(const joynr::types::DiscoveryEntry&)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::ignore = onError;
    auto future = std::make_shared<LocalCapabilitiesFuture>();
    lookup(participantId, future);
    std::vector<CapabilityEntry> capabilities = future->get();
    if (capabilities.size() > 1) {
        JOYNR_LOG_ERROR(logger,
                        "participantId {} has more than 1 capability entry:\n {}\n {}",
                        participantId,
                        capabilities[0].toString(),
                        capabilities[1].toString());
    }

    types::DiscoveryEntry result;
    if (!capabilities.empty()) {
        convertCapabilityEntryIntoDiscoveryEntry(capabilities.front(), result);
    }
    onSuccess(result);
}

// inherited method from joynr::system::DiscoveryProvider
void LocalCapabilitiesDirectory::remove(
        const std::string& participantId,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::ignore = onError;
    remove(participantId);
    onSuccess();
}

void LocalCapabilitiesDirectory::addProviderRegistrationObserver(
        std::shared_ptr<LocalCapabilitiesDirectory::IProviderRegistrationObserver> observer)
{
    observers.push_back(observer);
}

void LocalCapabilitiesDirectory::removeProviderRegistrationObserver(
        std::shared_ptr<LocalCapabilitiesDirectory::IProviderRegistrationObserver> observer)
{
    util::removeAll(observers, observer);
}

void LocalCapabilitiesDirectory::saveToFile()
{
    try {
        joynr::util::saveStringToFile(localCapabilitiesDirectoryFileName, serializeToJson());
    } catch (const std::runtime_error& ex) {
        JOYNR_LOG_ERROR(logger, ex.what());
    }
}

std::string LocalCapabilitiesDirectory::serializeToJson() const
{
    const std::vector<std::string>& capEntriesKeys = participantId2LocalCapability.getKeys();

    // put all entries in a vector to be serialized
    std::vector<CapabilityEntry> toBeSerialized;

    // convert each entry with Joynr::serialize
    for (const auto& key : capEntriesKeys) {
        auto capEntriesAtKey = participantId2LocalCapability.lookUpAll(key);
        toBeSerialized.insert(
                toBeSerialized.end(), capEntriesAtKey.cbegin(), capEntriesAtKey.cend());
    }

    std::stringstream outputJson;
    outputJson << "{";
    outputJson << "\"listOfCapabilities\":";
    outputJson << JsonSerializer::serialize<std::vector<CapabilityEntry>>(toBeSerialized);
    outputJson << "}";
    return outputJson.str();
}

void LocalCapabilitiesDirectory::deserializeFromJson(const std::string& jsonString)
{
    if (jsonString.empty()) {
        return;
    }

    std::vector<CapabilityEntry> vectorOfCapEntries =
            JsonSerializer::deserialize<std::vector<CapabilityEntry>>(jsonString);

    if (vectorOfCapEntries.empty()) {
        return;
    }

    cleanCaches();

    // move all capability entries into local cache
    for (const auto& entry : vectorOfCapEntries) {
        insertInCache(entry, true, entry.isGlobal());
    }
}

void LocalCapabilitiesDirectory::loadFromFile(std::string fileName)
{
    // update reference file
    localCapabilitiesDirectoryFileName = std::move(fileName);

    try {
        deserializeFromJson(joynr::util::loadStringFromFile(localCapabilitiesDirectoryFileName));
    } catch (const std::runtime_error& ex) {
        JOYNR_LOG_ERROR(logger, ex.what());
    }
}

/**
 * Private convenience methods.
 */
void LocalCapabilitiesDirectory::insertInCache(const CapabilityEntry& entry,
                                               bool localCache,
                                               bool globalCache)
{
    std::lock_guard<std::mutex> lock(cacheLock);
    InterfaceAddress addr(entry.getDomain(), entry.getInterfaceName());

    // add entry to local cache
    if (localCache) {
        interfaceAddress2LocalCapabilities.insert(addr, entry);
        participantId2LocalCapability.insert(entry.getParticipantId(), entry);
    }

    // add entry to global cache
    if (globalCache) {
        interfaceAddress2GlobalCapabilities.insert(addr, entry);
        participantId2GlobalCapabilities.insert(entry.getParticipantId(), entry);
    }
}

void LocalCapabilitiesDirectory::insertInCache(const joynr::types::DiscoveryEntry& discoveryEntry,
                                               bool isGlobal,
                                               bool localCache,
                                               bool globalCache)
{
    CapabilityEntry entry;
    convertDiscoveryEntryIntoCapabilityEntry(discoveryEntry, entry);
    entry.setGlobal(isGlobal);

    // do not dublicate entries:
    // the combination participantId is unique for [domain, interfaceName, authtoken]
    // check only for local registration: when register in the global cache, a second entry is an
    // update of the age and a refresh
    bool foundMatch = false;
    if (localCache) {
        std::vector<CapabilityEntry> entryList =
                searchCache(discoveryEntry.getParticipantId(), std::chrono::milliseconds(-1), true);
        CapabilityEntry newEntry;
        convertDiscoveryEntryIntoCapabilityEntry(discoveryEntry, newEntry);
        entry.setGlobal(isGlobal);
        for (CapabilityEntry oldEntry : entryList) {
            if (oldEntry == newEntry) {
                foundMatch = true;
                break;
            }
        }
    }

    // after logic about duplicate entries stated in comment above
    // TypedClientMultiCache updates the age as stated in comment above
    bool allowInsertInLocalCache = !foundMatch && localCache;
    insertInCache(entry, allowInsertInLocalCache, globalCache);
}

std::vector<CapabilityEntry> LocalCapabilitiesDirectory::searchCache(
        const InterfaceAddress& interfaceAddress,
        std::chrono::milliseconds maxCacheAge,
        bool localEntries)
{
    std::lock_guard<std::mutex> lock(cacheLock);

    // search in local
    if (localEntries) {
        return interfaceAddress2LocalCapabilities.lookUpAll(interfaceAddress);
    } else {
        return interfaceAddress2GlobalCapabilities.lookUp(interfaceAddress, maxCacheAge);
    }
}

std::vector<CapabilityEntry> LocalCapabilitiesDirectory::searchCache(
        const std::string& participantId,
        std::chrono::milliseconds maxCacheAge,
        bool localEntries)
{
    std::lock_guard<std::mutex> lock(cacheLock);

    // search in local
    if (localEntries) {
        return participantId2LocalCapability.lookUpAll(participantId);
    } else {
        return participantId2GlobalCapabilities.lookUp(participantId, maxCacheAge);
    }
}

void LocalCapabilitiesDirectory::convertCapabilityEntryIntoDiscoveryEntry(
        const CapabilityEntry& capabilityEntry,
        joynr::types::DiscoveryEntry& discoveryEntry)
{
    discoveryEntry.setProviderVersion(capabilityEntry.getProviderVersion());
    discoveryEntry.setDomain(capabilityEntry.getDomain());
    discoveryEntry.setInterfaceName(capabilityEntry.getInterfaceName());
    discoveryEntry.setParticipantId(capabilityEntry.getParticipantId());
    discoveryEntry.setQos(capabilityEntry.getQos());
}

void LocalCapabilitiesDirectory::convertDiscoveryEntryIntoCapabilityEntry(
        const joynr::types::DiscoveryEntry& discoveryEntry,
        CapabilityEntry& capabilityEntry)
{
    capabilityEntry.setProviderVersion(discoveryEntry.getProviderVersion());
    capabilityEntry.setDomain(discoveryEntry.getDomain());
    capabilityEntry.setInterfaceName(discoveryEntry.getInterfaceName());
    capabilityEntry.setParticipantId(discoveryEntry.getParticipantId());
    capabilityEntry.setQos(discoveryEntry.getQos());
}

void LocalCapabilitiesDirectory::convertCapabilityEntriesIntoDiscoveryEntries(
        const std::vector<CapabilityEntry>& capabilityEntries,
        std::vector<types::DiscoveryEntry>& discoveryEntries)
{
    for (const CapabilityEntry& capabilityEntry : capabilityEntries) {
        joynr::types::DiscoveryEntry discoveryEntry;
        convertCapabilityEntryIntoDiscoveryEntry(capabilityEntry, discoveryEntry);
        discoveryEntries.push_back(discoveryEntry);
    }
}

void LocalCapabilitiesDirectory::convertDiscoveryEntriesIntoCapabilityEntries(
        const std::vector<types::DiscoveryEntry>& discoveryEntries,
        std::vector<CapabilityEntry>& capabilityEntries)
{
    for (const types::DiscoveryEntry& discoveryEntry : discoveryEntries) {
        CapabilityEntry capabilityEntry;
        convertDiscoveryEntryIntoCapabilityEntry(discoveryEntry, capabilityEntry);
        capabilityEntries.push_back(capabilityEntry);
    }
}

void LocalCapabilitiesDirectory::informObserversOnAdd(const types::DiscoveryEntry& discoveryEntry)
{
    for (const std::shared_ptr<IProviderRegistrationObserver>& observer : observers) {
        observer->onProviderAdd(discoveryEntry);
    }
}

void LocalCapabilitiesDirectory::informObserversOnRemove(
        const types::DiscoveryEntry& discoveryEntry)
{
    for (const std::shared_ptr<IProviderRegistrationObserver>& observer : observers) {
        observer->onProviderRemove(discoveryEntry);
    }
}

LocalCapabilitiesFuture::LocalCapabilitiesFuture() : futureSemaphore(0), capabilities()
{
}

void LocalCapabilitiesFuture::capabilitiesReceived(const std::vector<CapabilityEntry>& capabilities)
{
    this->capabilities = capabilities;
    futureSemaphore.notify();
}

std::vector<CapabilityEntry> LocalCapabilitiesFuture::get()
{
    futureSemaphore.wait();
    futureSemaphore.notify();
    return capabilities;
}

std::vector<CapabilityEntry> LocalCapabilitiesFuture::get(std::chrono::milliseconds timeout)
{
    if (futureSemaphore.waitFor(timeout)) {
        futureSemaphore.notify();
    }
    return capabilities;
}

} // namespace joynr
