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
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

#include "tests/utils/Gmock.h"
#include "tests/utils/Gtest.h"

#include "joynr/IMessageSender.h"
#include "joynr/InterfaceRegistrar.h"
#include "joynr/LibjoynrSettings.h"
#include "joynr/Logger.h"
#include "joynr/OnChangeSubscriptionQos.h"
#include "joynr/Semaphore.h"
#include "joynr/SingleThreadedIOService.h"

#include "joynr/tests/testRequestInterpreter.h"

#include "tests/JoynrTest.h"
#include "tests/mock/MockMessageSender.h"
#include "tests/mock/MockPublicationSender.h"
#include "tests/mock/MockTestRequestCaller.h"

using ::testing::_;
using ::testing::A;
using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Between;
using ::testing::ByRef;
using ::testing::Eq;
using ::testing::MakeMatcher;
using ::testing::Matcher;
using ::testing::MatcherInterface;
using ::testing::MatchResultListener;
using ::testing::Mock;
using ::testing::ReturnRef;

using namespace joynr;

MATCHER_P3(messagingQosWithTtl, expectedTtlMs, toleranceMs, logger, "")
{
    std::int64_t actual = static_cast<std::int64_t>(arg.getTtl());
    std::int64_t diff = expectedTtlMs - actual;
    if (diff <= toleranceMs) {
        return true;
    }
    JOYNR_LOG_ERROR(logger,
                    "TTL={} differs {}ms (more than {}ms) from the expected value={}",
                    actual,
                    diff,
                    toleranceMs,
                    expectedTtlMs);
    return false;
}

class PublicationManagerTtlUpliftTest : public testing::Test
{
public:
    PublicationManagerTtlUpliftTest()
            : singleThreadedIOService(std::make_shared<SingleThreadedIOService>()),
              messageSender(std::make_shared<MockMessageSender>()),
              proxyId("ProxyId"),
              providerId("ProviderId"),
              mockPublicationSender(std::make_shared<MockPublicationSender>()),
              ttlUpliftMs(300),
              minInterval_ms(0),
              publicationTtlMs(1024),
              toleranceMs(50)
    {
        singleThreadedIOService->start();
        onChangeSubscriptionQos =
                std::make_shared<OnChangeSubscriptionQos>(0, publicationTtlMs, minInterval_ms);
        onChangeSubscriptionQos->setPublicationTtlMs(publicationTtlMs);
    }

    ~PublicationManagerTtlUpliftTest()
    {
        singleThreadedIOService->stop();
        messageSender.reset();
    }

protected:
    void testSubscriptionWithoutTtlUplift(
            const std::string& proxyId,
            const std::string& providerId,
            std::shared_ptr<MockPublicationSender> mockPublicationSender,
            std::shared_ptr<PublicationManager> publicationManager,
            SubscriptionRequest& subscriptionRequest,
            bool isBroadcastSubscription,
            std::int64_t sleepDurationMs,
            std::int64_t expectedSubscriptionReplyTtlMs,
            std::int64_t expectedPublicationTtlMs,
            std::function<void()> triggerPublication);

    void setExpectationSubscriptionWithoutTtlUpliftAndExpectNoMorePublications(
            const std::string& proxyId,
            const std::string& providerId,
            std::shared_ptr<MockPublicationSender> mockPublicationSender,
            std::int64_t expectedSubscriptionReplyTtlMs,
            std::int64_t expectedPublicationTtlMs,
            std::shared_ptr<Semaphore> semaphore,
            std::shared_ptr<Semaphore> semaphore2);

    void continueTestSubscriptionWithoutTtlUpliftAndExpectNoMorePublications(
            const std::string& proxyId,
            const std::string& providerId,
            std::shared_ptr<MockPublicationSender> mockPublicationSender,
            std::shared_ptr<PublicationManager> publicationManager,
            SubscriptionRequest& subscriptionRequest,
            bool isBroadcastSubscription,
            std::int64_t sleepDurationMs,
            std::function<void()> triggerPublication,
            std::shared_ptr<Semaphore> semaphore,
            std::shared_ptr<Semaphore> semaphore2,
            std::shared_ptr<MockTestRequestCaller> requestCaller);

    void setExpectationSubscriptionWithTtlUpliftAndExpectNoMorePublications(
            const std::string& proxyId,
            const std::string& providerId,
            std::shared_ptr<MockPublicationSender> mockPublicationSender,
            std::int64_t expectedSubscriptionReplyTtlMs,
            std::int64_t expectedPublicationTtlMs,
            std::shared_ptr<Semaphore> semaphore,
            std::shared_ptr<Semaphore> semaphore2,
            std::shared_ptr<Semaphore> semaphore3);

    void continueTestSubscriptionWithTtlUpliftAndExpectNoMorePublications(
            const std::string& proxyId,
            const std::string& providerId,
            std::shared_ptr<MockPublicationSender> mockPublicationSender,
            std::shared_ptr<PublicationManager> publicationManager,
            SubscriptionRequest& subscriptionRequest,
            bool isBroadcastSubscription,
            std::int64_t sleepDurationMs,
            std::function<void()> triggerPublication,
            std::shared_ptr<Semaphore> semaphore,
            std::shared_ptr<Semaphore> semaphore2,
            std::shared_ptr<Semaphore> semaphore3,
            std::shared_ptr<MockTestRequestCaller> requestCaller);

