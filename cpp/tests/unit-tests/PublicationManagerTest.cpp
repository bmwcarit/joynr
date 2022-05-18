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
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>

#include "tests/utils/Gtest.h"
#include "tests/utils/Gmock.h"

#include "joynr/CallContextStorage.h"
#include "joynr/InterfaceRegistrar.h"
#include "joynr/tests/testRequestInterpreter.h"
#include "joynr/SubscriptionPublication.h"
#include "joynr/SubscriptionAttributeListener.h"
#include "joynr/UnicastBroadcastListener.h"
#include "joynr/PeriodicSubscriptionQos.h"
#include "joynr/OnChangeSubscriptionQos.h"
#include "joynr/MulticastSubscriptionQos.h"
#include "joynr/LibjoynrSettings.h"
#include "tests/utils/TimeUtils.h"
#include "joynr/Logger.h"
#include "joynr/exceptions/MethodInvocationException.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/SubscriptionReply.h"
#include "joynr/Semaphore.h"
#include "joynr/IMessageSender.h"

#include "tests/JoynrTest.h"
#include "tests/mock/MockPublicationSender.h"
#include "tests/mock/MockTestRequestCaller.h"
#include "tests/mock/MockMessageSender.h"

using ::testing::A;
using ::testing::_;
using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Between;
using ::testing::ByRef;
using ::testing::ReturnRef;
using ::testing::MatcherInterface;
using ::testing::MatchResultListener;
using ::testing::Matcher;
using ::testing::MakeMatcher;
using ::testing::Eq;

using namespace joynr;

class PublicationManagerTest : public testing::Test
{
public:
    PublicationManagerTest()
            : _singleThreadedIOService(std::make_shared<SingleThreadedIOService>()),
              _messageSender(std::make_shared<MockMessageSender>()),
              _getLocationCalledSemaphore(0)
    {
        _singleThreadedIOService->start();
    }

    ~PublicationManagerTest() override
    {
        _singleThreadedIOService->stop();
    }

    void TearDown() override
    {
        _messageSender.reset();
    }

    void invokeLocationAndSaveCallContext(
            std::function<void(const joynr::types::Localisation::GpsLocation&)> onSuccess,
            std::function<void(const std::shared_ptr<joynr::exceptions::ProviderRuntimeException>&)>
                    /*onError*/)
    {
        onSuccess(joynr::types::Localisation::GpsLocation());
        _savedCallContext = CallContextStorage::get();
        _getLocationCalledSemaphore.notify();
    }

protected:
    void sendSubscriptionReplyOnSuccessfulRegistration(SubscriptionRequest& subscriptionRequest);
    void sendSubscriptionExceptionOnExpiredRegistration(SubscriptionRequest& subscriptionRequest);
    std::shared_ptr<SingleThreadedIOService> _singleThreadedIOService;
    std::shared_ptr<IMessageSender> _messageSender;
    joynr::CallContext _savedCallContext;
    joynr::Semaphore _getLocationCalledSemaphore;
    ADD_LOGGER(PublicationManagerTest)
};

// New matcher definition that works with the latest Google Mock.
class SubscriptionPublicationMatcherInterface
        : public MatcherInterface<const SubscriptionPublication&>
{
private:
    const SubscriptionPublication& _expectedSubscriptionPublication;

public:
    explicit SubscriptionPublicationMatcherInterface(
            SubscriptionPublication& expectedSubscriptionPublication)
            : _expectedSubscriptionPublication(expectedSubscriptionPublication)
    {
    }
    virtual void DescribeTo(::std::ostream* /*os*/) const
    {
    }
    virtual bool MatchAndExplain(const SubscriptionPublication& subscriptionPublication,
                                 MatchResultListener* /*listener*/) const
    {
        return subscriptionPublication == _expectedSubscriptionPublication;
    }
};

inline Matcher<const SubscriptionPublication&> SubscriptionPublicationMatcher(
        SubscriptionPublication& expectedValue)
{
    return MakeMatcher(new SubscriptionPublicationMatcherInterface(expectedValue));
}

TEST_F(PublicationManagerTest, add_requestCallerIsCalledCorrectlyByPublisherRunnables)
{
    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<joynr::tests::testRequestInterpreter>(
            joynr::tests::testProvider::INTERFACE_NAME());

    auto mockPublicationSender = std::make_shared<MockPublicationSender>();
    auto requestCaller = std::make_shared<MockTestRequestCaller>(Between(3, 5));

    // NOTE: it depends on the timing and especially on the CPU load of
    // the current machine how often the publication is exectuted. Hence,
    // we expect the publication to haben between 3 or 5 times.
    EXPECT_CALL(*mockPublicationSender, sendSubscriptionPublicationMock(_, _, _, _))
            .Times(Between(3, 5));

    auto publicationManager = std::make_shared<PublicationManager>(
            _singleThreadedIOService->getIOService(), _messageSender);

    // SubscriptionRequest
    std::string senderId = "SenderId";
    std::string receiverId = "ReceiverId";
    std::string attributeName("Location");
    // SubscriptionQos
    std::int64_t period_ms = 200;
    std::int64_t validity_ms = 1000;
    std::int64_t alertInterval_ms = 3000;
    std::int64_t publicationTtl_ms = 2000;
    auto qos = std::make_shared<PeriodicSubscriptionQos>(
            validity_ms, publicationTtl_ms, period_ms, alertInterval_ms);

    // will be deleted by the publication manager
    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(qos);
    JOYNR_LOG_DEBUG(logger(), "adding request");
    publicationManager->add(
            senderId, receiverId, requestCaller, subscriptionRequest, mockPublicationSender);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    publicationManager->shutdown();
}

TEST_F(PublicationManagerTest, stop_publications)
{
    auto mockPublicationSender = std::make_shared<MockPublicationSender>();
    auto requestCaller = std::make_shared<MockTestRequestCaller>(AtMost(2));

    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>(
            "tests/Test");

    EXPECT_CALL(*mockPublicationSender, sendSubscriptionPublicationMock(_, _, _, _))
            .Times(AtMost(2));

    auto publicationManager = std::make_shared<PublicationManager>(
            _singleThreadedIOService->getIOService(), _messageSender);

    // SubscriptionRequest
    std::string senderId = "SenderId";
    std::string receiverId = "ReceiverId";
    std::string attributeName("Location");
    // SubscriptionQos
    std::int64_t period_ms = 100;
    std::int64_t validity_ms = 10000;
    std::int64_t alertInterval_ms = 1000;
    std::int64_t publicationTtlMs = 2000;
    auto qos = std::make_shared<PeriodicSubscriptionQos>(
            validity_ms, publicationTtlMs, period_ms, alertInterval_ms);

    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(qos);

    publicationManager->add(
            senderId, receiverId, requestCaller, subscriptionRequest, mockPublicationSender);
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    publicationManager->stopPublication(subscriptionRequest.getSubscriptionId());
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    publicationManager->shutdown();
}

