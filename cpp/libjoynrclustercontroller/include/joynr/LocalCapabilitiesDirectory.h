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
#ifndef LOCALCAPABILITIESDIRECTORY_H
#define LOCALCAPABILITIESDIRECTORY_H

#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/asio/steady_timer.hpp>

#include "joynr/BoostIoserviceForwardDecl.h"
#include "joynr/CapabilitiesStorage.h"
#include "joynr/ClusterControllerDirectories.h"
#include "joynr/ILocalCapabilitiesCallback.h"
#include "joynr/InterfaceAddress.h"
#include "joynr/JoynrClusterControllerExport.h"
#include "joynr/Logger.h"
#include "joynr/MessagingSettings.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/Semaphore.h"
#include "joynr/system/DiscoveryAbstractProvider.h"
#include "joynr/system/ProviderReregistrationControllerProvider.h"
#include "joynr/types/DiscoveryEntry.h"
#include "joynr/types/DiscoveryError.h"
#include "joynr/types/DiscoveryQos.h"
#include "joynr/types/GlobalDiscoveryEntry.h"

namespace joynr
{
class IAccessController;
class IGlobalCapabilitiesDirectoryClient;
class ClusterControllerSettings;
class IMessageRouter;

/**
  * The local capabilities directory is the "first point of call" for accessing
  * any information related to the capabilities, i.e. in finding the channel id
  * for a given interface and domain, or to return the interface address for a
  * given channel id.
  * This class is responsible for looking up its local cache first, and depending
  * on whether the data is compatible with the users QoS (e.g. dataFreshness) the
  * cached value will be returned.  Otherwise, a request will be made via the
  * Global Capabilities Directory Client which will make the remote call to the
  * backend to retrieve the data.
  */
class JOYNRCLUSTERCONTROLLER_EXPORT LocalCapabilitiesDirectory
        : public joynr::system::DiscoveryAbstractProvider,
          public joynr::system::ProviderReregistrationControllerProvider,
          public std::enable_shared_from_this<LocalCapabilitiesDirectory>
{
public:
    // TODO: change shared_ptr to unique_ptr once JoynrClusterControllerRuntime is refactored
    LocalCapabilitiesDirectory(
            ClusterControllerSettings& messagingSettings,
            std::shared_ptr<IGlobalCapabilitiesDirectoryClient> globalCapabilitiesDirectoryClient,
            std::shared_ptr<joynr::capabilities::Storage> locallyRegisteredCapabilities,
            std::shared_ptr<joynr::capabilities::CachingStorage> globalLookupCache,
            const std::string& localAddress,
            std::weak_ptr<IMessageRouter> messageRouter,
            boost::asio::io_service& ioService,
            const std::string clusterControllerId,
            std::vector<std::string> knownGbids,
            std::int64_t defaultExpiryIntervalMs);

    ~LocalCapabilitiesDirectory() override;

    void init();

    void shutdown();

    /*
      * Returns a list of locally cached capabilitiy entries. This method is used
      * when capabilities from the global directory are received, to check if a new
      * local provider was registered in the meantime.
      */
    std::vector<types::DiscoveryEntry> getCachedLocalCapabilities(const std::string& participantId);
    std::vector<types::DiscoveryEntry> getCachedLocalCapabilities(
            const std::vector<InterfaceAddress>& interfaceAddress);
    /*
     * removes all discovery entries
     */
    void clear();

    /*
     * Call back methods which will update the local capabilities cache and call the
     * original callback with the results, this indirection was needed because we
     * need to convert a CapabilitiesInformation object into a DiscoveryEntry object.
     */
    void registerReceivedCapabilities(
            const std::unordered_multimap<std::string, types::DiscoveryEntry>&& capabilityEntries);

    // inherited method from joynr::system::DiscoveryProvider
    void add(const joynr::types::DiscoveryEntry& discoveryEntry,
             std::function<void()> onSuccess,
             std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            override;
    // inherited method from joynr::system::DiscoveryProvider
    void add(const joynr::types::DiscoveryEntry& discoveryEntry,
             const bool& awaitGlobalRegistration,
             std::function<void()> onSuccess,
             std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            override;
    // inherited method from joynr::system::DiscoveryProvider
    void add(const joynr::types::DiscoveryEntry& discoveryEntry,
             const bool& awaitGlobalRegistration,
             const std::vector<std::string>& gbids,
             std::function<void()> onSuccess,
             std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError)
            override;
    // inherited method from joynr::system::DiscoveryProvider
    void addToAll(const joynr::types::DiscoveryEntry& discoveryEntry,
                  const bool& awaitGlobalRegistration,
                  std::function<void()> onSuccess,
                  std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError)
            override;
    // inherited method from joynr::system::DiscoveryProvider
    void lookup(
            const std::vector<std::string>& domains,
            const std::string& interfaceName,
            const joynr::types::DiscoveryQos& discoveryQos,
            std::function<void(const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>& result)>
                    onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            override;
    // inherited method from joynr::system::DiscoveryProvider
    void lookup(
            const std::vector<std::string>& domains,
            const std::string& interfaceName,
            const joynr::types::DiscoveryQos& discoveryQos,
            const std::vector<std::string>& gbids,
            std::function<void(const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>& result)>
                    onSuccess,
            std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError)
            override;
    // inherited method from joynr::system::DiscoveryProvider
    void lookup(
            const std::string& participantId,
            std::function<void(const joynr::types::DiscoveryEntryWithMetaInfo& result)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            override;
    // inherited method from joynr::system::DiscoveryProvider
    void lookup(
            const std::string& participantId,
            const joynr::types::DiscoveryQos& discoveryQos,
            const std::vector<std::string>& gbids,
            std::function<void(const joynr::types::DiscoveryEntryWithMetaInfo& result)> onSuccess,
            std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError)
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

    /*
     * Persist the content of the local capabilities directory to a file.
     */
    void updatePersistedFile();

    /*
     * Load persisted capabilities from the specified file.
     */
    void loadPersistedFile();

    /*
     * Save the content of the local capabilities directory to the specified file.
     */
    void saveLocalCapabilitiesToFile(const std::string& fileName);

    /*
     * Load capabilities from the specified file.
     */
    void injectGlobalCapabilitiesFromFile(const std::string& fileName);

    /*
     * returns true if lookup calls with discovery scope LOCAL_THEN_GLOBAL are ongoing
     */
    bool hasPendingLookups();

    /*
     * Set AccessController so that registration of providers can be checked.
     */
    void setAccessController(std::weak_ptr<IAccessController> accessController);

    void triggerGlobalProviderReregistration(
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            final override;

    std::vector<types::DiscoveryEntry> getCachedGlobalDiscoveryEntries() const;

private:
    DISALLOW_COPY_AND_ASSIGN(LocalCapabilitiesDirectory);
    ClusterControllerSettings& _clusterControllerSettings; // to retrieve info about persistency

    struct ValidateGBIDsEnum
    {
        enum Enum : std::uint32_t { OK = 0, INVALID = 1, UNKNOWN = 2 };
    };
    static ValidateGBIDsEnum::Enum validateGbids(std::vector<std::string> gbids,
                                                 std::unordered_set<std::string> validGbids);

    bool containsOnlyEmptyString(const std::vector<std::string> gbids);
    void replaceGbidWithEmptyString(std::vector<joynr::types::GlobalDiscoveryEntry>& capabilities);

    types::GlobalDiscoveryEntry toGlobalDiscoveryEntry(
            const types::DiscoveryEntry& discoveryEntry) const;
    void capabilitiesReceived(const std::vector<types::GlobalDiscoveryEntry>& results,
                              std::vector<types::DiscoveryEntry>&& localEntries,
                              std::shared_ptr<ILocalCapabilitiesCallback> callback,
                              joynr::types::DiscoveryScope::Enum discoveryScope);

    /*
     * Returns a list of capabilities matching the given domain and interfaceName and gbids.
     * This is an asynchronous request, must supply a callback.
     */
    void lookup(const std::vector<std::string>& domains,
                const std::string& interfaceName,
                const std::vector<std::string>& gbids,
                std::shared_ptr<ILocalCapabilitiesCallback> callback,
                const joynr::types::DiscoveryQos& discoveryQos);

    /*
     * Returns a capability entry for a given participant ID and gbids or
     * an empty list if it cannot be found.
     */
    virtual void lookup(const std::string& participantId,
                        const types::DiscoveryQos& discoveryQos,
                        const std::vector<std::string>& gbids,
                        std::shared_ptr<ILocalCapabilitiesCallback> callback);

    bool getLocalAndCachedCapabilities(const std::vector<InterfaceAddress>& interfaceAddress,
                                       const joynr::types::DiscoveryQos& discoveryQos,
                                       const std::vector<std::string>& gbids,
                                       std::shared_ptr<ILocalCapabilitiesCallback> callback);
    bool getLocalAndCachedCapabilities(const std::string& participantId,
                                       const joynr::types::DiscoveryQos& discoveryQos,
                                       const std::vector<std::string>& gbids,
                                       std::shared_ptr<ILocalCapabilitiesCallback> callback);
    bool callReceiverIfPossible(joynr::types::DiscoveryScope::Enum& scope,
                                std::vector<types::DiscoveryEntry>&& localCapabilities,
                                std::vector<types::DiscoveryEntry>&& globalCapabilities,
                                std::shared_ptr<ILocalCapabilitiesCallback> callback);

    void insertInLocalCapabilitiesStorage(const types::DiscoveryEntry& entry);
    void insertInGlobalLookupCache(const types::DiscoveryEntry& entry,
                                   const std::vector<std::string>& gbids);

    std::vector<types::DiscoveryEntry> searchGlobalCache(
            const std::vector<InterfaceAddress>& interfaceAddress,
            const std::vector<std::string>& gbids,
            std::chrono::milliseconds maxCacheAge);
    std::vector<types::DiscoveryEntry> searchLocalCache(
            const std::vector<InterfaceAddress>& interfaceAddress);
    boost::optional<types::DiscoveryEntry> searchCaches(const std::string& participantId,
                                                        types::DiscoveryScope::Enum scope,
                                                        const std::vector<std::string>& gbids,
                                                        std::chrono::milliseconds maxCacheAge);

    ADD_LOGGER(LocalCapabilitiesDirectory)
    std::shared_ptr<IGlobalCapabilitiesDirectoryClient> _globalCapabilitiesDirectoryClient;
    std::shared_ptr<capabilities::Storage> _locallyRegisteredCapabilities;
    std::shared_ptr<capabilities::CachingStorage> _globalLookupCache;
    std::string _localAddress;
    mutable std::recursive_mutex _cacheLock;
    std::mutex _pendingLookupsLock;

    std::weak_ptr<IMessageRouter> _messageRouter;
    std::vector<std::shared_ptr<IProviderRegistrationObserver>> _observers;

    std::unordered_map<InterfaceAddress, std::vector<std::shared_ptr<ILocalCapabilitiesCallback>>>
            _pendingLookups;

    std::weak_ptr<IAccessController> _accessController;

    boost::asio::steady_timer _checkExpiredDiscoveryEntriesTimer;
    const bool _isLocalCapabilitiesDirectoryPersistencyEnabled;

    void scheduleCleanupTimer();
    void checkExpiredDiscoveryEntries(const boost::system::error_code& errorCode);
    std::string joinToString(const std::vector<types::DiscoveryEntry>& discoveryEntries) const;
    void remove(const types::DiscoveryEntry& discoveryEntry);
    boost::asio::steady_timer _freshnessUpdateTimer;
    std::string _clusterControllerId;
    const std::vector<std::string> _knownGbids;
    std::unordered_set<std::string> _knownGbidsSet;
    const std::int64_t _defaultExpiryIntervalMs;
    std::unordered_map<std::string, std::vector<std::string>> _globalParticipantIdsToGbidsMap;
    void scheduleFreshnessUpdate();
    void sendAndRescheduleFreshnessUpdate(const boost::system::error_code& timerError);
    void informObserversOnAdd(const types::DiscoveryEntry& discoveryEntry);
    void informObserversOnRemove(const types::DiscoveryEntry& discoveryEntry);
    void registerPendingLookup(const std::vector<InterfaceAddress>& interfaceAddresses,
                               const std::shared_ptr<ILocalCapabilitiesCallback>& callback);
    bool isCallbackCalled(const std::vector<InterfaceAddress>& interfaceAddresses,
                          const std::shared_ptr<ILocalCapabilitiesCallback>& callback,
                          const joynr::types::DiscoveryQos& discoveryQos);
    void callbackCalled(const std::vector<InterfaceAddress>& interfaceAddresses,
                        const std::shared_ptr<ILocalCapabilitiesCallback>& callback);
    void callPendingLookups(const InterfaceAddress& interfaceAddress);
    bool isGlobal(const types::DiscoveryEntry& discoveryEntry) const;

    void addInternal(const joynr::types::DiscoveryEntry& entry,
                     bool awaitGlobalRegistration,
                     const std::vector<std::string>& gbids,
                     std::function<void()> onSuccess,
                     std::function<void(const joynr::types::DiscoveryError::Enum&)> onError);
    bool hasProviderPermission(const types::DiscoveryEntry& discoveryEntry);
    std::size_t countGlobalCapabilities() const;

    std::vector<types::DiscoveryEntry> optionalToVector(
            boost::optional<types::DiscoveryEntry> optionalEntry);
    std::vector<types::DiscoveryEntryWithMetaInfo> filterDuplicates(
            std::vector<types::DiscoveryEntryWithMetaInfo>&& globalCapabilitiesWithMetaInfo,
            std::vector<types::DiscoveryEntryWithMetaInfo>&& localCapabilitiesWithMetaInfo);
    bool isEntryForGbid(const std::unique_lock<std::recursive_mutex>& cacheLock,
                        const types::DiscoveryEntry& entry,
                        const std::unordered_set<std::string> gbids);
    std::vector<types::DiscoveryEntry> filterDiscoveryEntriesByGbids(
            const std::unique_lock<std::recursive_mutex>& cacheLock,
            const std::vector<types::DiscoveryEntry>& entries,
            const std::unordered_set<std::string>& gbids);
};

class LocalCapabilitiesCallback : public ILocalCapabilitiesCallback
{
public:
    LocalCapabilitiesCallback(
            std::function<void(const std::vector<types::DiscoveryEntryWithMetaInfo>&)>&& onSuccess,
            std::function<void(const types::DiscoveryError::Enum&)>&& onError);
    void capabilitiesReceived(
            const std::vector<types::DiscoveryEntryWithMetaInfo>& capabilities) override;
    void onError(const types::DiscoveryError::Enum&) override;
    ~LocalCapabilitiesCallback() override = default;

private:
    DISALLOW_COPY_AND_ASSIGN(LocalCapabilitiesCallback);
    std::once_flag _onceFlag;
    std::function<void(const std::vector<types::DiscoveryEntryWithMetaInfo>&)> _onSuccess;
    std::function<void(const types::DiscoveryError::Enum&)> _onErrorCallback;
};

} // namespace joynr
#endif // LOCALCAPABILITIESDIRECTORY_H