    void setExpectationSubscriptionWithTtlUpliftAndExpectMorePublications(
            const std::string& proxyId,
            const std::string& providerId,
            std::shared_ptr<MockPublicationSender> mockPublicationSender,
            std::int64_t expectedSubscriptionReplyTtlMs,
            std::int64_t expectedPublicationTtlMs,
            std::shared_ptr<Semaphore> semaphore,
            std::shared_ptr<Semaphore> semaphore2,
            std::shared_ptr<Semaphore> semaphore3);

    void continueTestSubscriptionWithTtlUpliftAndExpectMorePublications(
            const std::string& proxyId,
            const std::string& providerId,
            std::shared_ptr<MockPublicationSender> mockPublicationSender,
            std::shared_ptr<PublicationManager> publicationManager,
            SubscriptionRequest& subscriptionRequest,
            bool isBroadcastSubscription,
            std::int64_t sleepDurationMs,
            std::function<void()> triggerPublication,
            std::shared_ptr<Semaphore> semaphore,
            std::shared_ptr<Semaphore> semaphore2,
            std::shared_ptr<Semaphore> semaphore3,
            std::shared_ptr<MockTestRequestCaller> requestCaller);

    void expectNoMoreSubscriptionPublications(
            std::shared_ptr<MockPublicationSender> mockPublicationSender,
            std::function<void()> triggerPublication);
    void expectAdditionalSubscriptionPublication(
            const std::string& proxyId,
            const std::string& providerId,
            std::shared_ptr<MockPublicationSender> mockPublicationSender,
            std::int64_t expectedPublicationTtlMs,
            std::function<void()> triggerPublication);
    void testSubscriptionWithTtlUplift(const std::string& proxyId,
                                       const std::string& providerId,
                                       std::shared_ptr<PublicationManager> publicationManager,
                                       std::shared_ptr<MockPublicationSender> mockPublicationSender,
                                       SubscriptionRequest& subscriptionRequest,
                                       bool isBroadcastSubscription,
                                       std::int64_t sleepDurationMs,
                                       std::int64_t expectedSubscriptionReplyTtlMs,
                                       std::int64_t expectedPublicationTtlMs,
                                       std::function<void()> triggerPublication);
    bool isSlowSystem();
    std::shared_ptr<SingleThreadedIOService> singleThreadedIOService;
    std::shared_ptr<IMessageSender> messageSender;

    std::string proxyId;
    std::string providerId;
    std::shared_ptr<MockPublicationSender> mockPublicationSender;

    std::uint64_t ttlUpliftMs;
    std::int64_t minInterval_ms;
    std::int64_t publicationTtlMs;
    std::int64_t toleranceMs;
    std::shared_ptr<OnChangeSubscriptionQos> onChangeSubscriptionQos;

    ADD_LOGGER(PublicationManagerTtlUpliftTest)
};

void PublicationManagerTtlUpliftTest::testSubscriptionWithoutTtlUplift(
        const std::string& proxyId,
        const std::string& providerId,
        std::shared_ptr<MockPublicationSender> mockPublicationSender,
        std::shared_ptr<PublicationManager> publicationManager,
        SubscriptionRequest& subscriptionRequest,
        bool isBroadcastSubscription,
        std::int64_t sleepDurationMs,
        std::int64_t expectedSubscriptionReplyTtlMs,
        std::int64_t expectedPublicationTtlMs,
        std::function<void()> triggerPublication)
{
    joynr::Semaphore semaphore(0);
    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>(
            "tests/Test");

    auto requestCaller = std::make_shared<MockTestRequestCaller>();

    EXPECT_CALL(*mockPublicationSender,
                sendSubscriptionReply(Eq(providerId), // sender participant ID
                                      Eq(proxyId),    // receiver participant ID
                                      messagingQosWithTtl(expectedSubscriptionReplyTtlMs,
                                                          toleranceMs,
                                                          logger()), // messaging QoS
                                      _                              // subscription reply
                                      ))
            .Times(1);

    // sending initial value plus the attributeValueChanged
    EXPECT_CALL(
            *mockPublicationSender,
            sendSubscriptionPublicationMock(
                    Eq(providerId), // sender participant ID
                    Eq(proxyId),    // receiver participant ID
                    messagingQosWithTtl(expectedPublicationTtlMs, 0l, logger()), // messaging QoS
                    _ // subscription publication
                    ))
            .Times(2)
            .WillRepeatedly(ReleaseSemaphore(&semaphore));

    JOYNR_LOG_DEBUG(logger(), "adding request");

    if (isBroadcastSubscription) {
        publicationManager->add(proxyId,
                                providerId,
                                requestCaller,
                                static_cast<BroadcastSubscriptionRequest&>(subscriptionRequest),
                                mockPublicationSender);
        // fire initial broadcast
        triggerPublication();
    } else {
        publicationManager->add(
                proxyId, providerId, requestCaller, subscriptionRequest, mockPublicationSender);
    }

    // wait for initial publication
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(50)));

    triggerPublication();
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepDurationMs + toleranceMs));
    EXPECT_EQ(1, semaphore.getStatus());
}

