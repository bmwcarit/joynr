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
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "tests/utils/MockObjects.h"
#include "joynr/InterfaceRegistrar.h"
#include "joynr/vehicle/GpsRequestInterpreter.h"
#include "joynr/tests/testRequestInterpreter.h"
#include "joynr/SubscriptionPublication.h"
#include "joynr/IAttributeListener.h"
#include "joynr/PeriodicSubscriptionQos.h"
#include "joynr/OnChangeSubscriptionQos.h"
#include "joynr/LibjoynrSettings.h"

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

using namespace joynr;

// New matcher definition that works with the latest Google Mock.
class GpsAttributeMatcherInterface : public MatcherInterface<const joynr::SubscriptionPublication&> {
 public:
    GpsAttributeMatcherInterface () {}
  virtual void DescribeTo(::std::ostream* os) const {}
  virtual bool MatchAndExplain(const joynr::SubscriptionPublication& value,
                               MatchResultListener* listener) const {
    // Returns true if value matches.
    joynr::types::GpsLocation defaultValue;
    joynr::types::GpsLocation receivedValue = value.getResponse().value<joynr::types::GpsLocation>();
    return receivedValue == defaultValue;
  }
};

inline Matcher<const SubscriptionPublication&> GpsAttributeMatcher() {
  return MakeMatcher(new GpsAttributeMatcherInterface());
}

TEST(PublicationManagerTest, add_requestCallerIsCalledCorrectlyByPublisherRunnables) {
    joynr_logging::Logger* logger = joynr_logging::Logging::getInstance()->getLogger("TEST", "PublicationManagerTest");
    QFile::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_STORAGE_FILENAME()); //remove stored subscriptions

    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<joynr::tests::testRequestInterpreter>("tests/Test");

    MockPublicationSender mockPublicationSender;
    MockTestRequestCaller* mockTestRequestCaller = new MockTestRequestCaller();

    // NOTE: it depends on the timing and especially on the CPU load of
    // the current machine how often the publication is exectuted. Hence,
    // we expect the publication to haben between 3 or 5 times.
    EXPECT_CALL(mockPublicationSender,
                sendSubscriptionPublication(_,_,_,_))
            .Times(Between(3, 5));

    EXPECT_CALL(*mockTestRequestCaller,
                getLocation(_,_))
            .Times(Between(3, 5));

    QSharedPointer<MockTestRequestCaller> requestCaller(mockTestRequestCaller);
    PublicationManager publicationManager;

    //SubscriptionRequest
    QString senderId = "SenderId";
    QString receiverId = "ReceiverId";
    QString attributeName = "Location";
    // SUbscriptionQos
    qint64 period_ms = 100;
    qint64 validity_ms = 500;
    qint64 alertInterval_ms = 2000;
    auto qos = QSharedPointer<SubscriptionQos>(new PeriodicSubscriptionQos(
                        validity_ms,
                        period_ms,
                        alertInterval_ms));

    // will be deleted by the publication manager
    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(qos);
    LOG_DEBUG(logger, "adding request");
    publicationManager.add(senderId, receiverId, requestCaller,subscriptionRequest,&mockPublicationSender);
    QThreadSleep::msleep(500);
}


