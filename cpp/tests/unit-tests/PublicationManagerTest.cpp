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
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "tests/utils/MockObjects.h"
#include "joynr/InterfaceRegistrar.h"
#include "joynr/tests/testRequestInterpreter.h"
#include "joynr/SubscriptionPublication.h"
#include "joynr/IAttributeListener.h"
#include "joynr/IBroadcastListener.h"
#include "joynr/PeriodicSubscriptionQos.h"
#include "joynr/LibjoynrSettings.h"
#include "tests/utils/TimeUtils.h"
#include "joynr/Logger.h"
#include "joynr/exceptions/MethodInvocationException.h"
#include <thread>

using ::testing::A;
using ::testing::_;
using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Between;
using ::testing::ReturnRef;
using ::testing::MatcherInterface;
using ::testing::MatchResultListener;
using ::testing::Matcher;
using ::testing::MakeMatcher;
#include <string>
#include <cstdint>
#include <algorithm>

using namespace joynr;

class PublicationManagerTest : public testing::Test {
public:
    void TearDown(){
        std::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME().c_str()); //remove stored subscriptions
        std::remove(LibjoynrSettings::DEFAULT_BROADCASTSUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME().c_str()); //remove stored broadcastsubscriptions
    }
protected:
    ADD_LOGGER(PublicationManagerTest);
};

INIT_LOGGER(PublicationManagerTest);

// New matcher definition that works with the latest Google Mock.
class SubscriptionPublicationMatcherInterface : public MatcherInterface<const SubscriptionPublication&> {
  private:
     SubscriptionPublication& expectedSubscriptionPublication;
  public:
     explicit SubscriptionPublicationMatcherInterface (SubscriptionPublication& expectedSubscriptionPublication) : expectedSubscriptionPublication(expectedSubscriptionPublication){}
  virtual void DescribeTo(::std::ostream* os) const {}
  virtual bool MatchAndExplain(const SubscriptionPublication& subscriptionPublication,
                               MatchResultListener* listener) const {
    return subscriptionPublication == expectedSubscriptionPublication;
  }
};

inline Matcher<const SubscriptionPublication&> SubscriptionPublicationMatcher(SubscriptionPublication& expectedValue) {
  return MakeMatcher(new SubscriptionPublicationMatcherInterface(expectedValue));
}

TEST_F(PublicationManagerTest, add_requestCallerIsCalledCorrectlyByPublisherRunnables) {
    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<joynr::tests::testRequestInterpreter>(joynr::tests::testProvider::INTERFACE_NAME());

    MockPublicationSender mockPublicationSender;
    MockTestRequestCaller* mockTestRequestCaller = new MockTestRequestCaller(Between(3, 5));

    // NOTE: it depends on the timing and especially on the CPU load of
    // the current machine how often the publication is exectuted. Hence,
    // we expect the publication to haben between 3 or 5 times.
    EXPECT_CALL(mockPublicationSender,
                sendSubscriptionPublication(_,_,_,_))
            .Times(Between(3, 5));

    std::shared_ptr<MockTestRequestCaller> requestCaller(mockTestRequestCaller);
    PublicationManager publicationManager;

    //SubscriptionRequest
    std::string senderId = "SenderId";
    std::string receiverId = "ReceiverId";
    std::string attributeName("Location");
    //SubscriptionQos
    std::int64_t period_ms = 100;
    std::int64_t validity_ms = 500;
    std::int64_t alertInterval_ms = 2000;
    Variant qos = Variant::make<PeriodicSubscriptionQos>(PeriodicSubscriptionQos(
                        validity_ms,
                        period_ms,
                        alertInterval_ms));

    // will be deleted by the publication manager
    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(qos);
    JOYNR_LOG_DEBUG(logger, "adding request");
    publicationManager.add(senderId, receiverId, requestCaller,subscriptionRequest,&mockPublicationSender);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}


TEST_F(PublicationManagerTest, stop_publications) {
    MockPublicationSender mockPublicationSender;
    MockTestRequestCaller* mockTestRequestCaller = new MockTestRequestCaller(AtMost(2));

    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>("tests/Test");

    EXPECT_CALL(mockPublicationSender,
                sendSubscriptionPublication(_,_,_,_))
            .Times(AtMost(2));

    std::shared_ptr<MockTestRequestCaller> requestCaller(mockTestRequestCaller);
    PublicationManager publicationManager;

    //SubscriptionRequest
    std::string senderId = "SenderId";
    std::string receiverId = "ReceiverId";
    std::string attributeName("Location");
    //SubscriptionQos
    std::int64_t period_ms = 100;
    std::int64_t validity_ms = 10000;
    std::int64_t alertInterval_ms = 1000;
    Variant qos = Variant::make<PeriodicSubscriptionQos>(PeriodicSubscriptionQos(
                        validity_ms,
                        period_ms,
                        alertInterval_ms));

    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(qos);

    publicationManager.add(
                senderId,
                receiverId,
                requestCaller,
                subscriptionRequest,
                &mockPublicationSender
    );
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    publicationManager.stopPublication(subscriptionRequest.getSubscriptionId());
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
}

TEST_F(PublicationManagerTest, remove_all_publications) {
    MockPublicationSender mockPublicationSender;
    MockTestRequestCaller* mockTestRequestCaller = new MockTestRequestCaller(AtMost(2));

    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>("tests/Test");

    EXPECT_CALL(mockPublicationSender,
                sendSubscriptionPublication(_,_,_,_))
            .Times(AtMost(2));

    std::shared_ptr<MockTestRequestCaller> requestCaller(mockTestRequestCaller);
    PublicationManager publicationManager;

    //SubscriptionRequest
    std::string senderId = "SenderId";
    std::string receiverId = "ReceiverId";
    std::string attributeName("Location");
    //SubscriptionQos
    std::int64_t period_ms = 100;
    std::int64_t validity_ms = 10000;
    std::int64_t alertInterval_ms = 1000;
    Variant qos = Variant::make<PeriodicSubscriptionQos>(PeriodicSubscriptionQos(
                        validity_ms,
                        period_ms,
                        alertInterval_ms));

    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(qos);

    publicationManager.add(senderId, receiverId, requestCaller,subscriptionRequest,&mockPublicationSender);
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    publicationManager.removeAllSubscriptions(receiverId);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
}

