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
#include "cluster-controller/capabilities-client/LocalCapabilitiesCallbackWrapper.h"
#include "joynr/ProxyQos.h"
#include "cluster-controller/capabilities-client/ICapabilitiesClient.h"
#include "joynr/JoynrMessagingEndpointAddress.h"
#include "joynr/exceptions.h"
#include "joynr/CapabilityEntry.h"
#include "joynr/ILocalCapabilitiesCallback.h"
#include "joynr/JoynrMessagingViaCCEndpointAddress.h"
#include "joynr/EndpointAddressBase.h"
#include "joynr/types/ProviderQosRequirements.h"
#include "joynr/DiscoveryQos.h"

#include <QMutexLocker>

namespace joynr {

using namespace joynr_logging;

Logger* LocalCapabilitiesDirectory::logger = Logging::getInstance()->getLogger("MSG", "LocalCapabilitiesClient");

const qint64& LocalCapabilitiesDirectory::NO_CACHE_FRESHNESS_REQ(){
    static qint64 value(-1);
    return value;
}

const qint64& LocalCapabilitiesDirectory::DONT_USE_CACHE(){
    static qint64 value(0);
    return value;
}

LocalCapabilitiesDirectory::LocalCapabilitiesDirectory(
        MessagingSettings& messagingSettings,
        ICapabilitiesClient *capabilitiesClientPtr,
        IMessagingEndpointDirectory *endpointDirectory
) :
        messagingSettings(messagingSettings),
        capabilitiesClient(capabilitiesClientPtr),
        cacheLock(new QMutex),
        interfaceAddress2GlobalCapabilities(),
        participantId2GlobalCapabilities(),
        interfaceAddress2LocalCapabilities(),
        participantId2LocalCapability(),
        registeredGlobalCapabilities(),
        endpointDirectory(endpointDirectory)
{

    //setting up the provisioned values for GlobalCapabilitiesClient
    //The GlobalCapabilitiesServer is also provisioned in MessageRouter
    QList<QSharedPointer<EndpointAddressBase> > endpointAddress;
    endpointAddress.append(QSharedPointer<JoynrMessagingViaCCEndpointAddress>(new JoynrMessagingViaCCEndpointAddress()));
    types::ProviderQos providerQos;
    providerQos.setPriority(1);
    this->insertInCache(messagingSettings.getDiscoveryDirectoriesDomain(),
                     infrastructure::IGlobalCapabilitiesDirectory::getInterfaceName(),
                     providerQos,
                     messagingSettings.getCapabilitiesDirectoryParticipantId(),
                     endpointAddress,
                     false,
                     true,
                     false
                     );

    //setting up the provisioned values for the ChannelUrlDirectory (domain, interface, participantId...)
    //The ChannelUrlDirectory is also provisioned in MessageRouter  (participantId -> channelId)
    QList<QSharedPointer<EndpointAddressBase> > channelUrlDirEndpointAddress;
    channelUrlDirEndpointAddress.append(QSharedPointer<JoynrMessagingViaCCEndpointAddress>(
                                            new JoynrMessagingViaCCEndpointAddress()));
    types::ProviderQos channelUrlDirProviderQos;
    channelUrlDirProviderQos.setPriority(1);
    this->insertInCache(messagingSettings.getDiscoveryDirectoriesDomain(),
                     infrastructure::IChannelUrlDirectory::getInterfaceName(),
                     channelUrlDirProviderQos,
                     messagingSettings.getChannelUrlDirectoryParticipantId(),
                     channelUrlDirEndpointAddress,
                     false,
                     true,
                     false
                     );
}

LocalCapabilitiesDirectory::~LocalCapabilitiesDirectory() {
    // cleanup
    delete cacheLock;
    interfaceAddress2GlobalCapabilities.cleanup(0);
    participantId2GlobalCapabilities.cleanup(0);

    interfaceAddress2LocalCapabilities.cleanup(0);
    participantId2LocalCapability.cleanup(0);
}

void LocalCapabilitiesDirectory::registerCapability(const QString &domain, const QString &interfaceName, const types::ProviderQos &qos, const QString &participantId, QList<QSharedPointer<EndpointAddressBase> > endpointAddresses) {
    bool isGlobal = qos.getScope() == types::ProviderScope::GLOBAL;

    // register locally
    this->insertInCache(domain, interfaceName, qos, participantId, endpointAddresses, isGlobal, true, isGlobal);

    // register globally
    if(isGlobal) {
        types::CapabilityInformation capInfo(domain, interfaceName, qos, capabilitiesClient->getLocalChannelId(), participantId);
        if(!registeredGlobalCapabilities.contains(capInfo)){
            registeredGlobalCapabilities.append(capInfo);
        }
        this->capabilitiesClient->registerCapabilities(registeredGlobalCapabilities);
    }
}

void LocalCapabilitiesDirectory::registerCapability(const QString &domain, const QString &interfaceName, const types::ProviderQos &qos, const QString &participantId) {
    registerCapability(domain,interfaceName,qos,participantId,QList<QSharedPointer<EndpointAddressBase> >());
}

void LocalCapabilitiesDirectory::removeCapability(const QString& domain, const QString& interfaceName, const types::ProviderQos& qos) {
    //TODO does it make sense to remove any capability for a domain/interfaceName without knowing which provider registered the capability
    QMutexLocker locker(cacheLock);
    QList<CapabilityEntry> entries = interfaceAddress2GlobalCapabilities.lookUpAll(InterfaceAddress(domain, interfaceName));
    for (int i = 0; i < entries.size(); ++i) {
        CapabilityEntry entry = entries.at(i);
        if (entry.isGlobal()) {
            QList<types::CapabilityInformation> capInfoList = createCapabilitiesInformationList(domain, interfaceName, capabilitiesClient->getLocalChannelId(), qos, entry.getParticipantId());
            registeredGlobalCapabilities.removeAll(types::CapabilityInformation(domain, interfaceName,qos, capabilitiesClient->getLocalChannelId(), entry.getParticipantId()));
            capabilitiesClient->removeCapabilities(capInfoList);
            participantId2GlobalCapabilities.remove(entry.getParticipantId(), entry);
            interfaceAddress2GlobalCapabilities.remove(InterfaceAddress(entry.getDomain(), entry.getInterfaceName()), entry);
        }
        participantId2LocalCapability.remove(entry.getParticipantId(),entry);
        interfaceAddress2LocalCapabilities.remove(InterfaceAddress(domain, interfaceName), entry);
    }
}

void LocalCapabilitiesDirectory::removeCapability(const QString& participantId) {
    QMutexLocker lock(cacheLock);
    CapabilityEntry entry = participantId2LocalCapability.take(participantId);
    interfaceAddress2LocalCapabilities.remove(InterfaceAddress(entry.getDomain(), entry.getInterfaceName()),entry);
    if(entry.isGlobal()) {
        participantId2GlobalCapabilities.remove(participantId, entry);
        interfaceAddress2GlobalCapabilities.remove(InterfaceAddress(entry.getDomain(), entry.getInterfaceName()), entry);

        QList<types::CapabilityInformation> capInfoList = createCapabilitiesInformationList(entry.getDomain(), entry.getInterfaceName(), capabilitiesClient->getLocalChannelId(), entry.getQos(), entry.getParticipantId());
        capabilitiesClient->removeCapabilities(capInfoList);
    }
}

bool LocalCapabilitiesDirectory::getLocalAndCachedCapabilities(const InterfaceAddress& interfaceAddress, const DiscoveryQos& discoveryQos,  QSharedPointer<ILocalCapabilitiesCallback> callBack) {
    DiscoveryQos::DiscoveryScope scope = discoveryQos.getDiscoveryScope();

    QList<CapabilityEntry> localCapabilities = searchCache(interfaceAddress, -1, true);
    QList<CapabilityEntry> globalCapabilities = searchCache(interfaceAddress, discoveryQos.getCacheMaxAge(), false);

    return callRecieverIfPossible(scope, localCapabilities, globalCapabilities, callBack);
}

bool LocalCapabilitiesDirectory::getLocalAndCachedCapabilities(const QString& participantId, const DiscoveryQos& discoveryQos, QSharedPointer<ILocalCapabilitiesCallback> callBack) {
    DiscoveryQos::DiscoveryScope scope = discoveryQos.getDiscoveryScope();

    QList<CapabilityEntry> localCapabilities = searchCache(participantId, -1, true);
    QList<CapabilityEntry> globalCapabilities = searchCache(participantId, discoveryQos.getCacheMaxAge(), false);

    return callRecieverIfPossible(scope, localCapabilities, globalCapabilities, callBack);
}

bool LocalCapabilitiesDirectory::callRecieverIfPossible(DiscoveryQos::DiscoveryScope& scope, QList<CapabilityEntry>& localCapabilities, QList<CapabilityEntry>& globalCapabilities, QSharedPointer<ILocalCapabilitiesCallback> callBack) {
    // return only local capabilities
    if(scope == DiscoveryQos::DiscoveryScope::LOCAL_ONLY) {
        callBack->capabilitiesReceived(localCapabilities);
        return true;
    }

    // return local then global capabilities
    if(scope == DiscoveryQos::DiscoveryScope::LOCAL_THEN_GLOBAL) {
        if(!localCapabilities.isEmpty()) {
            callBack->capabilitiesReceived(localCapabilities);
            return true;
        }
        if(!globalCapabilities.isEmpty()) {
            callBack->capabilitiesReceived(globalCapabilities);
            return true;
        }
    }

    // return local and global capabilities
    if(scope == DiscoveryQos::DiscoveryScope::LOCAL_AND_GLOBAL) {
        // remove doublicates
        QList<CapabilityEntry> result;
        foreach (CapabilityEntry entry, localCapabilities + globalCapabilities) {
            if(!result.contains(entry)) {
                result += entry;
            }
        }
        // return if entries found
        if(!result.isEmpty()) {
            callBack->capabilitiesReceived(result);
            return true;
        }
    }

    // return the global cached entries
    if(scope == DiscoveryQos::DiscoveryScope::GLOBAL_ONLY) {
        if(!globalCapabilities.isEmpty()) {
            callBack->capabilitiesReceived(globalCapabilities);
            return true;
        }
    }
    return false;
}

void LocalCapabilitiesDirectory::getCapabilities(
        const QString& participantId,
        QSharedPointer<ILocalCapabilitiesCallback> callBack,
        const DiscoveryQos& discoveryQos
) {
    // get the local and cached entries
    bool recieverCalled = getLocalAndCachedCapabilities(participantId, discoveryQos, callBack);

    // if no reciever is called, use the global capabilities directory
    if (!recieverCalled) {
        // search for global entires in the global capabilities directory
        QSharedPointer<LocalCapabilitiesCallbackWrapper> wrappedCallBack(new LocalCapabilitiesCallbackWrapper(this, callBack, participantId, discoveryQos));
        this->capabilitiesClient->getCapabilitiesForParticipantId(participantId, wrappedCallBack);
    }
}

void LocalCapabilitiesDirectory::getCapabilities(
        const QString& domain,
        const QString& interfaceName,
        QSharedPointer<ILocalCapabilitiesCallback> callBack,
        const DiscoveryQos& discoveryQos,
        const types::ProviderQosRequirements& qos
) {
    Q_UNUSED(qos); //provider Requirements are currently not supported.
    InterfaceAddress interfaceAddress(domain, interfaceName);

    // get the local and cached entries
    bool recieverCalled = getLocalAndCachedCapabilities(interfaceAddress, discoveryQos, callBack);

    // if no reciever is called, use the global capabilities directory
    if(!recieverCalled) {
        // search for global entires in the global capabilities directory
        QSharedPointer<LocalCapabilitiesCallbackWrapper> wrappedCallBack(new LocalCapabilitiesCallbackWrapper(this, callBack, interfaceAddress, discoveryQos));
        this->capabilitiesClient->getCapabilitiesForInterfaceAddress(domain, interfaceName, wrappedCallBack);
    }
}

QList<CapabilityEntry> LocalCapabilitiesDirectory::getCachedLocalCapabilities(const QString &participantId){
    return searchCache(participantId, -1, true);
}

QList<CapabilityEntry> LocalCapabilitiesDirectory::getCachedLocalCapabilities(const InterfaceAddress &interfaceAddress){
    return searchCache(interfaceAddress, -1, true);
}

void LocalCapabilitiesDirectory::cleanCache(qint64 maxAge_ms) {
    // QMutexLocker locks the mutex, and when the locker variable is destroyed (when it leaves this method)
    // it will unlock the mutex
    QMutexLocker locker(cacheLock);
    interfaceAddress2GlobalCapabilities.cleanup(maxAge_ms);
    participantId2GlobalCapabilities.cleanup(maxAge_ms);
    interfaceAddress2LocalCapabilities.cleanup(maxAge_ms);
    participantId2LocalCapability.cleanup(maxAge_ms);
}


void LocalCapabilitiesDirectory::registerReceivedCapabilities(QMap<QString, CapabilityEntry> capabilityEntries) {
    QMapIterator<QString, CapabilityEntry> entryIterator(capabilityEntries);
    while (entryIterator.hasNext()) {
        entryIterator.next();
        CapabilityEntry currentEntry = entryIterator.value();
        QSharedPointer<EndpointAddressBase> joynrAddress(new JoynrMessagingEndpointAddress(entryIterator.key()));
        endpointDirectory->add(currentEntry.getParticipantId(), joynrAddress);
        this->insertInCache(currentEntry, false, true);
    }
}

