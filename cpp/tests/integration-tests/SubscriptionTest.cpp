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
#include <memory>
#include <string>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/Dispatcher.h"
#include "joynr/ImmutableMessage.h"
#include "joynr/InterfaceRegistrar.h"
#include "joynr/LibjoynrSettings.h"
#include "joynr/Message.h"
#include "joynr/MessageSender.h"
#include "joynr/MutableMessage.h"
#include "joynr/MutableMessageFactory.h"
#include "joynr/OnChangeWithKeepAliveSubscriptionQos.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/Semaphore.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/SubscriptionManager.h"
#include "joynr/SubscriptionPublication.h"
#include "joynr/SubscriptionStop.h"
#include "joynr/UnicastSubscriptionCallback.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/tests/testRequestCaller.h"
#include "joynr/tests/testRequestInterpreter.h"
#include "joynr/tests/testTypes/TestEnum.h"
#include "joynr/types/Localisation/GpsLocation.h"

#include "tests/JoynrTest.h"
#include "tests/mock/MockMessageRouter.h"
#include "tests/mock/MockTestRequestCaller.h"
#include "tests/mock/MockTestProvider.h"
#include "tests/mock/MockSubscriptionListener.h"

using namespace ::testing;
using namespace joynr;

/**
  * Is an integration test. Tests from Dispatcher -> SubscriptionListener and RequestCaller
  */
class SubscriptionTest : public ::testing::Test {
public:
    SubscriptionTest() :
        singleThreadedIOService(std::make_shared<SingleThreadedIOService>()),
        mockMessageRouter(new MockMessageRouter(singleThreadedIOService->getIOService())),
        mockRequestCaller(new MockTestRequestCaller()),
        mockGpsLocationListener(new MockSubscriptionListenerOneType<types::Localisation::GpsLocation>()),
        mockTestEnumSubscriptionListener(new MockSubscriptionListenerOneType<tests::testTypes::TestEnum::Enum>()),
        gpsLocation1(1.1, 2.2, 3.3, types::Localisation::GpsFixEnum::MODE2D, 0.0, 0.0, 0.0, 0.0, 444, 444, 444),
        qos(2000),
        providerParticipantId("providerParticipantId"),
        proxyParticipantId("proxyParticipantId"),
        requestReplyId("requestReplyId"),
        persistencyEnabled(true),
        messageFactory(),
        messageSender(std::make_shared<MessageSender>(mockMessageRouter, nullptr)),
        dispatcher(std::make_shared<Dispatcher>(messageSender, singleThreadedIOService->getIOService())),
        subscriptionManager(),
        provider(new MockTestProvider),
        publicationManager(std::make_shared<PublicationManager>(singleThreadedIOService->getIOService(), messageSender, persistencyEnabled)),
        requestCaller(new joynr::tests::testRequestCaller(provider)),
        isLocalMessage(true)
    {
        singleThreadedIOService->start();
    }

    void SetUp(){
        std::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME().c_str()); //remove stored subscriptions
        subscriptionManager = std::make_shared<SubscriptionManager>(singleThreadedIOService->getIOService(), mockMessageRouter);
        dispatcher->registerPublicationManager(publicationManager);
        dispatcher->registerSubscriptionManager(subscriptionManager);
        InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>(tests::ItestBase::INTERFACE_NAME());
    }

    ~SubscriptionTest()
    {
        publicationManager->shutdown();
        dispatcher->shutdown();
        singleThreadedIOService->stop();
    }