TEST_F(PublicationManagerTest, remove_all_publications)
{
    auto mockPublicationSender = std::make_shared<MockPublicationSender>();
    auto requestCaller = std::make_shared<MockTestRequestCaller>(AtMost(2));

    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>(
            "tests/Test");

    EXPECT_CALL(*mockPublicationSender, sendSubscriptionPublicationMock(_, _, _, _))
            .Times(AtMost(2));

    auto publicationManager = std::make_shared<PublicationManager>(
            _singleThreadedIOService->getIOService(), _messageSender);

    // SubscriptionRequest
    std::string senderId = "SenderId";
    std::string receiverId = "ReceiverId";
    std::string attributeName("Location");
    // SubscriptionQos
    std::int64_t period_ms = 100;
    std::int64_t validity_ms = 10000;
    std::int64_t alertInterval_ms = 1000;
    std::int64_t publicationTtl_ms = 1000;
    auto qos = std::make_shared<PeriodicSubscriptionQos>(
            validity_ms, publicationTtl_ms, period_ms, alertInterval_ms);

    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(qos);

    publicationManager->add(
            senderId, receiverId, requestCaller, subscriptionRequest, mockPublicationSender);
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    publicationManager->removeAllSubscriptions(receiverId);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    publicationManager->shutdown();
}

TEST_F(PublicationManagerTest, add_onChangeSubscription)
{
    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>(
            "tests/Test");

    auto mockPublicationSender = std::make_shared<MockPublicationSender>();
    auto requestCaller = std::make_shared<MockTestRequestCaller>();

    // The attribute will change to this value
    joynr::types::Localisation::GpsLocation attributeValue;

    SubscriptionRequest subscriptionRequest;

    SubscriptionPublication expectedPublication;
    expectedPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId());
    expectedPublication.setResponse(std::move(attributeValue));
    // Expect an attribute change to send a publication as well as during registering subscription
    // request
    EXPECT_CALL(
            *mockPublicationSender,
            sendSubscriptionPublicationMock(
                    _,                                                  // sender participant ID
                    _,                                                  // receiver participant ID
                    _,                                                  // messaging QoS
                    SubscriptionPublicationMatcher(expectedPublication) // subscription publication
                    )).Times(Between(1, 2));

    // Expect a call to set up the on change subscription
    std::string attributeName = "Location";
    std::shared_ptr<SubscriptionAttributeListener> attributeListener;
    EXPECT_CALL(*requestCaller, registerAttributeListener(attributeName, _))
            .Times(1)
            .WillRepeatedly(testing::SaveArg<1>(&attributeListener));

    // Expect a call to remove the on change subscription
    EXPECT_CALL(*requestCaller, unregisterAttributeListener(attributeName, _)).Times(1);

    auto publicationManager = std::make_shared<PublicationManager>(
            _singleThreadedIOService->getIOService(), _messageSender);

    // SubscriptionRequest
    std::string senderId = "SenderId";
    std::string receiverId = "ReceiverId";
    // SubscriptionQos
    std::int64_t minInterval_ms = 50;
    std::int64_t validity_ms = 500;
    std::int64_t publicationTtl_ms = 1000;
    auto qos = std::make_shared<OnChangeSubscriptionQos>(
            validity_ms, publicationTtl_ms, minInterval_ms);

    // will be deleted by the publication manager
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(qos);
    JOYNR_LOG_DEBUG(logger(), "adding request");
    publicationManager->add(
            senderId, receiverId, requestCaller, subscriptionRequest, mockPublicationSender);

    // Fake an attribute change
    attributeListener->attributeValueChanged(std::move(attributeValue));

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    publicationManager->shutdown();
}

TEST_F(PublicationManagerTest, add_onChangeWithNoExpiryDate)
{
    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>(
            "tests/Test");

    auto mockPublicationSender = std::make_shared<MockPublicationSender>();
    auto requestCaller = std::make_shared<MockTestRequestCaller>();

    // The attribute will change to this value
    joynr::types::Localisation::GpsLocation attributeValue;

    SubscriptionRequest subscriptionRequest;

    SubscriptionPublication expectedPublication;
    expectedPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId());
    expectedPublication.setResponse(std::move(attributeValue));
    // Expect a single attribute change to send a publication + one publication when registering sub
    // request -> 2
    EXPECT_CALL(
            *mockPublicationSender,
            sendSubscriptionPublicationMock(
                    _,                                                  // sender participant ID
                    _,                                                  // receiver participant ID
                    _,                                                  // messaging QoS
                    SubscriptionPublicationMatcher(expectedPublication) // subscription publication
                    )).Times(2);

    // Expect calls to register an unregister an attribute listener
    std::string attributeName("Location");
    std::shared_ptr<SubscriptionAttributeListener> attributeListener;

    EXPECT_CALL(*requestCaller, registerAttributeListener(attributeName, _))
            .Times(1)
            .WillRepeatedly(testing::SaveArg<1>(&attributeListener));

    EXPECT_CALL(*requestCaller, unregisterAttributeListener(attributeName, _)).Times(1);

    auto publicationManager = std::make_shared<PublicationManager>(
            _singleThreadedIOService->getIOService(), _messageSender);

    // SubscriptionRequest
    std::string senderId = "SenderId";
    std::string receiverId = "ReceiverId";
    // SubscriptionQos
    std::int64_t minInterval_ms = 500;
    std::int64_t validity_ms = -1;
    std::int64_t publicationTtl_ms = 1000;
    auto qos = std::make_shared<OnChangeSubscriptionQos>(
            validity_ms, publicationTtl_ms, minInterval_ms);

    // will be deleted by the publication manager
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(qos);
    JOYNR_LOG_DEBUG(logger(), "adding request");
    publicationManager->add(
            senderId, receiverId, requestCaller, subscriptionRequest, mockPublicationSender);

    // Sleep so that the first publication is sent
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Fake many attribute changes - but expect only one publication to be sent by this loop
    for (int i = 0; i < 10; i++) {
        attributeListener->attributeValueChanged(std::move(attributeValue));
    }

    // Wait for the subscription to finish
    std::this_thread::sleep_for(std::chrono::milliseconds(700));
    publicationManager->shutdown();
}

