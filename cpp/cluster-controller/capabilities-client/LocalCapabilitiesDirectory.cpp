/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
#include "joynr/system/ChannelAddress.h"
#include "joynr/exceptions.h"
#include "joynr/CapabilityEntry.h"
#include "joynr/ILocalCapabilitiesCallback.h"
#include "joynr/system/Address.h"
#include "joynr/RequestStatus.h"
#include "joynr/RequestStatusCode.h"
#include "joynr/types/CapabilityInformation.h"
#include "common/InterfaceAddress.h"
#include <algorithm>

#include <QMutexLocker>

namespace joynr
{

using namespace joynr_logging;

Logger* LocalCapabilitiesDirectory::logger =
        Logging::getInstance()->getLogger("MSG", "LocalCapabilitiesDirectory");

const qint64& LocalCapabilitiesDirectory::NO_CACHE_FRESHNESS_REQ()
{
    static qint64 value(-1);
    return value;
}

const qint64& LocalCapabilitiesDirectory::DONT_USE_CACHE()
{
    static qint64 value(0);
    return value;
}

LocalCapabilitiesDirectory::LocalCapabilitiesDirectory(MessagingSettings& messagingSettings,
                                                       ICapabilitiesClient* capabilitiesClientPtr,
                                                       MessageRouter& messageRouter)
        : joynr::system::DiscoveryAbstractProvider(),
          messagingSettings(messagingSettings),
          capabilitiesClient(capabilitiesClientPtr),
          cacheLock(new QMutex),
          interfaceAddress2GlobalCapabilities(),
          participantId2GlobalCapabilities(),
          interfaceAddress2LocalCapabilities(),
          participantId2LocalCapability(),
          registeredGlobalCapabilities(),
          messageRouter(messageRouter),
          observers()
{
    providerQos.setCustomParameters(std::vector<joynr::types::StdCustomParameter>());
    providerQos.setProviderVersion(1);
    providerQos.setPriority(1);
    providerQos.setScope(joynr::types::StdProviderScope::LOCAL);
    providerQos.setSupportsOnChangeSubscriptions(false);
    // setting up the provisioned values for GlobalCapabilitiesClient
    // The GlobalCapabilitiesServer is also provisioned in MessageRouter
    std::vector<joynr::types::StdCommunicationMiddleware::Enum> middlewareConnections = {
            joynr::types::StdCommunicationMiddleware::JOYNR};
    types::StdProviderQos providerQos;
    providerQos.setPriority(1);
    this->insertInCache(
            joynr::types::StdDiscoveryEntry(
                    messagingSettings.getDiscoveryDirectoriesDomain().toStdString(),
                    infrastructure::IGlobalCapabilitiesDirectory::INTERFACE_NAME(),
                    messagingSettings.getCapabilitiesDirectoryParticipantId().toStdString(),
                    providerQos,
                    middlewareConnections),
            false,
            true,
            false);

    // setting up the provisioned values for the ChannelUrlDirectory (domain, interface,
    // participantId...)
    // The ChannelUrlDirectory is also provisioned in MessageRouter  (participantId -> channelId)
    types::StdProviderQos channelUrlDirProviderQos;
    channelUrlDirProviderQos.setPriority(1);
    this->insertInCache(
            joynr::types::StdDiscoveryEntry(
                    messagingSettings.getDiscoveryDirectoriesDomain().toStdString(),
                    infrastructure::IChannelUrlDirectory::INTERFACE_NAME(),
                    messagingSettings.getChannelUrlDirectoryParticipantId().toStdString(),
                    channelUrlDirProviderQos,
                    middlewareConnections),
            false,
            true,
            false);
}

LocalCapabilitiesDirectory::~LocalCapabilitiesDirectory()
{
    // cleanup
    delete cacheLock;
    interfaceAddress2GlobalCapabilities.cleanup(0);
    participantId2GlobalCapabilities.cleanup(0);

    interfaceAddress2LocalCapabilities.cleanup(0);
    participantId2LocalCapability.cleanup(0);
}

void LocalCapabilitiesDirectory::add(const joynr::types::StdDiscoveryEntry& discoveryEntry)
{
    bool isGlobal = discoveryEntry.getQos().getScope() == types::StdProviderScope::GLOBAL;

    // register locally
    this->insertInCache(discoveryEntry, isGlobal, true, isGlobal);

    // Inform observers
    informObserversOnAdd(discoveryEntry);

    // register globally
    if (isGlobal) {
        types::StdCapabilityInformation capInfo(discoveryEntry.getDomain(),
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
                                        const types::StdProviderQos& qos)
{
    // TODO does it make sense to remove any capability for a domain/interfaceName
    // without knowing which provider registered the capability
    QMutexLocker locker(cacheLock);
    QList<CapabilityEntry> entries = interfaceAddress2GlobalCapabilities.lookUpAll(InterfaceAddress(
            QString::fromStdString(domain), QString::fromStdString(interfaceName)));
    std::vector<std::string> participantIdsToRemove;

    types::StdDiscoveryEntry discoveryEntry;

    for (int i = 0; i < entries.size(); ++i) {
        CapabilityEntry entry = entries.at(i);
        if (entry.isGlobal()) {
            types::StdCapabilityInformation capInfo(domain,
                                                    interfaceName,
                                                    qos,
                                                    capabilitiesClient->getLocalChannelId(),
                                                    entry.getParticipantId().toStdString());
            while (registeredGlobalCapabilities.erase(
                           std::remove(registeredGlobalCapabilities.begin(),
                                       registeredGlobalCapabilities.end(),
                                       capInfo),
                           registeredGlobalCapabilities.end()) !=
                   registeredGlobalCapabilities.end()) {
            }
            participantIdsToRemove.push_back(entry.getParticipantId().toStdString());
            participantId2GlobalCapabilities.remove(entry.getParticipantId(), entry);
            interfaceAddress2GlobalCapabilities.remove(
                    InterfaceAddress(entry.getDomain(), entry.getInterfaceName()), entry);
        }
        participantId2LocalCapability.remove(entry.getParticipantId(), entry);
        interfaceAddress2LocalCapabilities.remove(
                InterfaceAddress(
                        QString::fromStdString(domain), QString::fromStdString(interfaceName)),
                entry);

        convertCapabilityEntryIntoDiscoveryEntry(entry, discoveryEntry);
        informObserversOnRemove(discoveryEntry);
    }
    if (!participantIdsToRemove.empty()) {
        capabilitiesClient->remove(participantIdsToRemove);
    }
}

void LocalCapabilitiesDirectory::remove(const std::string& participantId)
{
    QMutexLocker lock(cacheLock);
    CapabilityEntry entry =
            participantId2LocalCapability.take(QString::fromStdString(participantId));
    interfaceAddress2LocalCapabilities.remove(
            InterfaceAddress(entry.getDomain(), entry.getInterfaceName()), entry);
    if (entry.isGlobal()) {
        participantId2GlobalCapabilities.remove(QString::fromStdString(participantId), entry);
        interfaceAddress2GlobalCapabilities.remove(
                InterfaceAddress(entry.getDomain(), entry.getInterfaceName()), entry);
    }

    types::StdDiscoveryEntry discoveryEntry;
    convertCapabilityEntryIntoDiscoveryEntry(entry, discoveryEntry);
    informObserversOnRemove(discoveryEntry);

    capabilitiesClient->remove(participantId);
}

bool LocalCapabilitiesDirectory::getLocalAndCachedCapabilities(
        const InterfaceAddress& interfaceAddress,
        const joynr::types::DiscoveryQos& discoveryQos,
        QSharedPointer<ILocalCapabilitiesCallback> callback)
{
    joynr::types::DiscoveryScope::Enum scope = discoveryQos.getDiscoveryScope();

    std::vector<CapabilityEntry> localCapabilities = searchCache(interfaceAddress, -1, true);
    std::vector<CapabilityEntry> globalCapabilities =
            searchCache(interfaceAddress, discoveryQos.getCacheMaxAge(), false);

    return callRecieverIfPossible(scope, localCapabilities, globalCapabilities, callback);
}

bool LocalCapabilitiesDirectory::getLocalAndCachedCapabilities(
        const std::string& participantId,
        const joynr::types::DiscoveryQos& discoveryQos,
        QSharedPointer<ILocalCapabilitiesCallback> callback)
{
    joynr::types::DiscoveryScope::Enum scope = discoveryQos.getDiscoveryScope();

    std::vector<CapabilityEntry> localCapabilities = searchCache(participantId, -1, true);
    std::vector<CapabilityEntry> globalCapabilities =
            searchCache(participantId, discoveryQos.getCacheMaxAge(), false);

    return callRecieverIfPossible(scope, localCapabilities, globalCapabilities, callback);
}

bool LocalCapabilitiesDirectory::callRecieverIfPossible(
        joynr::types::DiscoveryScope::Enum& scope,
        std::vector<CapabilityEntry>& localCapabilities,
        std::vector<CapabilityEntry>& globalCapabilities,
        QSharedPointer<ILocalCapabilitiesCallback> callback)
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
            foreach (CapabilityEntry entry, localCapabilities) {
                if (std::find(result.begin(), result.end(), entry) == result.end()) {
                    result.push_back(entry);
                }
            }
            foreach (CapabilityEntry entry, globalCapabilities) {
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
        const std::vector<types::StdCapabilityInformation>& results,
        std::vector<CapabilityEntry> cachedLocalCapabilies,
        QSharedPointer<ILocalCapabilitiesCallback> callback,
        joynr::types::StdDiscoveryScope::Enum discoveryScope)
{
    QMap<std::string, CapabilityEntry> capabilitiesMap;
    std::vector<CapabilityEntry> mergedEntries;

    foreach (types::StdCapabilityInformation capInfo, results) {
        QList<joynr::types::CommunicationMiddleware::Enum> connections;
        connections.append(joynr::types::CommunicationMiddleware::JOYNR);
        CapabilityEntry capEntry(QString::fromStdString(capInfo.getDomain()),
                                 QString::fromStdString(capInfo.getInterfaceName()),
                                 types::ProviderQos::createQt(capInfo.getProviderQos()),
                                 QString::fromStdString(capInfo.getParticipantId()),
                                 connections,
                                 true);
        capabilitiesMap.insertMulti(capInfo.getChannelId(), capEntry);
        mergedEntries.push_back(capEntry);
    }
    registerReceivedCapabilities(capabilitiesMap);

    if (discoveryScope == joynr::types::StdDiscoveryScope::LOCAL_THEN_GLOBAL ||
        discoveryScope == joynr::types::StdDiscoveryScope::LOCAL_AND_GLOBAL) {
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
                                        QSharedPointer<ILocalCapabilitiesCallback> callback)
{
    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL);
    // get the local and cached entries
    bool receiverCalled = getLocalAndCachedCapabilities(participantId, discoveryQos, callback);

    // if no reciever is called, use the global capabilities directory
    if (!receiverCalled) {
        // search for global entires in the global capabilities directory
        auto callbackFct = [this, participantId, callback](
                const RequestStatus& status,
                const std::vector<joynr::types::StdCapabilityInformation>& result) {
            Q_UNUSED(status);
            this->capabilitiesReceived(result,
                                       getCachedLocalCapabilities(participantId),
                                       callback,
                                       joynr::types::StdDiscoveryScope::LOCAL_THEN_GLOBAL);
        };
        this->capabilitiesClient->lookup(participantId, callbackFct);
    }
}

void LocalCapabilitiesDirectory::lookup(const std::string& domain,
                                        const std::string& interfaceName,
                                        QSharedPointer<ILocalCapabilitiesCallback> callback,
                                        const joynr::types::StdDiscoveryQos& discoveryQos)
{
    InterfaceAddress interfaceAddress(
            QString::fromStdString(domain), QString::fromStdString(interfaceName));

    // get the local and cached entries
    bool receiverCalled = getLocalAndCachedCapabilities(
            interfaceAddress, types::DiscoveryQos::createQt(discoveryQos), callback);

    // if no reciever is called, use the global capabilities directory
    if (!receiverCalled) {
        // search for global entires in the global capabilities directory
        auto callbackFct = [this, interfaceAddress, callback, discoveryQos](
                RequestStatus status,
                std::vector<joynr::types::StdCapabilityInformation> capabilities) {
            Q_UNUSED(status);
            this->capabilitiesReceived(capabilities,
                                       getCachedLocalCapabilities(interfaceAddress),
                                       callback,
                                       discoveryQos.getDiscoveryScope());
        };
        this->capabilitiesClient->lookup(domain, interfaceName, callbackFct);
    }
}

std::vector<CapabilityEntry> LocalCapabilitiesDirectory::getCachedLocalCapabilities(
        const std::string& participantId)
{
    return searchCache(participantId, -1, true);
}

std::vector<CapabilityEntry> LocalCapabilitiesDirectory::getCachedLocalCapabilities(
        const InterfaceAddress& interfaceAddress)
{
    return searchCache(interfaceAddress, -1, true);
}

void LocalCapabilitiesDirectory::cleanCache(qint64 maxAge_ms)
{
    // QMutexLocker locks the mutex, and when the locker variable is destroyed (when
    // it leaves this method) it will unlock the mutex
    QMutexLocker locker(cacheLock);
    interfaceAddress2GlobalCapabilities.cleanup(maxAge_ms);
    participantId2GlobalCapabilities.cleanup(maxAge_ms);
    interfaceAddress2LocalCapabilities.cleanup(maxAge_ms);
    participantId2LocalCapability.cleanup(maxAge_ms);
}

void LocalCapabilitiesDirectory::registerReceivedCapabilities(
        QMap<std::string, CapabilityEntry> capabilityEntries)
{
    QMapIterator<std::string, CapabilityEntry> entryIterator(capabilityEntries);
    while (entryIterator.hasNext()) {
        entryIterator.next();
        CapabilityEntry currentEntry = entryIterator.value();
        QSharedPointer<joynr::system::Address> joynrAddress(
                new system::ChannelAddress(QString::fromStdString(entryIterator.key())));
        messageRouter.addNextHop(currentEntry.getParticipantId().toStdString(), joynrAddress);
        this->insertInCache(currentEntry, false, true);
    }
}

// inherited method from joynr::system::DiscoveryProvider
void LocalCapabilitiesDirectory::add(const types::StdDiscoveryEntry& discoveryEntry,
                                     std::function<void()> onSuccess)
{
    add(discoveryEntry);
    onSuccess();
}

// inherited method from joynr::system::DiscoveryProvider
void LocalCapabilitiesDirectory::lookup(
        const std::string& domain,
        const std::string& interfaceName,
        const types::StdDiscoveryQos& discoveryQos,
        std::function<void(const std::vector<joynr::types::StdDiscoveryEntry>& result)> onSuccess)
{
    QSharedPointer<LocalCapabilitiesFuture> future(new LocalCapabilitiesFuture());
    lookup(domain, interfaceName, future, discoveryQos);
    std::vector<CapabilityEntry> capabilities = future->get();
    std::vector<types::StdDiscoveryEntry> result;
    convertCapabilityEntriesIntoDiscoveryEntries(capabilities, result);
    onSuccess(result);
}

// inherited method from joynr::system::DiscoveryProvider
void LocalCapabilitiesDirectory::lookup(
        const std::string& participantId,
        std::function<void(const joynr::types::StdDiscoveryEntry&)> onSuccess)
{
    QSharedPointer<LocalCapabilitiesFuture> future(new LocalCapabilitiesFuture());
    lookup(participantId, future);
    std::vector<CapabilityEntry> capabilities = future->get();
    if (capabilities.size() > 1) {
        LOG_ERROR(logger,
                  QString("participantId %1 has more than 1 capability entry:\n %2\n %3")
                          .arg(QString::fromStdString(participantId))
                          .arg(capabilities[0].toString())
                          .arg(capabilities[1].toString()));
    }

    types::StdDiscoveryEntry result;
    if (!capabilities.empty()) {
        convertCapabilityEntryIntoDiscoveryEntry(capabilities.front(), result);
    }
    onSuccess(result);
}

// inherited method from joynr::system::DiscoveryProvider
void LocalCapabilitiesDirectory::remove(const std::string& participantId,
                                        std::function<void()> onSuccess)
{
    remove(participantId);
    onSuccess();
}

void LocalCapabilitiesDirectory::addProviderRegistrationObserver(
        QSharedPointer<LocalCapabilitiesDirectory::IProviderRegistrationObserver> observer)
{
    observers.append(observer);
}

void LocalCapabilitiesDirectory::removeProviderRegistrationObserver(
        QSharedPointer<LocalCapabilitiesDirectory::IProviderRegistrationObserver> observer)
{
    observers.removeAll(observer);
}

/**
 * Private convenience methods.
 */
void LocalCapabilitiesDirectory::insertInCache(const CapabilityEntry& entry,
                                               bool localCache,
                                               bool globalCache)
{
    QMutexLocker lock(cacheLock);
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

void LocalCapabilitiesDirectory::insertInCache(
        const joynr::types::StdDiscoveryEntry& discoveryEntry,
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
                searchCache(discoveryEntry.getParticipantId(), -1, true);
        CapabilityEntry newEntry;
        convertDiscoveryEntryIntoCapabilityEntry(discoveryEntry, newEntry);
        entry.setGlobal(isGlobal);
        foreach (CapabilityEntry oldEntry, entryList) {
            if (oldEntry == newEntry) {
                foundMatch = true;
            }
        }
    }

    insertInCache(entry, !foundMatch && localCache, globalCache);
}

std::vector<CapabilityEntry> LocalCapabilitiesDirectory::searchCache(
        const InterfaceAddress& interfaceAddress,
        const qint64& maxCacheAge,
        bool localEntries)
{
    QMutexLocker locker(cacheLock);

    // search in local
    if (localEntries) {
        return TypeUtil::toStd<CapabilityEntry>(
                interfaceAddress2LocalCapabilities.lookUpAll(interfaceAddress));
    } else {
        return TypeUtil::toStd<CapabilityEntry>(
                interfaceAddress2GlobalCapabilities.lookUp(interfaceAddress, maxCacheAge));
    }
}

std::vector<CapabilityEntry> LocalCapabilitiesDirectory::searchCache(
        const std::string& participantId,
        const qint64& maxCacheAge,
        bool localEntries)
{
    QMutexLocker locker(cacheLock);

    // search in local
    if (localEntries) {
        return TypeUtil::toStd<CapabilityEntry>(
                participantId2LocalCapability.lookUpAll(QString::fromStdString(participantId)));
    } else {
        return TypeUtil::toStd<CapabilityEntry>(participantId2GlobalCapabilities.lookUp(
                QString::fromStdString(participantId), maxCacheAge));
    }
}

void LocalCapabilitiesDirectory::convertCapabilityEntryIntoDiscoveryEntry(
        const CapabilityEntry& capabilityEntry,
        joynr::types::StdDiscoveryEntry& discoveryEntry)
{
    discoveryEntry.setDomain(capabilityEntry.getDomain().toStdString());
    discoveryEntry.setInterfaceName(capabilityEntry.getInterfaceName().toStdString());
    discoveryEntry.setParticipantId(capabilityEntry.getParticipantId().toStdString());
    discoveryEntry.setQos(types::ProviderQos::createStd(capabilityEntry.getQos()));
    discoveryEntry.setConnections(TypeUtil::toStd<joynr::types::CommunicationMiddleware::Enum,
                                                  joynr::types::StdCommunicationMiddleware::Enum,
                                                  joynr::types::CommunicationMiddleware>(
            capabilityEntry.getMiddlewareConnections()));
}

void LocalCapabilitiesDirectory::convertDiscoveryEntryIntoCapabilityEntry(
        const joynr::types::StdDiscoveryEntry& discoveryEntry,
        CapabilityEntry& capabilityEntry)
{
    capabilityEntry.setDomain(QString::fromStdString(discoveryEntry.getDomain()));
    capabilityEntry.setInterfaceName(QString::fromStdString(discoveryEntry.getInterfaceName()));
    capabilityEntry.setParticipantId(QString::fromStdString(discoveryEntry.getParticipantId()));
    capabilityEntry.setQos(types::ProviderQos::createQt(discoveryEntry.getQos()));
    capabilityEntry.setMiddlewareConnections(
            TypeUtil::toQt<joynr::types::StdCommunicationMiddleware::Enum,
                           joynr::types::CommunicationMiddleware::Enum,
                           joynr::types::CommunicationMiddleware>(discoveryEntry.getConnections()));
}

void LocalCapabilitiesDirectory::convertCapabilityEntriesIntoDiscoveryEntries(
        const std::vector<CapabilityEntry>& capabilityEntries,
        std::vector<types::StdDiscoveryEntry>& discoveryEntries)
{
    foreach (const CapabilityEntry& capabilityEntry, capabilityEntries) {
        joynr::types::StdDiscoveryEntry discoveryEntry;
        convertCapabilityEntryIntoDiscoveryEntry(capabilityEntry, discoveryEntry);
        discoveryEntries.push_back(discoveryEntry);
    }
}

void LocalCapabilitiesDirectory::informObserversOnAdd(
        const types::StdDiscoveryEntry& discoveryEntry)
{
    foreach (const QSharedPointer<IProviderRegistrationObserver>& observer, observers) {
        observer->onProviderAdd(discoveryEntry);
    }
}

void LocalCapabilitiesDirectory::informObserversOnRemove(
        const types::StdDiscoveryEntry& discoveryEntry)
{
    foreach (const QSharedPointer<IProviderRegistrationObserver>& observer, observers) {
        observer->onProviderRemove(discoveryEntry);
    }
}

LocalCapabilitiesFuture::LocalCapabilitiesFuture() : futureSemaphore(0), capabilities()
{
}

void LocalCapabilitiesFuture::capabilitiesReceived(std::vector<CapabilityEntry> capabilities)
{
    this->capabilities = capabilities;
    futureSemaphore.release(1);
}

std::vector<CapabilityEntry> LocalCapabilitiesFuture::get()
{
    futureSemaphore.acquire(1);
    futureSemaphore.release(1);
    return capabilities;
}

std::vector<CapabilityEntry> LocalCapabilitiesFuture::get(const qint64& timeout)
{

    int timeout_int(timeout);
    // prevent overflow during conversion from qint64 to int
    int maxint = std::numeric_limits<int>::max();
    if (timeout > maxint) {
        timeout_int = maxint;
    }

    if (futureSemaphore.tryAcquire(1, timeout_int)) {
        futureSemaphore.release(1);
    }
    return capabilities;
}

} // namespace joynr
