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
#include "joynr/PrivateCopyAssign.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "common/in-process/InProcessMessagingSkeleton.h"
#include "joynr/MessageRouter.h"
#include "joynr/MessagingStubFactory.h"
#include "joynr/ClusterControllerDirectories.h"
#include "joynr/JoynrMessage.h"
#include "joynr/Dispatcher.h"
#include "joynr/SubscriptionCallback.h"
#include "joynr/SubscriptionPublication.h"
#include "joynr/SubscriptionStop.h"
#include "joynr/JoynrMessageFactory.h"
#include "joynr/Request.h"
#include "joynr/Reply.h"
#include "joynr/InterfaceRegistrar.h"
#include "joynr/MetaTypeRegistrar.h"
#include "joynr/tests/testRequestInterpreter.h"
#include "tests/utils/MockObjects.h"
#include "joynr/OnChangeWithKeepAliveSubscriptionQos.h"
#include <QString>
#include <string>
#include "joynr/LibjoynrSettings.h"

#include "joynr/types/GpsLocation.h"

using namespace ::testing;

using namespace joynr;

ACTION_P(ReleaseSemaphore,semaphore)
{
    semaphore->release(1);
}

/**
  * Is an integration test. Tests from Dispatcher -> SubscriptionListener and RequestCaller
  */
class SubscriptionTest : public ::testing::Test {
public:
    SubscriptionTest() :
        mockMessageRouter(new MockMessageRouter()),
        mockCallback(new MockCallback<types::GpsLocation>()),
        mockRequestCaller(new MockTestRequestCaller()),
        mockReplyCaller(new MockReplyCaller<types::GpsLocation>(
                [this](const RequestStatus& status, const types::GpsLocation& location) {
                    mockCallback->callbackFct(status, location);
                },
                [] (const RequestStatus status){
                })),
        mockSubscriptionListener(new MockSubscriptionListenerOneType<types::GpsLocation>()),
        gpsLocation1(1.1, 2.2, 3.3, types::GpsFixEnum::MODE2D, 0.0, 0.0, 0.0, 0.0, 444, 444, 444),
        qos(2000),
        providerParticipantId("providerParticipantId"),
        proxyParticipantId("proxyParticipantId"),
        requestReplyId("requestReplyId"),
        messageFactory(),
        messageSender(mockMessageRouter),
        dispatcher(&messageSender),
        subscriptionManager(NULL),
        publicationManager(NULL)
    {
    }

    void SetUp(){
        QFile::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_STORAGE_FILENAME()); //remove stored subscriptions
        subscriptionManager = new SubscriptionManager();
        publicationManager = new PublicationManager();
        dispatcher.registerPublicationManager(publicationManager);
        dispatcher.registerSubscriptionManager(subscriptionManager);
        InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>(tests::ItestBase::INTERFACE_NAME());
        MetaTypeRegistrar::instance().registerMetaType<types::GpsLocation>();
    }

    void TearDown(){

    }

protected:
    QSharedPointer<MockMessageRouter> mockMessageRouter;
    QSharedPointer<MockCallback<types::GpsLocation> > mockCallback;

    QSharedPointer<MockTestRequestCaller> mockRequestCaller;
    QSharedPointer<MockReplyCaller<types::GpsLocation> > mockReplyCaller;
    QSharedPointer<MockSubscriptionListenerOneType<types::GpsLocation> > mockSubscriptionListener;

    types::GpsLocation gpsLocation1;

    // create test data
    MessagingQos qos;
    std::string providerParticipantId;
    std::string proxyParticipantId;
    QString requestReplyId;

    JoynrMessageFactory messageFactory;
    JoynrMessageSender messageSender;
    Dispatcher dispatcher;
    SubscriptionManager * subscriptionManager;
    PublicationManager* publicationManager;
private:
    DISALLOW_COPY_AND_ASSIGN(SubscriptionTest);
};


/**
  * Trigger:    The dispatcher receives a SubscriptionRequest.
  * Expected:   The PublicationManager creates a PublisherRunnable and polls
  *             the MockCaller for the attribute.
  */