TEST_F(PublicationManagerTest, add_onChangeSubscription) {
    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>("tests/Test");

    MockPublicationSender mockPublicationSender;
    MockTestRequestCaller* mockTestRequestCaller = new MockTestRequestCaller();

    // The attribute will change to this value
    joynr::types::Localisation::GpsLocation gpsLocation;
    Variant attributeValue = Variant::make<joynr::types::Localisation::GpsLocation>(gpsLocation);

    SubscriptionRequest subscriptionRequest;

    SubscriptionPublication expectedPublication;
    expectedPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId());
    std::vector<Variant> response;
    response.push_back(attributeValue);
    expectedPublication.setResponseVariant(response);
    // Expect an attribute change to send a publication as well as during registering subscription request
    EXPECT_CALL(
                mockPublicationSender,
                sendSubscriptionPublication(
                    _, // sender participant ID
                    _, // receiver participant ID
                    _, // messaging QoS
                    SubscriptionPublicationMatcher(expectedPublication) // subscription publication
                )
    )
            .Times(Between(1, 2));

    // Expect a call to set up the on change subscription
    std::string attributeName = "Location";
    IAttributeListener* attributeListener;
    EXPECT_CALL(
                *mockTestRequestCaller,
                registerAttributeListener(attributeName,_)
    )
            .Times(1)
            .WillRepeatedly(testing::SaveArg<1>(&attributeListener));

    // Expect a call to remove the on change subscription
    EXPECT_CALL(
                *mockTestRequestCaller,
                unregisterAttributeListener(attributeName,_)
    )
            .Times(1);

    std::shared_ptr<MockTestRequestCaller> requestCaller(mockTestRequestCaller);
    PublicationManager publicationManager;

    //SubscriptionRequest
    std::string senderId = "SenderId";
    std::string receiverId = "ReceiverId";
    //SubscriptionQos
    std::int64_t minInterval_ms = 50;
    std::int64_t validity_ms = 500;
    Variant qos = Variant::make<OnChangeSubscriptionQos>(OnChangeSubscriptionQos(
                        validity_ms,
                        minInterval_ms));

    // will be deleted by the publication manager
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(qos);
    JOYNR_LOG_DEBUG(logger, "adding request");
    publicationManager.add(senderId, receiverId, requestCaller, subscriptionRequest, &mockPublicationSender);

    // Fake an attribute change
    attributeListener->attributeValueChanged(attributeValue);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

TEST_F(PublicationManagerTest, add_onChangeWithNoExpiryDate) {
    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>("tests/Test");

    MockPublicationSender mockPublicationSender;
    MockTestRequestCaller* mockTestRequestCaller = new MockTestRequestCaller();

    // The attribute will change to this value
    joynr::types::Localisation::GpsLocation gpsLocation;
    Variant attributeValue = Variant::make<joynr::types::Localisation::GpsLocation>(gpsLocation);

    SubscriptionRequest subscriptionRequest;

    SubscriptionPublication expectedPublication;
    expectedPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId());
    std::vector<Variant> response;
    response.push_back(attributeValue);
    expectedPublication.setResponseVariant(response);
    // Expect a single attribute change to send a publication + one publication when registering sub request -> 2
    EXPECT_CALL(
                mockPublicationSender,
                sendSubscriptionPublication(
                    _, // sender participant ID
                    _, // receiver participant ID
                    _, // messaging QoS
                    SubscriptionPublicationMatcher(expectedPublication) // subscription publication
                )
    )
            .Times(2);

    // Expect calls to register an unregister an attribute listener
    std::string attributeName("Location");
    IAttributeListener* attributeListener;

    EXPECT_CALL(*mockTestRequestCaller,registerAttributeListener(attributeName, _))
            .Times(1)
            .WillRepeatedly(testing::SaveArg<1>(&attributeListener));

    EXPECT_CALL(*mockTestRequestCaller,unregisterAttributeListener(attributeName, _)).Times(1);

    std::shared_ptr<MockTestRequestCaller> requestCaller(mockTestRequestCaller);
    PublicationManager publicationManager;

    //SubscriptionRequest
    std::string senderId = "SenderId";
    std::string receiverId = "ReceiverId";
    //SubscriptionQos
    std::int64_t minInterval_ms = 500;
    std::int64_t validity_ms = -1; //no expiry date -> infinite subscription
    Variant qos = Variant::make<OnChangeSubscriptionQos>(OnChangeSubscriptionQos(
                        validity_ms,
                        minInterval_ms));

    // will be deleted by the publication manager
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(qos);
    JOYNR_LOG_DEBUG(logger, "adding request");
    publicationManager.add(senderId, receiverId, requestCaller,subscriptionRequest,&mockPublicationSender);

    // Sleep so that the first publication is sent
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Fake many attribute changes - but expect only one publication to be sent by this loop
    for (int i = 0; i < 10; i++) {
        attributeListener->attributeValueChanged(attributeValue);
    }

    // Wait for the subscription to finish
    std::this_thread::sleep_for(std::chrono::milliseconds(700));

}


