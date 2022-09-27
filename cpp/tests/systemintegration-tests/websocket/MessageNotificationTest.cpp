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

#include "tests/utils/Gmock.h"
#include "tests/utils/Gtest.h"

#include "joynr/OnChangeSubscriptionQos.h"
#include "joynr/Semaphore.h"
#include "joynr/Settings.h"
#include "joynr/SystemServicesSettings.h"

#include "joynr/system/MessageNotificationMessageQueuedForDeliveryBroadcastFilterParameters.h"
#include "joynr/system/MessageNotificationProxy.h"
#include "joynr/tests/DefaulttestProvider.h"
#include "joynr/tests/testProxy.h"
#include "joynr/types/ProviderQos.h"

#include "tests/JoynrTest.h"
#include "tests/mock/TestJoynrClusterControllerRuntime.h"
#include "tests/utils/TestLibJoynrWebSocketRuntime.h"

using namespace ::testing;
using namespace joynr;

class MessageNotificationTest : public ::testing::Test
{
public:
    MessageNotificationTest()
            : clusterControllerRuntime(),
              libjoynrProviderRuntime(),
              libjoynrProxyRuntime(),
              testDomain("testDomain"),
              settingsPath("test-resources/websocket-cc-tls.settings"),
              semaphore()
    {
        clusterControllerRuntime = std::make_shared<TestJoynrClusterControllerRuntime>(
                std::make_unique<Settings>(settingsPath), failOnFatalRuntimeError);
        clusterControllerRuntime->init();
        clusterControllerRuntime->start();
    }

    void SetUp() override
    {
        libjoynrProviderRuntime = std::make_shared<TestLibJoynrWebSocketRuntime>(
                std::make_unique<Settings>("test-resources/libjoynrSystemIntegration1.settings"));
        ASSERT_TRUE(libjoynrProviderRuntime->connect(std::chrono::milliseconds(2000)));

        libjoynrProxyRuntime = std::make_shared<TestLibJoynrWebSocketRuntime>(
                std::make_unique<Settings>("test-resources/libjoynrSystemIntegration2.settings"));
        ASSERT_TRUE(libjoynrProxyRuntime->connect(std::chrono::milliseconds(2000)));
    }

    ~MessageNotificationTest() override
    {
        if (clusterControllerRuntime) {
            clusterControllerRuntime->stop();
            clusterControllerRuntime->shutdown();
        }

        if (libjoynrProxyRuntime) {
            libjoynrProxyRuntime->shutdown();
            libjoynrProxyRuntime.reset();
        }

        if (libjoynrProviderRuntime) {
            libjoynrProviderRuntime->shutdown();
            libjoynrProviderRuntime.reset();
        }
        clusterControllerRuntime.reset();

        // Delete persisted files
        std::remove(ClusterControllerSettings::
                            DEFAULT_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCE_FILENAME()
                                    .c_str());
        std::remove(LibjoynrSettings::DEFAULT_PARTICIPANT_IDS_PERSISTENCE_FILENAME().c_str());
    }

protected:
    std::shared_ptr<TestJoynrClusterControllerRuntime> clusterControllerRuntime;
    std::shared_ptr<TestLibJoynrWebSocketRuntime> libjoynrProviderRuntime;
    std::shared_ptr<TestLibJoynrWebSocketRuntime> libjoynrProxyRuntime;

    const std::string testDomain;
    std::string settingsPath;
    Semaphore semaphore;
};

class MockSubscriptionListener : public joynr::ISubscriptionListener<std::string, std::string>
{
public:
    ~MockSubscriptionListener() override = default;
    MOCK_METHOD1(onSubscribed, void(const std::string& subscriptionId));
    MOCK_METHOD2(onReceive, void(const std::string& participantId, const std::string& messageType));
    MOCK_METHOD1(onError, void(const joynr::exceptions::JoynrRuntimeException&));
};

TEST_F(MessageNotificationTest, messageToDisconnectedProviderCausesBroadcast)
{

    // 1. register provider
    auto testProvider = std::make_shared<tests::DefaulttestProvider>();
    joynr::types::ProviderQos providerQos;
    providerQos.setScope(joynr::types::ProviderScope::LOCAL);
    std::string providerParticipantId =
            libjoynrProviderRuntime->registerProvider<tests::testProvider>(
                    testDomain, testProvider, providerQos);

    // 2. create proxy
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::LAST_SEEN);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::LOCAL_ONLY);
    MessagingQos messagingQos;
    messagingQos.setTtl(5000);

    auto testProxyBuilder = libjoynrProxyRuntime->createProxyBuilder<tests::testProxy>(testDomain);

    auto testProxy =
            testProxyBuilder->setMessagingQos(messagingQos)->setDiscoveryQos(discoveryQos)->build();

    Settings settings(settingsPath);
    SystemServicesSettings systemSettings(settings);
    auto messageNotificationProxyBuilder =
            libjoynrProxyRuntime->createProxyBuilder<joynr::system::MessageNotificationProxy>(
                    systemSettings.getDomain());

    DiscoveryQos messagingNotificationDiscoveryQos;
    messagingNotificationDiscoveryQos.setArbitrationStrategy(
            DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);
    messagingNotificationDiscoveryQos.addCustomParameter(
            "fixedParticipantId", systemSettings.getCcMessageNotificationProviderParticipantId());
    messagingNotificationDiscoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::LOCAL_ONLY);

    auto messageNotificationProxy = messageNotificationProxyBuilder->setMessagingQos(messagingQos)
                                            ->setDiscoveryQos(messagingNotificationDiscoveryQos)
                                            ->build();

    // 3. subscribe to message notification broadcast
    auto mockListener = std::make_shared<MockSubscriptionListener>();
    EXPECT_CALL(*mockListener, onSubscribed(_)).Times(1);
    EXPECT_CALL(*mockListener, onError(_)).Times(0);

    joynr::system::MessageNotificationMessageQueuedForDeliveryBroadcastFilterParameters
            filterParameters;
    filterParameters.setParticipantId(providerParticipantId);

    auto future = messageNotificationProxy->subscribeToMessageQueuedForDeliveryBroadcast(
            filterParameters, mockListener, std::make_shared<OnChangeSubscriptionQos>());
    std::string subscriptionId;
    future->get(subscriptionId);

    // 4. disconnect provider runtime
    libjoynrProviderRuntime->shutdown();
    libjoynrProviderRuntime.reset();

    // 5. execute call on proxy while provider is not connected to cluster-controller

    auto onSuccess = [](const std::int32_t&) { FAIL(); };

    auto onError = [](const joynr::exceptions::JoynrRuntimeException&) {};

    EXPECT_CALL(*mockListener, onReceive(Eq(providerParticipantId), _))
            .WillOnce(ReleaseSemaphore(&semaphore));

    testProxy->addNumbersAsync(1, 2, 3, onSuccess, onError);

    // 6. wait for a broadcast message to arrive
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));
    messageNotificationProxy->unsubscribeFromMessageQueuedForDeliveryBroadcast(subscriptionId);
}