protected:
    void receive_publicationWithException(std::shared_ptr<exceptions::JoynrRuntimeException> expectedException);

    std::shared_ptr<SingleThreadedIOService> singleThreadedIOService;
    std::shared_ptr<MockMessageRouter> mockMessageRouter;

    std::shared_ptr<MockTestRequestCaller> mockRequestCaller;
    std::shared_ptr<MockSubscriptionListenerOneType<types::Localisation::GpsLocation> > mockGpsLocationListener;
    std::shared_ptr<MockSubscriptionListenerOneType<tests::testTypes::TestEnum::Enum> > mockTestEnumSubscriptionListener;

    types::Localisation::GpsLocation gpsLocation1;

    // create test data
    MessagingQos qos;
    std::string providerParticipantId;
    std::string proxyParticipantId;
    std::string requestReplyId;
    const bool persistencyEnabled;

    MutableMessageFactory messageFactory;
    std::shared_ptr<MessageSender> messageSender;
    std::shared_ptr<Dispatcher> dispatcher;
    std::shared_ptr<SubscriptionManager> subscriptionManager;
    std::shared_ptr<MockTestProvider> provider;
    std::shared_ptr<PublicationManager> publicationManager;
    std::shared_ptr<joynr::tests::testRequestCaller> requestCaller;
    const bool isLocalMessage;
private:
    DISALLOW_COPY_AND_ASSIGN(SubscriptionTest);
};


/**
  * Trigger:    The dispatcher receives a SubscriptionRequest.
  * Expected:   The PublicationManager creates a PublisherRunnable and polls
  *             the MockCaller for the attribute.
  */
TEST_F(SubscriptionTest, receive_subscriptionRequestAndPollAttribute) {

    // Use a semaphore to count and wait on calls to the mockRequestCaller
    Semaphore semaphore(0);
    EXPECT_CALL(*mockRequestCaller, getLocationMock(_,_))
            .WillRepeatedly(
                DoAll(
                    Invoke(mockRequestCaller.get(), &MockTestRequestCaller::invokeLocationOnSuccessFct),
                    ReleaseSemaphore(&semaphore)));

    std::string attributeName = "Location";
    auto subscriptionQos = std::make_shared<OnChangeWithKeepAliveSubscriptionQos>(
                500, // validity_ms
                1000, // publication ttl
                1000, // minInterval_ms
                2000, // maxInterval_ms
                1000 // alertInterval_ms
    );
    std::string subscriptionId = "SubscriptionID";
    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscriptionId(subscriptionId);
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(subscriptionQos);

    MutableMessage mutableMessage = messageFactory.createSubscriptionRequest(
                proxyParticipantId,
                providerParticipantId,
                qos,
                subscriptionRequest,
                isLocalMessage);

    dispatcher->addRequestCaller(providerParticipantId, mockRequestCaller);
    dispatcher->receive(mutableMessage.getImmutableMessage());

    // Wait for a call to be made to the mockRequestCaller
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(1)));
}


/**
  * Trigger:    The dispatcher receives a Publication.
  * Expected:   The SubscriptionManager retrieves the correct SubscriptionCallback and the
  *             Interpreter executes it correctly
  */
TEST_F(SubscriptionTest, receive_publication ) {
    // Use a semaphore to count and wait on calls to the mockGpsLocationListener
    Semaphore publicationSemaphore(0);
    EXPECT_CALL(*mockGpsLocationListener, onReceive(A<const types::Localisation::GpsLocation&>()))
            .WillRepeatedly(ReleaseSemaphore(&publicationSemaphore));

    //register the subscription on the consumer side
    std::string attributeName = "Location";
    auto subscriptionQos = std::make_shared<OnChangeWithKeepAliveSubscriptionQos>(
                500, // validity_ms
                1000, // publication ttl
                1000, // minInterval_ms
                2000, // maxInterval_ms
                1000 // alertInterval_ms
    );

    SubscriptionRequest subscriptionRequest;
    //construct a reply containing a GpsLocation
    SubscriptionPublication subscriptionPublication;
    subscriptionPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId());
    subscriptionPublication.setResponse(gpsLocation1);

    auto future = std::make_shared<Future<std::string>>();
    auto subscriptionCallback = std::make_shared<UnicastSubscriptionCallback<types::Localisation::GpsLocation>
            >(subscriptionRequest.getSubscriptionId(), future, subscriptionManager);

    // subscriptionRequest is an out param
    subscriptionManager->registerSubscription(
                attributeName,
                subscriptionCallback,
                mockGpsLocationListener,
                subscriptionQos,
                subscriptionRequest);
    // incoming publication from the provider
    MutableMessage mutableMessage = messageFactory.createSubscriptionPublication(
                providerParticipantId,
                proxyParticipantId,
                qos,
                subscriptionPublication);

    dispatcher->receive(mutableMessage.getImmutableMessage());

    // Assert that only one subscription message is received by the subscription listener
    ASSERT_TRUE(publicationSemaphore.waitFor(std::chrono::seconds(1)));
    ASSERT_FALSE(publicationSemaphore.waitFor(std::chrono::seconds(1)));
}