TEST_F(PublicationManagerTest, add_onChangeWithMinInterval) {
    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>("tests/Test");

    MockPublicationSender mockPublicationSender;
    MockTestRequestCaller* mockTestRequestCaller = new MockTestRequestCaller();

    // The attribute will change to this value
    joynr::types::Localisation::GpsLocation gpsLocation;
    Variant attributeValue = Variant::make<joynr::types::Localisation::GpsLocation>(gpsLocation);

    SubscriptionRequest subscriptionRequest;

    SubscriptionPublication expectedPublication;
    expectedPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId());
    std::vector<Variant> response;
    response.push_back(attributeValue);
    expectedPublication.setResponseVariant(response);
    // Expect a single attribute change to send a publication + one publication when registering sub request -> 2
    EXPECT_CALL(
                mockPublicationSender,
                sendSubscriptionPublication(
                    _, // sender participant ID
                    _, // receiver participant ID
                    _, // messaging QoS
                    SubscriptionPublicationMatcher(expectedPublication) // subscription publication
                )
    )
            .Times(2);

    // Expect calls to register an unregister an attribute listener
    std::string attributeName("Location");
    IAttributeListener* attributeListener;

    EXPECT_CALL(*mockTestRequestCaller,registerAttributeListener(attributeName, _))
            .Times(1)
            .WillRepeatedly(testing::SaveArg<1>(&attributeListener));

    EXPECT_CALL(*mockTestRequestCaller,unregisterAttributeListener(attributeName, _)).Times(1);

    std::shared_ptr<MockTestRequestCaller> requestCaller(mockTestRequestCaller);
    PublicationManager publicationManager;

    //SubscriptionRequest
    std::string senderId = "SenderId";
    std::string receiverId = "ReceiverId";
    //SubscriptionQos
    std::int64_t minInterval_ms = 500;
    std::int64_t validity_ms = 600;
    Variant qos = Variant::make<OnChangeSubscriptionQos>(OnChangeSubscriptionQos(
                        validity_ms,
                        minInterval_ms));

    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(qos);
    JOYNR_LOG_DEBUG(logger, "adding request");
    publicationManager.add(senderId, receiverId, requestCaller,subscriptionRequest,&mockPublicationSender);

    // Sleep so that the first publication is sent
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Fake many attribute changes - but expect only one publication to be sent by this loop
    for (int i = 0; i < 10; i++) {
        attributeListener->attributeValueChanged(attributeValue);
    }

    // Wait for the subscription to finish
    std::this_thread::sleep_for(std::chrono::milliseconds(700));

}

TEST_F(PublicationManagerTest, attribute_add_withExistingSubscriptionId) {
    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>("tests/Test");

    // two request interpreters for the first and second add-API call
    MockPublicationSender mockPublicationSender;
    MockTestRequestCaller* mockTestRequestCaller = new MockTestRequestCaller();

    MockTestRequestCaller* mockTestRequestCaller2 = new MockTestRequestCaller();
    MockPublicationSender mockPublicationSender2;

    // The attribute will change to this value
    joynr::types::Localisation::GpsLocation gpsLocation;
    Variant attributeValue = Variant::make<joynr::types::Localisation::GpsLocation>(gpsLocation);

    // Expect calls to register an unregister an attribute listener
    std::string attributeName("Location");
    IAttributeListener* attributeListener;

    SubscriptionRequest subscriptionRequest;

    SubscriptionPublication expectedPublication;
    expectedPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId());
    std::vector<Variant> response;
    response.push_back(attributeValue);
    expectedPublication.setResponseVariant(response);
    EXPECT_CALL(
                mockPublicationSender,
                sendSubscriptionPublication(
                    _, // sender participant ID
                    _, // receiver participant ID
                    _, // messaging QoS
                    SubscriptionPublicationMatcher(expectedPublication) // subscription publication
                )
    )
            .Times(3);

    EXPECT_CALL(
                mockPublicationSender2,
                sendSubscriptionPublication(
                    _, // sender participant ID
                    _, // receiver participant ID
                    _, // messaging QoS
                    SubscriptionPublicationMatcher(expectedPublication) // subscription publication
                )
    )
            .Times(2);

    EXPECT_CALL(*mockTestRequestCaller,registerAttributeListener(attributeName, _))
            .Times(1)
            .WillRepeatedly(testing::SaveArg<1>(&attributeListener));

    EXPECT_CALL(*mockTestRequestCaller,unregisterAttributeListener(attributeName, _)).Times(1);

    EXPECT_CALL(*mockTestRequestCaller2,registerAttributeListener(attributeName, _))
            .Times(1)
            .WillRepeatedly(testing::SaveArg<1>(&attributeListener));

    EXPECT_CALL(*mockTestRequestCaller2,unregisterAttributeListener(attributeName, _)).Times(1);

    std::shared_ptr<MockTestRequestCaller> requestCaller(mockTestRequestCaller);
    std::shared_ptr<MockTestRequestCaller> requestCaller2(mockTestRequestCaller2);

    PublicationManager publicationManager;

    //SubscriptionRequest
    std::string senderId = "SenderId";
    std::string receiverId = "ReceiverId";
    //SubscriptionQos
    std::int64_t minInterval_ms = 100;
    std::int64_t validity_ms = 600;
    OnChangeSubscriptionQos qos{validity_ms,minInterval_ms};

    std::int64_t now = joynr::TimeUtils::getCurrentMillisSinceEpoch();
    qos.setExpiryDateMs(now + 5000);

    Variant qosVariant = Variant::make<OnChangeSubscriptionQos>(qos);
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(qosVariant);
    JOYNR_LOG_DEBUG(logger, "adding attribute subscription request");
    publicationManager.add(senderId, receiverId, requestCaller,subscriptionRequest,&mockPublicationSender);

    // Sleep so that the first publication is sent
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms + 50));

    // Fake attribute change
    attributeListener->attributeValueChanged(attributeValue);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    // now, we assume that two publications have been occured

    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms));

    // Fake attribute change
    attributeListener->attributeValueChanged(attributeValue);

    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms + 50));
    // now, we assume that three publications have been occured

    //now, let's update the subscription and check if the provided data is correctly processed by the PublicationManager
    // will be deleted by the publication manager

    qos.setMinIntervalMs(minInterval_ms + 500);
    qosVariant = Variant::make<OnChangeSubscriptionQos>(qos);
    subscriptionRequest.setQos(qosVariant);
    JOYNR_LOG_DEBUG(logger, "update attribute subscription request");
    publicationManager.add(senderId, receiverId, requestCaller2,subscriptionRequest,&mockPublicationSender2);

    // Sleep so that the first publication is sent
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms + 50));

    // Fake attribute change
    attributeListener->attributeValueChanged(attributeValue);

    // sleep, waiting for the async publication (which shouldn't come)
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // until now, only one publication should be arrived to mockPublicationSender2

    // Fake attribute change. This change shall not result in a new attribute value changed
    attributeListener->attributeValueChanged(attributeValue);

    // Wait for the subscription to finish
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms + 500));

    // now, we should got 2 publications on mockPublicationSender2
}