void PublicationManagerTtlUpliftTest::
        setExpectationSubscriptionWithoutTtlUpliftAndExpectNoMorePublications(
                const std::string& proxyId,
                const std::string& providerId,
                std::shared_ptr<MockPublicationSender> mockPublicationSender,
                std::int64_t expectedSubscriptionReplyTtlMs,
                std::int64_t expectedPublicationTtlMs,
                std::shared_ptr<Semaphore> semaphore,
                std::shared_ptr<Semaphore> semaphore2)
{
    EXPECT_CALL(*mockPublicationSender,
                sendSubscriptionReply(Eq(providerId), // sender participant ID
                                      Eq(proxyId),    // receiver participant ID
                                      messagingQosWithTtl(expectedSubscriptionReplyTtlMs,
                                                          toleranceMs,
                                                          logger()), // messaging QoS
                                      _                              // subscription reply
                                      ))
            .Times(1);

    EXPECT_CALL(*mockPublicationSender,
                sendSubscriptionPublicationMock(_, // sender participant ID
                                                _, // receiver participant ID
                                                _, // messaging QoS
                                                _  // subscription publication
                                                ))
            .Times(0)
            .WillRepeatedly(ReleaseSemaphore(semaphore2));

    // sending initial value plus the attributeValueChanged
    EXPECT_CALL(
            *mockPublicationSender,
            sendSubscriptionPublicationMock(
                    Eq(providerId), // sender participant ID
                    Eq(proxyId),    // receiver participant ID
                    messagingQosWithTtl(expectedPublicationTtlMs, 0l, logger()), // messaging QoS
                    _ // subscription publication
                    ))
            .Times(2)
            .WillRepeatedly(ReleaseSemaphore(semaphore))
            .RetiresOnSaturation();
}

void PublicationManagerTtlUpliftTest::
        continueTestSubscriptionWithoutTtlUpliftAndExpectNoMorePublications(
                const std::string& proxyId,
                const std::string& providerId,
                std::shared_ptr<MockPublicationSender> mockPublicationSender,
                std::shared_ptr<PublicationManager> publicationManager,
                SubscriptionRequest& subscriptionRequest,
                bool isBroadcastSubscription,
                std::int64_t sleepDurationMs,
                std::function<void()> triggerPublication,
                std::shared_ptr<Semaphore> semaphore,
                std::shared_ptr<Semaphore> semaphore2,
                std::shared_ptr<MockTestRequestCaller> requestCaller)
{
    JOYNR_LOG_DEBUG(logger(), "adding request");

    if (isBroadcastSubscription) {
        publicationManager->add(proxyId,
                                providerId,
                                requestCaller,
                                static_cast<BroadcastSubscriptionRequest&>(subscriptionRequest),
                                mockPublicationSender);
        // fire initial broadcast
        triggerPublication();
    } else {
        publicationManager->add(
                proxyId, providerId, requestCaller, subscriptionRequest, mockPublicationSender);
    }

    // wait for initial publication
    EXPECT_TRUE(semaphore->waitFor(std::chrono::milliseconds(50)));

    triggerPublication();
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepDurationMs + toleranceMs));
    EXPECT_EQ(1, semaphore->getStatus());

    triggerPublication();
    EXPECT_FALSE(semaphore2->waitFor(std::chrono::milliseconds(200)));
}

void PublicationManagerTtlUpliftTest::
        continueTestSubscriptionWithTtlUpliftAndExpectNoMorePublications(
                const std::string& proxyId,
                const std::string& providerId,
                std::shared_ptr<MockPublicationSender> mockPublicationSender,
                std::shared_ptr<PublicationManager> publicationManager,
                SubscriptionRequest& subscriptionRequest,
                bool isBroadcastSubscription,
                std::int64_t sleepDurationMs,
                std::function<void()> triggerPublication,
                std::shared_ptr<Semaphore> semaphore,
                std::shared_ptr<Semaphore> semaphore2,
                std::shared_ptr<Semaphore> semaphore3,
                std::shared_ptr<MockTestRequestCaller> requestCaller)
{
    JOYNR_LOG_DEBUG(logger(), "adding request");

    if (isBroadcastSubscription) {
        publicationManager->add(proxyId,
                                providerId,
                                requestCaller,
                                static_cast<BroadcastSubscriptionRequest&>(subscriptionRequest),
                                mockPublicationSender);
        // fire initial broadcast
        triggerPublication();
    } else {
        publicationManager->add(
                proxyId, providerId, requestCaller, subscriptionRequest, mockPublicationSender);
    }

    // wait for initial publication
    EXPECT_TRUE(semaphore->waitFor(std::chrono::milliseconds(50)));

    triggerPublication();
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepDurationMs + toleranceMs));
    EXPECT_EQ(1, semaphore->getStatus());

    triggerPublication();
    EXPECT_TRUE(semaphore2->waitFor(std::chrono::milliseconds(200)));
    std::this_thread::sleep_for(std::chrono::milliseconds(ttlUpliftMs));

    triggerPublication();
    EXPECT_FALSE(semaphore3->waitFor(std::chrono::milliseconds(200)));
}

