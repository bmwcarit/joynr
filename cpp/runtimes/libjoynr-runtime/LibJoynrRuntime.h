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

#include <QtCore/QString>
#include <QtCore/QSharedPointer>
#include <QtCore/QSettings>

#include "joynr/PrivateCopyAssign.h"

#include "joynr/JoynrRuntime.h"

#include "joynr/LibjoynrSettings.h"
#include "joynr/SystemServicesSettings.h"
#include "joynr/ProxyBuilder.h"
#include "joynr/IMessaging.h"
#include "joynr/JoynrMessageSender.h"
#include "joynr/CapabilitiesRegistrar.h"
#include "runtimes/libjoynr-runtime/JoynrRuntimeExecutor.h"

namespace joynr {

class IMessaging;
class JoynrMessageSender;
class MessageRouter;
class InProcessMessagingSkeleton;
class IMiddlewareMessagingStubFactory;

class LibJoynrRuntime : public JoynrRuntime {
public:
    LibJoynrRuntime(QSettings* settings);
    virtual ~LibJoynrRuntime();

    template <class T>
    static LibJoynrRuntime* create(QSettings* settings);
    void unregisterCapability(QString participantId);

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

    // take ownership, so a pointer is used
    QSettings* settings;
    // use pointer for settings object to check the configuration before initialization
    LibjoynrSettings* libjoynrSettings;

    QSharedPointer<InProcessMessagingSkeleton> dispatcherMessagingSkeleton;

    virtual void startLibJoynrMessagingSkeleton(MessageRouter &messageRouter) = 0;

    void init(
            IMiddlewareMessagingStubFactory *middlewareMessagingStubFactory,
            QSharedPointer<joynr::system::Address> libjoynrMessagingAddress,
            QSharedPointer<joynr::system::Address> ccMessagingAddress
    );

private:
    DISALLOW_COPY_AND_ASSIGN(LibJoynrRuntime);
    JoynrRuntimeExecutor *runtimeExecutor;
    void setRuntimeExecutor(JoynrRuntimeExecutor *runtimeExecutor);
};

template <class T>
LibJoynrRuntime *LibJoynrRuntime::create(QSettings* settings) {
    JoynrRuntimeExecutor *runtimeExecutor = new JoynrRuntimeExecutor();
    LibJoynrRuntime *runtime = runtimeExecutor->create<T>(settings);
    runtime->setRuntimeExecutor(runtimeExecutor);
    return runtime;
}

} // namespace joynr
#endif //LIBJOYNRRUNTIME_H
