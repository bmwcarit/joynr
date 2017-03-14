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

#include <string>
#include <memory>
#include <vector>

#include "cluster-controller/access-control/LocalDomainAccessController.h"
#include "cluster-controller/mqtt/MqttSettings.h"

#include "joynr/JoynrClusterControllerRuntimeExport.h"
#include "joynr/JoynrRuntime.h"
#include "joynr/LibjoynrSettings.h"
#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/RuntimeConfig.h"
#include "joynr/ClusterControllerSettings.h"

#include "libjoynr/websocket/WebSocketSettings.h"

#ifdef USE_DBUS_COMMONAPI_COMMUNICATION
#include "joynr/DBusMessageRouterAdapter.h"
#include "common/dbus/DbusSettings.h"
#endif // USE_DBUS_COMMONAPI_COMMUNICATION

class JoynrClusterControllerRuntimeTest;

namespace joynr
{

class LocalCapabilitiesDirectory;
class ILocalChannelUrlDirectory;
class IMessageReceiver;
class IMessageSender;
class SubscriptionManager;
class ConnectorFactory;
class InProcessConnectorFactory;
class JoynrMessagingConnectorFactory;
class IDispatcher;
class InProcessPublicationSender;
class InProcessMessagingSkeleton;
class HttpMessagingSkeleton;
class MqttMessagingSkeleton;
class MulticastMessagingSkeletonDirectory;
class IPlatformSecurityManager;
class Settings;
class JoynrMessageSender;
class IMessaging;
class CcMessageRouter;
class WebSocketMessagingStubFactory;
class MosquittoConnection;

namespace infrastructure
{
class ChannelUrlDirectoryProxy;
} // namespace infrastructure

class JOYNRCLUSTERCONTROLLERRUNTIME_EXPORT JoynrClusterControllerRuntime : public JoynrRuntime
{
public:
    JoynrClusterControllerRuntime(std::unique_ptr<Settings> settings,
                                  std::shared_ptr<IMessageReceiver> httpMessageReceiver = nullptr,
                                  std::shared_ptr<IMessageSender> httpMessageSender = nullptr,
                                  std::shared_ptr<IMessageReceiver> mqttMessageReceiver = nullptr,
                                  std::shared_ptr<IMessageSender> mqttMessageSender = nullptr);

    static std::unique_ptr<JoynrClusterControllerRuntime> create(
            std::unique_ptr<Settings> settings,
            const std::string& discoveryEntriesFile = "");

    ~JoynrClusterControllerRuntime() override;

    void start();
    void stop(bool deleteChannel = false);

    void runForever();

    // Functions used by integration tests
    void startMessaging();
    void stopMessaging();
    void deleteChannel();
    void registerRoutingProvider();
    void registerDiscoveryProvider();
    void registerMessageNotificationProvider();

    /*
     * Inject predefined capabilities stored in a JSON file.
     */
    void injectGlobalCapabilitiesFromFile(const std::string& fileName);

protected:
    void importMessageRouterFromFile();
    void initializeAllDependencies();
    void importPersistedLocalCapabilitiesDirectory();

    std::map<std::string, joynr::types::DiscoveryEntryWithMetaInfo> getProvisionedEntries()
            const final;

    std::shared_ptr<IMessageRouter> getMessageRouter() final;

    IDispatcher* joynrDispatcher;
    IDispatcher* inProcessDispatcher;

    std::shared_ptr<SubscriptionManager> subscriptionManager;
    IMessaging* joynrMessagingSendSkeleton;
    std::shared_ptr<JoynrMessageSender> joynrMessageSender;

    std::shared_ptr<LocalCapabilitiesDirectory> localCapabilitiesDirectory;

    std::shared_ptr<InProcessMessagingSkeleton> libJoynrMessagingSkeleton;

    std::shared_ptr<IMessageReceiver> httpMessageReceiver;
    std::shared_ptr<IMessageSender> httpMessageSender;
    std::shared_ptr<HttpMessagingSkeleton> httpMessagingSkeleton;

    std::shared_ptr<MosquittoConnection> mosquittoConnection;
    std::shared_ptr<IMessageReceiver> mqttMessageReceiver;
    std::shared_ptr<IMessageSender> mqttMessageSender;
    std::shared_ptr<MqttMessagingSkeleton> mqttMessagingSkeleton;

    std::vector<IDispatcher*> dispatcherList;
    InProcessPublicationSender* inProcessPublicationSender;

    std::unique_ptr<Settings> settings;
    LibjoynrSettings libjoynrSettings;
    std::unique_ptr<LocalDomainAccessController> localDomainAccessController;
    ClusterControllerSettings clusterControllerSettings;

#ifdef USE_DBUS_COMMONAPI_COMMUNICATION
    DbusSettings* dbusSettings;
    DBusMessageRouterAdapter* ccDbusMessageRouterAdapter;
#endif // USE_DBUS_COMMONAPI_COMMUNICATION
    WebSocketSettings wsSettings;
    std::shared_ptr<IMessaging> wsCcMessagingSkeleton;
    std::shared_ptr<IMessaging> wsTLSCcMessagingSkeleton;
    bool httpMessagingIsRunning;
    bool mqttMessagingIsRunning;
    bool doMqttMessaging;
    bool doHttpMessaging;
    std::shared_ptr<WebSocketMessagingStubFactory> wsMessagingStubFactory;

    ADD_LOGGER(JoynrClusterControllerRuntime);

private:
    void createWsCCMessagingSkeletons();

    DISALLOW_COPY_AND_ASSIGN(JoynrClusterControllerRuntime);
    MqttSettings mqttSettings;
    std::shared_ptr<MulticastMessagingSkeletonDirectory> multicastMessagingSkeletonDirectory;

    std::shared_ptr<CcMessageRouter> ccMessageRouter;

    void enableAccessController(
            MessagingSettings& messagingSettings,
            const std::map<std::string, types::DiscoveryEntryWithMetaInfo>& provisionedEntries);
    friend class ::JoynrClusterControllerRuntimeTest;
};

} // namespace joynr
#endif // JOYNRCLUSTERCONTROLLERRUNTIME_H
