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
#include "joynr/IBroadcastListener.h"
#include "joynr/QtPeriodicSubscriptionQos.h"
#include "joynr/QtOnChangeSubscriptionQos.h"
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
#include <string>
#include <chrono>
#include <stdint.h>

using namespace joynr;

using namespace std::chrono;

class PublicationManagerTest : public testing::Test {
public:
    PublicationManagerTest() :
        logger(joynr_logging::Logging::getInstance()->getLogger("TST", "PublicationManagerTest"))
    {}

protected:
    joynr_logging::Logger* logger;
};

// New matcher definition that works with the latest Google Mock.
class SubscriptionPublicationMatcherInterface : public MatcherInterface<const joynr::SubscriptionPublication&> {
  private:
     SubscriptionPublication& expectedSubscriptionPublication;
  public:
    SubscriptionPublicationMatcherInterface (SubscriptionPublication& expectedSubscriptionPublication) : expectedSubscriptionPublication(expectedSubscriptionPublication){}
  virtual void DescribeTo(::std::ostream* os) const {}
  virtual bool MatchAndExplain(const joynr::SubscriptionPublication& subscriptionPublication,
                               MatchResultListener* listener) const {
    return subscriptionPublication == expectedSubscriptionPublication;
  }
};

inline Matcher<const SubscriptionPublication&> SubscriptionPublicationMatcher(SubscriptionPublication& expectedValue) {
  return MakeMatcher(new SubscriptionPublicationMatcherInterface(expectedValue));
}

TEST_F(PublicationManagerTest, add_requestCallerIsCalledCorrectlyByPublisherRunnables) {
    QFile::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_STORAGE_FILENAME()); //remove stored subscriptions

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
    QString senderId = "SenderId";
    QString receiverId = "ReceiverId";
    std::string attributeName("Location");
    // SUbscriptionQos
    qint64 period_ms = 100;
    qint64 validity_ms = 500;
    qint64 alertInterval_ms = 2000;
    std::shared_ptr<QtSubscriptionQos> qos(new QtPeriodicSubscriptionQos(
                        validity_ms,
                        period_ms,
                        alertInterval_ms));

    // will be deleted by the publication manager
    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscribeToName(QString::fromStdString(attributeName));
    subscriptionRequest.setQos(qos);
    LOG_DEBUG(logger, "adding request");
    publicationManager.add(senderId, receiverId, requestCaller,subscriptionRequest,&mockPublicationSender);
    QThreadSleep::msleep(500);
}


TEST_F(PublicationManagerTest, stop_publications) {
    QFile::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_STORAGE_FILENAME()); //remove stored subscriptions
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
    QString senderId = "SenderId";
    QString receiverId = "ReceiverId";
    std::string attributeName("Location");
    //QtSubscriptionQos
    qint64 period_ms = 100;
    qint64 validity_ms = 10000;
    qint64 alertInterval_ms = 1000;
    std::shared_ptr<QtSubscriptionQos> qos(new QtPeriodicSubscriptionQos(
                        validity_ms,
                        period_ms,
                        alertInterval_ms));

    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscribeToName(QString::fromStdString(attributeName));
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

TEST_F(PublicationManagerTest, remove_all_publications) {
    QFile::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_STORAGE_FILENAME()); //remove stored subscriptions
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
    QString senderId = "SenderId";
    QString receiverId = "ReceiverId";
    std::string attributeName("Location");
    //QtSubscriptionQos
    qint64 period_ms = 100;
    qint64 validity_ms = 10000;
    qint64 alertInterval_ms = 1000;
    std::shared_ptr<QtSubscriptionQos> qos(new QtPeriodicSubscriptionQos(
                        validity_ms,
                        period_ms,
                        alertInterval_ms));

    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscribeToName(QString::fromStdString(attributeName));
    subscriptionRequest.setQos(qos);

    publicationManager.add(senderId, receiverId, requestCaller,subscriptionRequest,&mockPublicationSender);
    QThreadSleep::msleep(80);
    publicationManager.removeAllSubscriptions(receiverId);
    QThreadSleep::msleep(300);
}