TEST_F(PublicationManagerTest, add_onChangeWithMinInterval)
{
    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>(
            "tests/Test");

    auto mockPublicationSender = std::make_shared<MockPublicationSender>();
    auto requestCaller = std::make_shared<MockTestRequestCaller>();

    // The attribute will change to this value
    joynr::types::Localisation::GpsLocation attributeValue;

    SubscriptionRequest subscriptionRequest;

    SubscriptionPublication expectedPublication;
    expectedPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId());
    expectedPublication.setResponse(std::move(attributeValue));
    // Expect a single attribute change to send a publication + one publication when registering sub
    // request -> 2
    joynr::Semaphore semaphore(0);
    EXPECT_CALL(
            *mockPublicationSender,
            sendSubscriptionPublicationMock(
                    _,                                                  // sender participant ID
                    _,                                                  // receiver participant ID
                    _,                                                  // messaging QoS
                    SubscriptionPublicationMatcher(expectedPublication) // subscription publication
                    )).Times(2).WillOnce(ReleaseSemaphore(&semaphore));

    // Expect calls to register an unregister an attribute listener
    std::string attributeName("Location");
    std::shared_ptr<SubscriptionAttributeListener> attributeListener;

    EXPECT_CALL(*requestCaller, registerAttributeListener(attributeName, _))
            .Times(1)
            .WillRepeatedly(testing::SaveArg<1>(&attributeListener));

    EXPECT_CALL(*requestCaller, unregisterAttributeListener(attributeName, _)).Times(1);

    auto publicationManager = std::make_shared<PublicationManager>(
            _singleThreadedIOService->getIOService(), _messageSender);

    // SubscriptionRequest
    std::string senderId = "SenderId";
    std::string receiverId = "ReceiverId";
    // SubscriptionQos
    std::int64_t minInterval_ms = 1000;
    std::int64_t validity_ms = 1500;
    std::int64_t publicationTtl_ms = 2000;
    auto qos = std::make_shared<OnChangeSubscriptionQos>(
            validity_ms, publicationTtl_ms, minInterval_ms);

    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(qos);
    JOYNR_LOG_DEBUG(logger(), "adding request");
    publicationManager->add(
            senderId, receiverId, requestCaller, subscriptionRequest, mockPublicationSender);

    // Sleep so that the first publication is sent
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(500)));

    // Fake many attribute changes - but expect only one publication to be sent by this loop
    for (int i = 0; i < 10; i++) {
        attributeListener->attributeValueChanged(std::move(attributeValue));
    }

    // Wait for the subscription to finish
    std::this_thread::sleep_for(std::chrono::milliseconds(2800));
    publicationManager->shutdown();
}

TEST_F(PublicationManagerTest, attribute_add_withExistingSubscriptionId)
{
    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>(
            "tests/Test");

    // two request interpreters for the first and second add-API call
    auto mockPublicationSender = std::make_shared<MockPublicationSender>();
    auto requestCaller = std::make_shared<MockTestRequestCaller>();

    auto requestCaller2 = std::make_shared<MockTestRequestCaller>();
    auto mockPublicationSender2 = std::make_shared<MockPublicationSender>();

    // The attribute will change to this value
    joynr::types::Localisation::GpsLocation attributeValue;

    // Expect calls to register an unregister an attribute listener
    std::string attributeName("Location");
    std::shared_ptr<SubscriptionAttributeListener> attributeListener;

    SubscriptionRequest subscriptionRequest;

    SubscriptionPublication expectedPublication;
    expectedPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId());
    expectedPublication.setResponse(std::move(attributeValue));
    EXPECT_CALL(
            *mockPublicationSender,
            sendSubscriptionPublicationMock(
                    _,                                                  // sender participant ID
                    _,                                                  // receiver participant ID
                    _,                                                  // messaging QoS
                    SubscriptionPublicationMatcher(expectedPublication) // subscription publication
                    )).Times(3);

    EXPECT_CALL(
            *mockPublicationSender2,
            sendSubscriptionPublicationMock(
                    _,                                                  // sender participant ID
                    _,                                                  // receiver participant ID
                    _,                                                  // messaging QoS
                    SubscriptionPublicationMatcher(expectedPublication) // subscription publication
                    )).Times(2);

    EXPECT_CALL(*requestCaller, registerAttributeListener(attributeName, _))
            .Times(1)
            .WillRepeatedly(testing::SaveArg<1>(&attributeListener));

    EXPECT_CALL(*requestCaller, unregisterAttributeListener(attributeName, _)).Times(1);

    EXPECT_CALL(*requestCaller2, registerAttributeListener(attributeName, _))
            .Times(1)
            .WillRepeatedly(testing::SaveArg<1>(&attributeListener));

    EXPECT_CALL(*requestCaller2, unregisterAttributeListener(attributeName, _)).Times(1);

    auto publicationManager = std::make_shared<PublicationManager>(
            _singleThreadedIOService->getIOService(), _messageSender);

    // SubscriptionRequest
    std::string senderId = "SenderId";
    std::string receiverId = "ReceiverId";
    // SubscriptionQos
    std::int64_t minInterval_ms = 100;
    std::int64_t validity_ms = 600;
    std::int64_t publicationTtl_ms = 1000;
    auto qos = std::make_shared<OnChangeSubscriptionQos>(
            validity_ms, publicationTtl_ms, minInterval_ms);

    std::int64_t now = static_cast<std::int64_t>(joynr::TimeUtils::getCurrentMillisSinceEpoch());
    qos->setExpiryDateMs(now + 5000);

    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(qos);
    JOYNR_LOG_DEBUG(logger(), "adding attribute subscription request");
    publicationManager->add(
            senderId, receiverId, requestCaller, subscriptionRequest, mockPublicationSender);

    // Sleep so that the first publication is sent
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms + 50));

    // Fake attribute change
    attributeListener->attributeValueChanged(std::move(attributeValue));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    // now, we assume that two publications have been occured

    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms));

    // Fake attribute change
    attributeListener->attributeValueChanged(std::move(attributeValue));

    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms + 50));
    // now, we assume that three publications have been occured

    // now, let's update the subscription and check if the provided data is correctly processed by
    // the PublicationManager
    // will be deleted by the publication manager

    qos->setMinIntervalMs(minInterval_ms + 500);
    subscriptionRequest.setQos(qos);
    JOYNR_LOG_DEBUG(logger(), "update attribute subscription request");
    publicationManager->add(
            senderId, receiverId, requestCaller2, subscriptionRequest, mockPublicationSender2);

    // Sleep so that the first publication is sent
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms + 50));

    // Fake attribute change
    attributeListener->attributeValueChanged(std::move(attributeValue));

    // sleep, waiting for the async publication (which shouldn't come)
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // until now, only one publication should be arrived to mockPublicationSender2

    // Fake attribute change. This change shall not result in a new attribute value changed
    attributeListener->attributeValueChanged(std::move(attributeValue));

    // Wait for the subscription to finish
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms + 500));

    // now, we should got 2 publications on mockPublicationSender2
    publicationManager->shutdown();
}