TEST_F(PublicationManagerTest, attribute_add_withExistingSubscriptionId_testQos_withGreaterExpiryDate) {
    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>("tests/Test");

    MockPublicationSender mockPublicationSender;
    MockTestRequestCaller* mockTestRequestCaller = new MockTestRequestCaller();

    // The attribute will change to this value
    joynr::types::Localisation::GpsLocation gpsLocation;
    Variant attributeValue = Variant::make<joynr::types::Localisation::GpsLocation>(gpsLocation);

    // Expect calls to register an unregister an attribute listener
    std::string attributeName("Location");
    IAttributeListener* attributeListener;

    SubscriptionRequest subscriptionRequest;

    SubscriptionPublication expectedPublication;
    expectedPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId());
    std::vector<Variant> response;
    response.push_back(attributeValue);
    expectedPublication.setResponseVariant(response);
    EXPECT_CALL(
                mockPublicationSender,
                sendSubscriptionPublication(
                    _, // sender participant ID
                    _, // receiver participant ID
                    _, // messaging QoS
                    SubscriptionPublicationMatcher(expectedPublication) // subscription publication
                )
    )
            .Times(3);

    EXPECT_CALL(*mockTestRequestCaller,registerAttributeListener(attributeName, _))
            .Times(2)
            .WillRepeatedly(testing::SaveArg<1>(&attributeListener));

    EXPECT_CALL(*mockTestRequestCaller,unregisterAttributeListener(attributeName, _)).Times(2);

    std::shared_ptr<MockTestRequestCaller> requestCaller(mockTestRequestCaller);

    PublicationManager publicationManager;

    //SubscriptionRequest
    std::string senderId = "SenderId";
    std::string receiverId = "ReceiverId";
    //SubscriptionQos
    std::int64_t minInterval_ms = 50;
    std::int64_t validity_ms = 600;
    std::int64_t testRelExpiryDate = 500;
    std::int64_t now = joynr::TimeUtils::getCurrentMillisSinceEpoch();
    std::int64_t testAbsExpiryDate = now + testRelExpiryDate;
    OnChangeSubscriptionQos qos{validity_ms,minInterval_ms};

    qos.setExpiryDateMs(testAbsExpiryDate);

    Variant qosVariant = Variant::make<OnChangeSubscriptionQos>(qos);
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(qosVariant);

    JOYNR_LOG_DEBUG(logger, "adding attribute subscription request");
    publicationManager.add(senderId, receiverId, requestCaller,subscriptionRequest,&mockPublicationSender);

    // exceed the minInterval
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms+50));

    // now, we expect that one publication has been performed

    // now, let's update the subscription and check if the provided data is correctly processed by the PublicationManager
    // extend the expiry date
    qos.setExpiryDateMs(testAbsExpiryDate + 1000);
    qosVariant = Variant::make<OnChangeSubscriptionQos>(qos);
    subscriptionRequest.setQos(qosVariant);
    JOYNR_LOG_DEBUG(logger, "adding request");
    publicationManager.add(senderId, receiverId, requestCaller,subscriptionRequest,&mockPublicationSender);

    // Sleep so that the first publication is sent
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    //now we expect that two puclications have been performed

    //now, exceed the original expiryDate, and make an attribute change
    std::this_thread::sleep_for(std::chrono::milliseconds(testRelExpiryDate));
    attributeListener->attributeValueChanged(attributeValue);

    // wait for the async publication
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    // now, three publications should be noticed

    // wait for the subscription to finish
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms + testRelExpiryDate));
}