void SubscriptionTest::receive_publicationWithException(std::shared_ptr<exceptions::JoynrRuntimeException> expectedException)
{
    Semaphore semaphore(0);
    auto mockIntListener = std::make_shared<MockSubscriptionListenerOneType<std::int32_t>>();
    EXPECT_CALL(*mockIntListener, onReceive(A<const std::int32_t&>()))
            .Times(0);
    EXPECT_CALL(*mockIntListener, onError(
                    joynrException(
                        expectedException->getTypeName(),
                        expectedException->getMessage())))
            .Times(1)
            .WillOnce(ReleaseSemaphore(&semaphore));

    // register the subscription on the consumer side
    const std::string attributeName = "attributeWithProviderRuntimeException";

    auto subscriptionQos = std::make_shared<OnChangeWithKeepAliveSubscriptionQos>(
                500, // validity_ms
                1000, // publication ttl
                1000, // minInterval_ms
                2000, // maxInterval_ms
                1000 // alertInterval_ms
    );

    SubscriptionRequest subscriptionRequest;

    auto future = std::make_shared<Future<std::string>>();
    auto subscriptionCallback = std::make_shared<UnicastSubscriptionCallback<std::int32_t>
            >(subscriptionRequest.getSubscriptionId(), future, subscriptionManager);

    subscriptionManager->registerSubscription(
                attributeName,
                subscriptionCallback,
                mockIntListener,
                subscriptionQos,
                subscriptionRequest);

    // construct a subscriptionPublication containing a ProviderRuntimeException
    SubscriptionPublication subscriptionPublication;
    subscriptionPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId());
    subscriptionPublication.setError(expectedException);

    // incoming publication from the provider
    MutableMessage msg = messageFactory.createSubscriptionPublication(
                providerParticipantId,
                proxyParticipantId,
                qos,
                subscriptionPublication);

    dispatcher->receive(msg.getImmutableMessage());
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(1)));
}

TEST_F(SubscriptionTest, receive_publicationWithProviderRuntimeException) {
    auto expectedException = std::make_shared<exceptions::ProviderRuntimeException
            >("TESTreceive_publicationWithProviderRuntimeExceptionERROR");
    receive_publicationWithException(expectedException);
}

TEST_F(SubscriptionTest, receive_publicationWithMethodInvocationException) {
    auto expectedException = std::make_shared<exceptions::MethodInvocationException
            >("TESTreceive_publicationWithMethodInvocationExceptionERROR");

    receive_publicationWithException(expectedException);
}

/**
  * Trigger:    The dispatcher receives an enum Publication.
  * Expected:   The SubscriptionManager retrieves the correct SubscriptionCallback and the
  *             Interpreter executes it correctly
  */