TEST(PublicationManagerTest, stop_publications) {
    QFile::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_STORAGE_FILENAME()); //remove stored subscriptions
    MockPublicationSender mockPublicationSender;
    MockTestRequestCaller* mockTestRequestCaller = new MockTestRequestCaller();

    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>("tests/Test");

    EXPECT_CALL(mockPublicationSender,
                sendSubscriptionPublication(_,_,_,_))
            .Times(AtMost(2));

    EXPECT_CALL(*mockTestRequestCaller,
                getLocation(_,_))
            .Times(AtMost(2));

    QSharedPointer<MockTestRequestCaller> requestCaller(mockTestRequestCaller);
    PublicationManager publicationManager;

    //SubscriptionRequest
    QString senderId = "SenderId";
    QString receiverId = "ReceiverId";
    QString attributeName = "Location";
    //SubscriptionQos
    qint64 period_ms = 100;
    qint64 validity_ms = 10000;
    qint64 alertInterval_ms = 1000;
    auto qos = QSharedPointer<SubscriptionQos>(new PeriodicSubscriptionQos(
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
    QThreadSleep::msleep(80);
    publicationManager.stopPublication(subscriptionRequest.getSubscriptionId());
    QThreadSleep::msleep(300);
}

TEST(PublicationManagerTest, remove_all_publications) {
    QFile::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_STORAGE_FILENAME()); //remove stored subscriptions
    MockPublicationSender mockPublicationSender;
    MockTestRequestCaller* mockTestRequestCaller = new MockTestRequestCaller();

    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>("tests/Test");

    EXPECT_CALL(mockPublicationSender,
                sendSubscriptionPublication(_,_,_,_))
            .Times(AtMost(2));

    EXPECT_CALL(*mockTestRequestCaller,
                getLocation(_,_))
            .Times(AtMost(2));

    QSharedPointer<MockTestRequestCaller> requestCaller(mockTestRequestCaller);
    PublicationManager publicationManager;

    //SubscriptionRequest
    QString senderId = "SenderId";
    QString receiverId = "ReceiverId";
    QString attributeName = "Location";
    //SubscriptionQos
    qint64 period_ms = 100;
    qint64 validity_ms = 10000;
    qint64 alertInterval_ms = 1000;
    auto qos = QSharedPointer<SubscriptionQos>(new PeriodicSubscriptionQos(
                        validity_ms,
                        period_ms,
                        alertInterval_ms));

    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(qos);

    publicationManager.add(senderId, receiverId, requestCaller,subscriptionRequest,&mockPublicationSender);
    QThreadSleep::msleep(80);
    publicationManager.removeAllSubscriptions(receiverId);
    QThreadSleep::msleep(300);
}

TEST(PublicationManagerTest, restore_publications) {
    QFile::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_STORAGE_FILENAME());
    qRegisterMetaType<PeriodicSubscriptionQos>("PeriodicSubscriptionQos");
    MockPublicationSender mockPublicationSender;

    //the first publicationManager will get this requestCaller:
    QSharedPointer<MockTestRequestCaller> requestCaller(new MockTestRequestCaller());

    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>("tests/Test");

    EXPECT_CALL(*requestCaller,
                getLocation(_,_))
            .Times(Between(1,3));

    //the second publicationManager will get this requestCaller
    //if restoring works, this caller will be called as well.
    QSharedPointer<MockTestRequestCaller> requestCaller2(new MockTestRequestCaller());

    EXPECT_CALL(*requestCaller2,
                getLocation(_,_))
            .Times(AtLeast(2));

    PublicationManager* publicationManager = new PublicationManager() ;

    //SubscriptionRequest
    QString senderId = "SenderId";
    QString receiverId = "ReceiverId";
    QString attributeName = "Location";
    //SubscriptionQos
    qint64 period_ms = 100;
    qint64 validity_ms = 1000;
    qint64 alertInterval_ms = 1000;
    auto qos = QSharedPointer<SubscriptionQos>(new PeriodicSubscriptionQos(
                        validity_ms,
                        period_ms,
                        alertInterval_ms));

    // will be delete by the publication manager (destructor PublicationState)
    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(qos);

    publicationManager->add(senderId, receiverId,requestCaller,subscriptionRequest,&mockPublicationSender);
    QThreadSleep::msleep(100); //make sure, that the first request caller is actually called.
    delete publicationManager;

    PublicationManager* publicationManager2 = new PublicationManager();

    publicationManager2->restore(receiverId,
                                requestCaller2,
                                &mockPublicationSender);
    QThreadSleep::msleep(350);
    delete publicationManager2;
}