TEST_F(PublicationManagerTest, attribtue_add_withExistingSubscriptionId_testQos_withLowerExpiryDate) {
    JOYNR_LOG_DEBUG(logger, "DEFAULT_SUBSCRIPTIONREQUEST_STORAGE_FILENAME: {}",LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME());

    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>("tests/Test");

    MockPublicationSender mockPublicationSender;
    MockTestRequestCaller* mockTestRequestCaller = new MockTestRequestCaller();

    // The attribute will change to this value
    joynr::types::Localisation::GpsLocation gpsLocation;
    Variant attributeValue = Variant::make<joynr::types::Localisation::GpsLocation>(gpsLocation);

    // Expect calls to register an unregister an attribute listener
    std::string attributeName("Location");
    IAttributeListener* attributeListener;

    SubscriptionRequest subscriptionRequest;

    SubscriptionPublication expectedPublication;
    expectedPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId());
    std::vector<Variant> response;
    response.push_back(attributeValue);
    expectedPublication.setResponseVariant(response);
    EXPECT_CALL(
                mockPublicationSender,
                sendSubscriptionPublication(
                    _, // sender participant ID
                    _, // receiver participant ID
                    _, // messaging QoS
                    SubscriptionPublicationMatcher(expectedPublication) // subscription publication
                )
    )
            .Times(3);

    EXPECT_CALL(*mockTestRequestCaller,registerAttributeListener(attributeName, _))
            .Times(2)
            .WillRepeatedly(testing::SaveArg<1>(&attributeListener));

    EXPECT_CALL(*mockTestRequestCaller,unregisterAttributeListener(attributeName, _)).Times(2);

    std::shared_ptr<MockTestRequestCaller> requestCaller(mockTestRequestCaller);

    PublicationManager publicationManager;

    //SubscriptionRequest
    std::string senderId = "SenderId";
    std::string receiverId = "ReceiverId";
    //SubscriptionQos
    std::int64_t minInterval_ms = 50;
    std::int64_t validity_ms = 600;
    std::int64_t testExpiryDate_shift = 2500;
    std::int64_t testRelExpiryDate = 500 + testExpiryDate_shift;
    std::int64_t now = joynr::TimeUtils::getCurrentMillisSinceEpoch();
    std::int64_t testAbsExpiryDate = now + testRelExpiryDate;
    OnChangeSubscriptionQos qos{validity_ms,minInterval_ms};

    qos.setExpiryDateMs(testAbsExpiryDate);

    Variant qosVariant = Variant::make<OnChangeSubscriptionQos>(qos);
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(qosVariant);
    JOYNR_LOG_DEBUG(logger, "adding attribute subscription request");
    publicationManager.add(senderId, receiverId, requestCaller,subscriptionRequest,&mockPublicationSender);

    // exceed the minInterval
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    attributeListener->attributeValueChanged(attributeValue);
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms + 50));

    // now, we expect that two publications have been performed

    // now, let's update the subscription and check if the provided data is correctly processed by the PublicationManager
    // reduce the expiry date
    qos.setExpiryDateMs(testAbsExpiryDate - testExpiryDate_shift);
    qosVariant = Variant::make<OnChangeSubscriptionQos>(qos);
    subscriptionRequest.setQos(qosVariant);
    JOYNR_LOG_DEBUG(logger, "update attribute subscription request");
    publicationManager.add(senderId, receiverId, requestCaller,subscriptionRequest,&mockPublicationSender);

    // Sleep so that the first publication is sent
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    // now we expect that three puclications have been performed

    // now, exceed the new expiryDate, and make an attribute change
    std::this_thread::sleep_for(std::chrono::milliseconds(testRelExpiryDate - testExpiryDate_shift));
    // now, the subscription should be death

    attributeListener->attributeValueChanged(attributeValue);

    // wait for the async publication, which shouldn't arrive
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

TEST_F(PublicationManagerTest, broadcast_add_withExistingSubscriptionId) {
    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>("tests/Test");

    MockPublicationSender mockPublicationSender;
    MockTestRequestCaller* mockTestRequestCaller = new MockTestRequestCaller();

    MockTestRequestCaller* mockTestRequestCaller2 = new MockTestRequestCaller();
    MockPublicationSender mockPublicationSender2;

    // The broacast will fire this value
    joynr::types::Localisation::GpsLocation gpsLocation;
    std::vector<Variant> broadcastValues;
    broadcastValues.push_back(Variant::make<joynr::types::Localisation::GpsLocation>(gpsLocation));

    std::vector<std::shared_ptr<IBroadcastFilter> > filters;

    // Expect calls to register an unregister an broadcast listener
    std::string broadcastName("Location");
    IBroadcastListener* broadcastListener;

    BroadcastSubscriptionRequest subscriptionRequest;
    BroadcastFilterParameters filterParameters;
    subscriptionRequest.setFilterParameters(filterParameters);

    SubscriptionPublication expectedPublication;
    expectedPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId());
    expectedPublication.setResponseVariant(broadcastValues);

    EXPECT_CALL(
                mockPublicationSender,
                sendSubscriptionPublication(
                    _, // sender participant ID
                    _, // receiver participant ID
                    _, // messaging QoS
                    SubscriptionPublicationMatcher(expectedPublication) // subscription publication
                )
    )
            .Times(2);

    EXPECT_CALL(
                mockPublicationSender2,
                sendSubscriptionPublication(
                    _, // sender participant ID
                    _, // receiver participant ID
                    _, // messaging QoS
                    SubscriptionPublicationMatcher(expectedPublication) // subscription publication
                )
    )
            .Times(1);

    EXPECT_CALL(*mockTestRequestCaller,registerBroadcastListener(broadcastName, _))
            .Times(1)
            .WillRepeatedly(testing::SaveArg<1>(&broadcastListener));

    EXPECT_CALL(*mockTestRequestCaller,unregisterBroadcastListener(broadcastName, _)).Times(1);

    EXPECT_CALL(*mockTestRequestCaller2,registerBroadcastListener(broadcastName, _))
            .Times(1)
            .WillRepeatedly(testing::SaveArg<1>(&broadcastListener));

    EXPECT_CALL(*mockTestRequestCaller2,unregisterBroadcastListener(broadcastName, _)).Times(1);

    std::shared_ptr<MockTestRequestCaller> requestCaller(mockTestRequestCaller);
    std::shared_ptr<MockTestRequestCaller> requestCaller2(mockTestRequestCaller2);

    PublicationManager publicationManager;

    //SubscriptionRequest
    std::string senderId = "SenderId";
    std::string receiverId = "ReceiverId";
    //SubscriptionQos
    std::int64_t minInterval_ms = 100;
    std::int64_t validity_ms = 600;
    OnChangeSubscriptionQos qos{validity_ms,minInterval_ms};

    std::int64_t now = joynr::TimeUtils::getCurrentMillisSinceEpoch();
    qos.setExpiryDateMs(now + 5000);

    subscriptionRequest.setSubscribeToName(broadcastName);
    subscriptionRequest.setQos(qos);
    JOYNR_LOG_DEBUG(logger, "adding broadcast subscription request");
    publicationManager.add(senderId, receiverId, requestCaller,subscriptionRequest,&mockPublicationSender);

    // Fake broadcast
    broadcastListener->broadcastOccurred(broadcastValues, filters);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // now, we assume that one publication has been occured
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms));

    // Fake broadcast
    broadcastListener->broadcastOccurred(broadcastValues, filters);

    std::int64_t newMinInterval = minInterval_ms + 500;
    std::this_thread::sleep_for(std::chrono::milliseconds(50 + newMinInterval));
    // now, we assume that two publications have been occured

    //now, let's update the subscription an check if the provided data is correctly processed by the PublicationManager

    qos.setMinIntervalMs(newMinInterval);
    subscriptionRequest.setQos(qos);
    JOYNR_LOG_DEBUG(logger, "update broadcast subscription request");
    publicationManager.add(senderId, receiverId, requestCaller2,subscriptionRequest,&mockPublicationSender2);

    // Fake broadcast
    broadcastListener->broadcastOccurred(broadcastValues, filters);

    // sleep, waiting for the async publication (which shouldn't come)
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    // until now, only one publication should be arrived to mockPublicationSender2

    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms + 50));

    // Fake broadcast. This change shall not result in a new broadcast to the client
    broadcastListener->broadcastOccurred(broadcastValues, filters);

    // Wait for the subscription to finish
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