void PublicationManagerTtlUpliftTest::
        continueTestSubscriptionWithTtlUpliftAndExpectMorePublications(
                const std::string& proxyId,
                const std::string& providerId,
                std::shared_ptr<MockPublicationSender> mockPublicationSender,
                std::shared_ptr<PublicationManager> publicationManager,
                SubscriptionRequest& subscriptionRequest,
                bool isBroadcastSubscription,
                std::int64_t sleepDurationMs,
                std::function<void()> triggerPublication,
                std::shared_ptr<Semaphore> semaphore,
                std::shared_ptr<Semaphore> semaphore2,
                std::shared_ptr<Semaphore> semaphore3,
                std::shared_ptr<MockTestRequestCaller> requestCaller)
{
    JOYNR_LOG_DEBUG(logger(), "adding request");

    if (isBroadcastSubscription) {
        publicationManager->add(proxyId,
                                providerId,
                                requestCaller,
                                static_cast<BroadcastSubscriptionRequest&>(subscriptionRequest),
                                mockPublicationSender);
        // fire initial broadcast
        triggerPublication();
    } else {
        publicationManager->add(
                proxyId, providerId, requestCaller, subscriptionRequest, mockPublicationSender);
    }

    // wait for initial publication
    EXPECT_TRUE(semaphore->waitFor(std::chrono::milliseconds(50)));

    triggerPublication();
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepDurationMs + toleranceMs));
    EXPECT_EQ(1, semaphore->getStatus());

    triggerPublication();
    EXPECT_TRUE(semaphore2->waitFor(std::chrono::milliseconds(200)));

    std::this_thread::sleep_for(std::chrono::milliseconds(ttlUpliftMs));

    triggerPublication();
    EXPECT_TRUE(semaphore3->waitFor(std::chrono::milliseconds(200)));
}

void PublicationManagerTtlUpliftTest::
        setExpectationSubscriptionWithTtlUpliftAndExpectNoMorePublications(
                const std::string& proxyId,
                const std::string& providerId,
                std::shared_ptr<MockPublicationSender> mockPublicationSender,
                std::int64_t expectedSubscriptionReplyTtlMs,
                std::int64_t expectedPublicationTtlMs,
                std::shared_ptr<Semaphore> semaphore,
                std::shared_ptr<Semaphore> semaphore2,
                std::shared_ptr<Semaphore> semaphore3)
{
    EXPECT_CALL(*mockPublicationSender,
                sendSubscriptionReply(Eq(providerId), // sender participant ID
                                      Eq(proxyId),    // receiver participant ID
                                      messagingQosWithTtl(expectedSubscriptionReplyTtlMs,
                                                          toleranceMs,
                                                          logger()), // messaging QoS
                                      _                              // subscription reply
                                      ))
            .Times(1);

    EXPECT_CALL(*mockPublicationSender,
                sendSubscriptionPublicationMock(_, // sender participant ID
                                                _, // receiver participant ID
                                                _, // messaging QoS
                                                _  // subscription publication
                                                ))
            .Times(0)
            .WillRepeatedly(ReleaseSemaphore(semaphore3));

    EXPECT_CALL(
            *mockPublicationSender,
            sendSubscriptionPublicationMock(
                    Eq(providerId), // sender participant ID
                    Eq(proxyId),    // receiver participant ID
                    messagingQosWithTtl(expectedPublicationTtlMs, 0l, logger()), // messaging QoS
                    _ // subscription publication
                    ))
            .Times(1)
            .WillOnce(ReleaseSemaphore(semaphore2))
            .RetiresOnSaturation();

    // sending initial value plus the attributeValueChanged
    EXPECT_CALL(
            *mockPublicationSender,
            sendSubscriptionPublicationMock(
                    Eq(providerId), // sender participant ID
                    Eq(proxyId),    // receiver participant ID
                    messagingQosWithTtl(expectedPublicationTtlMs, 0l, logger()), // messaging QoS
                    _ // subscription publication
                    ))
            .Times(2)
            .WillRepeatedly(ReleaseSemaphore(semaphore))
            .RetiresOnSaturation();
}

void PublicationManagerTtlUpliftTest::
        setExpectationSubscriptionWithTtlUpliftAndExpectMorePublications(
                const std::string& proxyId,
                const std::string& providerId,
                std::shared_ptr<MockPublicationSender> mockPublicationSender,
                std::int64_t expectedSubscriptionReplyTtlMs,
                std::int64_t expectedPublicationTtlMs,
                std::shared_ptr<Semaphore> semaphore,
                std::shared_ptr<Semaphore> semaphore2,
                std::shared_ptr<Semaphore> semaphore3)
{
    EXPECT_CALL(*mockPublicationSender,
                sendSubscriptionReply(Eq(providerId), // sender participant ID
                                      Eq(proxyId),    // receiver participant ID
                                      messagingQosWithTtl(expectedSubscriptionReplyTtlMs,
                                                          toleranceMs,
                                                          logger()), // messaging QoS
                                      _                              // subscription reply
                                      ))
            .Times(1);

    EXPECT_CALL(
            *mockPublicationSender,
            sendSubscriptionPublicationMock(
                    Eq(providerId), // sender participant ID
                    Eq(proxyId),    // receiver participant ID
                    messagingQosWithTtl(expectedPublicationTtlMs, 0l, logger()), // messaging QoS
                    _ // subscription publication
                    ))
            .Times(1)
            .WillOnce(ReleaseSemaphore(semaphore3));

    EXPECT_CALL(
            *mockPublicationSender,
            sendSubscriptionPublicationMock(
                    Eq(providerId), // sender participant ID
                    Eq(proxyId),    // receiver participant ID
                    messagingQosWithTtl(expectedPublicationTtlMs, 0l, logger()), // messaging QoS
                    _ // subscription publication
                    ))
            .Times(1)
            .WillOnce(ReleaseSemaphore(semaphore2))
            .RetiresOnSaturation();

    // sending initial value plus the attributeValueChanged
    EXPECT_CALL(
            *mockPublicationSender,
            sendSubscriptionPublicationMock(
                    Eq(providerId), // sender participant ID
                    Eq(proxyId),    // receiver participant ID
                    messagingQosWithTtl(expectedPublicationTtlMs, 0l, logger()), // messaging QoS
                    _ // subscription publication
                    ))
            .Times(2)
            .WillRepeatedly(ReleaseSemaphore(semaphore))
            .RetiresOnSaturation();
}