TEST(PublicationManagerTest, add_onChangeSubscription) {
    joynr_logging::Logger* logger = joynr_logging::Logging::getInstance()->getLogger("TEST", "PublicationManagerTest");
    QFile::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_STORAGE_FILENAME()); //remove stored subscriptions

    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>("tests/Test");

    MockPublicationSender mockPublicationSender;
    MockTestRequestCaller* mockTestRequestCaller = new MockTestRequestCaller();

    // The attribute will change to this value
    joynr::types::GpsLocation gpsLocation;
    QVariant attributeValue = QVariant::fromValue(gpsLocation);

    // Expect an attribute change to send a publication as well as during registering subscription request
    EXPECT_CALL(
                mockPublicationSender,
                sendSubscriptionPublication(
                    _, // sender participant ID
                    _, // receiver participant ID
                    _, // messaging QoS
                    GpsAttributeMatcher() // subscription publication
                )
    )
            .Times(Between(1, 2));

    // Expect a call to set up the on change subscription
    QString attributeName = "Location";
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

    QSharedPointer<MockTestRequestCaller> requestCaller(mockTestRequestCaller);
    PublicationManager publicationManager;

    //SubscriptionRequest
    QString senderId = "SenderId";
    QString receiverId = "ReceiverId";
    //SubscriptionQos
    qint64 minInterval_ms = 50;
    qint64 validity_ms = 500;
    auto qos = QSharedPointer<SubscriptionQos>(new OnChangeSubscriptionQos(
                        validity_ms,
                        minInterval_ms));

    // will be deleted by the publication manager
    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(qos);
    LOG_DEBUG(logger, "adding request");
    publicationManager.add(senderId, receiverId, requestCaller,subscriptionRequest,&mockPublicationSender);

    // Fake an attribute change
    attributeListener->attributeValueChanged(attributeValue);

    QThreadSleep::msleep(500);
}

TEST(PublicationManagerTest, add_onChangeWithNoExpiryDate) {
    joynr_logging::Logger* logger = joynr_logging::Logging::getInstance()->getLogger("TEST", "PublicationManagerTest");
    QFile::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_STORAGE_FILENAME()); //remove stored subscriptions

    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>("tests/Test");

    MockPublicationSender mockPublicationSender;
    MockTestRequestCaller* mockTestRequestCaller = new MockTestRequestCaller();

    // The attribute will change to this value
    joynr::types::GpsLocation gpsLocation;
    QVariant attributeValue = QVariant::fromValue(gpsLocation);

    // Expect a single attribute change to send a publication + one publication when registering sub request -> 2
    EXPECT_CALL(
                mockPublicationSender,
                sendSubscriptionPublication(
                    _, // sender participant ID
                    _, // receiver participant ID
                    _, // messaging QoS
                    GpsAttributeMatcher() // subscription publication
                )
    )
            .Times(2);

    // Expect calls to register an unregister an attribute listener
    QString attributeName = "Location";
    IAttributeListener* attributeListener;

    EXPECT_CALL(*mockTestRequestCaller,registerAttributeListener(attributeName, _))
            .Times(1)
            .WillRepeatedly(testing::SaveArg<1>(&attributeListener));

    EXPECT_CALL(*mockTestRequestCaller,unregisterAttributeListener(attributeName, _)).Times(1);

    QSharedPointer<MockTestRequestCaller> requestCaller(mockTestRequestCaller);
    PublicationManager publicationManager;

    //SubscriptionRequest
    QString senderId = "SenderId";
    QString receiverId = "ReceiverId";
    //SubscriptionQos
    qint64 minInterval_ms = 500;
    qint64 validity_ms = -1; //no expiry date -> infinite subscription
    auto qos = QSharedPointer<SubscriptionQos>(new OnChangeSubscriptionQos(
                        validity_ms,
                        minInterval_ms));

    // will be deleted by the publication manager
    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(qos);
    LOG_DEBUG(logger, "adding request");
    publicationManager.add(senderId, receiverId, requestCaller,subscriptionRequest,&mockPublicationSender);

    // Sleep so that the first publication is sent
    QThreadSleep::msleep(50);

    // Fake many attribute changes - but expect only one publication to be sent by this loop
    for (int i = 0; i < 10; i++) {
        attributeListener->attributeValueChanged(attributeValue);
    }

    // Wait for the subscription to finish
    QThreadSleep::msleep(700);

}


