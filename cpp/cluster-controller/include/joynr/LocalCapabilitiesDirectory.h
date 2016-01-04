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

#include "joynr/JoynrClusterControllerExport.h"
#include "joynr/TypedClientMultiCache.h"
#include "joynr/joynrlogging.h"
#include "joynr/ClusterControllerDirectories.h"
#include "joynr/types/CapabilityInformation.h"
#include "joynr/ILocalCapabilitiesCallback.h"
#include "joynr/MessagingSettings.h"
#include "joynr/system/DiscoveryAbstractProvider.h"
#include "joynr/types/DiscoveryQos.h"
#include "joynr/Semaphore.h"
#include "common/InterfaceAddress.h"
#include "cluster-controller/capabilities-client/ICapabilitiesClient.h"
#include <vector>

#include <memory>
#include <mutex>
#include <string>
#include <chrono>

#include <QMap>

namespace joynr
{

class MessageRouter;
class ICapabilitiesClient;
class CapabilityEntry;

class InterfaceAddress;

class JOYNRCLUSTERCONTROLLER_EXPORT LocalCapabilitiesDirectory
        : public joynr::system::DiscoveryAbstractProvider
{
public:
    LocalCapabilitiesDirectory(MessagingSettings& messagingSettings,
                               ICapabilitiesClient* capabilitiesClientPtr,
                               MessageRouter& messageRouter);

    ~LocalCapabilitiesDirectory() override;

    void add(const joynr::types::DiscoveryEntry& entry);

    /*
     * Remove capability from the cache, and for permanent removal from the backend, this method
     * only allows removal of capabilities associated with this cluster controller.  Therefore
     * this method does not allow anyone to remove other capabilities from other cluster
     * controllers.
     */
    void remove(const std::string& domain,
                const std::string& interfaceName,
                const types::ProviderQos& qos);

    virtual void remove(const std::string& participantId);

    /*
     * Returns a list of capabilitiess matching the given domain and interfaceName,
     * this is an asynchronous request, must supply a callback.
     */
    virtual void lookup(const std::string& domain,
                        const std::string& interfaceName,
                        std::shared_ptr<ILocalCapabilitiesCallback> callback,
                        const joynr::types::DiscoveryQos& discoveryQos);

    /*
     * Returns a capability entry for a given participant ID or an empty list
     * if it cannot be found.
     */
    virtual void lookup(const std::string& participantId,
                        std::shared_ptr<ILocalCapabilitiesCallback> callback);

    /*
      * Returns a list of locally cached capabilitiy entries. This method is used
      * when capabilities from the global directory are received, to check if a new
      * local provider was registered in the meantime.
      */
    std::vector<CapabilityEntry> getCachedLocalCapabilities(const std::string& participantId);
    std::vector<CapabilityEntry> getCachedLocalCapabilities(
            const InterfaceAddress& interfaceAddress);
    /*
     * Performs maintenance on the cache and removes old entries
     */
    void cleanCache(std::chrono::milliseconds maxAge);

    /*
     * Call back methods which will update the local capabilities cache and call the
     * original callback with the results, this indirection was needed because we
     * need to convert a CapabilitiesInformation object into a CapabilityEntry object.
     */
    virtual void registerReceivedCapabilities(QMap<std::string, CapabilityEntry> capabilityEntries);

    // inherited method from joynr::system::DiscoveryProvider
    void add(const joynr::types::DiscoveryEntry& discoveryEntry,
             std::function<void()> onSuccess,
             std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            override;
    // inherited method from joynr::system::DiscoveryProvider
    void lookup(
            const std::string& domain,
            const std::string& interfaceName,
            const joynr::types::DiscoveryQos& discoveryQos,
            std::function<void(const std::vector<joynr::types::DiscoveryEntry>& result)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            override;
    // inherited method from joynr::system::DiscoveryProvider
    void lookup(const std::string& participantId,
                std::function<void(const joynr::types::DiscoveryEntry& result)> onSuccess,
                std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            override;
    // inherited method from joynr::system::DiscoveryProvider
    void remove(const std::string& participantId,
                std::function<void()> onSuccess,
                std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            override;

    /*
     * Objects that wish to receive provider register/unregister events can attach
     * themselves as observers
     */
    class IProviderRegistrationObserver
    {
    public:
        virtual ~IProviderRegistrationObserver() = default;
        virtual void onProviderAdd(const types::DiscoveryEntry& discoveryEntry) = 0;
        virtual void onProviderRemove(const types::DiscoveryEntry& discoveryEntry) = 0;
    };

    void addProviderRegistrationObserver(std::shared_ptr<IProviderRegistrationObserver> observer);
    void removeProviderRegistrationObserver(
            std::shared_ptr<IProviderRegistrationObserver> observer);

private:
    DISALLOW_COPY_AND_ASSIGN(LocalCapabilitiesDirectory);
    MessagingSettings& messagingSettings;
    void capabilitiesReceived(const std::vector<types::CapabilityInformation>& results,
                              std::vector<CapabilityEntry> cachedLocalCapabilies,
                              std::shared_ptr<ILocalCapabilitiesCallback> callback,
                              joynr::types::DiscoveryScope::Enum discoveryScope);

    bool getLocalAndCachedCapabilities(const InterfaceAddress& interfaceAddress,
                                       const joynr::types::DiscoveryQos& discoveryQos,
                                       std::shared_ptr<ILocalCapabilitiesCallback> callback);
    bool getLocalAndCachedCapabilities(const std::string& participantId,
                                       const joynr::types::DiscoveryQos& discoveryQos,
                                       std::shared_ptr<ILocalCapabilitiesCallback> callback);
    bool callRecieverIfPossible(joynr::types::DiscoveryScope::Enum& scope,
                                std::vector<CapabilityEntry>& localCapabilities,
                                std::vector<CapabilityEntry>& globalCapabilities,
                                std::shared_ptr<ILocalCapabilitiesCallback> callback);

    void insertInCache(const CapabilityEntry& entry, bool localCache, bool globalCache);
    void insertInCache(const joynr::types::DiscoveryEntry& entry,
                       bool isGlobal,
                       bool localCache,
                       bool globalCache);
    std::vector<CapabilityEntry> searchCache(const InterfaceAddress& interfaceAddress,
                                             std::chrono::milliseconds maxCacheAge,
                                             bool localEntries);
    std::vector<CapabilityEntry> searchCache(const std::string& participantId,
                                             std::chrono::milliseconds maxCacheAge,
                                             bool localEntries);

    static void convertDiscoveryEntryIntoCapabilityEntry(
            const joynr::types::DiscoveryEntry& discoveryEntry,
            CapabilityEntry& capabilityEntry);
    static void convertCapabilityEntryIntoDiscoveryEntry(
            const CapabilityEntry& capabilityEntry,
            joynr::types::DiscoveryEntry& discoveryEntry);
    static void convertCapabilityEntriesIntoDiscoveryEntries(
            const std::vector<CapabilityEntry>& capabilityEntries,
            std::vector<joynr::types::DiscoveryEntry>& discoveryEntries);

    static joynr_logging::Logger* logger;
    ICapabilitiesClient* capabilitiesClient;
    std::mutex cacheLock;

    TypedClientMultiCache<InterfaceAddress, CapabilityEntry> interfaceAddress2GlobalCapabilities;
    TypedClientMultiCache<std::string, CapabilityEntry> participantId2GlobalCapabilities;

    TypedClientMultiCache<InterfaceAddress, CapabilityEntry> interfaceAddress2LocalCapabilities;
    TypedClientMultiCache<std::string, CapabilityEntry> participantId2LocalCapability;

    std::vector<types::CapabilityInformation> registeredGlobalCapabilities;
    MessageRouter& messageRouter;
    std::vector<std::shared_ptr<IProviderRegistrationObserver>> observers;

    void informObserversOnAdd(const types::DiscoveryEntry& discoveryEntry);
    void informObserversOnRemove(const types::DiscoveryEntry& discoveryEntry);
};

// NOTE: This future is used to convert the synchronous call of the middleware
// to an asynchronous call to the local capabilities directory. It could be removed
// once we have the possibility to call provider asynchronous.
class LocalCapabilitiesFuture : public ILocalCapabilitiesCallback
{
public:
    LocalCapabilitiesFuture();
    void capabilitiesReceived(const std::vector<CapabilityEntry>& capabilities) override;
    std::vector<CapabilityEntry> get();
    std::vector<CapabilityEntry> get(std::chrono::milliseconds timeout);
    ~LocalCapabilitiesFuture() override = default;

private:
    joynr::Semaphore futureSemaphore;
    DISALLOW_COPY_AND_ASSIGN(LocalCapabilitiesFuture);
    std::vector<CapabilityEntry> capabilities;
};

} // namespace joynr
#endif // LOCALCAPABILITIESDIRECTORY_H