TEST_F(PublicationManagerTest, broadcast_add_withExistingSubscriptionId_testQos_withGreaterExpiryDate) {
    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>("tests/Test");

    MockPublicationSender mockPublicationSender;
    MockTestRequestCaller* mockTestRequestCaller = new MockTestRequestCaller();

    // The value will be fired by the broadcast
    joynr::types::Localisation::GpsLocation gpsLocation;
    std::vector<Variant> broadcastValues;
    broadcastValues.push_back(Variant::make<joynr::types::Localisation::GpsLocation>(gpsLocation));

    std::vector<std::shared_ptr<IBroadcastFilter> > filters;

    // Expect calls to register an unregister a broadcast listener
    std::string broadcastName("Location");
    IBroadcastListener* broadcastListener;

    BroadcastSubscriptionRequest subscriptionRequest;

    SubscriptionPublication expectedPublication;
    expectedPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId());
    expectedPublication.setResponseVariant(broadcastValues);

    EXPECT_CALL(
                mockPublicationSender,
                sendSubscriptionPublication(
                    _, // sender participant ID
                    _, // receiver participant ID
                    _, // messaging QoS
                    SubscriptionPublicationMatcher(expectedPublication) // subscription publication
                )
    )
            .Times(2);

    EXPECT_CALL(*mockTestRequestCaller,registerBroadcastListener(broadcastName, _))
            .Times(2)
            .WillRepeatedly(testing::SaveArg<1>(&broadcastListener));

    EXPECT_CALL(*mockTestRequestCaller,unregisterBroadcastListener(broadcastName, _)).Times(2);

    std::shared_ptr<MockTestRequestCaller> requestCaller(mockTestRequestCaller);

    PublicationManager publicationManager;

    //SubscriptionRequest
    std::string senderId = "SenderId";
    std::string receiverId = "ReceiverId";
    //SubscriptionQos
    std::int64_t minInterval_ms = 50;
    std::int64_t validity_ms = 600;
    std::int64_t testRelExpiryDate = 500;
    std::int64_t now = joynr::TimeUtils::getCurrentMillisSinceEpoch();
    std::int64_t testAbsExpiryDate = now + testRelExpiryDate;
    OnChangeSubscriptionQos qos{validity_ms,minInterval_ms};

    qos.setExpiryDateMs(testAbsExpiryDate);

    subscriptionRequest.setSubscribeToName(broadcastName);
    subscriptionRequest.setQos(qos);
    JOYNR_LOG_DEBUG(logger, "add broadcast subscription request");
    publicationManager.add(senderId, receiverId, requestCaller, subscriptionRequest, &mockPublicationSender);

    broadcastListener->broadcastOccurred(broadcastValues, filters);
    // exceed the minInterval
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms+50));

    // now, we expect that one publication has been performed

    // now, let's update the subscription and check if the provided data is correctly processed by the PublicationManager
    // extend the expiry date
    qos.setExpiryDateMs(testAbsExpiryDate + 1000);
    subscriptionRequest.setQos(qos);
    JOYNR_LOG_DEBUG(logger, "update broadcast subscription request");
    publicationManager.add(senderId, receiverId, requestCaller,subscriptionRequest,&mockPublicationSender);

    //now, exceed the original expiryDate, and make a broadcast
    std::this_thread::sleep_for(std::chrono::milliseconds(testRelExpiryDate));
    broadcastListener->broadcastOccurred(broadcastValues, filters);

    // wait for the async publication
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    //now, two publications should be noticed, even if the original subscription is expired
}