TEST_F(PublicationManagerTest, restore_publications) {
    QFile::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_STORAGE_FILENAME());
    qRegisterMetaType<QtPeriodicSubscriptionQos>("QtPeriodicSubscriptionQos");
    MockPublicationSender mockPublicationSender;

    //the first publicationManager will get this requestCaller:
    std::shared_ptr<MockTestRequestCaller> requestCaller(new MockTestRequestCaller(Between(1,3)));

    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>("tests/Test");


    //the second publicationManager will get this requestCaller
    //if restoring works, this caller will be called as well.
    std::shared_ptr<MockTestRequestCaller> requestCaller2(new MockTestRequestCaller(AtLeast(2)));

    PublicationManager* publicationManager = new PublicationManager() ;

    //SubscriptionRequest
    QString senderId = "SenderId";
    QString receiverId = "ReceiverId";
    std::string attributeName("Location");
    //QtSubscriptionQos
    qint64 period_ms = 100;
    qint64 validity_ms = 1000;
    qint64 alertInterval_ms = 1000;
    std::shared_ptr<QtSubscriptionQos> qos(new QtPeriodicSubscriptionQos(
                        validity_ms,
                        period_ms,
                        alertInterval_ms));

    // will be delete by the publication manager (destructor PublicationState)
    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscribeToName(QString::fromStdString(attributeName));
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

TEST_F(PublicationManagerTest, add_onChangeSubscription) {
    QFile::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_STORAGE_FILENAME()); //remove stored subscriptions

    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>("tests/Test");

    MockPublicationSender mockPublicationSender;
    MockTestRequestCaller* mockTestRequestCaller = new MockTestRequestCaller();

    // The attribute will change to this value
    joynr::types::Localisation::GpsLocation gpsLocation;
    Variant attributeValue = Variant::make<joynr::types::Localisation::GpsLocation>(gpsLocation);

    SubscriptionRequest subscriptionRequest;

    SubscriptionPublication expectedPublication;
    expectedPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId().toStdString());
    std::vector<Variant> response;
    response.push_back(attributeValue);
    expectedPublication.setResponse(response);
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
    QString senderId = "SenderId";
    QString receiverId = "ReceiverId";
    //QtSubscriptionQos
    qint64 minInterval_ms = 50;
    qint64 validity_ms = 500;
    std::shared_ptr<QtSubscriptionQos> qos(new QtOnChangeSubscriptionQos(
                        validity_ms,
                        minInterval_ms));

    // will be deleted by the publication manager
    subscriptionRequest.setSubscribeToName(QString::fromStdString(attributeName));
    subscriptionRequest.setQos(qos);
    LOG_DEBUG(logger, "adding request");
    publicationManager.add(senderId, receiverId, requestCaller,subscriptionRequest,&mockPublicationSender);

    // Fake an attribute change
    attributeListener->attributeValueChanged(attributeValue);

    QThreadSleep::msleep(500);
}

TEST_F(PublicationManagerTest, add_onChangeWithNoExpiryDate) {
    QFile::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_STORAGE_FILENAME()); //remove stored subscriptions

    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>("tests/Test");

    MockPublicationSender mockPublicationSender;
    MockTestRequestCaller* mockTestRequestCaller = new MockTestRequestCaller();

    // The attribute will change to this value
    joynr::types::Localisation::GpsLocation gpsLocation;
    Variant attributeValue = Variant::make<joynr::types::Localisation::GpsLocation>(gpsLocation);

    SubscriptionRequest subscriptionRequest;

    SubscriptionPublication expectedPublication;
    expectedPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId().toStdString());
    std::vector<Variant> response;
    response.push_back(attributeValue);
    expectedPublication.setResponse(response);
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
    QString senderId = "SenderId";
    QString receiverId = "ReceiverId";
    //QtSubscriptionQos
    qint64 minInterval_ms = 500;
    qint64 validity_ms = -1; //no expiry date -> infinite subscription
    std::shared_ptr<QtSubscriptionQos> qos(new QtOnChangeSubscriptionQos(
                        validity_ms,
                        minInterval_ms));

    // will be deleted by the publication manager
    subscriptionRequest.setSubscribeToName(QString::fromStdString(attributeName));
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