void PublicationManagerTtlUpliftTest::expectNoMoreSubscriptionPublications(
        std::shared_ptr<MockPublicationSender> mockPublicationSender,
        std::function<void()> triggerPublication)
{
    Mock::VerifyAndClearExpectations(mockPublicationSender.get());

    joynr::Semaphore semaphore(0);
    EXPECT_CALL(*mockPublicationSender,
                sendSubscriptionPublicationMock(_, // sender participant ID
                                                _, // receiver participant ID
                                                _, // messaging QoS
                                                _  // subscription publication
                                                ))
            .Times(0)
            .WillRepeatedly(ReleaseSemaphore(&semaphore));

    triggerPublication();
    EXPECT_FALSE(semaphore.waitFor(std::chrono::milliseconds(200)));
}

void PublicationManagerTtlUpliftTest::expectAdditionalSubscriptionPublication(
        const std::string& proxyId,
        const std::string& providerId,
        std::shared_ptr<MockPublicationSender> mockPublicationSender,
        std::int64_t expectedPublicationTtlMs,
        std::function<void()> triggerPublication)
{
    Mock::VerifyAndClearExpectations(mockPublicationSender.get());

    joynr::Semaphore semaphore(0);
    EXPECT_CALL(
            *mockPublicationSender,
            sendSubscriptionPublicationMock(
                    Eq(providerId), // sender participant ID
                    Eq(proxyId),    // receiver participant ID
                    messagingQosWithTtl(expectedPublicationTtlMs, 0l, logger()), // messaging QoS
                    _ // subscription publication
                    ))
            .Times(1)
            .WillOnce(ReleaseSemaphore(&semaphore));

    triggerPublication();
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(200)));
}

void PublicationManagerTtlUpliftTest::testSubscriptionWithTtlUplift(
        const std::string& proxyId,
        const std::string& providerId,
        std::shared_ptr<PublicationManager> publicationManager,
        std::shared_ptr<MockPublicationSender> mockPublicationSender,
        SubscriptionRequest& subscriptionRequest,
        bool isBroadcastSubscription,
        std::int64_t sleepDurationMs,
        std::int64_t expectedSubscriptionReplyTtlMs,
        std::int64_t expectedPublicationTtlMs,
        std::function<void()> triggerPublication)
{
    testSubscriptionWithoutTtlUplift(proxyId,
                                     providerId,
                                     mockPublicationSender,
                                     publicationManager,
                                     subscriptionRequest,
                                     isBroadcastSubscription,
                                     sleepDurationMs,
                                     expectedSubscriptionReplyTtlMs,
                                     expectedPublicationTtlMs,
                                     triggerPublication);

    expectAdditionalSubscriptionPublication(proxyId,
                                            providerId,
                                            mockPublicationSender,
                                            expectedPublicationTtlMs,
                                            triggerPublication);

    std::this_thread::sleep_for(std::chrono::milliseconds(ttlUpliftMs));
}

bool PublicationManagerTtlUpliftTest::isSlowSystem()
{
    return std::getenv("OECORE_SDK_VERSION") != nullptr;
}

TEST_F(PublicationManagerTtlUpliftTest, testBroadcastSubscriptionWithoutTtlUplift)
{
    if (isSlowSystem()) {
        return;
    }
    auto publicationManager = std::make_shared<PublicationManager>(
            singleThreadedIOService->getIOService(), messageSender, 0);

    // SubscriptionRequest
    std::string broadcastName("Location");
    std::int64_t validity_ms = 300;
    onChangeSubscriptionQos->setValidityMs(validity_ms);
    BroadcastSubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscribeToName(broadcastName);
    subscriptionRequest.setQos(onChangeSubscriptionQos);

    std::string subscriptionId = subscriptionRequest.getSubscriptionId();

    std::int64_t sleepDurationMs = validity_ms;
    std::int64_t expectedSubscriptionReplyTtlMs = validity_ms;
    std::int64_t expectedPublicationTtlMs = publicationTtlMs;

    auto triggerPublication = [publicationManager, subscriptionId]() {
        joynr::types::Localisation::GpsLocation valueToPublish;
        publicationManager->broadcastOccurred(subscriptionId, valueToPublish);
    };

    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>(
            "tests/Test");
    auto requestCaller = std::make_shared<MockTestRequestCaller>();

    std::shared_ptr<joynr::Semaphore> semaphore = std::make_shared<Semaphore>(0);
    std::shared_ptr<joynr::Semaphore> semaphore2 = std::make_shared<Semaphore>(0);
    setExpectationSubscriptionWithoutTtlUpliftAndExpectNoMorePublications(
            proxyId,
            providerId,
            mockPublicationSender,
            expectedSubscriptionReplyTtlMs,
            expectedPublicationTtlMs,
            semaphore,
            semaphore2);

    continueTestSubscriptionWithoutTtlUpliftAndExpectNoMorePublications(proxyId,
                                                                        providerId,
                                                                        mockPublicationSender,
                                                                        publicationManager,
                                                                        subscriptionRequest,
                                                                        true,
                                                                        sleepDurationMs,
                                                                        triggerPublication,
                                                                        semaphore,
                                                                        semaphore2,
                                                                        requestCaller);

    publicationManager->shutdown();
}

