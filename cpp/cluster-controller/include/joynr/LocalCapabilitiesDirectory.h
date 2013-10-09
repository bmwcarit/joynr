/*
 * #%L
 * joynr::C++
 * $Id:$
 * $HeadURL:$
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
#ifndef LOCALCAPABILITIESDIRECTORY_H
#define LOCALCAPABILITIESDIRECTORY_H
#include "joynr/PrivateCopyAssign.h"

/**
  * The local capabilities directory is the "first point of call" for accessing
  * any information related to the capabilities, i.e. in finding the channel id
  * for a given interface and domain, or to return the interface address for a
  * given channel id.
  * This class is responsible for looking up its local cache first, and depending
  * on whether the data is compatible with the users QoS (e.g. dataFreshness) the
  * cached value will be returned.  Otherwise, a request will be made via the
  * Capabilities Client which will make the remote call to the backend to retrieve
  * the data.
  */

// Disable warning due to use of forward declaration with QSharedPointer
// https://bugreports.qt-project.org/browse/QTBUG-7302
// http://doc.qt.digia.com/qt/qscopedpointer.html#forward-declared-pointers
#ifdef _MSC_VER
    #pragma warning( push )
    #pragma warning( disable : 4150 )
#endif
#ifdef _MSC_VER
    #pragma warning( pop )
#endif

#include "joynr/JoynrClusterControllerExport.h"
#include "joynr/TypedClientMultiCache.h"
#include "joynr/Directory.h"
#include "joynr/joynrlogging.h"
#include "joynr/ClusterControllerDirectories.h"
#include "joynr/types/CapabilityInformation.h"
#include "joynr/ILocalCapabilitiesCallback.h"
#include "joynr/DiscoveryQos.h"

#include <QCache>
#include <QVariantMap>
#include <QMutex>

namespace joynr {

class ICapabilitiesClient;
class CapabilityEntry;
class DiscoveryQos;

class InterfaceAddress;
class EndpointAddressBase;
namespace types { class ProviderQosRequirements;
                  class ProviderQos;
                  class CapabilitiesInformation;
                }

class JOYNRCLUSTERCONTROLLER_EXPORT LocalCapabilitiesDirectory {
public:
    LocalCapabilitiesDirectory(ICapabilitiesClient* capabilitiesClientPtr, IMessagingEndpointDirectory* endpointDirectory);

    virtual ~LocalCapabilitiesDirectory();

    static const qint64& NO_CACHE_FRESHNESS_REQ();
    static const qint64& DONT_USE_CACHE();

    static const QString& CAPABILITIES_DIRECTORY_DOMAIN();
    static const QString& CAPABILITIES_DIRECTORY_INTERFACENAME();
    static const QString& CAPABILITIES_DIRECTORY_CHANNELID();
    static const QString& CAPABILITIES_DIRECTORY_PARTICIPANTID();



    void registerCapability(const QString& domain, const QString& interfaceName, const types::ProviderQos& qos, const QString& participantId, QList<QSharedPointer<EndpointAddressBase> > endpointAddresses);
    void registerCapability(const QString& domain, const QString& interfaceName, const types::ProviderQos& qos, const QString& participantId);

    /*
     * Remove capability from the cache, and for permanent removal from the backend, this method
     * only allows removal of capabilities associated with this cluster controller.  Therefore
     * this method does not allow anyone to remove other capabilities from other cluster
     * controllers.
     */
    void removeCapability(const QString& domain, const QString& interfaceName, const types::ProviderQos& qos);


    virtual void removeCapability(const QString& participantId);

    /*
     * Returns a list of capabilities matching the given domain and interfaceName, this is an asynchronous request,
     * must supply a callback.
     * Will later be extended by ProviderQosReq so that only capabilities matching the Requirements will be returned
     */
    virtual void getCapabilities(const QString& domain, const QString& interfaceName, QSharedPointer<ILocalCapabilitiesCallback> callBack, const DiscoveryQos& discoveryQos, const types::ProviderQosRequirements& qos);