TEST_F(PublicationManagerTest, add_onChangeWithMinInterval) {
    QFile::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_STORAGE_FILENAME()); //remove stored subscriptions

    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>("tests/Test");

    MockPublicationSender mockPublicationSender;
    MockTestRequestCaller* mockTestRequestCaller = new MockTestRequestCaller();

    // The attribute will change to this value
    joynr::types::Localisation::GpsLocation gpsLocation;
    Variant attributeValue = Variant::make<joynr::types::Localisation::GpsLocation>(gpsLocation);

    SubscriptionRequest subscriptionRequest;

    SubscriptionPublication expectedPublication;
    expectedPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId().toStdString());
    std::vector<Variant> response;
    response.push_back(attributeValue);
    expectedPublication.setResponse(response);
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
    QString senderId = "SenderId";
    QString receiverId = "ReceiverId";
    //QtSubscriptionQos
    qint64 minInterval_ms = 500;
    qint64 validity_ms = 600;
    std::shared_ptr<QtSubscriptionQos> qos(new QtOnChangeSubscriptionQos(
                        validity_ms,
                        minInterval_ms));

    subscriptionRequest.setSubscribeToName(QString::fromStdString(attributeName));
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

TEST_F(PublicationManagerTest, attribute_add_withExistingSubscriptionId) {
    QFile::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_STORAGE_FILENAME()); //remove stored subscriptions

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
    expectedPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId().toStdString());
    std::vector<Variant> response;
    response.push_back(attributeValue);
    expectedPublication.setResponse(response);
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
    QString senderId = "SenderId";
    QString receiverId = "ReceiverId";
    //QtSubscriptionQos
    qint64 minInterval_ms = 100;
    qint64 validity_ms = 600;
    std::shared_ptr<QtOnChangeSubscriptionQos> qos(new QtOnChangeSubscriptionQos(
                        validity_ms,
                        minInterval_ms));

    int64_t now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    qos->setExpiryDate(now + 5000);

    subscriptionRequest.setSubscribeToName(QString::fromStdString(attributeName));
    subscriptionRequest.setQos(qos);
    LOG_DEBUG(logger, "adding attribute subscription request");
    publicationManager.add(senderId, receiverId, requestCaller,subscriptionRequest,&mockPublicationSender);

    // Sleep so that the first publication is sent
    QThreadSleep::msleep(minInterval_ms + 50);

    // Fake attribute change
    attributeListener->attributeValueChanged(attributeValue);

    QThreadSleep::msleep(50);
    // now, we assume that two publications have been occured

    QThreadSleep::msleep(minInterval_ms);

    // Fake attribute change
    attributeListener->attributeValueChanged(attributeValue);

    QThreadSleep::msleep(minInterval_ms + 50);
    // now, we assume that three publications have been occured

    //now, let's update the subscription and check if the provided data is correctly processed by the PublicationManager
    // will be deleted by the publication manager

    qos->setMinInterval(minInterval_ms + 500);
    subscriptionRequest.setQos(qos);
    LOG_DEBUG(logger, "update attribute subscription request");
    publicationManager.add(senderId, receiverId, requestCaller2,subscriptionRequest,&mockPublicationSender2);

    // Sleep so that the first publication is sent
    QThreadSleep::msleep(minInterval_ms + 50);

    // Fake attribute change
    attributeListener->attributeValueChanged(attributeValue);

    // sleep, waiting for the async publication (which shouldn't come)
    QThreadSleep::msleep(50);

    // until now, only one publication should be arrived to mockPublicationSender2

    // Fake attribute change. This change shall not result in a new attribute value changed
    attributeListener->attributeValueChanged(attributeValue);

    // Wait for the subscription to finish
    QThreadSleep::msleep(minInterval_ms + 500);

    // now, we should got 2 publications on mockPublicationSender2
}

TEST_F(PublicationManagerTest, attribute_add_withExistingSubscriptionId_testQos_withGreaterExpiryDate) {
    QFile::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_STORAGE_FILENAME()); //remove stored subscriptions

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
    expectedPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId().toStdString());
    std::vector<Variant> response;
    response.push_back(attributeValue);
    expectedPublication.setResponse(response);
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
    QString senderId = "SenderId";
    QString receiverId = "ReceiverId";
    //QtSubscriptionQos
    qint64 minInterval_ms = 50;
    qint64 validity_ms = 600;
    qint64 testRelExpiryDate = 500;
    int64_t now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    int64_t testAbsExpiryDate = now + testRelExpiryDate;
    std::shared_ptr<QtOnChangeSubscriptionQos> qos(new QtOnChangeSubscriptionQos(
                        validity_ms,
                        minInterval_ms));

    qos->setExpiryDate(testAbsExpiryDate);

    subscriptionRequest.setSubscribeToName(QString::fromStdString(attributeName));
    subscriptionRequest.setQos(qos);

    LOG_DEBUG(logger, "adding attribute subscription request");
    publicationManager.add(senderId, receiverId, requestCaller,subscriptionRequest,&mockPublicationSender);

    // exceed the minInterval
    QThreadSleep::msleep(minInterval_ms+50);

    // now, we expect that one publication has been performed

    // now, let's update the subscription and check if the provided data is correctly processed by the PublicationManager
    // extend the expiry date
    qos->setExpiryDate(testAbsExpiryDate + 1000);
    subscriptionRequest.setQos(qos);
    LOG_DEBUG(logger, "adding request");
    publicationManager.add(senderId, receiverId, requestCaller,subscriptionRequest,&mockPublicationSender);

    // Sleep so that the first publication is sent
    QThreadSleep::msleep(50);
    //now we expect that two puclications have been performed

    //now, exceed the original expiryDate, and make an attribute change
    QThreadSleep::msleep(testRelExpiryDate);
    attributeListener->attributeValueChanged(attributeValue);

    // wait for the async publication
    QThreadSleep::msleep(50);
    // now, three publications should be noticed

    // wait for the subscription to finish
    QThreadSleep::msleep(minInterval_ms + testRelExpiryDate);
}