TEST_F(PublicationManagerTtlUpliftTest, testAttributeSubscriptionWithoutTtlUplift)
{
    if (isSlowSystem()) {
        return;
    }
    auto publicationManager = std::make_shared<PublicationManager>(
            singleThreadedIOService->getIOService(), messageSender, 0);

    // SubscriptionRequest
    std::string attributeName("Location");
    std::int64_t validity_ms = 300;
    onChangeSubscriptionQos->setValidityMs(validity_ms);
    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(onChangeSubscriptionQos);

    std::string subscriptionId = subscriptionRequest.getSubscriptionId();

    std::int64_t sleepDurationMs = validity_ms;
    std::int64_t expectedSubscriptionReplyTtlMs = validity_ms;
    std::int64_t expectedPublicationTtlMs = publicationTtlMs;

    auto triggerPublication = [publicationManager, subscriptionId]() {
        joynr::types::Localisation::GpsLocation attributeValue;
        publicationManager->attributeValueChanged(subscriptionId, attributeValue);
    };

    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>(
            "tests/Test");
    auto requestCaller = std::make_shared<MockTestRequestCaller>();

    std::shared_ptr<Semaphore> semaphore = std::make_shared<Semaphore>(0);
    std::shared_ptr<Semaphore> semaphore2 = std::make_shared<Semaphore>(0);
    setExpectationSubscriptionWithoutTtlUpliftAndExpectNoMorePublications(
            proxyId,
            providerId,
            mockPublicationSender,
            expectedSubscriptionReplyTtlMs,
            expectedPublicationTtlMs,
            semaphore,
            semaphore2);

    JOYNR_LOG_DEBUG(logger(), "adding request");

    continueTestSubscriptionWithoutTtlUpliftAndExpectNoMorePublications(proxyId,
                                                                        providerId,
                                                                        mockPublicationSender,
                                                                        publicationManager,
                                                                        subscriptionRequest,
                                                                        false,
                                                                        sleepDurationMs,
                                                                        triggerPublication,
                                                                        semaphore,
                                                                        semaphore2,
                                                                        requestCaller);

    publicationManager->shutdown();
}

TEST_F(PublicationManagerTtlUpliftTest, testAttributeSubscriptionWithTtlUplift)
{
    if (isSlowSystem()) {
        return;
    }
    auto publicationManager = std::make_shared<PublicationManager>(
            singleThreadedIOService->getIOService(), messageSender, ttlUpliftMs);

    // SubscriptionRequest
    std::string attributeName("Location");
    std::int64_t validity_ms = 300;
    onChangeSubscriptionQos->setValidityMs(validity_ms);
    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(onChangeSubscriptionQos);

    std::string subscriptionId = subscriptionRequest.getSubscriptionId();

    std::int64_t sleepDurationMs = validity_ms;
    std::int64_t expectedSubscriptionReplyTtlMs = validity_ms;
    std::int64_t expectedPublicationTtlMs = publicationTtlMs;

    auto triggerPublication = [publicationManager, subscriptionId]() {
        joynr::types::Localisation::GpsLocation attributeValue;
        publicationManager->attributeValueChanged(subscriptionId, attributeValue);
    };

    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>(
            "tests/Test");
    auto requestCaller = std::make_shared<MockTestRequestCaller>();

    std::shared_ptr<joynr::Semaphore> semaphore = std::make_shared<Semaphore>(0);
    std::shared_ptr<joynr::Semaphore> semaphore2 = std::make_shared<Semaphore>(0);
    std::shared_ptr<joynr::Semaphore> semaphore3 = std::make_shared<Semaphore>(0);

    setExpectationSubscriptionWithTtlUpliftAndExpectNoMorePublications(
            proxyId,
            providerId,
            mockPublicationSender,
            expectedSubscriptionReplyTtlMs,
            expectedPublicationTtlMs,
            semaphore,
            semaphore2,
            semaphore3);

    continueTestSubscriptionWithTtlUpliftAndExpectNoMorePublications(proxyId,
                                                                     providerId,
                                                                     mockPublicationSender,
                                                                     publicationManager,
                                                                     subscriptionRequest,
                                                                     false,
                                                                     sleepDurationMs,
                                                                     triggerPublication,
                                                                     semaphore,
                                                                     semaphore2,
                                                                     semaphore3,
                                                                     requestCaller);
    publicationManager->shutdown();
}