TEST_F(PublicationManagerTest,
       attribute_add_withExistingSubscriptionId_testQos_withGreaterExpiryDate)
{
    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>(
            "tests/Test");

    auto mockPublicationSender = std::make_shared<MockPublicationSender>();
    auto requestCaller = std::make_shared<MockTestRequestCaller>();

    // The attribute will change to this value
    joynr::types::Localisation::GpsLocation attributeValue;

    // Expect calls to register an unregister an attribute listener
    std::string attributeName("Location");
    std::shared_ptr<SubscriptionAttributeListener> attributeListener;

    SubscriptionRequest subscriptionRequest;

    SubscriptionPublication expectedPublication;
    expectedPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId());
    expectedPublication.setResponse(std::move(attributeValue));
    EXPECT_CALL(
            *mockPublicationSender,
            sendSubscriptionPublicationMock(
                    _,                                                  // sender participant ID
                    _,                                                  // receiver participant ID
                    _,                                                  // messaging QoS
                    SubscriptionPublicationMatcher(expectedPublication) // subscription publication
                    )).Times(3);

    EXPECT_CALL(*requestCaller, registerAttributeListener(attributeName, _))
            .Times(2)
            .WillRepeatedly(testing::SaveArg<1>(&attributeListener));

    EXPECT_CALL(*requestCaller, unregisterAttributeListener(attributeName, _)).Times(2);

    auto publicationManager = std::make_shared<PublicationManager>(
            _singleThreadedIOService->getIOService(), _messageSender);

    // SubscriptionRequest
    std::string senderId = "SenderId";
    std::string receiverId = "ReceiverId";
    // SubscriptionQos
    std::int64_t minInterval_ms = 50;
    std::int64_t validity_ms = 600;
    std::int64_t publicationTtl_ms = 1000;
    std::int64_t testRelExpiryDate = 500;
    std::int64_t now = static_cast<std::int64_t>(joynr::TimeUtils::getCurrentMillisSinceEpoch());
    std::int64_t testAbsExpiryDate = now + testRelExpiryDate;

    auto qos = std::make_shared<OnChangeSubscriptionQos>(
            validity_ms, publicationTtl_ms, minInterval_ms);
    qos->setExpiryDateMs(testAbsExpiryDate);

    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(qos);

    JOYNR_LOG_DEBUG(logger(), "adding attribute subscription request");
    publicationManager->add(
            senderId, receiverId, requestCaller, subscriptionRequest, mockPublicationSender);

    // exceed the minInterval
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms + 50));

    // now, we expect that one publication has been performed

    // now, let's update the subscription and check if the provided data is correctly processed by
    // the PublicationManager
    // extend the expiry date
    qos->setExpiryDateMs(testAbsExpiryDate + 1000);
    subscriptionRequest.setQos(qos);
    JOYNR_LOG_DEBUG(logger(), "adding request");
    publicationManager->add(
            senderId, receiverId, requestCaller, subscriptionRequest, mockPublicationSender);

    // Sleep so that the first publication is sent
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    // now we expect that two puclications have been performed

    // now, exceed the original expiryDate, and make an attribute change
    std::this_thread::sleep_for(std::chrono::milliseconds(testRelExpiryDate));
    attributeListener->attributeValueChanged(std::move(attributeValue));

    // wait for the async publication
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    // now, three publications should be noticed

    // wait for the subscription to finish
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms + testRelExpiryDate));
    publicationManager->shutdown();
}

