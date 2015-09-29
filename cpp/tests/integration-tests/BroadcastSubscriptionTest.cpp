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
#include "joynr/MessageRouter.h"
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
#include "joynr/QtOnChangeWithKeepAliveSubscriptionQos.h"
#include <QString>
#include "joynr/LibjoynrSettings.h"

#include "joynr/types/Localisation/QtGpsLocation.h"

using namespace ::testing;

using namespace joynr;

ACTION_P(ReleaseSemaphore,semaphore)
{
    semaphore->release(1);
}

/**
  * Is an integration test. Tests from Dispatcher -> SubscriptionListener and RequestCaller
  */
class BroadcastSubscriptionTest : public ::testing::Test {
public:
    BroadcastSubscriptionTest() :
        mockMessageRouter(new MockMessageRouter()),
        mockRequestCaller(new MockTestRequestCaller()),
        mockSubscriptionListenerOne(new MockSubscriptionListenerOneType<types::Localisation::QtGpsLocation>()),
        mockSubscriptionListenerTwo(new MockSubscriptionListenerTwoTypes<types::Localisation::QtGpsLocation, double>()),
        gpsLocation1(1.1, 2.2, 3.3, types::Localisation::QtGpsFixEnum::MODE2D, 0.0, 0.0, 0.0, 0.0, 444, 444, 444),
        speed1(100),
        qos(2000),
        providerParticipantId("providerParticipantId"),
        proxyParticipantId("proxyParticipantId"),
        messageFactory(),
        messageSender(mockMessageRouter),
        dispatcher(&messageSender),
        subscriptionManager(NULL)
    {
    }

    void SetUp(){
        QFile::remove(LibjoynrSettings::DEFAULT_BROADCASTSUBSCRIPTIONREQUEST_STORAGE_FILENAME()); //remove stored subscriptions
        subscriptionManager = new SubscriptionManager();
        dispatcher.registerSubscriptionManager(subscriptionManager);
        InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>(tests::ItestBase::INTERFACE_NAME());
        MetaTypeRegistrar::instance().registerMetaType<types::Localisation::QtGpsLocation>();
        MetaTypeRegistrar::instance().registerMetaType<types::Localisation::QtGpsLocation, double>();
    }

    void TearDown(){

    }

protected:
    QSharedPointer<MockMessageRouter> mockMessageRouter;
    QSharedPointer<MockTestRequestCaller> mockRequestCaller;
    std::shared_ptr<MockSubscriptionListenerOneType<types::Localisation::QtGpsLocation> > mockSubscriptionListenerOne;
    std::shared_ptr<MockSubscriptionListenerTwoTypes<types::Localisation::QtGpsLocation, double> > mockSubscriptionListenerTwo;

    types::Localisation::QtGpsLocation gpsLocation1;
    double speed1;

    // create test data
    MessagingQos qos;
    QString providerParticipantId;
    QString proxyParticipantId;

    JoynrMessageFactory messageFactory;
    JoynrMessageSender messageSender;
    Dispatcher dispatcher;
    SubscriptionManager * subscriptionManager;
private:
    DISALLOW_COPY_AND_ASSIGN(BroadcastSubscriptionTest);
};

/**
  * Trigger:    The dispatcher receives a Publication from a broadcast with a single output parameter.
  * Expected:   The SubscriptionManager retrieves the correct SubscriptionCallback and the
  *             Interpreter executes it correctly
  */