TEST_F(PublicationManagerTtlUpliftTest, testBroadcastSubscriptionWithTtlUplift)
{
    if (isSlowSystem()) {
        return;
    }
    auto publicationManager = std::make_shared<PublicationManager>(
            singleThreadedIOService->getIOService(), messageSender, ttlUpliftMs);

    // SubscriptionRequest
    std::string broadcastName("Location");
    std::int64_t validity_ms = 300;
    onChangeSubscriptionQos->setValidityMs(validity_ms);
    BroadcastSubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscribeToName(broadcastName);
    subscriptionRequest.setQos(onChangeSubscriptionQos);

    std::string subscriptionId = subscriptionRequest.getSubscriptionId();

    std::int64_t sleepDurationMs = validity_ms;
    std::int64_t expectedSubscriptionReplyTtlMs = validity_ms;
    std::int64_t expectedPublicationTtlMs = publicationTtlMs;

    auto triggerPublication = [publicationManager, subscriptionId]() {
        joynr::types::Localisation::GpsLocation valueToPublish;
        publicationManager->broadcastOccurred(subscriptionId, valueToPublish);
    };

    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>(
            "tests/Test");
    auto requestCaller = std::make_shared<MockTestRequestCaller>();

    std::shared_ptr<joynr::Semaphore> semaphore = std::make_shared<Semaphore>(0);
    std::shared_ptr<joynr::Semaphore> semaphore2 = std::make_shared<Semaphore>(0);
    std::shared_ptr<joynr::Semaphore> semaphore3 = std::make_shared<Semaphore>(0);

    setExpectationSubscriptionWithTtlUpliftAndExpectNoMorePublications(
            proxyId,
            providerId,
            mockPublicationSender,
            expectedSubscriptionReplyTtlMs,
            expectedPublicationTtlMs,
            semaphore,
            semaphore2,
            semaphore3);

    continueTestSubscriptionWithTtlUpliftAndExpectNoMorePublications(proxyId,
                                                                     providerId,
                                                                     mockPublicationSender,
                                                                     publicationManager,
                                                                     subscriptionRequest,
                                                                     true,
                                                                     sleepDurationMs,
                                                                     triggerPublication,
                                                                     semaphore,
                                                                     semaphore2,
                                                                     semaphore3,
                                                                     requestCaller);

    expectNoMoreSubscriptionPublications(mockPublicationSender, triggerPublication);
    publicationManager->shutdown();
}

TEST_F(PublicationManagerTtlUpliftTest, testAttributeSubscriptionWithTtlUpliftWithNoExpiryDate)
{
    if (isSlowSystem()) {
        return;
    }
    auto publicationManager = std::make_shared<PublicationManager>(
            singleThreadedIOService->getIOService(), messageSender, ttlUpliftMs);

    // SubscriptionRequest
    std::string attributeName("Location");
    onChangeSubscriptionQos->setExpiryDateMs(SubscriptionQos::NO_EXPIRY_DATE());
    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(onChangeSubscriptionQos);

    std::string subscriptionId = subscriptionRequest.getSubscriptionId();

    std::int64_t sleepDurationMs = 300;
    std::int64_t expectedSubscriptionReplyTtlMs = INT64_MAX;
    std::int64_t expectedPublicationTtlMs = publicationTtlMs;

    auto triggerPublication = [publicationManager, subscriptionId]() {
        joynr::types::Localisation::GpsLocation attributeValue;
        publicationManager->attributeValueChanged(subscriptionId, attributeValue);
    };
    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>(
            "tests/Test");
    auto requestCaller = std::make_shared<MockTestRequestCaller>();

    std::shared_ptr<joynr::Semaphore> semaphore = std::make_shared<Semaphore>(0);
    std::shared_ptr<joynr::Semaphore> semaphore2 = std::make_shared<Semaphore>(0);
    std::shared_ptr<joynr::Semaphore> semaphore3 = std::make_shared<Semaphore>(0);

    setExpectationSubscriptionWithTtlUpliftAndExpectMorePublications(proxyId,
                                                                     providerId,
                                                                     mockPublicationSender,
                                                                     expectedSubscriptionReplyTtlMs,
                                                                     expectedPublicationTtlMs,
                                                                     semaphore,
                                                                     semaphore2,
                                                                     semaphore3);

    continueTestSubscriptionWithTtlUpliftAndExpectMorePublications(proxyId,
                                                                     providerId,
                                                                     mockPublicationSender,
                                                                     publicationManager,
                                                                     subscriptionRequest,
                                                                     false,
                                                                     sleepDurationMs,
                                                                     triggerPublication,
                                                                     semaphore,
                                                                     semaphore2,
                                                                     semaphore3,
                                                                     requestCaller);
    publicationManager->shutdown();
}

