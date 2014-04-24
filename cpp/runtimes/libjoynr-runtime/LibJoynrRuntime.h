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

#ifndef LIBJOYNRRUNTIME_H
#define LIBJOYNRRUNTIME_H


#include "joynr/PrivateCopyAssign.h"

#include "joynr/JoynrClusterControllerExport.h"
#include "joynr/JoynrRuntime.h"

#include <QString>
#include <QSharedPointer>
#include <QSettings>
#include "joynr/LibjoynrSettings.h"
#include "joynr/SystemServicesSettings.h"
#include "joynr/ProxyBuilder.h"
#include "joynr/IMessaging.h"
#include "joynr/JoynrMessageSender.h"
#include "joynr/CapabilitiesRegistrar.h"
#include "common/dbus/DbusSettings.h"

namespace joynr {

class DBusMessageRouterAdapter;
class IMessaging;
class JoynrMessageSender;
class DbusSettings;
class MessageRouter;
class InProcessMessagingSkeleton;

class JOYNRCLUSTERCONTROLLERRUNTIME_EXPORT LibJoynrRuntime: public JoynrRuntime {
public:

    LibJoynrRuntime(QSettings* settings);

    static LibJoynrRuntime* create(QSettings* settings);

    virtual ~LibJoynrRuntime();

    void unregisterCapability(QString participantId);

private:
    DISALLOW_COPY_AND_ASSIGN(LibJoynrRuntime);

protected:
    ConnectorFactory* connectorFactory;
    PublicationManager* publicationManager;
    SubscriptionManager* subscriptionManager;
    InProcessPublicationSender* inProcessPublicationSender;
    InProcessConnectorFactory* inProcessConnectorFactory;
    JoynrMessagingConnectorFactory* joynrMessagingConnectorFactory;
    QSharedPointer<IMessaging> joynrMessagingSendStub;
    JoynrMessageSender* joynrMessageSender;
    IDispatcher* joynrDispatcher;
    IDispatcher* inProcessDispatcher;

    DBusMessageRouterAdapter* dbusMessageRouterAdapter;

    // take ownership, so a pointer is used
    QSettings* settings;
    // use pointer for settings object to check the configuration before initialization
    LibjoynrSettings* libjoynrSettings;
    DbusSettings* dbusSettings;

    Directory<QString, joynr::system::Address >* routingTable;
    QSharedPointer<InProcessMessagingSkeleton> dispatcherMessagingSkeleton;

    void initializeAllDependencies();
};


} // namespace joynr
#endif //LIBJOYNRRUNTIME_H
