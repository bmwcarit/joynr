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
#include "joynr/JoynrMessageSender.h"
#include "joynr/JoynrMessageFactory.h"
#include "joynr/Dispatcher.h"
#include "joynr/SubscriptionCallback.h"
#include "joynr/SubscriptionPublication.h"
#include "joynr/Request.h"
#include "joynr/Reply.h"
#include "joynr/InterfaceRegistrar.h"
#include "joynr/MetaTypeRegistrar.h"
#include "joynr/tests/testRequestInterpreter.h"
#include "tests/utils/MockObjects.h"
#include "joynr/OnChangeWithKeepAliveSubscriptionQos.h"
#include <string>
#include "joynr/LibjoynrSettings.h"

#include "joynr/types/Localisation/GpsLocation.h"

using namespace ::testing;

using namespace joynr;

ACTION_P(ReleaseSemaphore,semaphore)
{
    semaphore->notify();
}

/**
  * Is an integration test. Tests from Dispatcher -> SubscriptionListener and RequestCaller
  */
class BroadcastSubscriptionTest : public ::testing::Test {
public:
    BroadcastSubscriptionTest() :
        mockMessageRouter(new MockMessageRouter()),
        mockRequestCaller(new MockTestRequestCaller()),
        mockSubscriptionListenerOne(new MockSubscriptionListenerOneType<types::Localisation::GpsLocation>()),
        mockSubscriptionListenerTwo(new MockSubscriptionListenerTwoTypes<types::Localisation::GpsLocation, double>()),
        gpsLocation1(1.1, 2.2, 3.3, types::Localisation::GpsFixEnum::MODE2D, 0.0, 0.0, 0.0, 0.0, 444, 444, 444),
        speed1(100),
        qos(2000),
        providerParticipantId("providerParticipantId"),
        proxyParticipantId("proxyParticipantId"),
        messageFactory(),
        messageSender(mockMessageRouter),
        dispatcher(&messageSender),
        subscriptionManager(nullptr)
    {
    }

    void SetUp(){
        //remove stored subscriptions
        std::remove(LibjoynrSettings::DEFAULT_BROADCASTSUBSCRIPTIONREQUEST_STORAGE_FILENAME().c_str());
        subscriptionManager = new SubscriptionManager();
        dispatcher.registerSubscriptionManager(subscriptionManager);
        InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>(tests::ItestBase::INTERFACE_NAME());
        MetaTypeRegistrar::instance().registerMetaType<types::Localisation::GpsLocation>();
        MetaTypeRegistrar::instance().registerMetaType<types::Localisation::GpsLocation, double>();
    }

    void TearDown(){

    }

protected:
    std::shared_ptr<MockMessageRouter> mockMessageRouter;
    std::shared_ptr<MockTestRequestCaller> mockRequestCaller;
    std::shared_ptr<MockSubscriptionListenerOneType<types::Localisation::GpsLocation> > mockSubscriptionListenerOne;
    std::shared_ptr<MockSubscriptionListenerTwoTypes<types::Localisation::GpsLocation, double> > mockSubscriptionListenerTwo;

    types::Localisation::GpsLocation gpsLocation1;
    double speed1;

    // create test data
    MessagingQos qos;
    std::string providerParticipantId;
    std::string proxyParticipantId;

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

    // Use a semaphore to count and wait on calls to the mockSubscriptionListener
    joynr::Semaphore semaphore(0);
    EXPECT_CALL(*mockSubscriptionListenerOne, onReceive(A<const types::Localisation::GpsLocation&>()))
            .WillRepeatedly(ReleaseSemaphore(&semaphore));

    //register the subscription on the consumer side
    std::string subscribeToName = "locationUpdate";
    Variant subscriptionQos = Variant::make<OnChangeWithKeepAliveSubscriptionQos>(OnChangeWithKeepAliveSubscriptionQos(
                80, // validity_ms
                100, // minInterval_ms
                200, // maxInterval_ms
                80 // alertInterval_ms
    ));

    BroadcastSubscriptionRequest subscriptionRequest;
    //construct a reply containing a GpsLocation
    SubscriptionPublication subscriptionPublication;
    subscriptionPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId());
    std::vector<Variant> response;
    response.push_back(Variant::make<types::Localisation::GpsLocation>(gpsLocation1));
    subscriptionPublication.setResponse(response);

    std::shared_ptr<SubscriptionCallback<types::Localisation::GpsLocation>> subscriptionCallback(
            new SubscriptionCallback<types::Localisation::GpsLocation>(mockSubscriptionListenerOne));


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
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(1)));
    ASSERT_FALSE(semaphore.waitFor(std::chrono::milliseconds(250)));
}

/**
  * Trigger:    The dispatcher receives a Publication from a broadcast with multiple output parameters.
  * Expected:   The SubscriptionManager retrieves the correct SubscriptionCallback and the
  *             Interpreter executes it correctly
  */
TEST_F(BroadcastSubscriptionTest, receive_publication_multipleOutputParameters ) {

    // Use a semaphore to count and wait on calls to the mockSubscriptionListener
    joynr::Semaphore semaphore(0);
    EXPECT_CALL(*mockSubscriptionListenerTwo, onReceive(A<const types::Localisation::GpsLocation&>(), A<const double&>()))
            .WillRepeatedly(ReleaseSemaphore(&semaphore));

    //register the subscription on the consumer side
    std::string subscribeToName = "locationUpdateWithSpeed";
    Variant subscriptionQos = Variant::make<OnChangeWithKeepAliveSubscriptionQos>(OnChangeWithKeepAliveSubscriptionQos(
                80, // validity_ms
                100, // minInterval_ms
                200, // maxInterval_ms
                80 // alertInterval_ms
    ));

    BroadcastSubscriptionRequest subscriptionRequest;
    //construct a reply containing a GpsLocation
    SubscriptionPublication subscriptionPublication;
    subscriptionPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId());
    std::vector<Variant> response;
    response.push_back(Variant::make<types::Localisation::GpsLocation>(gpsLocation1));
    response.push_back(Variant::make<double>(speed1));
    subscriptionPublication.setResponse(response);

    std::shared_ptr<SubscriptionCallback<types::Localisation::GpsLocation, double>> subscriptionCallback(
            new SubscriptionCallback<types::Localisation::GpsLocation, double>(mockSubscriptionListenerTwo));

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
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(1)));
    ASSERT_FALSE(semaphore.waitFor(std::chrono::milliseconds(250)));
}
