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

#ifndef JOYNRCLUSTERCONTROLLERRUNTIME_H
#define JOYNRCLUSTERCONTROLLERRUNTIME_H

#include <string>
#include <memory>

#include "joynr/PrivateCopyAssign.h"
#include "joynr/JoynrClusterControllerRuntimeExport.h"
#include "joynr/JoynrConfig.h"
#include "joynr/ProxyBuilder.h"
#include "joynr/ClientQCache.h"
#include "joynr/exceptions.h"
#include "joynr/MessageRouter.h"
#include "joynr/MessagingStubFactory.h"
#include "joynr/joynrlogging.h"
#include "joynr/LibjoynrSettings.h"
#include "joynr/JoynrRuntime.h"
#include "libjoynr/websocket/WebSocketSettings.h"

#include "joynr/RuntimeConfig.h"
#ifdef USE_DBUS_COMMONAPI_COMMUNICATION
#include "joynr/DBusMessageRouterAdapter.h"
#include "common/dbus/DbusSettings.h"
#endif // USE_DBUS_COMMONAPI_COMMUNICATION

class QCoreApplication;
class QString;
class JoynrClusterControllerRuntimeTest;

namespace joynr
{

class InProcessLibJoynrMessagingSkeleton;
class InProcessClusterControllerMessagingSkeleton;
class LocalCapabilitiesDirectory;
class ILocalChannelUrlDirectory;
class IMessageReceiver;
class IMessageSender;
class CapabilitiesClient;
class ICapabilitiesClient;
class PublicationManager;
class SubscriptionManager;
class InProcessDispatcher;
class ConnectorFactory;
class InProcessConnectorFactory;
class JoynrMessagingConnectorFactory;
class MessagingSettings;
class Dispatcher;
class InProcessPublicationSender;
class WebSocketCcMessagingSkeleton;
class IPlatformSecurityManager;

namespace infrastructure
{
class ChannelUrlDirectoryProxy;
}
template <typename Key, typename T>
class Directory;

class JOYNRCLUSTERCONTROLLERRUNTIME_EXPORT JoynrClusterControllerRuntime : public JoynrRuntime
{
public:
    JoynrClusterControllerRuntime(QCoreApplication* app,
                                  QSettings* settings,
                                  IMessageReceiver* messageReceiver = NULL,
                                  IMessageSender* = NULL);

    static JoynrClusterControllerRuntime* create(QSettings* settings);

    virtual ~JoynrClusterControllerRuntime();

    void unregisterProvider(const std::string& participantId);
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
    void initializeAllDependencies();
    virtual ConnectorFactory* createConnectorFactory(
            InProcessConnectorFactory* inProcessConnectorFactory,
            JoynrMessagingConnectorFactory* joynrMessagingConnectorFactory);

    IDispatcher* joynrDispatcher;
    IDispatcher* inProcessDispatcher;
    IDispatcher* ccDispatcher;
    SubscriptionManager* subscriptionManager;
    IMessaging* joynrMessagingSendSkeleton;
    JoynrMessageSender* joynrMessageSender;
    QCoreApplication* app;
    ICapabilitiesClient* capabilitiesClient;
    std::shared_ptr<LocalCapabilitiesDirectory> localCapabilitiesDirectory;
    QSharedPointer<ILocalChannelUrlDirectory> channelUrlDirectory;
    // Reason why CapabilitiesAggregator (CA) has to be a QSP:
    // CA has to be a member variable, because it is passed to ProxyBuilder in getProxyBuilder()
    // CA has to be a pointer instead of a reference, because it has to be initialised to NULL
    // (because other members are needed for its constructor)
    // CA is passed into different other classes, so ownership cannot be transferred.
    // => CA needs to be a QSP
    ClientQCache cache;
    // messageRouter must be shared pointer since it is also registered as
    // joynr::system::Routing provider and register capability expects shared pointer
    QSharedPointer<infrastructure::ChannelUrlDirectoryProxy> channelUrlDirectoryProxy;

    QSharedPointer<InProcessMessagingSkeleton> libJoynrMessagingSkeleton;

    QSharedPointer<IMessageReceiver> messageReceiver;
    QSharedPointer<IMessageSender> messageSender;

    QList<IDispatcher*> dispatcherList;
    InProcessConnectorFactory* inProcessConnectorFactory;
    InProcessPublicationSender* inProcessPublicationSender;
    JoynrMessagingConnectorFactory* joynrMessagingConnectorFactory;
    ConnectorFactory* connectorFactory;
    // take ownership, so a pointer is used
    QSettings* settings;
    // use pointer for settings object to check the configuration before initialization
    MessagingSettings* messagingSettings;
    LibjoynrSettings* libjoynrSettings;

#ifdef USE_DBUS_COMMONAPI_COMMUNICATION
    DbusSettings* dbusSettings;
    DBusMessageRouterAdapter* ccDbusMessageRouterAdapter;
#endif // USE_DBUS_COMMONAPI_COMMUNICATION
    WebSocketSettings wsSettings;
    WebSocketCcMessagingSkeleton* wsCcMessagingSkeleton;
    IPlatformSecurityManager* securityManager;

    static joynr_logging::Logger* logger;

private:
    DISALLOW_COPY_AND_ASSIGN(JoynrClusterControllerRuntime);

    friend class ::JoynrClusterControllerRuntimeTest;
};

} // namespace joynr
#endif // JOYNRCLUSTERCONTROLLERRUNTIME_H
