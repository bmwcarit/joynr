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
#include "joynr/PrivateCopyAssign.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include "tests/utils/MockObjects.h"
#include "runtimes/cluster-controller-runtime/JoynrClusterControllerRuntime.h"
#include "joynr/tests/testProxy.h"
#include "joynr/types/ProviderQos.h"
#include "joynr/MessagingSettings.h"
#include "joynr/OnChangeSubscriptionQos.h"
#include "joynr/TypeUtil.h"
#include "joynr/tests/testAbstractProvider.h"
#include "joynr/LibjoynrSettings.h"

using namespace ::testing;
using namespace joynr;

ACTION_P(ReleaseSemaphore,semaphore)
{
    semaphore->notify();
}

static const std::string messagingPropertiesPersistenceFileName1(
        "End2EndSubscriptionTest-runtime1-joynr.settings");
static const std::string messagingPropertiesPersistenceFileName2(
        "End2EndSubscriptionTest-runtime2-joynr.settings");

namespace joynr {

class End2EndSubscriptionTest : public Test {
public:
    JoynrClusterControllerRuntime* runtime1;
    JoynrClusterControllerRuntime* runtime2;
    Settings settings1;
    Settings settings2;
    MessagingSettings messagingSettings1;
    MessagingSettings messagingSettings2;
    std::string baseUuid;
    std::string uuid;
    std::string domainName;
    joynr::Semaphore semaphore;
    unsigned long registerProviderWait;
    unsigned long subscribeToAttributeWait;
    joynr::types::Localisation::GpsLocation gpsLocation;

    End2EndSubscriptionTest() :
        runtime1(nullptr),
        runtime2(nullptr),
        settings1("test-resources/SystemIntegrationTest1.settings"),
        settings2("test-resources/SystemIntegrationTest2.settings"),
        messagingSettings1(settings1),
        messagingSettings2(settings2),
        baseUuid(util::createUuid()),
        uuid( "_" + baseUuid.substr(1, baseUuid.length()-2)),
        domainName("cppEnd2EndSubscriptionTest_Domain" + uuid),
        semaphore(0),
        registerProviderWait(1000),
        subscribeToAttributeWait(2000)

    {
        messagingSettings1.setMessagingPropertiesPersistenceFilename(
                    messagingPropertiesPersistenceFileName1);
        messagingSettings2.setMessagingPropertiesPersistenceFilename(
                    messagingPropertiesPersistenceFileName2);

        Settings* settings_1 = new Settings("test-resources/SystemIntegrationTest1.settings");
        Settings integration1Settings{"test-resources/libjoynrSystemIntegration1.settings"};
        Settings::merge(integration1Settings, *settings_1, false);

        runtime1 = new JoynrClusterControllerRuntime(nullptr, settings_1);

        Settings* settings_2 = new Settings("test-resources/SystemIntegrationTest2.settings");
        Settings integration2Settings{"test-resources/libjoynrSystemIntegration2.settings"};
        Settings::merge(integration2Settings, *settings_2, false);

        runtime2 = new JoynrClusterControllerRuntime(nullptr, settings_2);
    }

    void SetUp() {
        runtime1->start();
        runtime2->start();
    }

    void TearDown() {
        bool deleteChannel = true;
        runtime1->stop(deleteChannel);
        runtime2->stop(deleteChannel);

        // Delete the persisted participant ids so that each test uses different participant ids
        std::remove(LibjoynrSettings::DEFAULT_PARTICIPANT_IDS_PERSISTENCE_FILENAME().c_str());
    }

    /*
     *  This wait is necessary, because subcriptions are async, and a publication could occur
     * before the subscription has started.
     */
    void waitForAttributeSubscriptionArrivedAtProvider(
            std::shared_ptr<tests::testAbstractProvider> testProvider,
            const std::string& attributeName)
    {
        std::uint64_t delay = 0;

        while (testProvider->attributeListeners.find(attributeName) == testProvider->attributeListeners.cend()
               && delay <= subscribeToAttributeWait
        ) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            delay+=50;
        }

