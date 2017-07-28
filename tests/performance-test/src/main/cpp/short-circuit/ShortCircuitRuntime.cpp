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

#include "joynr/CapabilitiesRegistrar.h"
#include "joynr/MessagingStubFactory.h"
#include "joynr/MessageSender.h"
#include "joynr/Dispatcher.h"
#include "joynr/InProcessMessagingAddress.h"
#include "libjoynr/in-process/InProcessMessagingStubFactory.h"
#include "libjoynr/in-process/InProcessMessagingSkeleton.h"
#include "joynr/SubscriptionManager.h"
#include "joynr/InProcessDispatcher.h"
#include "joynr/InProcessPublicationSender.h"
#include "joynr/MqttMulticastAddressCalculator.h"
#include "libjoynrclustercontroller/include/joynr/CcMessageRouter.h"

namespace joynr
{

ShortCircuitRuntime::ShortCircuitRuntime()
{
    auto messagingStubFactory = std::make_unique<MessagingStubFactory>();

    messagingStubFactory->registerStubFactory(std::make_unique<InProcessMessagingStubFactory>());

    const std::string multicastTopicPrefix = "";

    std::unique_ptr<IMulticastAddressCalculator> addressCalculator =
            std::make_unique<MqttMulticastAddressCalculator>(nullptr, multicastTopicPrefix);

    const std::string& globalClusterControllerAddress("globalAddress");

    messageRouter = std::make_shared<CcMessageRouter>(std::move(messagingStubFactory),
                                                      nullptr,
                                                      nullptr,
                                                      singleThreadedIOService.getIOService(),
                                                      std::move(addressCalculator),
                                                      globalClusterControllerAddress);

    messageSender = std::make_shared<MessageSender>(messageRouter);
    joynrDispatcher =
            std::make_shared<Dispatcher>(messageSender, singleThreadedIOService.getIOService());
    messageSender->registerDispatcher(joynrDispatcher);

    dispatcherMessagingSkeleton = std::make_shared<InProcessMessagingSkeleton>(joynrDispatcher);
    dispatcherAddress = std::make_shared<InProcessMessagingAddress>(dispatcherMessagingSkeleton);

    publicationManager =
            new PublicationManager(singleThreadedIOService.getIOService(), messageSender.get());
    subscriptionManager = std::make_shared<SubscriptionManager>(
            singleThreadedIOService.getIOService(), messageRouter);
    inProcessDispatcher =
            std::make_shared<InProcessDispatcher>(singleThreadedIOService.getIOService());

    inProcessPublicationSender = std::make_unique<InProcessPublicationSender>(subscriptionManager);
    auto inProcessConnectorFactory = std::make_unique<InProcessConnectorFactory>(
            subscriptionManager.get(),
            publicationManager,
            inProcessPublicationSender.get(),
            std::dynamic_pointer_cast<IRequestCallerDirectory>(inProcessDispatcher));
    auto joynrMessagingConnectorFactory =
            std::make_unique<JoynrMessagingConnectorFactory>(messageSender, subscriptionManager);
    auto connectorFactory = std::make_unique<ConnectorFactory>(
            std::move(inProcessConnectorFactory), std::move(joynrMessagingConnectorFactory));
    proxyFactory = std::make_unique<ProxyFactory>(std::move(connectorFactory));

    std::string persistenceFilename = "dummy.txt";
    participantIdStorage = std::make_shared<ParticipantIdStorage>(persistenceFilename);

    std::vector<std::shared_ptr<IDispatcher>> dispatcherList;
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
                                                    *publicationManager,
                                                    globalClusterControllerAddress);

    maximumTtlMs = std::chrono::milliseconds(std::chrono::hours(24) * 30).count();
}

} // namespace joynr
