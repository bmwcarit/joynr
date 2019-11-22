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

#ifndef JOYNRCLUSTERCONTROLLERRUNTIME_H
#define JOYNRCLUSTERCONTROLLERRUNTIME_H

#include <memory>
#include <string>
#include <vector>
#include <list>
#include <map>

#include "joynr/IClusterControllerSignalHandler.h"
#include "joynr/CapabilitiesStorage.h"
#include "joynr/ClusterControllerSettings.h"
#include "joynr/IKeychain.h"
#include "joynr/JoynrClusterControllerRuntimeExport.h"
#include "joynr/JoynrRuntime.h"
#include "joynr/LibjoynrSettings.h"
#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/Semaphore.h"
#include "joynr/types/CustomParameter.h"
#include "joynr/WebSocketSettings.h"

class JoynrClusterControllerRuntimeTest;

namespace joynr
{

class AccessController;
class AccessControlListEditor;
class LocalCapabilitiesDirectory;
class ILocalChannelUrlDirectory;
class ITransportMessageReceiver;
class ITransportMessageSender;
class SubscriptionManager;
class JoynrClusterControllerMqttConnectionData;
class JoynrMessagingConnectorFactory;
class IDispatcher;
class InProcessMessagingSkeleton;
class HttpMessagingSkeleton;
class AbstractGlobalMessagingSkeleton;
class MqttReceiver;
class MulticastMessagingSkeletonDirectory;
class IPlatformSecurityManager;
class Settings;
class IMessageRouter;
class IMessageSender;
class IWebsocketCcMessagingSkeleton;
class CcMessageRouter;
class WebSocketMessagingStubFactory;
class LocalDomainAccessController;

namespace infrastructure
{
class ChannelUrlDirectoryProxy;
class GlobalDomainAccessControllerProxy;
} // namespace infrastructure

namespace system
{
namespace RoutingTypes
{
class Address;
} // namespace RoutingTypes
} // namespace system

class JOYNRCLUSTERCONTROLLERRUNTIME_EXPORT JoynrClusterControllerRuntime
        : public JoynrRuntimeImpl,
          public IClusterControllerSignalHandler
{
public:
    using MqttMessagingSkeletonFactory =
            std::function<std::shared_ptr<AbstractGlobalMessagingSkeleton>(
                    std::weak_ptr<IMessageRouter> messageRouter,
                    std::shared_ptr<MqttReceiver> mqttReceiver,
                    const std::string& gbid,
                    const std::string& multicastTopicPrefix,
                    std::uint64_t ttlUplift)>;
    JoynrClusterControllerRuntime(
            std::unique_ptr<Settings> settings,
            std::shared_ptr<IKeychain> _keyChain = nullptr,
            MqttMessagingSkeletonFactory mqttMessagingSkeletonFactory = nullptr,
            std::shared_ptr<ITransportMessageReceiver> httpMessageReceiver = nullptr,
            std::shared_ptr<ITransportMessageSender> httpMessageSender = nullptr,
            std::vector<std::shared_ptr<JoynrClusterControllerMqttConnectionData>>
                    mqttConnectionDataVector = std::vector<
                            std::shared_ptr<JoynrClusterControllerMqttConnectionData>>());

    static std::shared_ptr<JoynrClusterControllerRuntime> create(
            std::size_t argc,
            char* argv[],
            std::shared_ptr<IKeychain> _keyChain = nullptr,
            MqttMessagingSkeletonFactory mqttMessagingSkeletonFactory = nullptr);

    static std::shared_ptr<JoynrClusterControllerRuntime> create(
            std::unique_ptr<Settings> settings,
            const std::string& discoveryEntriesFile = "",
            std::shared_ptr<IKeychain> _keyChain = nullptr,
            MqttMessagingSkeletonFactory mqttMessagingSkeletonFactory = nullptr);

    ~JoynrClusterControllerRuntime() override;

    void start();
    void stop(bool deleteChannel = false);
    void shutdown() final;
    void runForever();

    // Implement IClusterControllerSignalHandler
    void startExternalCommunication() final;
    void stopExternalCommunication() final;
    void shutdownClusterController() final;

    // Functions used by integration tests
    void deleteChannel();

    void init();

    /*
     * Inject predefined capabilities stored in a JSON file.
     */
    void injectGlobalCapabilitiesFromFile(const std::string& fileName);

protected:
    void importMessageRouterFromFile();
    void importPersistedLocalCapabilitiesDirectory();

    std::map<std::string, joynr::types::DiscoveryEntryWithMetaInfo> getProvisionedEntries()
            const final;

    std::shared_ptr<IMessageRouter> getMessageRouter() final;

    std::shared_ptr<IDispatcher> _joynrDispatcher;

    std::shared_ptr<SubscriptionManager> _subscriptionManager;
    std::shared_ptr<IMessageSender> _messageSender;

    std::shared_ptr<LocalCapabilitiesDirectory> _localCapabilitiesDirectory;

    std::shared_ptr<InProcessMessagingSkeleton> _libJoynrMessagingSkeleton;

    std::shared_ptr<ITransportMessageReceiver> _httpMessageReceiver;
    std::shared_ptr<ITransportMessageSender> _httpMessageSender;
    std::shared_ptr<HttpMessagingSkeleton> _httpMessagingSkeleton;
    MqttMessagingSkeletonFactory _mqttMessagingSkeletonFactory;

    std::vector<std::shared_ptr<JoynrClusterControllerMqttConnectionData>>
            _mqttConnectionDataVector;

    std::vector<std::shared_ptr<IDispatcher>> _dispatcherList;

    std::unique_ptr<Settings> _settings;
    LibjoynrSettings _libjoynrSettings;
    std::shared_ptr<LocalDomainAccessController> _localDomainAccessController;
    ClusterControllerSettings _clusterControllerSettings;

    WebSocketSettings _wsSettings;
    std::shared_ptr<IWebsocketCcMessagingSkeleton> _wsCcMessagingSkeleton;
    std::shared_ptr<IWebsocketCcMessagingSkeleton> _wsTLSCcMessagingSkeleton;
    bool _httpMessagingIsRunning;
    bool _mqttMessagingIsRunning;
    bool _doMqttMessaging;
    bool _doHttpMessaging;
    std::shared_ptr<WebSocketMessagingStubFactory> _wsMessagingStubFactory;

    ADD_LOGGER(JoynrClusterControllerRuntime)

private:
    template <typename T>
    std::string registerInternalSystemServiceProvider(std::shared_ptr<T> provider,
                                                      const std::string& participantId)
    {
        const std::string domain(_systemServicesSettings.getDomain());
        const std::string interfaceName(T::INTERFACE_NAME());

        _participantIdStorage->setProviderParticipantId(
                domain, interfaceName, T::MAJOR_VERSION, participantId);

        joynr::types::ProviderQos systemProviderQos;
        std::vector<joynr::types::CustomParameter> customParameters;
        customParameters.push_back(joynr::types::CustomParameter(
                std::string("___CC.InternalProvider___"), std::string("true")));
        systemProviderQos.setCustomParameters(customParameters);
        systemProviderQos.setPriority(1);
        systemProviderQos.setScope(joynr::types::ProviderScope::LOCAL);
        systemProviderQos.setSupportsOnChangeSubscriptions(false);

        return registerProvider(domain, provider, systemProviderQos);
    }

    void registerInternalSystemServiceProviders();
    void unregisterInternalSystemServiceProviders();
    void unregisterInternalSystemServiceProvider(const std::string& participantId);
    void startLocalCommunication();
    std::shared_ptr<joynr::infrastructure::GlobalDomainAccessControllerProxy>
    createGlobalDomainAccessControllerProxy();
    std::string getSerializedGlobalClusterControllerAddress() const;
    const system::RoutingTypes::Address& getGlobalClusterControllerAddress() const;

    DISALLOW_COPY_AND_ASSIGN(JoynrClusterControllerRuntime);
    std::shared_ptr<MulticastMessagingSkeletonDirectory> _multicastMessagingSkeletonDirectory;

    std::shared_ptr<CcMessageRouter> _ccMessageRouter;
    std::shared_ptr<AccessControlListEditor> _aclEditor;

    void enableAccessController(
            const std::map<std::string, types::DiscoveryEntryWithMetaInfo>& provisionedEntries);
    friend class ::JoynrClusterControllerRuntimeTest;

    Semaphore _lifetimeSemaphore;

    std::shared_ptr<joynr::AccessController> _accessController;
    std::shared_ptr<capabilities::Storage> _locallyRegisteredCapabilities;
    std::shared_ptr<capabilities::CachingStorage> _globalLookupCache;

    std::string _routingProviderParticipantId;
    std::string _discoveryProviderParticipantId;
    std::string _providerReregistrationControllerParticipantId;
    std::string _messageNotificationProviderParticipantId;
    std::string _accessControlListEditorProviderParticipantId;
    bool _isShuttingDown;
    const system::RoutingTypes::Address _dummyGlobalAddress;

    std::vector<std::string> _availableGbids;
    void fillAvailableGbidsVector();
};

} // namespace joynr
#endif // JOYNRCLUSTERCONTROLLERRUNTIME_H