TEST_F(PublicationManagerTest, attribtue_add_withExistingSubscriptionId_testQos_withLowerExpiryDate) {
    QFile::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_STORAGE_FILENAME()); //remove stored subscriptions

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
    expectedPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId().toStdString());
    std::vector<Variant> response;
    response.push_back(attributeValue);
    expectedPublication.setResponse(response);
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
    QString senderId = "SenderId";
    QString receiverId = "ReceiverId";
    //QtSubscriptionQos
    qint64 minInterval_ms = 50;
    qint64 validity_ms = 600;
    qint64 testExpiryDate_shift = 2500;
    qint64 testRelExpiryDate = 500 + testExpiryDate_shift;
    int64_t now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    int64_t testAbsExpiryDate = now + testRelExpiryDate;
    std::shared_ptr<QtOnChangeSubscriptionQos> qos(new QtOnChangeSubscriptionQos(
                        validity_ms,
                        minInterval_ms));

    qos->setExpiryDate(testAbsExpiryDate);

    subscriptionRequest.setSubscribeToName(QString::fromStdString(attributeName));
    subscriptionRequest.setQos(qos);
    LOG_DEBUG(logger, "adding attribute subscription request");
    publicationManager.add(senderId, receiverId, requestCaller,subscriptionRequest,&mockPublicationSender);

    // exceed the minInterval
    QThreadSleep::msleep(50);

    attributeListener->attributeValueChanged(attributeValue);
    QThreadSleep::msleep(minInterval_ms + 50);

    // now, we expect that two publications have been performed

    // now, let's update the subscription and check if the provided data is correctly processed by the PublicationManager
    // reduce the expiry date
    qos->setExpiryDate(testAbsExpiryDate - testExpiryDate_shift);
    subscriptionRequest.setQos(qos);
    LOG_DEBUG(logger, "update attribute subscription request");
    publicationManager.add(senderId, receiverId, requestCaller,subscriptionRequest,&mockPublicationSender);

    // Sleep so that the first publication is sent
    QThreadSleep::msleep(50);
    // now we expect that three puclications have been performed

    // now, exceed the new expiryDate, and make an attribute change
    QThreadSleep::msleep(testRelExpiryDate - testExpiryDate_shift);
    // now, the subscription should be death

    attributeListener->attributeValueChanged(attributeValue);

    // wait for the async publication, which shouldn't arrive
    QThreadSleep::msleep(50);
}