TEST_F(PublicationManagerTtlUpliftTest, testBroadcastSubscriptionWithTtlUpliftWithNoExpiryDate)
{
    if (isSlowSystem()) {
        return;
    }
    auto publicationManager = std::make_shared<PublicationManager>(
            singleThreadedIOService->getIOService(), messageSender, ttlUpliftMs);

    // SubscriptionRequest
    std::string broadcastName("Location");
    onChangeSubscriptionQos->setExpiryDateMs(SubscriptionQos::NO_EXPIRY_DATE());
    BroadcastSubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscribeToName(broadcastName);
    subscriptionRequest.setQos(onChangeSubscriptionQos);

    std::string subscriptionId = subscriptionRequest.getSubscriptionId();

    std::int64_t sleepDurationMs = 300;
    std::int64_t expectedSubscriptionReplyTtlMs = INT64_MAX;
    std::int64_t expectedPublicationTtlMs = publicationTtlMs;

    auto triggerPublication = [publicationManager, subscriptionId]() {
        joynr::types::Localisation::GpsLocation valueToPublish;
        publicationManager->broadcastOccurred(subscriptionId, valueToPublish);
    };

    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>(
            "tests/Test");
    auto requestCaller = std::make_shared<MockTestRequestCaller>();

    std::shared_ptr<joynr::Semaphore> semaphore = std::make_shared<Semaphore>(0);
    std::shared_ptr<joynr::Semaphore> semaphore2 = std::make_shared<Semaphore>(0);
    std::shared_ptr<joynr::Semaphore> semaphore3 = std::make_shared<Semaphore>(0);
    setExpectationSubscriptionWithTtlUpliftAndExpectMorePublications(proxyId,
                                                                     providerId,
                                                                     mockPublicationSender,
                                                                     expectedSubscriptionReplyTtlMs,
                                                                     expectedPublicationTtlMs,
                                                                     semaphore,
                                                                     semaphore2,
                                                                     semaphore3);

    continueTestSubscriptionWithTtlUpliftAndExpectMorePublications(proxyId,
                                                                     providerId,
                                                                     mockPublicationSender,
                                                                     publicationManager,
                                                                     subscriptionRequest,
                                                                     true,
                                                                     sleepDurationMs,
                                                                     triggerPublication,
                                                                     semaphore,
                                                                     semaphore2,
                                                                     semaphore3,
                                                                     requestCaller);
    publicationManager->shutdown();
}

TEST_F(PublicationManagerTtlUpliftTest,
       DISABLED_testAttributeSubscriptionWithTtlUpliftWithLargeExpiryDate)
{
    if (isSlowSystem()) {
        return;
    }
    auto publicationManager = std::make_shared<PublicationManager>(
            singleThreadedIOService->getIOService(), messageSender, ttlUpliftMs);

    // SubscriptionRequest
    std::string attributeName("Location");
    std::int64_t expiryDateMs = INT64_MAX - static_cast<std::int64_t>(ttlUpliftMs) + 1;
    onChangeSubscriptionQos->setExpiryDateMs(expiryDateMs);
    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(onChangeSubscriptionQos);

    std::string subscriptionId = subscriptionRequest.getSubscriptionId();

    std::int64_t sleepDurationMs = 300;
    std::int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::system_clock::now().time_since_epoch())
                               .count();
    std::int64_t expectedSubscriptionReplyTtlMs = expiryDateMs - now;
    std::int64_t expectedPublicationTtlMs = publicationTtlMs;

    auto triggerPublication = [publicationManager, subscriptionId]() {
        joynr::types::Localisation::GpsLocation attributeValue;
        publicationManager->attributeValueChanged(subscriptionId, attributeValue);
    };

    testSubscriptionWithTtlUplift(proxyId,
                                  providerId,
                                  publicationManager,
                                  mockPublicationSender,
                                  subscriptionRequest,
                                  false,
                                  sleepDurationMs,
                                  expectedSubscriptionReplyTtlMs,
                                  expectedPublicationTtlMs,
                                  triggerPublication);

    expectAdditionalSubscriptionPublication(proxyId,
                                            providerId,
                                            mockPublicationSender,
                                            expectedPublicationTtlMs,
                                            triggerPublication);
    publicationManager->shutdown();
}

TEST_F(PublicationManagerTtlUpliftTest,
       DISABLED_testBroadcastSubscriptionWithTtlUpliftWithLargeExpiryDate)
{
    if (isSlowSystem()) {
        return;
    }
    auto publicationManager = std::make_shared<PublicationManager>(
            singleThreadedIOService->getIOService(), messageSender, ttlUpliftMs);

    // SubscriptionRequest
    std::string broadcastName("Location");
    std::int64_t expiryDateMs = INT64_MAX - static_cast<std::int64_t>(ttlUpliftMs) + 1;
    onChangeSubscriptionQos->setExpiryDateMs(expiryDateMs);
    BroadcastSubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscribeToName(broadcastName);
    subscriptionRequest.setQos(onChangeSubscriptionQos);

    std::string subscriptionId = subscriptionRequest.getSubscriptionId();

    std::int64_t sleepDurationMs = 300;
    std::int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::system_clock::now().time_since_epoch())
                               .count();
    std::int64_t expectedSubscriptionReplyTtlMs = expiryDateMs - now;
    std::int64_t expectedPublicationTtlMs = publicationTtlMs;

    auto triggerPublication = [publicationManager, subscriptionId]() {
        joynr::types::Localisation::GpsLocation valueToPublish;
        publicationManager->broadcastOccurred(subscriptionId, valueToPublish);
    };

    testSubscriptionWithTtlUplift(proxyId,
                                  providerId,
                                  publicationManager,
                                  mockPublicationSender,
                                  subscriptionRequest,
                                  true,
                                  sleepDurationMs,
                                  expectedSubscriptionReplyTtlMs,
                                  expectedPublicationTtlMs,
                                  triggerPublication);

    expectAdditionalSubscriptionPublication(proxyId,
                                            providerId,
                                            mockPublicationSender,
                                            expectedPublicationTtlMs,
                                            triggerPublication);
    publicationManager->shutdown();
}