TEST_F(PublicationManagerTest, attribtue_add_withExistingSubscriptionId_testQos_withLowerExpiryDate)
{
    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>(
            "tests/Test");

    auto mockPublicationSender = std::make_shared<MockPublicationSender>();
    auto requestCaller = std::make_shared<MockTestRequestCaller>();

    // The attribute will change to this value
    joynr::types::Localisation::GpsLocation attributeValue;

    // Expect calls to register an unregister an attribute listener
    std::string attributeName("Location");
    std::shared_ptr<SubscriptionAttributeListener> attributeListener;

    SubscriptionRequest subscriptionRequest;

    SubscriptionPublication expectedPublication;
    expectedPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId());
    expectedPublication.setResponse(std::move(attributeValue));
    EXPECT_CALL(
            *mockPublicationSender,
            sendSubscriptionPublicationMock(
                    _,                                                  // sender participant ID
                    _,                                                  // receiver participant ID
                    _,                                                  // messaging QoS
                    SubscriptionPublicationMatcher(expectedPublication) // subscription publication
                    )).Times(3);

    EXPECT_CALL(*requestCaller, registerAttributeListener(attributeName, _))
            .Times(2)
            .WillRepeatedly(testing::SaveArg<1>(&attributeListener));

    EXPECT_CALL(*requestCaller, unregisterAttributeListener(attributeName, _)).Times(2);

    auto publicationManager = std::make_shared<PublicationManager>(
            _singleThreadedIOService->getIOService(), _messageSender);

    // SubscriptionRequest
    std::string senderId = "SenderId";
    std::string receiverId = "ReceiverId";
    // SubscriptionQos
    std::int64_t minInterval_ms = 50;
    std::int64_t validity_ms = 600;
    std::int64_t publicationTtl_ms = 1000;
    std::int64_t testExpiryDate_shift = 2500;
    std::int64_t testRelExpiryDate = 500 + testExpiryDate_shift;
    std::int64_t now = static_cast<std::int64_t>(joynr::TimeUtils::getCurrentMillisSinceEpoch());
    std::int64_t testAbsExpiryDate = now + testRelExpiryDate;
    auto qos = std::make_shared<OnChangeSubscriptionQos>(
            validity_ms, publicationTtl_ms, minInterval_ms);

    qos->setExpiryDateMs(testAbsExpiryDate);

    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(qos);
    JOYNR_LOG_DEBUG(logger(), "adding attribute subscription request");
    publicationManager->add(
            senderId, receiverId, requestCaller, subscriptionRequest, mockPublicationSender);

    // exceed the minInterval
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    attributeListener->attributeValueChanged(std::move(attributeValue));
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms + 50));

    // now, we expect that two publications have been performed

    // now, let's update the subscription and check if the provided data is correctly processed by
    // the PublicationManager
    // reduce the expiry date
    qos->setExpiryDateMs(testAbsExpiryDate - testExpiryDate_shift);
    subscriptionRequest.setQos(qos);
    JOYNR_LOG_DEBUG(logger(), "update attribute subscription request");
    publicationManager->add(
            senderId, receiverId, requestCaller, subscriptionRequest, mockPublicationSender);

    // Sleep so that the first publication is sent
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    // now we expect that three puclications have been performed

    // now, exceed the new expiryDate, and make an attribute change
    std::this_thread::sleep_for(
            std::chrono::milliseconds(testRelExpiryDate - testExpiryDate_shift));
    // now, the subscription should be death

    attributeListener->attributeValueChanged(std::move(attributeValue));

    // wait for the async publication, which shouldn't arrive
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    publicationManager->shutdown();
}

TEST_F(PublicationManagerTest, broadcast_add_withExistingSubscriptionId)
{
    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>(
            "tests/Test");

    auto mockPublicationSender = std::make_shared<MockPublicationSender>();
    auto requestCaller = std::make_shared<MockTestRequestCaller>();

    auto mockPublicationSender2 = std::make_shared<MockPublicationSender>();
    auto requestCaller2 = std::make_shared<MockTestRequestCaller>();

    // The broacast will fire this value
    joynr::types::Localisation::GpsLocation broadcastValue;

    // Expect calls to register and unregister an broadcast listener
    std::string broadcastName("Location");
    std::shared_ptr<UnicastBroadcastListener> broadcastListener;

    BroadcastSubscriptionRequest subscriptionRequest;
    BroadcastFilterParameters filterParameters;
    subscriptionRequest.setFilterParameters(filterParameters);

    SubscriptionPublication expectedPublication;
    expectedPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId());
    expectedPublication.setResponse(std::move(broadcastValue));

    EXPECT_CALL(
            *mockPublicationSender,
            sendSubscriptionPublicationMock(
                    _,                                                  // sender participant ID
                    _,                                                  // receiver participant ID
                    _,                                                  // messaging QoS
                    SubscriptionPublicationMatcher(expectedPublication) // subscription publication
                    )).Times(2);

    EXPECT_CALL(
            *mockPublicationSender2,
            sendSubscriptionPublicationMock(
                    _,                                                  // sender participant ID
                    _,                                                  // receiver participant ID
                    _,                                                  // messaging QoS
                    SubscriptionPublicationMatcher(expectedPublication) // subscription publication
                    )).Times(1);

    EXPECT_CALL(*requestCaller, registerBroadcastListener(broadcastName, _))
            .Times(1)
            .WillRepeatedly(testing::SaveArg<1>(&broadcastListener));

    EXPECT_CALL(*requestCaller, unregisterBroadcastListener(broadcastName, _)).Times(1);

    EXPECT_CALL(*requestCaller2, registerBroadcastListener(broadcastName, _))
            .Times(1)
            .WillRepeatedly(testing::SaveArg<1>(&broadcastListener));

    EXPECT_CALL(*requestCaller2, unregisterBroadcastListener(broadcastName, _)).Times(1);

    auto publicationManager = std::make_shared<PublicationManager>(
            _singleThreadedIOService->getIOService(), _messageSender);

    // SubscriptionRequest
    std::string senderId = "SenderId";
    std::string receiverId = "ReceiverId";
    // SubscriptionQos
    std::int64_t minInterval_ms = 100;
    std::int64_t validity_ms = 600;
    std::int64_t publicationTtl_ms = 1000;
    auto qos = std::make_shared<OnChangeSubscriptionQos>(
            validity_ms, publicationTtl_ms, minInterval_ms);

    std::int64_t now = static_cast<std::int64_t>(joynr::TimeUtils::getCurrentMillisSinceEpoch());
    qos->setExpiryDateMs(now + 5000);

    subscriptionRequest.setSubscribeToName(broadcastName);
    subscriptionRequest.setQos(qos);
    JOYNR_LOG_DEBUG(logger(), "adding broadcast subscription request");
    publicationManager->add(
            senderId, receiverId, requestCaller, subscriptionRequest, mockPublicationSender);

    // Fake broadcast
    broadcastListener->broadcastOccurred(broadcastValue);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // now, we assume that one publication has been occured
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms));

    // Fake broadcast
    broadcastListener->broadcastOccurred(broadcastValue);

    std::int64_t newMinInterval = minInterval_ms + 500;
    std::this_thread::sleep_for(std::chrono::milliseconds(50 + newMinInterval));
    // now, we assume that two publications have been occured

    // now, let's update the subscription an check if the provided data is correctly processed by
    // the PublicationManager

    qos->setMinIntervalMs(newMinInterval);
    subscriptionRequest.setQos(qos);
    JOYNR_LOG_DEBUG(logger(), "update broadcast subscription request");
    publicationManager->add(
            senderId, receiverId, requestCaller2, subscriptionRequest, mockPublicationSender2);

    // Fake broadcast
    broadcastListener->broadcastOccurred(broadcastValue);

    // sleep, waiting for the async publication (which shouldn't come)
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    // until now, only one publication should be arrived to mockPublicationSender2

    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms + 50));

    // Fake broadcast. This change shall not result in a new broadcast to the client
    broadcastListener->broadcastOccurred(broadcastValue);

    // Wait for the subscription to finish
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    publicationManager->shutdown();
}

