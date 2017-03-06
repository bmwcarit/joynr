/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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

#include "ShortCircuitRuntime.h"

#include <chrono>
#include <limits>

#include "joynr/Util.h"
#include "joynr/CapabilitiesRegistrar.h"
#include "joynr/MessagingStubFactory.h"
#include "joynr/JoynrMessageSender.h"
#include "joynr/Dispatcher.h"
#include "joynr/InProcessMessagingAddress.h"
#include "libjoynr/in-process/InProcessMessagingStubFactory.h"
#include "libjoynr/in-process/InProcessLibJoynrMessagingSkeleton.h"
#include "joynr/SubscriptionManager.h"
#include "joynr/InProcessDispatcher.h"
#include "joynr/InProcessPublicationSender.h"
#include "joynr/system/RoutingTypes/WebSocketClientAddress.h"
#include "joynr/MqttMulticastAddressCalculator.h"
#include "cluster-controller/include/joynr/CcMessageRouter.h"

namespace joynr
{

ShortCircuitRuntime::ShortCircuitRuntime()
{
    std::string libjoynrMessagingId = "libjoynr.messaging.participantid_short-circuit-uuid";
    auto libjoynrMessagingAddress =
            std::make_shared<joynr::system::RoutingTypes::WebSocketClientAddress>(
                    libjoynrMessagingId);

    auto messagingStubFactory = std::make_unique<MessagingStubFactory>();

    messagingStubFactory->registerStubFactory(std::make_unique<InProcessMessagingStubFactory>());

    std::unique_ptr<IMulticastAddressCalculator> addressCalculator =
            std::make_unique<MqttMulticastAddressCalculator>(nullptr);

    messageRouter = std::make_shared<CcMessageRouter>(std::move(messagingStubFactory),
                                                      nullptr,
                                                      nullptr,
                                                      singleThreadedIOService.getIOService(),
                                                      std::move(addressCalculator));

    joynrMessageSender = std::make_shared<JoynrMessageSender>(messageRouter);
    joynrDispatcher = new Dispatcher(joynrMessageSender, singleThreadedIOService.getIOService());
    joynrMessageSender->registerDispatcher(joynrDispatcher);

    dispatcherMessagingSkeleton =
            std::make_shared<InProcessLibJoynrMessagingSkeleton>(joynrDispatcher);
    dispatcherAddress = std::make_shared<InProcessMessagingAddress>(dispatcherMessagingSkeleton);

    publicationManager = new PublicationManager(
            singleThreadedIOService.getIOService(), joynrMessageSender.get());
    subscriptionManager = std::make_shared<SubscriptionManager>(
            singleThreadedIOService.getIOService(), messageRouter);
    inProcessDispatcher = new InProcessDispatcher(singleThreadedIOService.getIOService());

    inProcessPublicationSender = std::make_unique<InProcessPublicationSender>(subscriptionManager);
    inProcessConnectorFactory = new InProcessConnectorFactory(
            subscriptionManager.get(),
            publicationManager,
            inProcessPublicationSender.get(),
            dynamic_cast<IRequestCallerDirectory*>(inProcessDispatcher));
    joynrMessagingConnectorFactory =
            new JoynrMessagingConnectorFactory(joynrMessageSender, subscriptionManager);

    auto connectorFactory = std::make_unique<ConnectorFactory>(
            inProcessConnectorFactory, joynrMessagingConnectorFactory);
    proxyFactory =
            std::make_unique<ProxyFactory>(libjoynrMessagingAddress, std::move(connectorFactory));

    std::string persistenceFilename = "dummy.txt";
    participantIdStorage = std::make_shared<ParticipantIdStorage>(persistenceFilename);

    std::vector<IDispatcher*> dispatcherList;
    dispatcherList.push_back(inProcessDispatcher);
    dispatcherList.push_back(joynrDispatcher);

    joynrDispatcher->registerPublicationManager(publicationManager);
    joynrDispatcher->registerSubscriptionManager(subscriptionManager);

    discoveryProxy = std::make_unique<DummyDiscovery>();
    capabilitiesRegistrar =
            std::make_unique<CapabilitiesRegistrar>(dispatcherList,
                                                    *discoveryProxy,
                                                    participantIdStorage,
                                                    dispatcherAddress,
                                                    messageRouter,
                                                    std::numeric_limits<std::int64_t>::max(),
                                                    *publicationManager);

    maximumTtlMs = std::chrono::milliseconds(std::chrono::hours(24) * 30).count();
}

} // namespace joynr