        EXPECT_FALSE(testProvider->attributeListeners.find(attributeName) == testProvider->attributeListeners.cend() ||
                    testProvider->attributeListeners.find(attributeName)->second.empty());
    }

    ~End2EndSubscriptionTest(){
        delete runtime1;
        delete runtime2;
    }

private:
    DISALLOW_COPY_AND_ASSIGN(End2EndSubscriptionTest);

protected:
    template <typename ChangeAttribute, typename SubscribeTo, typename T>
    void testOneShotAttributeSubscription(const T& expectedValue,
                                          SubscribeTo subscribeTo,
                                          ChangeAttribute setAttribute,
                                          const std::string& attributeName) {
        MockSubscriptionListenerOneType<T>* mockListener =
                new MockSubscriptionListenerOneType<T>();

        // Use a semaphore to count and wait on calls to the mock listener
        ON_CALL(*mockListener, onReceive(Eq(expectedValue)))
                .WillByDefault(ReleaseSemaphore(&semaphore));

        std::shared_ptr<ISubscriptionListener<T>> subscriptionListener(
                        mockListener);

        std::shared_ptr<tests::DefaulttestProvider> testProvider(new tests::DefaulttestProvider());
        runtime1->registerProvider<tests::testProvider>(domainName, testProvider);

        //This wait is necessary, because registerProvider is async, and a lookup could occur
        // before the register has finished.
        std::this_thread::sleep_for(std::chrono::milliseconds(registerProviderWait));

        (*testProvider.*setAttribute)(expectedValue, [](){}, [](const joynr::exceptions::ProviderRuntimeException&) {});
        ProxyBuilder<tests::testProxy>* testProxyBuilder
                = runtime2->createProxyBuilder<tests::testProxy>(domainName);
        DiscoveryQos discoveryQos;
        discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
        discoveryQos.setDiscoveryTimeout(1000);
        discoveryQos.setRetryInterval(250);

        std::int64_t qosRoundTripTTL = 500;

        // Send a message and expect to get a result
        tests::testProxy* testProxy = testProxyBuilder
                ->setMessagingQos(MessagingQos(qosRoundTripTTL))
                ->setCached(false)
                ->setDiscoveryQos(discoveryQos)
                ->build();

        std::int64_t minInterval_ms = 50;
        OnChangeSubscriptionQos subscriptionQos(
                    500000,   // validity_ms
                    minInterval_ms);  // minInterval_ms

        subscribeTo(testProxy, subscriptionListener, subscriptionQos);
        waitForAttributeSubscriptionArrivedAtProvider(testProvider, attributeName);

        // Wait for a subscription message to arrive
        ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));

        delete testProxyBuilder;
        delete testProxy;
    }
};

} // namespace joynr


TEST_F(End2EndSubscriptionTest, subscribeToEnumAttribute) {
    tests::testTypes::TestEnum::Enum expectedTestEnum = tests::testTypes::TestEnum::TWO;

    testOneShotAttributeSubscription(expectedTestEnum,
                                 [](tests::testProxy* testProxy,
                                    std::shared_ptr<ISubscriptionListener<tests::testTypes::TestEnum::Enum>> subscriptionListener,
                                    const OnChangeSubscriptionQos& subscriptionQos) {
                                    testProxy->subscribeToEnumAttribute(subscriptionListener, subscriptionQos);
                                 },
                                 &tests::testProvider::setEnumAttribute,
                                 "enumAttribute");
}

TEST_F(End2EndSubscriptionTest, subscribeToByteBufferAttribute) {
    joynr::ByteBuffer expectedByteBuffer {0,1,2,3,4,5,6,7,8,9,8,7,6,5,4,3,2,1,0};

    testOneShotAttributeSubscription(expectedByteBuffer,
                                 [](tests::testProxy* testProxy,
                                    std::shared_ptr<ISubscriptionListener<joynr::ByteBuffer>> subscriptionListener,
                                    const OnChangeSubscriptionQos& subscriptionQos) {
                                    testProxy->subscribeToByteBufferAttribute(subscriptionListener, subscriptionQos);
                                 },
                                 &tests::testProvider::setByteBufferAttribute,
                                 "byteBufferAttribute");

}