TEST_F(PublicationManagerTest, broadcast_add_withExistingSubscriptionId) {
    QFile::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_STORAGE_FILENAME()); //remove stored subscriptions

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

    QList<std::shared_ptr<IBroadcastFilter> > filters;

    // Expect calls to register an unregister an broadcast listener
    std::string broadcastName("Location");
    IBroadcastListener* broadcastListener;

    BroadcastSubscriptionRequest subscriptionRequest;

    SubscriptionPublication expectedPublication;
    expectedPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId().toStdString());
    expectedPublication.setResponse(broadcastValues);

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
    QString senderId = "SenderId";
    QString receiverId = "ReceiverId";
    //QtSubscriptionQos
    qint64 minInterval_ms = 100;
    qint64 validity_ms = 600;
    std::shared_ptr<QtOnChangeSubscriptionQos> qos(new QtOnChangeSubscriptionQos(
                        validity_ms,
                        minInterval_ms));
    int64_t now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    qos->setExpiryDate(now + 5000);

    subscriptionRequest.setSubscribeToName(QString::fromStdString(broadcastName));
    subscriptionRequest.setQos(qos);
    LOG_DEBUG(logger, "adding broadcast subscription request");
    publicationManager.add(senderId, receiverId, requestCaller,subscriptionRequest,&mockPublicationSender);

    // Fake broadcast
    broadcastListener->broadcastOccurred(broadcastValues, filters);

    QThreadSleep::msleep(50);

    // now, we assume that one publication has been occured
    QThreadSleep::msleep(minInterval_ms);

    // Fake broadcast
    broadcastListener->broadcastOccurred(broadcastValues, filters);

    qint64 newMinInterval = minInterval_ms + 500;
    QThreadSleep::msleep(50 + newMinInterval);
    // now, we assume that two publications have been occured

    //now, let's update the subscription an check if the provided data is correctly processed by the PublicationManager

    qos->setMinInterval(newMinInterval);
    subscriptionRequest.setQos(qos);
    LOG_DEBUG(logger, "update broadcast subscription request");
    publicationManager.add(senderId, receiverId, requestCaller2,subscriptionRequest,&mockPublicationSender2);

    // Fake broadcast
    broadcastListener->broadcastOccurred(broadcastValues, filters);

    // sleep, waiting for the async publication (which shouldn't come)
    QThreadSleep::msleep(50);
    // until now, only one publication should be arrived to mockPublicationSender2

    QThreadSleep::msleep(minInterval_ms + 50);

    // Fake broadcast. This change shall not result in a new broadcast to the client
    broadcastListener->broadcastOccurred(broadcastValues, filters);

    // Wait for the subscription to finish
    QThreadSleep::msleep(500);
}

TEST_F(PublicationManagerTest, broadcast_add_withExistingSubscriptionId_testQos_withGreaterExpiryDate) {
    QFile::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_STORAGE_FILENAME()); //remove stored subscriptions

    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>("tests/Test");

    MockPublicationSender mockPublicationSender;
    MockTestRequestCaller* mockTestRequestCaller = new MockTestRequestCaller();

    // The value will be fired by the broadcast
    joynr::types::Localisation::GpsLocation gpsLocation;
    std::vector<Variant> broadcastValues;
    broadcastValues.push_back(Variant::make<joynr::types::Localisation::GpsLocation>(gpsLocation));

    QList<std::shared_ptr<IBroadcastFilter> > filters;

    // Expect calls to register an unregister a broadcast listener
    std::string broadcastName("Location");
    IBroadcastListener* broadcastListener;

    BroadcastSubscriptionRequest subscriptionRequest;

    SubscriptionPublication expectedPublication;
    expectedPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId().toStdString());
    expectedPublication.setResponse(broadcastValues);

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
    QString senderId = "SenderId";
    QString receiverId = "ReceiverId";
    //QtSubscriptionQos
    qint64 minInterval_ms = 50;
    qint64 validity_ms = 600;
    qint64 testRelExpiryDate = 500;
    int64_t now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    int64_t testAbsExpiryDate = now + testRelExpiryDate;
    std::shared_ptr<QtOnChangeSubscriptionQos> qos(new QtOnChangeSubscriptionQos(
                        validity_ms,
                        minInterval_ms));

    qos->setExpiryDate(testAbsExpiryDate);

    subscriptionRequest.setSubscribeToName(QString::fromStdString(broadcastName));
    subscriptionRequest.setQos(qos);
    LOG_DEBUG(logger, "add broadcast subscription request");
    publicationManager.add(senderId, receiverId, requestCaller, subscriptionRequest, &mockPublicationSender);

    broadcastListener->broadcastOccurred(broadcastValues, filters);
    // exceed the minInterval
    QThreadSleep::msleep(minInterval_ms+50);

    // now, we expect that one publication has been performed

    // now, let's update the subscription and check if the provided data is correctly processed by the PublicationManager
    // extend the expiry date
    qos->setExpiryDate(testAbsExpiryDate + 1000);
    subscriptionRequest.setQos(qos);
    LOG_DEBUG(logger, "update broadcast subscription request");
    publicationManager.add(senderId, receiverId, requestCaller,subscriptionRequest,&mockPublicationSender);

    //now, exceed the original expiryDate, and make a broadcast
    QThreadSleep::msleep(testRelExpiryDate);
    broadcastListener->broadcastOccurred(broadcastValues, filters);

    // wait for the async publication
    QThreadSleep::msleep(50);
    //now, two publications should be noticed, even if the original subscription is expired
}