TEST_F(PublicationManagerTest,
       broadcast_add_withExistingSubscriptionId_testQos_withGreaterExpiryDate)
{
    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>(
            "tests/Test");

    auto mockPublicationSender = std::make_shared<MockPublicationSender>();
    auto requestCaller = std::make_shared<MockTestRequestCaller>();

    // The broacast will fire this value
    joynr::types::Localisation::GpsLocation broadcastValue;

    // Expect calls to register an unregister an broadcast listener
    std::string broadcastName("Location");
    std::shared_ptr<UnicastBroadcastListener> broadcastListener;

    BroadcastSubscriptionRequest subscriptionRequest;
    BroadcastFilterParameters filterParameters;
    subscriptionRequest.setFilterParameters(filterParameters);

    SubscriptionPublication expectedPublication;
    expectedPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId());
    expectedPublication.setResponse(std::move(broadcastValue));

    EXPECT_CALL(
            *mockPublicationSender,
            sendSubscriptionPublicationMock(
                    _,                                                  // sender participant ID
                    _,                                                  // receiver participant ID
                    _,                                                  // messaging QoS
                    SubscriptionPublicationMatcher(expectedPublication) // subscription publication
                    )).Times(2);

    EXPECT_CALL(*requestCaller, registerBroadcastListener(broadcastName, _))
            .Times(2)
            .WillRepeatedly(testing::SaveArg<1>(&broadcastListener));

    EXPECT_CALL(*requestCaller, unregisterBroadcastListener(broadcastName, _)).Times(2);

    auto publicationManager = std::make_shared<PublicationManager>(
            _singleThreadedIOService->getIOService(), _messageSender);

    // SubscriptionRequest
    std::string senderId = "SenderId";
    std::string receiverId = "ReceiverId";
    // SubscriptionQos
    std::int64_t minInterval_ms = 50;
    std::int64_t validity_ms = 600;
    std::int64_t publicationTtl_ms = 1000;
    std::int64_t testRelExpiryDate = 500;
    std::int64_t now = static_cast<std::int64_t>(joynr::TimeUtils::getCurrentMillisSinceEpoch());
    std::int64_t testAbsExpiryDate = now + testRelExpiryDate;
    auto qos = std::make_shared<OnChangeSubscriptionQos>(
            validity_ms, publicationTtl_ms, minInterval_ms);

    qos->setExpiryDateMs(testAbsExpiryDate);

    subscriptionRequest.setSubscribeToName(broadcastName);
    subscriptionRequest.setQos(qos);
    JOYNR_LOG_DEBUG(logger(), "add broadcast subscription request");
    publicationManager->add(
            senderId, receiverId, requestCaller, subscriptionRequest, mockPublicationSender);

    broadcastListener->broadcastOccurred(broadcastValue);
    // exceed the minInterval
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms + 50));

    // now, we expect that one publication has been performed

    // now, let's update the subscription and check if the provided data is correctly processed by
    // the PublicationManager
    // extend the expiry date
    qos->setExpiryDateMs(testAbsExpiryDate + 1000);
    subscriptionRequest.setQos(qos);
    JOYNR_LOG_DEBUG(logger(), "update broadcast subscription request");
    publicationManager->add(
            senderId, receiverId, requestCaller, subscriptionRequest, mockPublicationSender);

    // now, exceed the original expiryDate, and make a broadcast
    std::this_thread::sleep_for(std::chrono::milliseconds(testRelExpiryDate));
    broadcastListener->broadcastOccurred(broadcastValue);

    // wait for the async publication
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    // now, two publications should be noticed, even if the original subscription is expired
    publicationManager->shutdown();
}

TEST_F(PublicationManagerTest, broadcast_add_withExistingSubscriptionId_testQos_withLowerExpiryDate)
{
    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>(
            "tests/Test");

    auto mockPublicationSender = std::make_shared<MockPublicationSender>();
    auto requestCaller = std::make_shared<MockTestRequestCaller>();

    // The broacast will fire this value
    joynr::types::Localisation::GpsLocation broadcastValue;

    // Expect calls to register an unregister an broadcast listener
    std::string broadcastName("Location");
    std::shared_ptr<UnicastBroadcastListener> broadcastListener;

    BroadcastSubscriptionRequest subscriptionRequest;
    BroadcastFilterParameters filterParameters;
    subscriptionRequest.setFilterParameters(filterParameters);

    SubscriptionPublication expectedPublication;
    expectedPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId());
    expectedPublication.setResponse(std::move(broadcastValue));

    EXPECT_CALL(
            *mockPublicationSender,
            sendSubscriptionPublicationMock(
                    _,                                                  // sender participant ID
                    _,                                                  // receiver participant ID
                    _,                                                  // messaging QoS
                    SubscriptionPublicationMatcher(expectedPublication) // subscription publication
                    )).Times(1);

    EXPECT_CALL(*requestCaller, registerBroadcastListener(broadcastName, _))
            .Times(2)
            .WillRepeatedly(testing::SaveArg<1>(&broadcastListener));

    EXPECT_CALL(*requestCaller, unregisterBroadcastListener(broadcastName, _)).Times(2);

    auto publicationManager = std::make_shared<PublicationManager>(
            _singleThreadedIOService->getIOService(), _messageSender);

    // SubscriptionRequest
    std::string senderId = "SenderId";
    std::string receiverId = "ReceiverId";
    // SubscriptionQos
    std::int64_t minInterval_ms = 50;
    std::int64_t validity_ms = 600;
    std::int64_t publicationTtl_ms = 1000;
    std::int64_t testExpiryDate_shift = 2500;
    std::int64_t testRelExpiryDate = 500 + testExpiryDate_shift;
    std::int64_t now = static_cast<std::int64_t>(joynr::TimeUtils::getCurrentMillisSinceEpoch());
    std::int64_t testAbsExpiryDate = now + testRelExpiryDate;
    auto qos = std::make_shared<OnChangeSubscriptionQos>(
            validity_ms, publicationTtl_ms, minInterval_ms);

    qos->setExpiryDateMs(testAbsExpiryDate);

    subscriptionRequest.setSubscribeToName(broadcastName);
    subscriptionRequest.setQos(qos);
    JOYNR_LOG_DEBUG(logger(), "adding broadcast subscription request");
    publicationManager->add(
            senderId, receiverId, requestCaller, subscriptionRequest, mockPublicationSender);

    broadcastListener->broadcastOccurred(broadcastValue);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // now, we expect that one publications have been performed

    // now, let's update the subscription and check if the provided data is correctly processed by
    // the PublicationManager
    // reduce the expiry date
    qos->setExpiryDateMs(testAbsExpiryDate - testExpiryDate_shift);
    subscriptionRequest.setQos(qos);
    JOYNR_LOG_DEBUG(logger(), "update broadcast subscription request");
    publicationManager->add(
            senderId, receiverId, requestCaller, subscriptionRequest, mockPublicationSender);

    // now, exceed the new expiryDate, and make a broadcast
    std::this_thread::sleep_for(
            std::chrono::milliseconds(testRelExpiryDate - testExpiryDate_shift));
    // now, the subscription should be death

    broadcastListener->broadcastOccurred(broadcastValue);

    // wait for the async publication (which shouldn't arrive)
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // now, no new publication should be received, even if the expiry date of the original request
    // hasn't been expired
    // -> one publication expected
    publicationManager->shutdown();
}