TEST_F(BroadcastSubscriptionTest, receive_publication_singleOutputParameter ) {

    qRegisterMetaType<SubscriptionPublication>("SubscriptionPublication");

    // Use a semaphore to count and wait on calls to the mockSubscriptionListener
    QSemaphore semaphore(0);
    EXPECT_CALL(*mockSubscriptionListenerOne, onReceive(A<const types::Localisation::QtGpsLocation&>()))
            .WillRepeatedly(ReleaseSemaphore(&semaphore));

    //register the subscription on the consumer side
    QString subscribeToName = "locationUpdate";
    auto subscriptionQos = QSharedPointer<QtOnChangeSubscriptionQos>(new QtOnChangeWithKeepAliveSubscriptionQos(
                80, // validity_ms
                100, // minInterval_ms
                200, // maxInterval_ms
                80 // alertInterval_ms
    ));

    BroadcastSubscriptionRequest subscriptionRequest;
    //construct a reply containing a QtGpsLocation
    SubscriptionPublication subscriptionPublication;
    subscriptionPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId());
    QList<QVariant> response;
    response.append(QVariant::fromValue(gpsLocation1));
    subscriptionPublication.setResponse(response);

    QSharedPointer<SubscriptionCallback<types::Localisation::QtGpsLocation>> subscriptionCallback(
            new SubscriptionCallback<types::Localisation::QtGpsLocation>(mockSubscriptionListenerOne));


    // subscriptionRequest is an out param
    subscriptionManager->registerSubscription(
                subscribeToName,
                subscriptionCallback,
                subscriptionQos,
                subscriptionRequest);
    // incoming publication from the provider
    JoynrMessage msg = messageFactory.createSubscriptionPublication(
                providerParticipantId,
                proxyParticipantId,
                qos,
                subscriptionPublication);

    dispatcher.receive(msg);

    // Assert that only one subscription message is received by the subscription listener
    ASSERT_TRUE(semaphore.tryAcquire(1, 1000));
    ASSERT_FALSE(semaphore.tryAcquire(1, 250));
}

/**
  * Trigger:    The dispatcher receives a Publication from a broadcast with multiple output parameters.
  * Expected:   The SubscriptionManager retrieves the correct SubscriptionCallback and the
  *             Interpreter executes it correctly
  */
TEST_F(BroadcastSubscriptionTest, receive_publication_multipleOutputParameters ) {

    qRegisterMetaType<SubscriptionPublication>("SubscriptionPublication");

    // Use a semaphore to count and wait on calls to the mockSubscriptionListener
    QSemaphore semaphore(0);
    EXPECT_CALL(*mockSubscriptionListenerTwo, onReceive(A<const types::Localisation::QtGpsLocation&>(), A<const double&>()))
            .WillRepeatedly(ReleaseSemaphore(&semaphore));

    //register the subscription on the consumer side
    QString subscribeToName = "locationUpdateWithSpeed";
    auto subscriptionQos = QSharedPointer<QtOnChangeSubscriptionQos>(new QtOnChangeWithKeepAliveSubscriptionQos(
                80, // validity_ms
                100, // minInterval_ms
                200, // maxInterval_ms
                80 // alertInterval_ms
    ));

    BroadcastSubscriptionRequest subscriptionRequest;
    //construct a reply containing a QtGpsLocation
    SubscriptionPublication subscriptionPublication;
    subscriptionPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId());
    QList<QVariant> response;
    response.append(QVariant::fromValue(gpsLocation1));
    response.append(QVariant::fromValue(speed1));
    subscriptionPublication.setResponse(response);

    QSharedPointer<SubscriptionCallback<types::Localisation::QtGpsLocation, double>> subscriptionCallback(
            new SubscriptionCallback<types::Localisation::QtGpsLocation, double>(mockSubscriptionListenerTwo));

    // subscriptionRequest is an out param
    subscriptionManager->registerSubscription(
                subscribeToName,
                subscriptionCallback,
                subscriptionQos,
                subscriptionRequest);
    // incoming publication from the provider
    JoynrMessage msg = messageFactory.createSubscriptionPublication(
                providerParticipantId,
                proxyParticipantId,
                qos,
                subscriptionPublication);

    dispatcher.receive(msg);

    // Assert that only one subscription message is received by the subscription listener
    ASSERT_TRUE(semaphore.tryAcquire(1, 1000));
    ASSERT_FALSE(semaphore.tryAcquire(1, 250));
}