 /**
  * Private convenience methods.
  */
void LocalCapabilitiesDirectory::insertInCache(const CapabilityEntry& entry, bool localCache, bool globalCache) {
    QMutexLocker lock(cacheLock);
    InterfaceAddress addr(entry.getDomain(), entry.getInterfaceName());

    // add entry to local cache
    if(localCache) {
        interfaceAddress2LocalCapabilities.insert(addr, entry);
        participantId2LocalCapability.insert(entry.getParticipantId(), entry);
    }

    // add entry to global cache
    if(globalCache) {
        interfaceAddress2GlobalCapabilities.insert(addr, entry );
        participantId2GlobalCapabilities.insert(entry.getParticipantId(), entry);
    }
}

void LocalCapabilitiesDirectory::insertInCache(const QString& domain, const QString& interfaceName, const types::ProviderQos& qos, const QString& participantId, QList<QSharedPointer<EndpointAddressBase> > endpointAddresses, bool isGlobal, bool localCache, bool globalCache) {
    CapabilityEntry entry(domain, interfaceName, qos, participantId, endpointAddresses, isGlobal);

    // do not dublicate entries:
    // the combination participantId is unique for [domain, interfaceName, authtoken]
    // check only for local registration: when register in the global cache, a second entry is an
    // update of the age and a refresh
    bool foundMatch = false;
    if(localCache) {
        QList<CapabilityEntry> entryList = searchCache(participantId, -1, true);
        CapabilityEntry newEntry = CapabilityEntry(domain, interfaceName, qos, participantId, endpointAddresses, isGlobal);
        foreach (CapabilityEntry oldEntry, entryList) {
            if (oldEntry == newEntry){
                foundMatch = true;
            }
        }
    }

    insertInCache(entry, !foundMatch && localCache, globalCache);
}

QList<CapabilityEntry> LocalCapabilitiesDirectory::searchCache(const InterfaceAddress& interfaceAddress, const qint64& maxCacheAge, bool localEntries) {
    QMutexLocker locker(cacheLock);

    // search in local
    if(localEntries) {
        return interfaceAddress2LocalCapabilities.lookUpAll(interfaceAddress);
    } else {
        return interfaceAddress2GlobalCapabilities.lookUp(interfaceAddress, maxCacheAge);
    }
}

QList<CapabilityEntry> LocalCapabilitiesDirectory::searchCache(const QString& participantId, const qint64& maxCacheAge, bool localEntries) {
    QMutexLocker locker(cacheLock);

    // search in local
    if(localEntries) {
        return participantId2LocalCapability.lookUpAll(participantId);
    } else {
        return participantId2GlobalCapabilities.lookUp(participantId, maxCacheAge);
    }
}

QList<types::CapabilityInformation> LocalCapabilitiesDirectory::createCapabilitiesInformationList(const QString& domain, const QString& interfaceName, const QString& channelId, const types::ProviderQos& qos, const QString& participantId) {
    QList<types::CapabilityInformation> capInfoList;
    capInfoList.push_back(types::CapabilityInformation(domain, interfaceName, qos, channelId, participantId));
    return capInfoList;
}
} // namespace joynr