TEST_F(SubscriptionTest, receive_enumPublication ) {
    // Use a semaphore to count and wait on calls to the mockTestEnumSubscriptionListener
    Semaphore semaphore(0);
    EXPECT_CALL(*mockTestEnumSubscriptionListener, onReceive(A<const joynr::tests::testTypes::TestEnum::Enum&>()))
            .WillRepeatedly(ReleaseSemaphore(&semaphore));

    //register the subscription on the consumer side
    std::string attributeName = "testEnum";
    auto subscriptionQos = std::make_shared<OnChangeWithKeepAliveSubscriptionQos>(
                500, // validity_ms
                1000, // publication ttl
                1000, // minInterval_ms
                2000, // maxInterval_ms
                1000 // alertInterval_ms
    );

    SubscriptionRequest subscriptionRequest;
    //construct a reply containing a GpsLocation
    SubscriptionPublication subscriptionPublication;
    subscriptionPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId());
    subscriptionPublication.setResponse(tests::testTypes::TestEnum::ZERO);
    auto future = std::make_shared<Future<std::string>>();
    auto subscriptionCallback = std::make_shared<UnicastSubscriptionCallback<joynr::tests::testTypes::TestEnum::Enum>
            >(subscriptionRequest.getSubscriptionId(), future, subscriptionManager);

    // subscriptionRequest is an out param
    subscriptionManager->registerSubscription(
                attributeName,
                subscriptionCallback,
                mockTestEnumSubscriptionListener,
                subscriptionQos,
                subscriptionRequest);
    // incoming publication from the provider
    MutableMessage mutableMessage = messageFactory.createSubscriptionPublication(
                providerParticipantId,
                proxyParticipantId,
                qos,
                subscriptionPublication);

    dispatcher->receive(mutableMessage.getImmutableMessage());

    // Assert that only one subscription message is received by the subscription listener
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(1)));
    ASSERT_FALSE(semaphore.waitFor(std::chrono::seconds(1)));
}

/**
  * Precondition: Dispatcher receives a SubscriptionRequest for a not(yet) existing Provider.
  * Trigger:    The provider is registered.
  * Expected:   The PublicationManager registers the provider and notifies the PublicationManager
  *             to restore waiting Subscriptions
  */
TEST_F(SubscriptionTest, receive_RestoresSubscription) {

    // Use a semaphore to count and wait on calls to the mockRequestCaller
    Semaphore semaphore(0);
    EXPECT_CALL(
            *mockRequestCaller,
            getLocationMock(A<std::function<void(const types::Localisation::GpsLocation&)>>(),
                        A<std::function<void(const std::shared_ptr<joynr::exceptions::ProviderRuntimeException>&)>>())
    )
            .WillOnce(DoAll(
                    Invoke(mockRequestCaller.get(), &MockTestRequestCaller::invokeLocationOnSuccessFct),
                    ReleaseSemaphore(&semaphore)
            ));
    std::string attributeName = "Location";
    auto subscriptionQos = std::make_shared<OnChangeWithKeepAliveSubscriptionQos>(
                500, // validity_ms
                1000, // publication ttl
                1000, // minInterval_ms
                2000, // maxInterval_ms
                1000 // alertInterval_ms
    );
    std::string subscriptionId = "SubscriptionID";

    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscriptionId(subscriptionId);
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(subscriptionQos);

    MutableMessage mutableMessage = messageFactory.createSubscriptionRequest(
                proxyParticipantId,
                providerParticipantId,
                qos,
                subscriptionRequest,
                isLocalMessage);
    // first received message with subscription request

    dispatcher->receive(mutableMessage.getImmutableMessage());
    dispatcher->addRequestCaller(providerParticipantId, mockRequestCaller);
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(15)));
    //Try to acquire a semaphore for up to 15 seconds. Acquireing the semaphore will only work, if the mockRequestCaller has been called
    //and will be much faster than waiting for 1s to make sure it has been called
}