TEST_F(PublicationManagerTest, remove_onChangeSubscription)
{
    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>(
            "tests/Test");

    auto mockPublicationSender = std::make_shared<MockPublicationSender>();
    auto requestCaller = std::make_shared<MockTestRequestCaller>();

    // A publication should never be sent
    EXPECT_CALL(*mockPublicationSender,
                sendSubscriptionPublicationMock(_, // sender participant ID
                                                _, // receiver participant ID
                                                _, // messaging QoS
                                                _  // subscription publication
                                                )).Times(1);

    // Expect calls to register an unregister an attribute listener
    std::string attributeName("Location");
    std::shared_ptr<SubscriptionAttributeListener> attributeListener;

    EXPECT_CALL(*requestCaller, registerAttributeListener(attributeName, _))
            .Times(1)
            .WillRepeatedly(testing::SaveArg<1>(&attributeListener));

    joynr::Semaphore semaphore(0);
    EXPECT_CALL(*requestCaller, unregisterAttributeListener(attributeName, _))
            .Times(1)
            .WillOnce(ReleaseSemaphore(&semaphore));

    auto publicationManager = std::make_shared<PublicationManager>(
            _singleThreadedIOService->getIOService(), _messageSender);

    // SubscriptionRequest
    std::string senderId = "SenderId";
    std::string receiverId = "ReceiverId";
    // SubscriptionQos
    std::int64_t minInterval_ms = 1;
    std::int64_t validity_ms = 200;
    std::int64_t publicationTtl_ms = 1000;
    auto qos = std::make_shared<OnChangeSubscriptionQos>(
            validity_ms, publicationTtl_ms, minInterval_ms);

    // will be deleted by the publication manager
    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(qos);
    JOYNR_LOG_DEBUG(logger(), "adding request");
    publicationManager->add(
            senderId, receiverId, requestCaller, subscriptionRequest, mockPublicationSender);

    // Wait for the subscription to expire
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(400)));
    publicationManager->shutdown();
}

TEST_F(PublicationManagerTest, forwardProviderRuntimeExceptionToPublicationSender)
{
    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>(
            "tests/Test");

    auto mockPublicationSender = std::make_shared<MockPublicationSender>();
    auto requestCaller = std::make_shared<MockTestRequestCaller>();

    // The value will be fired by the broadcast
    auto expected = std::make_shared<exceptions::ProviderRuntimeException>(
            requestCaller->_providerRuntimeExceptionTestMsg);

    // SubscriptionRequest
    const std::string senderId = "SenderId";
    const std::string receiverId = "ReceiverId";
    const std::string attributeName("attributeWithProviderRuntimeException");
    const std::int64_t period_ms = 200;
    const std::int64_t validity_ms = 1000;
    const std::int64_t publicationTtl_ms = 1000;
    const std::int64_t alertInterval_ms = 1000;
    const auto qos = std::make_shared<PeriodicSubscriptionQos>(
            validity_ms, publicationTtl_ms, period_ms, alertInterval_ms);
    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(qos);

    SubscriptionPublication expectedPublication;
    expectedPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId());
    expectedPublication.setError(expected);

    EXPECT_CALL(
            *mockPublicationSender,
            sendSubscriptionPublicationMock(
                    Eq(receiverId),                                     // sender participant ID
                    Eq(senderId),                                       // receiver participant ID
                    _,                                                  // messaging QoS
                    SubscriptionPublicationMatcher(expectedPublication) // subscription publication
                    )).Times(2);

    auto publicationManager = std::make_shared<PublicationManager>(
            _singleThreadedIOService->getIOService(), _messageSender);

    publicationManager->add(
            senderId, receiverId, requestCaller, subscriptionRequest, mockPublicationSender);

    // wait for the async publication
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    // now, two publications should be noticed, even if the original subscription is expired
    publicationManager->shutdown();
}

TEST_F(PublicationManagerTest, forwardMethodInvocationExceptionToPublicationSender)
{
    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>(
            "tests/Test");

    auto mockPublicationSender = std::make_shared<MockPublicationSender>();
    auto requestCaller = std::make_shared<MockTestRequestCaller>();

    // The value will be fired by the broadcast
    auto expected = std::make_shared<exceptions::MethodInvocationException>(
            "unknown method name for interface test: getNotExistingAttribute");

    // SubscriptionRequest
    const std::string senderId = "SenderId";
    const std::string receiverId = "ReceiverId";
    const std::string attributeName("notExistingAttribute");
    // SubscriptionQos
    const std::int64_t period_ms = 100;
    const std::int64_t validity_ms = 1000;
    const std::int64_t publicationTtl_ms = 1000;
    const std::int64_t alertInterval_ms = 1000;
    const auto qos = std::make_shared<PeriodicSubscriptionQos>(
            validity_ms, publicationTtl_ms, period_ms, alertInterval_ms);
    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(qos);

    SubscriptionPublication expectedPublication;
    expectedPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId());
    expectedPublication.setError(expected);

    EXPECT_CALL(
            *mockPublicationSender,
            sendSubscriptionPublicationMock(
                    Eq(receiverId),                                     // sender participant ID
                    Eq(senderId),                                       // receiver participant ID
                    _,                                                  // messaging QoS
                    SubscriptionPublicationMatcher(expectedPublication) // subscription publication
                    )).Times(2);

    auto publicationManager = std::make_shared<PublicationManager>(
            _singleThreadedIOService->getIOService(), _messageSender);

    publicationManager->add(
            senderId, receiverId, requestCaller, subscriptionRequest, mockPublicationSender);

    // wait for the async publication
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    // now, two publications should be noticed, even if the original subscription is expired
    publicationManager->shutdown();
}

