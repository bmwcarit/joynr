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
#include "joynr/Dispatcher.h"
#include "joynr/IKeychain.h"
#include "joynr/InProcessMessagingAddress.h"
#include "joynr/MessageQueue.h"
#include "joynr/MessageSender.h"
#include "joynr/MessagingStubFactory.h"
#include "joynr/MqttMulticastAddressCalculator.h"
#include "joynr/Settings.h"
#include "joynr/SubscriptionManager.h"

#include "libjoynr/in-process/InProcessMessagingSkeleton.h"
#include "libjoynr/in-process/InProcessMessagingStubFactory.h"

#include "libjoynrclustercontroller/include/joynr/CcMessageRouter.h"

namespace joynr
{

class ITransportStatus;

ShortCircuitRuntime::ShortCircuitRuntime(std::unique_ptr<Settings> settings,
                                         std::shared_ptr<IKeychain> keyChain)
        : JoynrRuntimeImpl(*settings, [](const exceptions::JoynrRuntimeException&) {}),
          _keyChain(std::move(keyChain)),
          _clusterControllerSettings(*settings),
          _ownAddress()
{
    auto messagingStubFactory = std::make_unique<MessagingStubFactory>();
    _requestCallerDirectory = std::make_shared<DummyRequestCallerDirectory>();

    messagingStubFactory->registerStubFactory(std::make_unique<InProcessMessagingStubFactory>());

    const std::string multicastTopicPrefix = "";

    fillAvailableGbidsVector(_messagingSettings);

    std::unique_ptr<IMulticastAddressCalculator> addressCalculator =
            std::make_unique<MqttMulticastAddressCalculator>(multicastTopicPrefix, _availableGbids);

    const std::string& globalClusterControllerAddress("globalAddress");
    const std::string messageNotificationProviderParticipantId(
            "messageNotificationProviderParticipantId");

    _messageRouter = std::make_shared<CcMessageRouter>(
            _messagingSettings,
            _clusterControllerSettings,
            std::move(messagingStubFactory),
            nullptr,
            nullptr,
            _singleThreadedIOService.getIOService(),
            std::move(addressCalculator),
            globalClusterControllerAddress,
            messageNotificationProviderParticipantId,
            std::vector<std::shared_ptr<ITransportStatus>>{},
            std::make_unique<MessageQueue<std::string>>(),
            std::make_unique<MessageQueue<std::shared_ptr<ITransportStatus>>>(),
            _ownAddress,
            _availableGbids);

    _messageSender = std::make_shared<MessageSender>(_messageRouter, _keyChain);
    _joynrDispatcher =
            std::make_shared<Dispatcher>(_messageSender, _singleThreadedIOService.getIOService());
    _messageSender->registerDispatcher(_joynrDispatcher);

    _dispatcherMessagingSkeleton = std::make_shared<InProcessMessagingSkeleton>(_joynrDispatcher);
    _dispatcherAddress = std::make_shared<InProcessMessagingAddress>(_dispatcherMessagingSkeleton);

    _publicationManager = std::make_shared<PublicationManager>(
            _singleThreadedIOService.getIOService(), _messageSender);
    _subscriptionManager = std::make_shared<SubscriptionManager>(
            _singleThreadedIOService.getIOService(), _messageRouter);

    auto joynrMessagingConnectorFactory =
            std::make_unique<JoynrMessagingConnectorFactory>(_messageSender, _subscriptionManager);
    _proxyFactory = std::make_unique<ProxyFactory>(std::move(joynrMessagingConnectorFactory));

    std::string persistenceFilename = "dummy.txt";
    _participantIdStorage = std::make_shared<ParticipantIdStorage>(persistenceFilename);

    _joynrDispatcher->registerPublicationManager(_publicationManager);
    _joynrDispatcher->registerSubscriptionManager(_subscriptionManager);

    _discoveryProxy = std::make_shared<DummyDiscovery>();
    _capabilitiesRegistrar =
            std::make_unique<CapabilitiesRegistrar>(_joynrDispatcher,
                                                    _discoveryProxy,
                                                    _participantIdStorage,
                                                    _dispatcherAddress,
                                                    _messageRouter,
                                                    std::numeric_limits<std::int64_t>::max(),
                                                    _publicationManager,
                                                    globalClusterControllerAddress);

    _maximumTtlMs = std::chrono::milliseconds(std::chrono::hours(24) * 30).count();
}

void ShortCircuitRuntime::fillAvailableGbidsVector(const MessagingSettings& messagingSettings)
{
    _availableGbids.emplace_back(messagingSettings.getGbid());

    std::uint8_t additionalBackends = messagingSettings.getAdditionalBackendsCount();
    for (std::uint8_t index = 0; index < additionalBackends; index++) {
        _availableGbids.emplace_back(messagingSettings.getAdditionalBackendGbid(index));
    }
}

} // namespace joynr
