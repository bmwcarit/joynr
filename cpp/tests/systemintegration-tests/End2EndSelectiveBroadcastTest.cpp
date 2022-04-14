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
#include "End2EndBroadcastTestBase.cpp"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/tests/testProxy.h"
#include "joynr/OnChangeSubscriptionQos.h"
#include "joynr/tests/TestBroadcastWithFilteringBroadcastFilter.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/Future.h"

#include "tests/JoynrTest.h"

using namespace ::testing;
using namespace joynr;

namespace joynr
{

class End2EndSelectiveBroadcastTest : public End2EndBroadcastTestBase
{
public:
    End2EndSelectiveBroadcastTest() : End2EndBroadcastTestBase()
    {
    }

private:
    DISALLOW_COPY_AND_ASSIGN(End2EndSelectiveBroadcastTest);
};

} // namespace joynr

class MockTestBroadcastWithFilteringBroadcastFilter
        : public joynr::tests::TestBroadcastWithFilteringBroadcastFilter
{
public:
    MOCK_METHOD6(
            filter,
            bool(const std::string& stringOut,
                 const std::vector<std::string>& stringArrayOut,
                 const std::vector<joynr::tests::testTypes::TestEnum::Enum>& enumerationArrayOut,
                 const joynr::types::TestTypes::TEverythingStruct& structWithStringArrayOut,
                 const std::vector<joynr::types::TestTypes::TEverythingStruct>&
                         structWithStringArrayArrayOut,
                 const joynr::tests::TestBroadcastWithFilteringBroadcastFilterParameters&
                         filterParameters));
};

TEST_P(End2EndSelectiveBroadcastTest, subscribeToBroadcastWithFiltering)
{
    std::string stringOut = "expectedString";
    std::vector<std::string> stringArrayOut{stringOut};
    std::vector<joynr::tests::testTypes::TestEnum::Enum> enumerationArrayOut = {
            joynr::tests::testTypes::TestEnum::TWO};
    joynr::types::TestTypes::TEverythingStruct structWithStringArrayOut;
    std::vector<joynr::types::TestTypes::TEverythingStruct> structWithStringArrayArrayOut{
            structWithStringArrayOut};

    MockSubscriptionListenerFiveTypes<std::string,
                                      std::vector<std::string>,
                                      std::vector<joynr::tests::testTypes::TestEnum::Enum>,
                                      joynr::types::TestTypes::TEverythingStruct,
                                      std::vector<joynr::types::TestTypes::TEverythingStruct>>*
            mockListener = new MockSubscriptionListenerFiveTypes<
                    std::string,
                    std::vector<std::string>,
                    std::vector<joynr::tests::testTypes::TestEnum::Enum>,
                    joynr::types::TestTypes::TEverythingStruct,
                    std::vector<joynr::types::TestTypes::TEverythingStruct>>();

    // Use a semaphore to count and wait on calls to the mock listener
    ON_CALL(*mockListener,
            onReceive(Eq(stringOut),
                      Eq(stringArrayOut),
                      Eq(enumerationArrayOut),
                      Eq(structWithStringArrayOut),
                      Eq(structWithStringArrayArrayOut)))
            .WillByDefault(ReleaseSemaphore(&semaphore));

    std::shared_ptr<ISubscriptionListener<std::string,
                                          std::vector<std::string>,
                                          std::vector<joynr::tests::testTypes::TestEnum::Enum>,
                                          joynr::types::TestTypes::TEverythingStruct,
                                          std::vector<joynr::types::TestTypes::TEverythingStruct>>>
            subscriptionListener(mockListener);

    auto filter = std::make_shared<MockTestBroadcastWithFilteringBroadcastFilter>();
    ON_CALL(*filter,
            filter(Eq(stringOut),
                   Eq(stringArrayOut),
                   Eq(enumerationArrayOut),
                   Eq(structWithStringArrayOut),
                   Eq(structWithStringArrayArrayOut),
                   _)).WillByDefault(DoAll(ReleaseSemaphore(&altSemaphore), Return(true)));

    testOneShotBroadcastSubscriptionWithFiltering(
            subscriptionListener,
            [this](tests::testProxy* testProxy,
                   std::shared_ptr<joynr::ISubscriptionListener<
                           std::string,
                           std::vector<std::string>,
                           std::vector<joynr::tests::testTypes::TestEnum::Enum>,
                           joynr::types::TestTypes::TEverythingStruct,
                           std::vector<joynr::types::TestTypes::TEverythingStruct>>>
                           subscriptionListener,
                   std::shared_ptr<OnChangeSubscriptionQos> subscriptionQos,
                   std::string& subscriptionId) {
                joynr::tests::TestBroadcastWithFilteringBroadcastFilterParameters filterParameters;
                std::shared_ptr<Future<std::string>> subscriptionIdFuture =
                        testProxy->subscribeToBroadcastWithFilteringBroadcast(
                                filterParameters, subscriptionListener, subscriptionQos);
                JOYNR_EXPECT_NO_THROW(
                        subscriptionIdFuture->get(subscribeToBroadcastWait, subscriptionId));
            },
            [](tests::testProxy* testProxy, std::string& subscriptionId) {
                testProxy->unsubscribeFromBroadcastWithFilteringBroadcast(subscriptionId);
            },
            &tests::testProvider::fireBroadcastWithFiltering,
            filter,
            stringOut,
            stringArrayOut,
            enumerationArrayOut,
            structWithStringArrayOut,
            structWithStringArrayArrayOut);

    // Wait for a subscription message to arrive
    ASSERT_TRUE(altSemaphore.waitFor(std::chrono::seconds(3)));
}