TEST_F(SubscriptionTest, receive_subscriptionRequestAndPollAttribute) {
    qRegisterMetaType<OnChangeWithKeepAliveSubscriptionQos>("OnChangeWithKeepAliveSubscriptionQos");
    qRegisterMetaType<SubscriptionRequest>("SubscriptionRequest");

    // Use a semaphore to count and wait on calls to the mockRequestCaller
    QSemaphore semaphore(0);
    EXPECT_CALL(*mockRequestCaller, getLocation(_))
            .WillRepeatedly(
                DoAll(
                    Invoke(mockRequestCaller.data(), &MockTestRequestCaller::invokeCallbackFct),
                    ReleaseSemaphore(&semaphore)));

    QString attributeName = "Location";
    auto subscriptionQos = QSharedPointer<SubscriptionQos>(new OnChangeWithKeepAliveSubscriptionQos(
                80, // validity_ms
                100, // minInterval_ms
                200, // maxInterval_ms
                80 // alertInterval_ms
    ));
    QString subscriptionId = "SubscriptionID";
    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscriptionId(subscriptionId);
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(subscriptionQos);

    JoynrMessage msg = messageFactory.createSubscriptionRequest(
                QString::fromStdString(proxyParticipantId),
                QString::fromStdString(providerParticipantId),
                qos,
                subscriptionRequest);

    dispatcher.addRequestCaller(providerParticipantId, mockRequestCaller);
    dispatcher.receive(msg);

    // Wait for a call to be made to the mockRequestCaller
    ASSERT_TRUE(semaphore.tryAcquire(1,1000));
}


/**
  * Trigger:    The dispatcher receives a Publication.
  * Expected:   The SubscriptionManager retrieves the correct SubscriptionCallback and the
  *             Interpreter executes it correctly
  */
TEST_F(SubscriptionTest, receive_publication ) {

    qRegisterMetaType<SubscriptionPublication>("SubscriptionPublication");

    // getType is used by the ReplyInterpreterFactory to create an interpreter for the reply
    // so this has to match with the type being passed to the dispatcher in the reply
    ON_CALL(*mockReplyCaller, getType()).WillByDefault(Return(QString("GpsLocation")));

    // Use a semaphore to count and wait on calls to the mockSubscriptionListener
    QSemaphore semaphore(0);
    EXPECT_CALL(*mockSubscriptionListener, onReceive(A<const types::GpsLocation&>()))
            .WillRepeatedly(ReleaseSemaphore(&semaphore));

    //register the subscription on the consumer side
    QString attributeName = "Location";
    auto subscriptionQos = QSharedPointer<SubscriptionQos>(new OnChangeWithKeepAliveSubscriptionQos(
                80, // validity_ms
                100, // minInterval_ms
                200, // maxInterval_ms
                80 // alertInterval_ms
    ));

    SubscriptionRequest subscriptionRequest;
    //construct a reply containing a GpsLocation
    SubscriptionPublication subscriptionPublication;
    subscriptionPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId());
    QVariant response;
    response.setValue(gpsLocation1);
    subscriptionPublication.setResponse(response);

    QSharedPointer<SubscriptionCallback<types::GpsLocation>> subscriptionCallback(
            new SubscriptionCallback<types::GpsLocation>(mockSubscriptionListener));


    // subscriptionRequest is an out param
    subscriptionManager->registerSubscription(
                attributeName,
                subscriptionCallback,
                subscriptionQos,
                subscriptionRequest);
    // incoming publication from the provider
    JoynrMessage msg = messageFactory.createSubscriptionPublication(
                QString::fromStdString(providerParticipantId),
                QString::fromStdString(proxyParticipantId),
                qos,
                subscriptionPublication);

    dispatcher.receive(msg);

    // Assert that only one subscription message is received by the subscription listener
    ASSERT_TRUE(semaphore.tryAcquire(1, 1000));
    ASSERT_FALSE(semaphore.tryAcquire(1, 250));
}


/**
  * Precondition: Dispatcher receives a SubscriptionRequest for a not(yet) existing Provider.
  * Trigger:    The provider is registered.
  * Expected:   The PublicationManager registers the provider and notifies the PublicationManager
  *             to restore waiting Subscriptions
  */
TEST_F(SubscriptionTest, receive_RestoresSubscription) {

    qRegisterMetaType<OnChangeWithKeepAliveSubscriptionQos>("OnChangeWithKeepAliveSubscriptionQos");
    qRegisterMetaType<SubscriptionRequest>("SubscriptionRequest");

    // Use a semaphore to count and wait on calls to the mockRequestCaller
    QSemaphore semaphore(0);
    EXPECT_CALL(*mockRequestCaller, getLocation(
                    A<std::function<void(const RequestStatus&, const types::GpsLocation&)>>()))
            .WillOnce(
                DoAll(
                    Invoke(mockRequestCaller.data(), &MockTestRequestCaller::invokeCallbackFct),
                    ReleaseSemaphore(&semaphore)));
    QString attributeName = "Location";
    auto subscriptionQos = QSharedPointer<SubscriptionQos>(new OnChangeWithKeepAliveSubscriptionQos(
                80, // validity_ms
                100, // minInterval_ms
                200, // maxInterval_ms
                80 // alertInterval_ms
    ));
    QString subscriptionId = "SubscriptionID";

    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscriptionId(subscriptionId);
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(subscriptionQos);

    JoynrMessage msg = messageFactory.createSubscriptionRequest(
                QString::fromStdString(proxyParticipantId),
                QString::fromStdString(providerParticipantId),
                qos,
                subscriptionRequest);
    // first received message with subscription request

    dispatcher.receive(msg);
    dispatcher.addRequestCaller(providerParticipantId, mockRequestCaller);
    ASSERT_TRUE(semaphore.tryAcquire(1,15000));
    //Try to acquire a semaphore for up to 5 seconds. Acquireing the semaphore will only work, if the mockRequestCaller has been called
    //and will be much faster than waiting for 500ms to make sure it has been called
}