TEST_F(SubscriptionTest, sendPublication_attributeWithSingleArrayParam) {

    using ImmutableMessagePtr = std::shared_ptr<ImmutableMessage>;

    std::string subscriptionId = "SubscriptionID";
    auto subscriptionQos = std::make_shared<OnChangeSubscriptionQos>(
                800, // validity_ms
                1000, // publication ttl
                0 // minInterval_ms
    );

    // Use a semaphore to count and wait on calls to the mockRequestCaller
    Semaphore semaphore(0);

    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscriptionId(subscriptionId);
    subscriptionRequest.setSubscribeToName("listOfStrings");
    subscriptionRequest.setQos(subscriptionQos);

    EXPECT_CALL(
            *provider,
            getListOfStrings(A<std::function<void(const std::vector<std::string> &)>>(),
                    A<std::function<void(const joynr::exceptions::ProviderRuntimeException&)>>())
    )
            .WillOnce(DoAll(
                    Invoke(provider.get(), &MockTestProvider::invokeListOfStringsOnSuccess),
                    ReleaseSemaphore(&semaphore)
            ));

    /* ensure the serialization succeeds and the first publication and the subscriptionReply are sent to the proxy */
    EXPECT_CALL(*mockMessageRouter, route(
                     AllOf(
                         A<ImmutableMessagePtr>(),
                         MessageHasSender(providerParticipantId),
                         MessageHasRecipient(proxyParticipantId)
                        ),
                     _
                     )).Times(2);

    publicationManager->add(
                proxyParticipantId,
                providerParticipantId,
                requestCaller,
                subscriptionRequest,
                messageSender);

    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(15)));

    std::vector<std::string> listOfStrings;
    listOfStrings.push_back("1");
    listOfStrings.push_back("2");

    /* ensure the value change leads to another publication */
    Mock::VerifyAndClear(mockMessageRouter.get());
    EXPECT_CALL(*mockMessageRouter, route(
                     AllOf(
                         A<ImmutableMessagePtr>(),
                         MessageHasSender(providerParticipantId),
                         MessageHasRecipient(proxyParticipantId)
                        ),
                     _
                     ));

    provider->listOfStringsChanged(listOfStrings);
}

TEST_F(SubscriptionTest, sendPublication_attributeWithProviderRuntimeException) {
    using ImmutableMessagePtr = std::shared_ptr<ImmutableMessage>;

    joynr::Semaphore semaphore(0);
    const std::string subscriptionId = "SubscriptionID";
    auto subscriptionQos = std::make_shared<OnChangeWithKeepAliveSubscriptionQos>(
                600, // validity_ms
                1000, // publication ttl
                100, // minInterval_ms
                400, // maxInterval_ms
                1000 // alertInterval_ms
    );

    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscriptionId(subscriptionId);
    subscriptionRequest.setSubscribeToName("attributeWithProviderRuntimeException");
    subscriptionRequest.setQos(subscriptionQos);

    auto expectedException =std::make_shared<exceptions::ProviderRuntimeException
            >(provider->providerRuntimeExceptionTestMsg);
    SubscriptionPublication expectedPublication;
    expectedPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId());
    expectedPublication.setError(expectedException);

    EXPECT_CALL(*mockMessageRouter,
                route(
                    AllOf(
                        A<ImmutableMessagePtr>(),
                        MessageHasType(joynr::Message::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY()),
                        MessageHasSender(providerParticipantId),
                        MessageHasRecipient(proxyParticipantId)
                        ),
                    _
                    )
                );

    EXPECT_CALL(*mockMessageRouter,
                route(
                    AllOf(
                        A<ImmutableMessagePtr>(),
                        MessageHasType(joynr::Message::VALUE_MESSAGE_TYPE_PUBLICATION()),
                        MessageHasSender(providerParticipantId),
                        MessageHasRecipient(proxyParticipantId),
                        ImmutableMessageHasPayload(joynr::serializer::serializeToJson(expectedPublication))
                        ),
                    _
                    )
                )
            .Times(2)
            .WillRepeatedly(ReleaseSemaphore(&semaphore));

    publicationManager->add(
                proxyParticipantId,
                providerParticipantId,
                requestCaller,
                subscriptionRequest,
                messageSender);

    // wait for the 2 async publications
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10)));
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10)));
}