    /*
     * Returns a list of capabilities matching the given channelId.
     * Will later be extended by ProviderQosReq so that only capabilities matching the Requirements will be returned
     */
    virtual void getCapabilities(const QString& participantId, QSharedPointer<ILocalCapabilitiesCallback> callBack, const DiscoveryQos& discoveryQos);

    /*
      * Returns a list of locally cached capabilitiy entries. This method is used when capabilities from the
      * global directory are received, to check if a new local provider was registered in the meantime.
      */
    QList<CapabilityEntry> getCachedLocalCapabilities(const QString& participantId);
    QList<CapabilityEntry> getCachedLocalCapabilities(const InterfaceAddress& interfaceAddress);
    /*
     * Performs maintenance on the cache and removes old entries
     */
    void cleanCache(qint64 maxAge_ms);

    /*
     * Call back methods which will update the local capabilities cache and call the original callback with the results, this indirection was
     * needed because we need to convert a CapabilitiesInformation object into a CapabilityEntry object.
     */
    virtual void registerReceivedCapabilities(QMap<QString, CapabilityEntry> capabilityEntries);






private:


    DISALLOW_COPY_AND_ASSIGN(LocalCapabilitiesDirectory);

    QList<types::CapabilityInformation> createCapabilitiesInformationList(const QString& domain, const QString& interfaceName, const QString& channelId, const types::ProviderQos& qos, const QString& participantId);
/*
    void addToCache(const QString& domain, const QString& interfaceName, const types::ProviderQos& qos, const QString& participantId, QList<QSharedPointer<EndpointAddressBase> > endpointAddresses, bool isGlobal);
    void addToCache(const CapabilityEntry& entry);

    void addToCacheIfAbsent(const QString& domain, const QString& interfaceName, const types::ProviderQos& qos, const QString& participantId, QList<QSharedPointer<EndpointAddressBase> > endpointAddresses, bool isGlobal);

    QList<CapabilityEntry> lookupCache(const InterfaceAddress& interfaceAddress, const qint64& reqCacheDataFreshness_ms);

    QList<CapabilityEntry> lookupCache(const QString& participantId, const qint64& reqCacheDataFreshness_ms);

    QList<CapabilityEntry> filterCapabilities(const QList<CapabilityEntry>& list, bool global);
*/

    bool getLocalAndCachedCapabilities(const InterfaceAddress& interfaceAddress, const DiscoveryQos& discoveryQos, QSharedPointer<ILocalCapabilitiesCallback> callBack);
    bool getLocalAndCachedCapabilities(const QString& participantId, const DiscoveryQos& discoveryQos, QSharedPointer<ILocalCapabilitiesCallback> callBack);
    bool callRecieverIfPossible(DiscoveryQos::DiscoveryScope& scope, QList<CapabilityEntry>& localCapabilities, QList<CapabilityEntry>& globalCapabilities, QSharedPointer<ILocalCapabilitiesCallback> callBack);

    void insertInCache(const CapabilityEntry& entry, bool localCache, bool globalCache);
    void insertInCache(const QString& domain, const QString& interfaceName, const types::ProviderQos& qos, const QString& participantId, QList<QSharedPointer<EndpointAddressBase> > endpointAddresses, bool isGlobal, bool localCache, bool globalCache);
    QList<CapabilityEntry> searchCache(const InterfaceAddress& interfaceAddress, const qint64& maxCacheAge, bool localEntries);
    QList<CapabilityEntry> searchCache(const QString& participantId, const qint64& maxCacheAge, bool localEntries);

    static joynr_logging::Logger* logger;
    ICapabilitiesClient* capabilitiesClient;
    QMutex* cacheLock;

    TypedClientMultiCache<InterfaceAddress, CapabilityEntry> interfaceAddress2GlobalCapabilities;
    TypedClientMultiCache<QString, CapabilityEntry> participantId2GlobalCapabilities;

    TypedClientMultiCache<InterfaceAddress, CapabilityEntry> interfaceAddress2LocalCapabilities;
    TypedClientMultiCache<QString, CapabilityEntry> participantId2LocalCapability;

    QList<types::CapabilityInformation> registeredGlobalCapabilities;
    IMessagingEndpointDirectory* endpointDirectory;

};


} // namespace joynr
#endif //LOCALCAPABILITIESDIRECTORY_H