TEST_F(PublicationManagerTest, broadcast_add_withExistingSubscriptionId_testQos_withLowerExpiryDate) {
    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>("tests/Test");

    MockPublicationSender mockPublicationSender;
    MockTestRequestCaller* mockTestRequestCaller = new MockTestRequestCaller();

    // The broadcast will fire this value
    joynr::types::Localisation::GpsLocation gpsLocation;
    std::vector<Variant> broadcastValues;
    broadcastValues.push_back(Variant::make<joynr::types::Localisation::GpsLocation>(gpsLocation));

    std::vector<std::shared_ptr<IBroadcastFilter> > filters;

    // Expect calls to register an unregister a broadcast listener
    std::string broadcastName("Location");
    IBroadcastListener* broadcastListener;

    BroadcastSubscriptionRequest subscriptionRequest;

    SubscriptionPublication expectedPublication;
    expectedPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId());
    expectedPublication.setResponseVariant(broadcastValues);

    EXPECT_CALL(
                mockPublicationSender,
                sendSubscriptionPublication(
                    _, // sender participant ID
                    _, // receiver participant ID
                    _, // messaging QoS
                    SubscriptionPublicationMatcher(expectedPublication) // subscription publication
                )
    )
            .Times(1);

    EXPECT_CALL(*mockTestRequestCaller,registerBroadcastListener(broadcastName, _))
            .Times(2)
            .WillRepeatedly(testing::SaveArg<1>(&broadcastListener));

    EXPECT_CALL(*mockTestRequestCaller,unregisterBroadcastListener(broadcastName, _)).Times(2);

    std::shared_ptr<MockTestRequestCaller> requestCaller(mockTestRequestCaller);

    PublicationManager publicationManager;

    //SubscriptionRequest
    std::string senderId = "SenderId";
    std::string receiverId = "ReceiverId";
    //SubscriptionQos
    std::int64_t minInterval_ms = 50;
    std::int64_t validity_ms = 600;
    std::int64_t testExpiryDate_shift = 2500;
    std::int64_t testRelExpiryDate = 500 + testExpiryDate_shift;
    std::int64_t now = joynr::TimeUtils::getCurrentMillisSinceEpoch();
    std::int64_t testAbsExpiryDate = now + testRelExpiryDate;
    OnChangeSubscriptionQos qos{validity_ms,minInterval_ms};

    qos.setExpiryDateMs(testAbsExpiryDate);

    subscriptionRequest.setSubscribeToName(broadcastName);
    subscriptionRequest.setQos(qos);
    JOYNR_LOG_DEBUG(logger, "adding broadcast subscription request");
    publicationManager.add(senderId, receiverId, requestCaller,subscriptionRequest,&mockPublicationSender);

    broadcastListener->broadcastOccurred(broadcastValues, filters);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // now, we expect that one publications have been performed

    // now, let's update the subscription and check if the provided data is correctly processed by the PublicationManager
    // reduce the expiry date
    qos.setExpiryDateMs(testAbsExpiryDate - testExpiryDate_shift);
    subscriptionRequest.setQos(qos);
    JOYNR_LOG_DEBUG(logger, "update broadcast subscription request");
    publicationManager.add(senderId, receiverId, requestCaller,subscriptionRequest,&mockPublicationSender);

    // now, exceed the new expiryDate, and make a broadcast
    std::this_thread::sleep_for(std::chrono::milliseconds(testRelExpiryDate - testExpiryDate_shift));
    // now, the subscription should be death

    broadcastListener->broadcastOccurred(broadcastValues, filters);

    // wait for the async publication (which shouldn't arrive)
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // now, no new publication should be received, even if the expiry date of the original request hasn't been expired
    // -> one publication expected
}

TEST_F(PublicationManagerTest, remove_onChangeSubscription) {
    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>("tests/Test");

    MockPublicationSender mockPublicationSender;
    MockTestRequestCaller* mockTestRequestCaller = new MockTestRequestCaller();

    // A publication should never be sent
    EXPECT_CALL(
                mockPublicationSender,
                sendSubscriptionPublication(
                    _, // sender participant ID
                    _, // receiver participant ID
                    _, // messaging QoS
                    _ // subscription publication
                )
    )
            .Times(1);

    // Expect calls to register an unregister an attribute listener
    std::string attributeName("Location");
    IAttributeListener* attributeListener;

    EXPECT_CALL(*mockTestRequestCaller,registerAttributeListener(attributeName, _))
            .Times(1)
            .WillRepeatedly(testing::SaveArg<1>(&attributeListener));

    EXPECT_CALL(*mockTestRequestCaller,unregisterAttributeListener(attributeName, _)).Times(1);

    std::shared_ptr<MockTestRequestCaller> requestCaller(mockTestRequestCaller);
    PublicationManager publicationManager;

    //SubscriptionRequest
    std::string senderId = "SenderId";
    std::string receiverId = "ReceiverId";
    //SubscriptionQos
    std::int64_t minInterval_ms = 1;
    std::int64_t validity_ms = 100;
    Variant qos = Variant::make<OnChangeSubscriptionQos>(OnChangeSubscriptionQos(
                        validity_ms,
                        minInterval_ms));

    // will be deleted by the publication manager
    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(qos);
    JOYNR_LOG_DEBUG(logger, "adding request");
    publicationManager.add(senderId, receiverId, requestCaller,subscriptionRequest,&mockPublicationSender);

    // Wait for the subscription to expire
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

TEST_F(PublicationManagerTest, forwardProviderRuntimeExceptionToPublicationSender) {
    std::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME().c_str()); //remove stored subscriptions

    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>("tests/Test");

    MockPublicationSender mockPublicationSender;
    std::shared_ptr<MockTestRequestCaller> requestCaller = std::make_shared<MockTestRequestCaller>();

    // The value will be fired by the broadcast
    Variant expected =
            Variant::make<exceptions::ProviderRuntimeException>(requestCaller->providerRuntimeExceptionTestMsg);

    //SubscriptionRequest
    std::string senderId = "SenderId";
    std::string receiverId = "ReceiverId";
    std::string attributeName("attributeWithProviderRuntimeException");
    std::int64_t period_ms = 100;
    std::int64_t validity_ms = 1000;
    std::int64_t alertInterval_ms = 1000;
    Variant qos = Variant::make<PeriodicSubscriptionQos>(PeriodicSubscriptionQos(
                        validity_ms,
                        period_ms,
                        alertInterval_ms));
    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(qos);

    SubscriptionPublication expectedPublication;
    expectedPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId());
    expectedPublication.setErrorVariant(expected);

    EXPECT_CALL(
                mockPublicationSender,
                sendSubscriptionPublication(
                    Eq(receiverId), // sender participant ID
                    Eq(senderId), // receiver participant ID
                    _, // messaging QoS
                    SubscriptionPublicationMatcher(expectedPublication) // subscription publication
                )
    )
            .Times(2);

    PublicationManager publicationManager;

    publicationManager.add(senderId, receiverId, requestCaller, subscriptionRequest, &mockPublicationSender);

    // wait for the async publication
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    //now, two publications should be noticed, even if the original subscription is expired
}