TEST_F(PublicationManagerTest, broadcast_add_withExistingSubscriptionId_testQos_withLowerExpiryDate) {
    QFile::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_STORAGE_FILENAME()); //remove stored subscriptions

    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>("tests/Test");

    MockPublicationSender mockPublicationSender;
    MockTestRequestCaller* mockTestRequestCaller = new MockTestRequestCaller();

    // The broadcast will fire this value
    joynr::types::Localisation::GpsLocation gpsLocation;
    std::vector<Variant> broadcastValues;
    broadcastValues.push_back(Variant::make<joynr::types::Localisation::GpsLocation>(gpsLocation));

    QList<std::shared_ptr<IBroadcastFilter> > filters;

    // Expect calls to register an unregister a broadcast listener
    std::string broadcastName("Location");
    IBroadcastListener* broadcastListener;

    BroadcastSubscriptionRequest subscriptionRequest;

    SubscriptionPublication expectedPublication;
    expectedPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId().toStdString());
    expectedPublication.setResponse(broadcastValues);

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
    QString senderId = "SenderId";
    QString receiverId = "ReceiverId";
    //QtSubscriptionQos
    qint64 minInterval_ms = 50;
    qint64 validity_ms = 600;
    qint64 testExpiryDate_shift = 2500;
    qint64 testRelExpiryDate = 500 + testExpiryDate_shift;
    int64_t now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    int64_t testAbsExpiryDate = now + testRelExpiryDate;
    std::shared_ptr<QtOnChangeSubscriptionQos> qos(new QtOnChangeSubscriptionQos(
                        validity_ms,
                        minInterval_ms));

    qos->setExpiryDate(testAbsExpiryDate);

    subscriptionRequest.setSubscribeToName(QString::fromStdString(broadcastName));
    subscriptionRequest.setQos(qos);
    LOG_DEBUG(logger, "adding broadcast subscription request");
    publicationManager.add(senderId, receiverId, requestCaller,subscriptionRequest,&mockPublicationSender);

    broadcastListener->broadcastOccurred(broadcastValues, filters);
    QThreadSleep::msleep(50);

    // now, we expect that one publications have been performed

    // now, let's update the subscription and check if the provided data is correctly processed by the PublicationManager
    // reduce the expiry date
    qos->setExpiryDate(testAbsExpiryDate - testExpiryDate_shift);
    subscriptionRequest.setQos(qos);
    LOG_DEBUG(logger, "update broadcast subscription request");
    publicationManager.add(senderId, receiverId, requestCaller,subscriptionRequest,&mockPublicationSender);

    // now, exceed the new expiryDate, and make a broadcast
    QThreadSleep::msleep(testRelExpiryDate - testExpiryDate_shift);
    // now, the subscription should be death

    broadcastListener->broadcastOccurred(broadcastValues, filters);

    // wait for the async publication (which shouldn't arrive)
    QThreadSleep::msleep(50);

    // now, no new publication should be received, even if the expiry date of the original request hasn't been expired
    // -> one publication expected
}

TEST_F(PublicationManagerTest, remove_onChangeSubscription) {
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
    std::string attributeName("Location");
    IAttributeListener* attributeListener;

    EXPECT_CALL(*mockTestRequestCaller,registerAttributeListener(attributeName, _))
            .Times(1)
            .WillRepeatedly(testing::SaveArg<1>(&attributeListener));

    EXPECT_CALL(*mockTestRequestCaller,unregisterAttributeListener(attributeName, _)).Times(1);

    std::shared_ptr<MockTestRequestCaller> requestCaller(mockTestRequestCaller);
    PublicationManager publicationManager;

    //SubscriptionRequest
    QString senderId = "SenderId";
    QString receiverId = "ReceiverId";
    //QtSubscriptionQos
    qint64 minInterval_ms = 1;
    qint64 validity_ms = 100;
    std::shared_ptr<QtSubscriptionQos> qos(new QtOnChangeSubscriptionQos(
                        validity_ms,
                        minInterval_ms));

    // will be deleted by the publication manager
    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscribeToName(QString::fromStdString(attributeName));
    subscriptionRequest.setQos(qos);
    LOG_DEBUG(logger, "adding request");
    publicationManager.add(senderId, receiverId, requestCaller,subscriptionRequest,&mockPublicationSender);

    // Wait for the subscription to expire
    QThreadSleep::msleep(200);
}
