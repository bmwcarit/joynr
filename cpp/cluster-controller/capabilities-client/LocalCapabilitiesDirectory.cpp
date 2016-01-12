/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
#include "joynr/infrastructure/IGlobalCapabilitiesDirectory.h"
#include "joynr/infrastructure/IChannelUrlDirectory.h"
#include "cluster-controller/capabilities-client/ICapabilitiesClient.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "joynr/CapabilityEntry.h"
#include "joynr/ILocalCapabilitiesCallback.h"
#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/MessageRouter.h"
#include "common/InterfaceAddress.h"

namespace joynr
{

INIT_LOGGER(LocalCapabilitiesDirectory);

LocalCapabilitiesDirectory::LocalCapabilitiesDirectory(MessagingSettings& messagingSettings,
                                                       ICapabilitiesClient* capabilitiesClientPtr,
                                                       MessageRouter& messageRouter)
        : joynr::system::DiscoveryAbstractProvider(),
          messagingSettings(messagingSettings),
          capabilitiesClient(capabilitiesClientPtr),
          cacheLock(),
          interfaceAddress2GlobalCapabilities(),
          participantId2GlobalCapabilities(),
          interfaceAddress2LocalCapabilities(),
          participantId2LocalCapability(),
          registeredGlobalCapabilities(),
          messageRouter(messageRouter),
          observers()
{
    providerQos.setCustomParameters(std::vector<joynr::types::CustomParameter>());
    providerQos.setProviderVersion(1);
    providerQos.setPriority(1);
    providerQos.setScope(joynr::types::ProviderScope::LOCAL);
    providerQos.setSupportsOnChangeSubscriptions(false);
    // setting up the provisioned values for GlobalCapabilitiesClient
    // The GlobalCapabilitiesServer is also provisioned in MessageRouter
    std::vector<joynr::types::CommunicationMiddleware::Enum> middlewareConnections = {
            joynr::types::CommunicationMiddleware::JOYNR};
    types::ProviderQos providerQos;
    providerQos.setPriority(1);
    this->insertInCache(joynr::types::DiscoveryEntry(
                                messagingSettings.getDiscoveryDirectoriesDomain(),
                                infrastructure::IGlobalCapabilitiesDirectory::INTERFACE_NAME(),
                                messagingSettings.getCapabilitiesDirectoryParticipantId(),
                                providerQos,
                                middlewareConnections),
                        false,
                        true,
                        false);

    // setting up the provisioned values for the ChannelUrlDirectory (domain, interface,
    // participantId...)
    // The ChannelUrlDirectory is also provisioned in MessageRouter  (participantId -> channelId)
    types::ProviderQos channelUrlDirProviderQos;
    channelUrlDirProviderQos.setPriority(1);
    this->insertInCache(
            joynr::types::DiscoveryEntry(messagingSettings.getDiscoveryDirectoriesDomain(),
                                         infrastructure::IChannelUrlDirectory::INTERFACE_NAME(),
                                         messagingSettings.getChannelUrlDirectoryParticipantId(),
                                         channelUrlDirProviderQos,
                                         middlewareConnections),
            false,
            true,
            false);
}

LocalCapabilitiesDirectory::~LocalCapabilitiesDirectory()
{
    // cleanup
    const auto zero = std::chrono::milliseconds(0);
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
        types::CapabilityInformation capInfo(discoveryEntry.getDomain(),
                                             discoveryEntry.getInterfaceName(),
                                             discoveryEntry.getQos(),
                                             capabilitiesClient->getLocalChannelId(),
                                             discoveryEntry.getParticipantId());
        if (std::find(registeredGlobalCapabilities.begin(),
                      registeredGlobalCapabilities.end(),
                      capInfo) == registeredGlobalCapabilities.end()) {
            registeredGlobalCapabilities.push_back(capInfo);
        }
        this->capabilitiesClient->add(registeredGlobalCapabilities);
    }
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
            types::CapabilityInformation capInfo(domain,
                                                 interfaceName,
                                                 qos,
                                                 capabilitiesClient->getLocalChannelId(),
                                                 entry.getParticipantId());
            while (registeredGlobalCapabilities.erase(
                           std::remove(registeredGlobalCapabilities.begin(),
                                       registeredGlobalCapabilities.end(),
                                       capInfo),
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

    return callRecieverIfPossible(scope, localCapabilities, globalCapabilities, callback);
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

    return callRecieverIfPossible(scope, localCapabilities, globalCapabilities, callback);
}

bool LocalCapabilitiesDirectory::callRecieverIfPossible(
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
        const std::vector<types::CapabilityInformation>& results,
        std::vector<CapabilityEntry> cachedLocalCapabilies,
        std::shared_ptr<ILocalCapabilitiesCallback> callback,
        joynr::types::DiscoveryScope::Enum discoveryScope)
{
    QMap<std::string, CapabilityEntry> capabilitiesMap;
    std::vector<CapabilityEntry> mergedEntries;

    for (types::CapabilityInformation capInfo : results) {
        std::vector<joynr::types::CommunicationMiddleware::Enum> connections;
        connections.push_back(joynr::types::CommunicationMiddleware::JOYNR);
        CapabilityEntry capEntry(capInfo.getDomain(),
                                 capInfo.getInterfaceName(),
                                 capInfo.getProviderQos(),
                                 capInfo.getParticipantId(),
                                 connections,
                                 true);
        capabilitiesMap.insertMulti(capInfo.getChannelId(), capEntry);
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

    // if no reciever is called, use the global capabilities directory
    if (!receiverCalled) {
        // search for global entires in the global capabilities directory
        auto onSuccess = [this, participantId, callback](
                const std::vector<joynr::types::CapabilityInformation>& result) {
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

    // if no reciever is called, use the global capabilities directory
    if (!receiverCalled) {
        // search for global entires in the global capabilities directory
        auto onSuccess = [this, interfaceAddress, callback, discoveryQos](
                std::vector<joynr::types::CapabilityInformation> capabilities) {
            this->capabilitiesReceived(capabilities,
                                       getCachedLocalCapabilities(interfaceAddress),
                                       callback,
                                       discoveryQos.getDiscoveryScope());
        };
        this->capabilitiesClient->lookup(domain, interfaceName, onSuccess);
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
        std::shared_ptr<joynr::system::RoutingTypes::Address> joynrAddress(
                new system::RoutingTypes::ChannelAddress(entryIterator.key()));
        messageRouter.addNextHop(currentEntry.getParticipantId(), joynrAddress);
        this->insertInCache(currentEntry, false, true);
    }
}

// inherited method from joynr::system::DiscoveryProvider
void LocalCapabilitiesDirectory::add(
        const types::DiscoveryEntry& discoveryEntry,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    (void)onError;
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
    (void)onError;
    std::shared_ptr<LocalCapabilitiesFuture> future(new LocalCapabilitiesFuture());
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
    (void)onError;
    std::shared_ptr<LocalCapabilitiesFuture> future(new LocalCapabilitiesFuture());
    lookup(participantId, future);
    std::vector<CapabilityEntry> capabilities = future->get();
    if (capabilities.size() > 1) {
        JOYNR_LOG_ERROR(logger) << "participantId " << participantId
                                << " has more than 1 capability entry:\n "
                                << capabilities[0].toString() << "\n "
                                << capabilities[1].toString();
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
    (void)onError;
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
    removeAll(observers, observer);
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
    discoveryEntry.setDomain(capabilityEntry.getDomain());
    discoveryEntry.setInterfaceName(capabilityEntry.getInterfaceName());
    discoveryEntry.setParticipantId(capabilityEntry.getParticipantId());
    discoveryEntry.setQos(capabilityEntry.getQos());
    discoveryEntry.setConnections(capabilityEntry.getMiddlewareConnections());
}

void LocalCapabilitiesDirectory::convertDiscoveryEntryIntoCapabilityEntry(
        const joynr::types::DiscoveryEntry& discoveryEntry,
        CapabilityEntry& capabilityEntry)
{
    capabilityEntry.setDomain(discoveryEntry.getDomain());
    capabilityEntry.setInterfaceName(discoveryEntry.getInterfaceName());
    capabilityEntry.setParticipantId(discoveryEntry.getParticipantId());
    capabilityEntry.setQos(discoveryEntry.getQos());
    capabilityEntry.setMiddlewareConnections(discoveryEntry.getConnections());
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
