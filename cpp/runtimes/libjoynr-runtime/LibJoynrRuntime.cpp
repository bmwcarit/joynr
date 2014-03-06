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
#include "runtimes/libjoynr-runtime/LibJoynrRuntime.h"
#include "joynr/Dispatcher.h"
#include "joynr/InProcessDispatcher.h"
#include "common/dbus/DbusMessagingStubAdapter.h"
#include "libjoynr/dbus/DbusCapabilitiesStubAdapter.h"
#include "libjoynr/dbus/DbusMessagingEndpointAddress.h"
#include "libjoynr/dbus/DBusDispatcherAdapter.h"
#include "joynr/PublicationManager.h"
#include "joynr/SubscriptionManager.h"
#include "joynr/InProcessPublicationSender.h"
#include "joynr/JoynrMessageSender.h"
#include "common/dbus/DbusSettings.h"

#include "joynr/Util.h"

namespace joynr {

LibJoynrRuntime::LibJoynrRuntime(QSettings* settings):
    JoynrRuntime(),
    connectorFactory(NULL),
    publicationManager(NULL),
    subscriptionManager(NULL),
    inProcessPublicationSender(NULL),
    inProcessConnectorFactory(NULL),
    joynrMessagingConnectorFactory(NULL),
    joynrMessagingSendStub(NULL),
    joynrMessageSender(NULL),
    joynrDispatcher(NULL),
    inProcessDispatcher(NULL),
    joynrDispatcherAdapter(NULL),
    settings(settings),
    libjoynrSettings(NULL),
    dbusSettings(NULL)
{
    initializeAllDependencies();
}

LibJoynrRuntime::~LibJoynrRuntime() {
    delete proxyFactory;
    delete joynrCapabilitiesSendStub;
    delete inProcessDispatcher;
    delete capabilitiesRegistrar;
    delete joynrMessageSender;
    delete joynrDispatcher;
    delete joynrDispatcherAdapter;
    delete libjoynrSettings;
    delete dbusSettings;
    settings->clear();
    settings->deleteLater();
}

void LibJoynrRuntime::initializeAllDependencies() {
    assert(settings);
    libjoynrSettings = new LibjoynrSettings(*settings);
    libjoynrSettings->printSettings();
    dbusSettings = new DbusSettings(*settings);
    dbusSettings->printSettings();

    publicationManager = new PublicationManager();
    subscriptionManager = new SubscriptionManager();
    inProcessDispatcher = new InProcessDispatcher();

    // create messaging send stub
    QString ccMessagingAddress(dbusSettings->getClusterControllerMessagingAddress());
    joynrMessagingSendStub = QSharedPointer<IMessaging>(new DbusMessagingStubAdapter(ccMessagingAddress));
    joynrMessageSender = new JoynrMessageSender(joynrMessagingSendStub);
    joynrDispatcher = new Dispatcher(joynrMessageSender);
    joynrMessageSender->registerDispatcher(joynrDispatcher);

    // create capabilities send stub
    QString ccCapabilitiesAddress(dbusSettings->getClusterControllerCapabilitiesAddress());
    joynrCapabilitiesSendStub = new DbusCapabilitiesStubAdapter(ccCapabilitiesAddress);

    // register messaging skeleton using uuid
    QString messagingUuid = Util::createUuid().replace("-", "");
    QString libjoynrMessagingAddress("local:org.genivi.commonapi.joynr:libjoynr.messaging.id_" + messagingUuid);
    QSharedPointer<joynr::system::Address> libjoynrMessagingEndpoint(new DbusMessagingEndpointAddress(libjoynrMessagingAddress));
    joynrDispatcherAdapter = new DBusDispatcherAdapter(*joynrDispatcher, libjoynrMessagingAddress);

    inProcessPublicationSender = new InProcessPublicationSender(subscriptionManager);
    inProcessConnectorFactory = new InProcessConnectorFactory(subscriptionManager, publicationManager, inProcessPublicationSender);
    joynrMessagingConnectorFactory = new JoynrMessagingConnectorFactory(joynrMessageSender, subscriptionManager);

    connectorFactory = new ConnectorFactory(inProcessConnectorFactory, joynrMessagingConnectorFactory);

    proxyFactory = new ProxyFactory(joynrCapabilitiesSendStub, libjoynrMessagingEndpoint, connectorFactory, NULL);

    capabilitiesAggregator = QSharedPointer<CapabilitiesAggregator>(new CapabilitiesAggregator(joynrCapabilitiesSendStub, dynamic_cast<IRequestCallerDirectory*>(inProcessDispatcher)));

    // Set up the persistence file for storing provider participant ids
    QString persistenceFilename = libjoynrSettings->getParticipantIdsPersistenceFilename();
    QSharedPointer<ParticipantIdStorage> participantIdStorage(new ParticipantIdStorage(persistenceFilename));

    // initialize the dispatchers
    QList<IDispatcher *> dispatcherList;
    dispatcherList.append(inProcessDispatcher);
    dispatcherList.append(joynrDispatcher);

    capabilitiesRegistrar = new CapabilitiesRegistrar(dispatcherList, capabilitiesAggregator, libjoynrMessagingEndpoint, participantIdStorage);

    joynrDispatcher->registerPublicationManager(publicationManager);
    joynrDispatcher->registerSubscriptionManager(subscriptionManager);
}

void LibJoynrRuntime::unregisterCapability(QString participantId){
    assert(capabilitiesRegistrar);
    capabilitiesRegistrar->unregisterCapability(participantId);
}

LibJoynrRuntime *LibJoynrRuntime::create(QSettings* settings) {
    LibJoynrRuntime* runtime = new LibJoynrRuntime(settings);
    return runtime;
}

} // namespace joynr