void PublicationManagerTest::sendSubscriptionReplyOnSuccessfulRegistration(
        SubscriptionRequest& subscriptionRequest)
{
    joynr::Semaphore semaphore(0);
    auto mockPublicationSender = std::make_shared<MockPublicationSender>();
    auto publicationManager = std::make_shared<PublicationManager>(
            _singleThreadedIOService->getIOService(), _messageSender);
    auto requestCaller = std::make_shared<MockTestRequestCaller>();

    std::string proxyId = "ProxyId";
    std::string providerId = "ProviderId";
    std::string subscriptionId = "testSubscriptionId";
    subscriptionRequest.setSubscriptionId(subscriptionId);

    // expected subscription reply
    SubscriptionReply expectedSubscriptionReply;
    expectedSubscriptionReply.setSubscriptionId(subscriptionId);

    EXPECT_CALL(*mockPublicationSender,
                sendSubscriptionReply(Eq(providerId),               // sender participant ID
                                      Eq(proxyId),                  // receiver participant ID
                                      _,                            // messaging QoS
                                      Eq(expectedSubscriptionReply) // subscription reply
                                      ))
            .Times(1)
            .WillOnce(ReleaseSemaphore(&semaphore));

    JOYNR_LOG_DEBUG(logger(), "adding subscription request");
    publicationManager->add(
            proxyId, providerId, requestCaller, subscriptionRequest, mockPublicationSender);

    // remove subscription before deletion of mockPublicationSender
    publicationManager->removeAllSubscriptions(providerId);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(5000)));
    publicationManager->shutdown();
}

TEST_F(PublicationManagerTest, attribute_sendSubscriptionReplyOnSuccessfulRegistration)
{
    SubscriptionRequest subscriptionRequest;
    std::string subscribeToName = "testAttribute";
    subscriptionRequest.setSubscribeToName(subscribeToName);
    subscriptionRequest.setQos(std::make_shared<OnChangeSubscriptionQos>());
    sendSubscriptionReplyOnSuccessfulRegistration(subscriptionRequest);
}

TEST_F(PublicationManagerTest, broadcast_sendSubscriptionReplyOnSuccessfulRegistration)
{
    BroadcastSubscriptionRequest subscriptionRequest;
    std::string subscribeToName = "testBroadcast";
    subscriptionRequest.setSubscribeToName(subscribeToName);
    subscriptionRequest.setQos(std::make_shared<OnChangeSubscriptionQos>());
    sendSubscriptionReplyOnSuccessfulRegistration(subscriptionRequest);
}

TEST_F(PublicationManagerTest, multicast_sendSubscriptionReplyOnSuccessfulRegistration)
{
    MulticastSubscriptionRequest subscriptionRequest;
    std::string subscribeToName = "testBroadcast";
    subscriptionRequest.setSubscribeToName(subscribeToName);
    subscriptionRequest.setMulticastId("multicastId");
    subscriptionRequest.setQos(std::make_shared<MulticastSubscriptionQos>());
    sendSubscriptionReplyOnSuccessfulRegistration(subscriptionRequest);
}

void PublicationManagerTest::sendSubscriptionExceptionOnExpiredRegistration(
        SubscriptionRequest& subscriptionRequest)
{
    joynr::Semaphore semaphore(0);
    auto mockPublicationSender = std::make_shared<MockPublicationSender>();
    auto publicationManager = std::make_shared<PublicationManager>(
            _singleThreadedIOService->getIOService(), _messageSender);
    auto requestCaller = std::make_shared<MockTestRequestCaller>();

    std::string proxyId = "ProxyId";
    std::string providerId = "ProviderId";
    std::string subscriptionId = "testSubscriptionId";
    subscriptionRequest.setSubscriptionId(subscriptionId);
    // SubscriptionQos
    std::int64_t minInterval_ms = 50;
    std::int64_t validity_ms = 0;
    std::int64_t publicationTtl_ms = 1000;
    auto qos = std::make_shared<OnChangeSubscriptionQos>(
            validity_ms, publicationTtl_ms, minInterval_ms);
    subscriptionRequest.setQos(qos);

    // expected subscription reply
    SubscriptionReply expectedSubscriptionReply;
    expectedSubscriptionReply.setSubscriptionId(subscriptionId);
    auto exception = std::make_shared<exceptions::SubscriptionException>(
            "publication end is in the past", subscriptionId);
    expectedSubscriptionReply.setError(exception);

    EXPECT_CALL(*mockPublicationSender,
                sendSubscriptionReply(Eq(providerId),               // sender participant ID
                                      Eq(proxyId),                  // receiver participant ID
                                      _,                            // messaging QoS
                                      Eq(expectedSubscriptionReply) // subscription reply
                                      ))
            .Times(1)
            .WillOnce(ReleaseSemaphore(&semaphore));

    // wait some time to ensure the subscription is expired
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    JOYNR_LOG_DEBUG(logger(), "adding request");
    publicationManager->add(
            proxyId, providerId, requestCaller, subscriptionRequest, mockPublicationSender);
    // remove subscription before deletion of mockPublicationSender
    publicationManager->removeAllSubscriptions(providerId);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(5000)));
    publicationManager->shutdown();
}

TEST_F(PublicationManagerTest, attribute_sendSubscriptionExceptionOnExpiredSubscriptionRequest)
{
    // SubscriptionRequest
    SubscriptionRequest subscriptionRequest;
    std::string subscribeToName = "testAttribute";
    subscriptionRequest.setSubscribeToName(subscribeToName);

    sendSubscriptionExceptionOnExpiredRegistration(subscriptionRequest);
}

TEST_F(PublicationManagerTest, broadcast_sendSubscriptionExceptionOnExpiredSubscriptionRequest)
{
    // SubscriptionRequest
    BroadcastSubscriptionRequest subscriptionRequest;
    std::string subscribeToName = "testBroadcast";
    subscriptionRequest.setSubscribeToName(subscribeToName);

    sendSubscriptionExceptionOnExpiredRegistration(subscriptionRequest);
}
