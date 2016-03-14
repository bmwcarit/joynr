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

#ifndef JOYNRCLUSTERCONTROLLERRUNTIME_H
#define JOYNRCLUSTERCONTROLLERRUNTIME_H

#include <string>
#include <memory>
#include <vector>

#include "cluster-controller/mqtt/MqttSettings.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/JoynrClusterControllerRuntimeExport.h"
#include "joynr/ClientQCache.h"
#include "joynr/Logger.h"
#include "joynr/JoynrRuntime.h"
#include "libjoynr/websocket/WebSocketSettings.h"
#include "joynr/MessagingSettings.h"
#include "joynr/LibjoynrSettings.h"

#include "joynr/RuntimeConfig.h"
#ifdef USE_DBUS_COMMONAPI_COMMUNICATION
#include "joynr/DBusMessageRouterAdapter.h"
#include "common/dbus/DbusSettings.h"
#endif // USE_DBUS_COMMONAPI_COMMUNICATION

class QCoreApplication;
class JoynrClusterControllerRuntimeTest;

namespace joynr
{

class LocalCapabilitiesDirectory;
class ILocalChannelUrlDirectory;
class IMessageReceiver;
class IMessageSender;
class ICapabilitiesClient;
class SubscriptionManager;
class ConnectorFactory;
class InProcessConnectorFactory;
class JoynrMessagingConnectorFactory;
class IDispatcher;
class InProcessPublicationSender;
class WebSocketCcMessagingSkeleton;
class InProcessMessagingSkeleton;
class HttpMessagingSkeleton;
class MqttMessagingSkeleton;
class IPlatformSecurityManager;
class Settings;
class JoynrMessageSender;
class IMessaging;

namespace infrastructure
{
class ChannelUrlDirectoryProxy;
} // namespace infrastructure

class JOYNRCLUSTERCONTROLLERRUNTIME_EXPORT JoynrClusterControllerRuntime : public JoynrRuntime
{
public:
    JoynrClusterControllerRuntime(QCoreApplication* app,
                                  Settings* settings,
                                  IMessageReceiver* httpMessageReceiver = nullptr,
                                  IMessageSender* = nullptr,
                                  IMessageReceiver* mqttMessageReceiver = nullptr,
                                  IMessageSender* = nullptr);

    static JoynrClusterControllerRuntime* create(Settings* settings);

    ~JoynrClusterControllerRuntime() override;

    void unregisterProvider(const std::string& participantId) override;
    void start();
    void stop(bool deleteChannel = false);

    void runForever();

    // Functions used by integration tests
    void startMessaging();
    void stopMessaging();
    void waitForChannelCreation();
    void deleteChannel();
    void registerRoutingProvider();
    void registerDiscoveryProvider();

protected:
    void importMessageRouterFromFile();
    void initializeAllDependencies();
    void importPersistedLocalCapabilitiesDirectory();

    IDispatcher* joynrDispatcher;
    IDispatcher* inProcessDispatcher;
    IDispatcher* ccDispatcher;
    SubscriptionManager* subscriptionManager;
    IMessaging* joynrMessagingSendSkeleton;
    JoynrMessageSender* joynrMessageSender;
    QCoreApplication* app;
    ICapabilitiesClient* capabilitiesClient;
    std::shared_ptr<LocalCapabilitiesDirectory> localCapabilitiesDirectory;
    std::shared_ptr<ILocalChannelUrlDirectory> channelUrlDirectory;
    ClientQCache cache;
    std::shared_ptr<infrastructure::ChannelUrlDirectoryProxy> channelUrlDirectoryProxy;

    std::shared_ptr<InProcessMessagingSkeleton> libJoynrMessagingSkeleton;

    std::shared_ptr<HttpMessagingSkeleton> httpMessagingSkeleton;
    std::shared_ptr<MqttMessagingSkeleton> mqttMessagingSkeleton;

    std::shared_ptr<IMessageReceiver> httpMessageReceiver;
    std::shared_ptr<IMessageSender> httpMessageSender;

    std::shared_ptr<IMessageReceiver> mqttMessageReceiver;
    std::shared_ptr<IMessageSender> mqttMessageSender;

    std::vector<IDispatcher*> dispatcherList;
    InProcessConnectorFactory* inProcessConnectorFactory;
    InProcessPublicationSender* inProcessPublicationSender;
    JoynrMessagingConnectorFactory* joynrMessagingConnectorFactory;
    ConnectorFactory* connectorFactory;
    // take ownership, so a pointer is used
    Settings* settings;
    LibjoynrSettings libjoynrSettings;

#ifdef USE_DBUS_COMMONAPI_COMMUNICATION
    DbusSettings* dbusSettings;
    DBusMessageRouterAdapter* ccDbusMessageRouterAdapter;
#endif // USE_DBUS_COMMONAPI_COMMUNICATION
    WebSocketSettings wsSettings;
    WebSocketCcMessagingSkeleton* wsCcMessagingSkeleton;
    bool httpMessagingIsRunning;
    bool mqttMessagingIsRunning;
    bool doMqttMessaging;
    bool doHttpMessaging;

    ADD_LOGGER(JoynrClusterControllerRuntime);

private:
    DISALLOW_COPY_AND_ASSIGN(JoynrClusterControllerRuntime);
    MqttSettings mqttSettings;

    friend class ::JoynrClusterControllerRuntimeTest;
};

} // namespace joynr
#endif // JOYNRCLUSTERCONTROLLERRUNTIME_H