TEST_P(End2EndSelectiveBroadcastTest, subscribeToSelectiveBroadcast_FilterSuccess)
{

    MockGpsSubscriptionListener* mockListener = new MockGpsSubscriptionListener();

    // Use a semaphore to count and wait on calls to the mock listener
    EXPECT_CALL(*mockListener, onReceive(Eq(gpsLocation2))).WillOnce(ReleaseSemaphore(&semaphore));

    EXPECT_CALL(*mockListener, onReceive(Eq(gpsLocation3))).WillOnce(ReleaseSemaphore(&semaphore));

    EXPECT_CALL(*mockListener, onReceive(Eq(gpsLocation4))).WillOnce(ReleaseSemaphore(&semaphore));

    std::shared_ptr<ISubscriptionListener<types::Localisation::GpsLocation>> subscriptionListener(
            mockListener);

    ON_CALL(*filter, filter(_, Eq(filterParameters))).WillByDefault(Return(true));

    std::shared_ptr<MyTestProvider> testProvider = registerProvider();
    testProvider->addBroadcastFilter(filter);

    std::shared_ptr<tests::testProxy> testProxy = buildProxy();

    std::int64_t minInterval_ms = 50;
    auto subscriptionQos =
            std::make_shared<OnChangeSubscriptionQos>(500000,          // validity_ms
                                                      1000,            // publication ttl
                                                      minInterval_ms); // minInterval_ms

    std::shared_ptr<joynr::Future<std::string>> subscriptionBroadcastResult =
            testProxy->subscribeToLocationUpdateSelectiveBroadcast(
                    filterParameters, subscriptionListener, subscriptionQos);
    std::string subscriptionId;
    JOYNR_EXPECT_NO_THROW(
            subscriptionBroadcastResult->get(subscribeToBroadcastWait, subscriptionId));

    // Change the location 3 times

    testProvider->fireLocationUpdateSelective(gpsLocation2);

    // Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));

    // Waiting between broadcast occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms));

    testProvider->fireLocationUpdateSelective(gpsLocation3);

    // Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));

    // Waiting between broadcast occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms));

    testProvider->fireLocationUpdateSelective(gpsLocation4);

    // Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));
    JOYNR_EXPECT_NO_THROW(
            testProxy->unsubscribeFromLocationUpdateSelectiveBroadcast(subscriptionId));
}

TEST_P(End2EndSelectiveBroadcastTest, subscribeToSelectiveBroadcast_FilterFail)
{

    std::shared_ptr<MockGpsSubscriptionListener> mockSubscriptionListener =
            std::make_shared<MockGpsSubscriptionListener>();

    EXPECT_CALL(*mockSubscriptionListener, onReceive(A<const types::Localisation::GpsLocation&>()))
            .Times(0);

    ON_CALL(*filter, filter(_, Eq(filterParameters))).WillByDefault(Return(false));

    std::shared_ptr<MyTestProvider> testProvider = registerProvider();
    testProvider->addBroadcastFilter(filter);

    std::shared_ptr<tests::testProxy> testProxy = buildProxy();

    std::int64_t minInterval_ms = 50;
    auto subscriptionQos =
            std::make_shared<OnChangeSubscriptionQos>(500000,          // validity_ms
                                                      1000,            // publication ttl
                                                      minInterval_ms); // minInterval_ms

    std::shared_ptr<joynr::Future<std::string>> subscriptionBroadcastResult =
            testProxy->subscribeToLocationUpdateSelectiveBroadcast(
                    filterParameters, mockSubscriptionListener, subscriptionQos);
    std::string subscriptionId;
    JOYNR_EXPECT_NO_THROW(
            subscriptionBroadcastResult->get(subscribeToBroadcastWait, subscriptionId));

    // Change the location 3 times

    testProvider->fireLocationUpdate(gpsLocation2);

    // Waiting between broadcast occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms));

    testProvider->fireLocationUpdateSelective(gpsLocation3);

    // Waiting between broadcast occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms));

    testProvider->fireLocationUpdateSelective(gpsLocation4);

    // ensure to wait for the minInterval_ms before ending
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms));
    JOYNR_EXPECT_NO_THROW(
            testProxy->unsubscribeFromLocationUpdateSelectiveBroadcast(subscriptionId));
}

using namespace std::string_literals;

INSTANTIATE_TEST_SUITE_P(
        Mqtt,
        End2EndSelectiveBroadcastTest,
        testing::Values(std::make_tuple("test-resources/MqttSystemIntegrationTest1.settings"s,
                                        "test-resources/MqttSystemIntegrationTest2.settings"s)));