TEST_F(PublicationManagerTest, forwardMethodInvocationExceptionToPublicationSender) {
    std::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME().c_str()); //remove stored subscriptions

    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>("tests/Test");

    MockPublicationSender mockPublicationSender;
    std::shared_ptr<MockTestRequestCaller> requestCaller = std::make_shared<MockTestRequestCaller>();

    // The value will be fired by the broadcast
    Variant expected =
            Variant::make<exceptions::MethodInvocationException>("unknown method name for interface test: getNotExistingAttribute");

    //SubscriptionRequest
    std::string senderId = "SenderId";
    std::string receiverId = "ReceiverId";
    std::string attributeName("notExistingAttribute");
    //QtSubscriptionQos
    std::int64_t period_ms = 100;
    std::int64_t validity_ms = 1000;
    std::int64_t alertInterval_ms = 1000;
    Variant qos = Variant::make<PeriodicSubscriptionQos>(PeriodicSubscriptionQos(
                        validity_ms,
                        period_ms,
                        alertInterval_ms));
    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(qos);

    SubscriptionPublication expectedPublication;
    expectedPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId());
    expectedPublication.setErrorVariant(expected);

    EXPECT_CALL(
                mockPublicationSender,
                sendSubscriptionPublication(
                    Eq(receiverId), // sender participant ID
                    Eq(senderId), // receiver participant ID
                    _, // messaging QoS
                    SubscriptionPublicationMatcher(expectedPublication) // subscription publication
                )
    )
            .Times(2);

    PublicationManager publicationManager;

    publicationManager.add(senderId, receiverId, requestCaller, subscriptionRequest, &mockPublicationSender);

    // wait for the async publication
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    //now, two publications should be noticed, even if the original subscription is expired
}

TEST_F(PublicationManagerTest, restorePersistetAttributeSubscriptions) {
    MockPublicationSender* mockPublicationSender = new MockPublicationSender;

    const std::string attributeSubscriptionsPersistenceFilename = "test-SubscriptionRequest.persist";
    std::remove(attributeSubscriptionsPersistenceFilename.c_str());

    PublicationManager* publicationManager = new PublicationManager();

    publicationManager->loadSavedAttributeSubscriptionRequestsMap(attributeSubscriptionsPersistenceFilename);

    //SubscriptionRequest
    const std::string senderId = "SenderId";
    const std::string receiverId = "ReceiverId";
    const std::string attributeName ="Location";
    //SubscriptionQos
    const std::int64_t period_ms = 100;
    const std::int64_t validity_ms = 1000;
    const std::int64_t alertInterval_ms = 1000;
    const Variant qos = Variant::make<PeriodicSubscriptionQos>(PeriodicSubscriptionQos(
                        validity_ms,
                        period_ms,
                        alertInterval_ms));

    // will be deleted by the publication manager (destructor PublicationState)
    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(qos);

    publicationManager->add(senderId, receiverId, subscriptionRequest);

    delete publicationManager;

    PublicationManager* publicationManager2 = new PublicationManager();
    //if restoring works, this caller will be called.
    auto requestCaller = std::make_shared<MockTestRequestCaller>(AtLeast(1));

    publicationManager2->loadSavedAttributeSubscriptionRequestsMap(attributeSubscriptionsPersistenceFilename);

    const Variant attributeValue = Variant::make<std::string>("attributeValue");
    std::vector<Variant> values;
    values.push_back(attributeValue);
    SubscriptionPublication subscriptionPublication;
    subscriptionPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId());
    subscriptionPublication.setResponse(values);
    EXPECT_CALL(*mockPublicationSender, sendSubscriptionPublication(Eq(receiverId), Eq(senderId), _, Eq(subscriptionPublication))).Times(AtLeast(2));
    publicationManager2->restore(receiverId,
                                requestCaller,
                                mockPublicationSender);

    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    publicationManager2->attributeValueChanged(subscriptionRequest.getSubscriptionId(), attributeValue);

    delete publicationManager2;
    delete mockPublicationSender;
    std::remove(attributeSubscriptionsPersistenceFilename.c_str());
}

TEST_F(PublicationManagerTest, restorePersistetBroadcastSubscriptions) {
    MockPublicationSender* mockPublicationSender = new MockPublicationSender;

    const std::string broadcastSubscriptionsPersistenceFilename = "test-BroadcastSubscriptionRequest.persist";
    std::remove(broadcastSubscriptionsPersistenceFilename.c_str());

    PublicationManager* publicationManager = new PublicationManager();

    publicationManager->loadSavedBroadcastSubscriptionRequestsMap(broadcastSubscriptionsPersistenceFilename);

    // BroadcastSubscriptionRequest
    const std::string broadcastSenderId = "BroadcastSenderId";
    const std::string broadcastReceiverId = "BroadcastReceiverId";
    const std::string broadcastSubscriptionId = "Location";
    BroadcastSubscriptionRequest broadcastSubscriptionRequest;
    broadcastSubscriptionRequest.setSubscriptionId(broadcastSubscriptionId);

    publicationManager->add(broadcastSenderId, broadcastReceiverId, broadcastSubscriptionRequest);

    delete publicationManager;

    PublicationManager* publicationManager2 = new PublicationManager();

    publicationManager2->loadSavedBroadcastSubscriptionRequestsMap(broadcastSubscriptionsPersistenceFilename);
    //if restoring works, this caller will be called.
    auto requestCaller = std::make_shared<MockTestRequestCaller>();

    publicationManager2->restore(broadcastReceiverId,
                                requestCaller,
                                mockPublicationSender);

    const std::vector<Variant> broadcastValues;
    std::vector<std::shared_ptr<IBroadcastFilter>> filters;
    SubscriptionPublication broadcastSubscriptionPublication;
    broadcastSubscriptionPublication.setSubscriptionId(broadcastSubscriptionId);
    broadcastSubscriptionPublication.setResponse(broadcastValues);

    EXPECT_CALL(*mockPublicationSender, sendSubscriptionPublication(Eq(broadcastReceiverId), Eq(broadcastSenderId), _, broadcastSubscriptionPublication));
    publicationManager2->broadcastOccurred(broadcastSubscriptionId, broadcastValues, filters);

    delete publicationManager2;
    delete mockPublicationSender;
    std::remove(broadcastSubscriptionsPersistenceFilename.c_str());
}