/**
  * Precondition: A provider is registered and there is at least one subscription for it.
  * Trigger:    The request caller is removed from the dispatcher
  * Expected:   The PublicationManager stops all subscriptions for this provider
  */
TEST_F(SubscriptionTest, removeRequestCaller_stopsPublications) {
    qRegisterMetaType<OnChangeWithKeepAliveSubscriptionQos>("OnChangeWithKeepAliveSubscriptionQos");
    qRegisterMetaType<SubscriptionRequest>("SubscriptionRequest");
    qRegisterMetaType<SubscriptionStop>("SubscriptionStop");

    // Use a semaphore to count and wait on calls to the mockRequestCaller
    QSemaphore semaphore(0);
    EXPECT_CALL(*mockRequestCaller, getLocation(_))
            .WillRepeatedly(
                DoAll(
                    Invoke(mockRequestCaller.data(), &MockTestRequestCaller::invokeCallbackFct),
                    ReleaseSemaphore(&semaphore)));

    dispatcher.addRequestCaller(providerParticipantId, mockRequestCaller);
    QString attributeName = "Location";
    auto subscriptionQos = QSharedPointer<SubscriptionQos>(new OnChangeWithKeepAliveSubscriptionQos(
                1200, // validity_ms
                10, // minInterval_ms
                100, // maxInterval_ms
                1100 // alertInterval_ms
    ));
    QString subscriptionId = "SubscriptionID";
    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscriptionId(subscriptionId);
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(subscriptionQos);

    JoynrMessage msg = messageFactory.createSubscriptionRequest(
                QString::fromStdString(proxyParticipantId),
                QString::fromStdString(providerParticipantId),
                qos,
                subscriptionRequest);
    // first received message with subscription request
    dispatcher.receive(msg);
    // wait for two requests from the subscription
    ASSERT_TRUE(semaphore.tryAcquire(2, 1000));
    // remove the request caller
    dispatcher.removeRequestCaller(providerParticipantId);
    // assert that less than 2 requests happen in the next 300 milliseconds
    ASSERT_FALSE(semaphore.tryAcquire(2, 300));
}

/**
  * Precondition: A provider is registered and there is at least one subscription for it.
  * Trigger:    A subscription stop message is received
  * Expected:   The PublicationManager stops the publications for this provider
  */
TEST_F(SubscriptionTest, stopMessage_stopsPublications) {

    qRegisterMetaType<OnChangeWithKeepAliveSubscriptionQos>("OnChangeWithKeepAliveSubscriptionQos");
    qRegisterMetaType<SubscriptionRequest>("SubscriptionRequest");

    // Use a semaphore to count and wait on calls to the mockRequestCaller
    QSemaphore semaphore(0);
    EXPECT_CALL(*mockRequestCaller, getLocation(_))
            .WillRepeatedly(
                DoAll(
                    Invoke(mockRequestCaller.data(), &MockTestRequestCaller::invokeCallbackFct),
                    ReleaseSemaphore(&semaphore)));

    dispatcher.addRequestCaller(providerParticipantId, mockRequestCaller);
    QString attributeName = "Location";
    auto subscriptionQos = QSharedPointer<SubscriptionQos>(new OnChangeWithKeepAliveSubscriptionQos(
                1200, // validity_ms
                10, // minInterval_ms
                100, // maxInterval_ms
                1100 // alertInterval_ms
    ));
    QString subscriptionId = "SubscriptionID";
    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscriptionId(subscriptionId);
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(subscriptionQos);

    JoynrMessage msg = messageFactory.createSubscriptionRequest(
                QString::fromStdString(proxyParticipantId),
                QString::fromStdString(providerParticipantId),
                qos,
                subscriptionRequest);
    // first received message with subscription request
    dispatcher.receive(msg);

    // wait for two requests from the subscription
    ASSERT_TRUE(semaphore.tryAcquire(2, 1000));

    SubscriptionStop subscriptionStop;
    subscriptionStop.setSubscriptionId(subscriptionRequest.getSubscriptionId());
    // receive a subscription stop message
    msg = messageFactory.createSubscriptionStop(
                QString::fromStdString(proxyParticipantId),
                QString::fromStdString(providerParticipantId),
                qos,
                subscriptionStop);
    dispatcher.receive(msg);

    // assert that less than 2 requests happen in the next 300 milliseconds
    ASSERT_FALSE(semaphore.tryAcquire(2, 300));
}