/**
  * Precondition: A provider is registered and there is at least one subscription for it.
  * Trigger:    The request caller is removed from the dispatcher
  * Expected:   The PublicationManager stops all subscriptions for this provider
  */
TEST_F(SubscriptionTest, removeRequestCaller_stopsPublications) {

    // Use a semaphore to count and wait on calls to the mockRequestCaller
    Semaphore semaphore(0);
    EXPECT_CALL(*mockRequestCaller, getLocationMock(_,_))
            .WillRepeatedly(
                DoAll(
                    Invoke(mockRequestCaller.get(), &MockTestRequestCaller::invokeLocationOnSuccessFct),
                    ReleaseSemaphore(&semaphore)));

    dispatcher->addRequestCaller(providerParticipantId, mockRequestCaller);
    auto subscriptionQos = std::make_shared<OnChangeWithKeepAliveSubscriptionQos>(
                1200, // validity_ms
                1000, // publication ttl
                10, // minInterval_ms
                100, // maxInterval_ms
                1100 // alertInterval_ms
    );
    std::string subscriptionId = "SubscriptionID";
    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscriptionId(subscriptionId);
    std::string attributeName = "Location";
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(subscriptionQos);

    MutableMessage mutableMessage = messageFactory.createSubscriptionRequest(
                proxyParticipantId,
                providerParticipantId,
                qos,
                subscriptionRequest,
                isLocalMessage);
    // first received message with subscription request
    dispatcher->receive(mutableMessage.getImmutableMessage());
    // wait for two requests from the subscription
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(1)));
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(1)));
    // remove the request caller
    dispatcher->removeRequestCaller(providerParticipantId);
    // assert that less than 2 requests happen in the next 300 milliseconds
    semaphore.waitFor(std::chrono::milliseconds(300));
    ASSERT_FALSE(semaphore.waitFor(std::chrono::milliseconds(300)));
}

/**
  * Precondition: A provider is registered and there is at least one subscription for it.
  * Trigger:    A subscription stop message is received
  * Expected:   The PublicationManager stops the publications for this provider
  */
TEST_F(SubscriptionTest, stopMessage_stopsPublications) {

    // Use a semaphore to count and wait on calls to the mockRequestCaller
    Semaphore semaphore(0);
    EXPECT_CALL(*mockRequestCaller, getLocationMock(_,_))
            .WillRepeatedly(
                DoAll(
                    Invoke(mockRequestCaller.get(), &MockTestRequestCaller::invokeLocationOnSuccessFct),
                    ReleaseSemaphore(&semaphore)));

    dispatcher->addRequestCaller(providerParticipantId, mockRequestCaller);
    std::string attributeName = "Location";
    auto subscriptionQos = std::make_shared<OnChangeWithKeepAliveSubscriptionQos>(
                1200, // validity_ms
                1000, // publication ttl
                10, // minInterval_ms
                500, // maxInterval_ms
                1100 // alertInterval_ms
    );
    std::string subscriptionId = "SubscriptionID";
    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscriptionId(subscriptionId);
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(subscriptionQos);

    MutableMessage mutableMessage = messageFactory.createSubscriptionRequest(
                proxyParticipantId,
                providerParticipantId,
                qos,
                subscriptionRequest,
                isLocalMessage);
    // first received message with subscription request
    dispatcher->receive(mutableMessage.getImmutableMessage());

    // wait for two requests from the subscription
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(1)));
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(1)));

    SubscriptionStop subscriptionStop;
    subscriptionStop.setSubscriptionId(subscriptionRequest.getSubscriptionId());
    // receive a subscription stop message
    mutableMessage = messageFactory.createSubscriptionStop(
                proxyParticipantId,
                providerParticipantId,
                qos,
                subscriptionStop);
    dispatcher->receive(mutableMessage.getImmutableMessage());

    ASSERT_FALSE(semaphore.waitFor(std::chrono::seconds(1)));
}