TEST(PublicationManagerTest, add_onChangeWithMinInterval) {
    joynr_logging::Logger* logger = joynr_logging::Logging::getInstance()->getLogger("TEST", "PublicationManagerTest");
    QFile::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_STORAGE_FILENAME()); //remove stored subscriptions

    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>("tests/Test");

    MockPublicationSender mockPublicationSender;
    MockTestRequestCaller* mockTestRequestCaller = new MockTestRequestCaller();

    // The attribute will change to this value
    joynr::types::GpsLocation gpsLocation;
    QVariant attributeValue = QVariant::fromValue(gpsLocation);

    // Expect a single attribute change to send a publication + one publication when registering sub request -> 2
    EXPECT_CALL(
                mockPublicationSender,
                sendSubscriptionPublication(
                    _, // sender participant ID
                    _, // receiver participant ID
                    _, // messaging QoS
                    GpsAttributeMatcher() // subscription publication
                )
    )
            .Times(2);

    // Expect calls to register an unregister an attribute listener
    QString attributeName = "Location";
    IAttributeListener* attributeListener;

    EXPECT_CALL(*mockTestRequestCaller,registerAttributeListener(attributeName, _))
            .Times(1)
            .WillRepeatedly(testing::SaveArg<1>(&attributeListener));

    EXPECT_CALL(*mockTestRequestCaller,unregisterAttributeListener(attributeName, _)).Times(1);

    QSharedPointer<MockTestRequestCaller> requestCaller(mockTestRequestCaller);
    PublicationManager publicationManager;

    //SubscriptionRequest
    QString senderId = "SenderId";
    QString receiverId = "ReceiverId";
    //SubscriptionQos
    qint64 minInterval_ms = 500;
    qint64 validity_ms = 600;
    auto qos = QSharedPointer<SubscriptionQos>(new OnChangeSubscriptionQos(
                        validity_ms,
                        minInterval_ms));

    // will be deleted by the publication manager
    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(qos);
    LOG_DEBUG(logger, "adding request");
    publicationManager.add(senderId, receiverId, requestCaller,subscriptionRequest,&mockPublicationSender);

    // Sleep so that the first publication is sent
    QThreadSleep::msleep(50);

    // Fake many attribute changes - but expect only one publication to be sent by this loop
    for (int i = 0; i < 10; i++) {
        attributeListener->attributeValueChanged(attributeValue);
    }

    // Wait for the subscription to finish
    QThreadSleep::msleep(700);

}


TEST(PublicationManagerTest, remove_onChangeSubscription) {
    joynr_logging::Logger* logger = joynr_logging::Logging::getInstance()->getLogger("TEST", "PublicationManagerTest");
    QFile::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_STORAGE_FILENAME()); //remove stored subscriptions

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
    QString attributeName = "Location";
    IAttributeListener* attributeListener;

    EXPECT_CALL(*mockTestRequestCaller,registerAttributeListener(attributeName, _))
            .Times(1)
            .WillRepeatedly(testing::SaveArg<1>(&attributeListener));

    EXPECT_CALL(*mockTestRequestCaller,unregisterAttributeListener(attributeName, _)).Times(1);

    QSharedPointer<MockTestRequestCaller> requestCaller(mockTestRequestCaller);
    PublicationManager publicationManager;

    //SubscriptionRequest
    QString senderId = "SenderId";
    QString receiverId = "ReceiverId";
    //SubscriptionQos
    qint64 minInterval_ms = 1;
    qint64 validity_ms = 100;
    auto qos = QSharedPointer<SubscriptionQos>(new OnChangeSubscriptionQos(
                        validity_ms,
                        minInterval_ms));

    // will be deleted by the publication manager
    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(qos);
    LOG_DEBUG(logger, "adding request");
    publicationManager.add(senderId, receiverId, requestCaller,subscriptionRequest,&mockPublicationSender);

    // Wait for the subscription to expire
    QThreadSleep::msleep(200);
}
