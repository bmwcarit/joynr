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

#include "tests/utils/Gtest.h"
#include "tests/utils/Gmock.h"

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
class SubscriptionTest : public ::testing::Test
{
public:
    SubscriptionTest()
            : _singleThreadedIOService(std::make_shared<SingleThreadedIOService>()),
              _mockMessageRouter(new MockMessageRouter(_singleThreadedIOService->getIOService())),
              _mockRequestCaller(new MockTestRequestCaller()),
              _mockGpsLocationListener(
                      new MockSubscriptionListenerOneType<types::Localisation::GpsLocation>()),
              _mockTestEnumSubscriptionListener(
                      new MockSubscriptionListenerOneType<tests::testTypes::TestEnum::Enum>()),
              _gpsLocation1(1.1,
                           2.2,
                           3.3,
                           types::Localisation::GpsFixEnum::MODE2D,
                           0.0,
                           0.0,
                           0.0,
                           0.0,
                           444,
                           444,
                           444),
              _qos(2000),
              _providerParticipantId("providerParticipantId"),
              _proxyParticipantId("proxyParticipantId"),
              _requestReplyId("requestReplyId"),
              _messageFactory(),
              _messageSender(std::make_shared<MessageSender>(_mockMessageRouter, nullptr)),
              _dispatcher(std::make_shared<Dispatcher>(_messageSender,
                                                      _singleThreadedIOService->getIOService())),
              _subscriptionManager(),
              _provider(new MockTestProvider),
              _publicationManager(
                      std::make_shared<PublicationManager>(_singleThreadedIOService->getIOService(),
                                                           _messageSender)),
              _requestCaller(new joynr::tests::testRequestCaller(_provider)),
              _isLocalMessage(true)
    {
        _singleThreadedIOService->start();
    }

    void SetUp()
    {
        _subscriptionManager = std::make_shared<SubscriptionManager>(
                _singleThreadedIOService->getIOService(), _mockMessageRouter);
        _dispatcher->registerPublicationManager(_publicationManager);
        _dispatcher->registerSubscriptionManager(_subscriptionManager);
        InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>(
                tests::ItestBase::INTERFACE_NAME());
    }

    ~SubscriptionTest()
    {
        _publicationManager->shutdown();
        _dispatcher->shutdown();
        _singleThreadedIOService->stop();
    }

protected:
    void receive_publicationWithException(
            std::shared_ptr<exceptions::JoynrRuntimeException> expectedException);

    std::shared_ptr<SingleThreadedIOService> _singleThreadedIOService;
    std::shared_ptr<MockMessageRouter> _mockMessageRouter;

    std::shared_ptr<MockTestRequestCaller> _mockRequestCaller;
    std::shared_ptr<MockSubscriptionListenerOneType<types::Localisation::GpsLocation>>
            _mockGpsLocationListener;
    std::shared_ptr<MockSubscriptionListenerOneType<tests::testTypes::TestEnum::Enum>>
            _mockTestEnumSubscriptionListener;

    types::Localisation::GpsLocation _gpsLocation1;

    // create test data
    MessagingQos _qos;
    std::string _providerParticipantId;
    std::string _proxyParticipantId;
    std::string _requestReplyId;

    MutableMessageFactory _messageFactory;
    std::shared_ptr<MessageSender> _messageSender;
    std::shared_ptr<Dispatcher> _dispatcher;
    std::shared_ptr<SubscriptionManager> _subscriptionManager;
    std::shared_ptr<MockTestProvider> _provider;
    std::shared_ptr<PublicationManager> _publicationManager;
    std::shared_ptr<joynr::tests::testRequestCaller> _requestCaller;
    const bool _isLocalMessage;

private:
    DISALLOW_COPY_AND_ASSIGN(SubscriptionTest);
};

/**
  * Trigger:    The dispatcher receives a SubscriptionRequest.
  * Expected:   The PublicationManager creates a PublisherRunnable and polls
  *             the MockCaller for the attribute.
  */
TEST_F(SubscriptionTest, receive_subscriptionRequestAndPollAttribute)
{

    // Use a semaphore to count and wait on calls to the mockRequestCaller
    auto semaphore = std::make_shared<Semaphore>(0);
    EXPECT_CALL(*_mockRequestCaller, getLocationMock(_, _)).WillRepeatedly(DoAll(
            Invoke(_mockRequestCaller.get(), &MockTestRequestCaller::invokeLocationOnSuccessFct),
            ReleaseSemaphore(semaphore)));

    std::string attributeName = "Location";
    auto subscriptionQos =
            std::make_shared<OnChangeWithKeepAliveSubscriptionQos>(500,  // validity_ms
                                                                   1000, // publication ttl
                                                                   1000, // minInterval_ms
                                                                   2000, // maxInterval_ms
                                                                   1000  // alertInterval_ms
                                                                   );
    std::string subscriptionId = "SubscriptionID";
    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscriptionId(subscriptionId);
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(subscriptionQos);

    MutableMessage mutableMessage = _messageFactory.createSubscriptionRequest(
            _proxyParticipantId, _providerParticipantId, _qos, subscriptionRequest, _isLocalMessage);

    _dispatcher->addRequestCaller(_providerParticipantId, _mockRequestCaller);
    _dispatcher->receive(mutableMessage.getImmutableMessage());

    // Wait for a call to be made to the mockRequestCaller
    ASSERT_TRUE(semaphore->waitFor(std::chrono::seconds(1)));
}

/**
  * Trigger:    The dispatcher receives a Publication.
  * Expected:   The SubscriptionManager retrieves the correct SubscriptionCallback and the
  *             Interpreter executes it correctly
  */
TEST_F(SubscriptionTest, receive_publication)
{
    // Use a semaphore to count and wait on calls to the mockGpsLocationListener
    auto publicationSemaphore = std::make_shared<Semaphore>(0);
    EXPECT_CALL(*_mockGpsLocationListener, onReceive(A<const types::Localisation::GpsLocation&>()))
            .WillRepeatedly(ReleaseSemaphore(publicationSemaphore));

    // register the subscription on the consumer side
    std::string attributeName = "Location";
    auto subscriptionQos =
            std::make_shared<OnChangeWithKeepAliveSubscriptionQos>(500,  // validity_ms
                                                                   1000, // publication ttl
                                                                   1000, // minInterval_ms
                                                                   2000, // maxInterval_ms
                                                                   1000  // alertInterval_ms
                                                                   );

    SubscriptionRequest subscriptionRequest;
    // construct a reply containing a GpsLocation
    SubscriptionPublication subscriptionPublication;
    subscriptionPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId());
    subscriptionPublication.setResponse(_gpsLocation1);

    auto future = std::make_shared<Future<std::string>>();
    auto subscriptionCallback =
            std::make_shared<UnicastSubscriptionCallback<types::Localisation::GpsLocation>>(
                    subscriptionRequest.getSubscriptionId(), future, _subscriptionManager, nullptr);

    // subscriptionRequest is an out param
    _subscriptionManager->registerSubscription(attributeName,
                                              subscriptionCallback,
                                              _mockGpsLocationListener,
                                              subscriptionQos,
                                              subscriptionRequest);
    // incoming publication from the provider
    MutableMessage mutableMessage = _messageFactory.createSubscriptionPublication(
            _providerParticipantId, _proxyParticipantId, _qos, subscriptionPublication);

    _dispatcher->receive(mutableMessage.getImmutableMessage());

    // Assert that only one subscription message is received by the subscription listener
    ASSERT_TRUE(publicationSemaphore->waitFor(std::chrono::seconds(1)));
    ASSERT_FALSE(publicationSemaphore->waitFor(std::chrono::seconds(1)));
}

void SubscriptionTest::receive_publicationWithException(
        std::shared_ptr<exceptions::JoynrRuntimeException> expectedException)
{
    auto semaphore = std::make_shared<Semaphore>(0);
    auto mockIntListener = std::make_shared<MockSubscriptionListenerOneType<std::int32_t>>();
    EXPECT_CALL(*mockIntListener, onReceive(A<const std::int32_t&>())).Times(0);
    EXPECT_CALL(*mockIntListener,
                onError(joynrException(
                        expectedException->getTypeName(), expectedException->getMessage())))
            .Times(1)
            .WillOnce(ReleaseSemaphore(semaphore));

    // register the subscription on the consumer side
    const std::string attributeName = "attributeWithProviderRuntimeException";

    auto subscriptionQos =
            std::make_shared<OnChangeWithKeepAliveSubscriptionQos>(500,  // validity_ms
                                                                   1000, // publication ttl
                                                                   1000, // minInterval_ms
                                                                   2000, // maxInterval_ms
                                                                   1000  // alertInterval_ms
                                                                   );

    SubscriptionRequest subscriptionRequest;

    auto future = std::make_shared<Future<std::string>>();
    auto subscriptionCallback = std::make_shared<UnicastSubscriptionCallback<std::int32_t>>(
            subscriptionRequest.getSubscriptionId(), future, _subscriptionManager, nullptr);

    _subscriptionManager->registerSubscription(attributeName,
                                              subscriptionCallback,
                                              mockIntListener,
                                              subscriptionQos,
                                              subscriptionRequest);

    // construct a subscriptionPublication containing a ProviderRuntimeException
    SubscriptionPublication subscriptionPublication;
    subscriptionPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId());
    subscriptionPublication.setError(expectedException);

    // incoming publication from the provider
    MutableMessage msg = _messageFactory.createSubscriptionPublication(
            _providerParticipantId, _proxyParticipantId, _qos, subscriptionPublication);

    _dispatcher->receive(msg.getImmutableMessage());
    ASSERT_TRUE(semaphore->waitFor(std::chrono::seconds(1)));
}

TEST_F(SubscriptionTest, receive_publicationWithProviderRuntimeException)
{
    auto expectedException = std::make_shared<exceptions::ProviderRuntimeException>(
            "TESTreceive_publicationWithProviderRuntimeExceptionERROR");
    receive_publicationWithException(expectedException);
}

TEST_F(SubscriptionTest, receive_publicationWithMethodInvocationException)
{
    auto expectedException = std::make_shared<exceptions::MethodInvocationException>(
            "TESTreceive_publicationWithMethodInvocationExceptionERROR");

    receive_publicationWithException(expectedException);
}

/**
  * Trigger:    The dispatcher receives an enum Publication.
  * Expected:   The SubscriptionManager retrieves the correct SubscriptionCallback and the
  *             Interpreter executes it correctly
  */
TEST_F(SubscriptionTest, receive_enumPublication)
{
    // Use a semaphore to count and wait on calls to the mockTestEnumSubscriptionListener
    auto semaphore = std::make_shared<Semaphore>(0);
    EXPECT_CALL(*_mockTestEnumSubscriptionListener,
                onReceive(A<const joynr::tests::testTypes::TestEnum::Enum&>()))
            .WillRepeatedly(ReleaseSemaphore(semaphore));

    // register the subscription on the consumer side
    std::string attributeName = "testEnum";
    auto subscriptionQos =
            std::make_shared<OnChangeWithKeepAliveSubscriptionQos>(500,  // validity_ms
                                                                   1000, // publication ttl
                                                                   1000, // minInterval_ms
                                                                   2000, // maxInterval_ms
                                                                   1000  // alertInterval_ms
                                                                   );

    SubscriptionRequest subscriptionRequest;
    // construct a reply containing a GpsLocation
    SubscriptionPublication subscriptionPublication;
    subscriptionPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId());
    subscriptionPublication.setResponse(tests::testTypes::TestEnum::ZERO);
    auto future = std::make_shared<Future<std::string>>();
    auto subscriptionCallback =
            std::make_shared<UnicastSubscriptionCallback<joynr::tests::testTypes::TestEnum::Enum>>(
                    subscriptionRequest.getSubscriptionId(), future, _subscriptionManager, nullptr);

    // subscriptionRequest is an out param
    _subscriptionManager->registerSubscription(attributeName,
                                              subscriptionCallback,
                                              _mockTestEnumSubscriptionListener,
                                              subscriptionQos,
                                              subscriptionRequest);
    // incoming publication from the provider
    MutableMessage mutableMessage = _messageFactory.createSubscriptionPublication(
            _providerParticipantId, _proxyParticipantId, _qos, subscriptionPublication);

    _dispatcher->receive(mutableMessage.getImmutableMessage());

    // Assert that only one subscription message is received by the subscription listener
    ASSERT_TRUE(semaphore->waitFor(std::chrono::seconds(1)));
    ASSERT_FALSE(semaphore->waitFor(std::chrono::seconds(1)));
}

/**
  * Precondition: Dispatcher receives a SubscriptionRequest for a not(yet) existing Provider.
  * Trigger:    The provider is registered.
  * Expected:   The PublicationManager registers the provider and notifies the PublicationManager
  *             to restore waiting Subscriptions
  */
TEST_F(SubscriptionTest, receive_RestoresSubscription)
{

    // Use a semaphore to count and wait on calls to the mockRequestCaller
    auto semaphore = std::make_shared<Semaphore>(0);
    EXPECT_CALL(*_mockRequestCaller,
                getLocationMock(A<std::function<void(const types::Localisation::GpsLocation&)>>(),
                                A<std::function<void(const std::shared_ptr<
                                        joynr::exceptions::ProviderRuntimeException>&)>>()))
            .WillOnce(DoAll(Invoke(_mockRequestCaller.get(),
                                   &MockTestRequestCaller::invokeLocationOnSuccessFct),
                            ReleaseSemaphore(semaphore)));
    std::string attributeName = "Location";
    auto subscriptionQos =
            std::make_shared<OnChangeWithKeepAliveSubscriptionQos>(500,  // validity_ms
                                                                   1000, // publication ttl
                                                                   1000, // minInterval_ms
                                                                   2000, // maxInterval_ms
                                                                   1000  // alertInterval_ms
                                                                   );
    std::string subscriptionId = "SubscriptionID";

    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscriptionId(subscriptionId);
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(subscriptionQos);

    MutableMessage mutableMessage = _messageFactory.createSubscriptionRequest(
            _proxyParticipantId, _providerParticipantId, _qos, subscriptionRequest, _isLocalMessage);
    // first received message with subscription request

    _dispatcher->receive(mutableMessage.getImmutableMessage());
    _dispatcher->addRequestCaller(_providerParticipantId, _mockRequestCaller);
    ASSERT_TRUE(semaphore->waitFor(std::chrono::seconds(15)));
    // Try to acquire a semaphore for up to 15 seconds. Acquireing the semaphore will only work, if
    // the mockRequestCaller has been called
    // and will be much faster than waiting for 1s to make sure it has been called
}

TEST_F(SubscriptionTest, sendPublication_attributeWithSingleArrayParam)
{

    using ImmutableMessagePtr = std::shared_ptr<ImmutableMessage>;

    std::string subscriptionId = "SubscriptionID";
    auto subscriptionQos = std::make_shared<OnChangeSubscriptionQos>(800,  // validity_ms
                                                                     1000, // publication ttl
                                                                     0     // minInterval_ms
                                                                     );

    // Use a semaphore to count and wait on calls to the mockRequestCaller
    auto semaphore = std::make_shared<Semaphore>(0);

    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscriptionId(subscriptionId);
    subscriptionRequest.setSubscribeToName("listOfStrings");
    subscriptionRequest.setQos(subscriptionQos);

    EXPECT_CALL(
            *_provider,
            getListOfStrings(
                    A<std::function<void(const std::vector<std::string>&)>>(),
                    A<std::function<void(const joynr::exceptions::ProviderRuntimeException&)>>()))
            .WillOnce(DoAll(Invoke(_provider.get(), &MockTestProvider::invokeListOfStringsOnSuccess),
                            ReleaseSemaphore(semaphore)));

    /* ensure the serialization succeeds and the first publication and the subscriptionReply are
     * sent to the proxy */
    EXPECT_CALL(*_mockMessageRouter,
                route(AllOf(A<ImmutableMessagePtr>(),
                            MessageHasSender(_providerParticipantId),
                            MessageHasRecipient(_proxyParticipantId)),
                      _)).Times(2);

    _publicationManager->add(_proxyParticipantId,
                            _providerParticipantId,
                            _requestCaller,
                            subscriptionRequest,
                            _messageSender);

    ASSERT_TRUE(semaphore->waitFor(std::chrono::seconds(15)));

    std::vector<std::string> listOfStrings;
    listOfStrings.push_back("1");
    listOfStrings.push_back("2");

    /* ensure the value change leads to another publication */
    Mock::VerifyAndClear(_mockMessageRouter.get());
    EXPECT_CALL(*_mockMessageRouter,
                route(AllOf(A<ImmutableMessagePtr>(),
                            MessageHasSender(_providerParticipantId),
                            MessageHasRecipient(_proxyParticipantId)),
                      _));

    _provider->listOfStringsChanged(listOfStrings);
}

TEST_F(SubscriptionTest, sendPublication_attributeWithProviderRuntimeException)
{
    using ImmutableMessagePtr = std::shared_ptr<ImmutableMessage>;

    auto semaphore = std::make_shared<Semaphore>(0);
    const std::string subscriptionId = "SubscriptionID";
    auto subscriptionQos =
            std::make_shared<OnChangeWithKeepAliveSubscriptionQos>(600,  // validity_ms
                                                                   1000, // publication ttl
                                                                   100,  // minInterval_ms
                                                                   400,  // maxInterval_ms
                                                                   1000  // alertInterval_ms
                                                                   );

    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscriptionId(subscriptionId);
    subscriptionRequest.setSubscribeToName("attributeWithProviderRuntimeException");
    subscriptionRequest.setQos(subscriptionQos);

    auto expectedException = std::make_shared<exceptions::ProviderRuntimeException>(
            _provider->_providerRuntimeExceptionTestMsg);
    SubscriptionPublication expectedPublication;
    expectedPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId());
    expectedPublication.setError(expectedException);

    EXPECT_CALL(*_mockMessageRouter,
                route(AllOf(A<ImmutableMessagePtr>(),
                            MessageHasType(joynr::Message::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY()),
                            MessageHasSender(_providerParticipantId),
                            MessageHasRecipient(_proxyParticipantId)),
                      _));

    EXPECT_CALL(*_mockMessageRouter,
                route(AllOf(A<ImmutableMessagePtr>(),
                            MessageHasType(joynr::Message::VALUE_MESSAGE_TYPE_PUBLICATION()),
                            MessageHasSender(_providerParticipantId),
                            MessageHasRecipient(_proxyParticipantId),
                            ImmutableMessageHasPayload(
                                    joynr::serializer::serializeToJson(expectedPublication))),
                      _))
            .Times(2)
            .WillRepeatedly(ReleaseSemaphore(semaphore));

    _publicationManager->add(_proxyParticipantId,
                            _providerParticipantId,
                            _requestCaller,
                            subscriptionRequest,
                            _messageSender);

    // wait for the 2 async publications
    ASSERT_TRUE(semaphore->waitFor(std::chrono::seconds(10)));
    ASSERT_TRUE(semaphore->waitFor(std::chrono::seconds(10)));
}

/**
  * Precondition: A provider is registered and there is at least one subscription for it.
  * Trigger:    The request caller is removed from the dispatcher
  * Expected:   The PublicationManager stops all subscriptions for this provider
  */
TEST_F(SubscriptionTest, removeRequestCaller_stopsPublications)
{

    // Use a semaphore to count and wait on calls to the mockRequestCaller
    auto semaphore = std::make_shared<Semaphore>(0);
    EXPECT_CALL(*_mockRequestCaller, getLocationMock(_, _)).WillRepeatedly(DoAll(
            Invoke(_mockRequestCaller.get(), &MockTestRequestCaller::invokeLocationOnSuccessFct),
            ReleaseSemaphore(semaphore)));

    _dispatcher->addRequestCaller(_providerParticipantId, _mockRequestCaller);
    auto subscriptionQos =
            std::make_shared<OnChangeWithKeepAliveSubscriptionQos>(1200, // validity_ms
                                                                   1000, // publication ttl
                                                                   10,   // minInterval_ms
                                                                   100,  // maxInterval_ms
                                                                   1100  // alertInterval_ms
                                                                   );
    std::string subscriptionId = "SubscriptionID";
    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscriptionId(subscriptionId);
    std::string attributeName = "Location";
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(subscriptionQos);

    MutableMessage mutableMessage = _messageFactory.createSubscriptionRequest(
            _proxyParticipantId, _providerParticipantId, _qos, subscriptionRequest, _isLocalMessage);
    // first received message with subscription request
    _dispatcher->receive(mutableMessage.getImmutableMessage());
    // wait for two requests from the subscription
    ASSERT_TRUE(semaphore->waitFor(std::chrono::seconds(1)));
    ASSERT_TRUE(semaphore->waitFor(std::chrono::seconds(1)));
    // remove the request caller
    _dispatcher->removeRequestCaller(_providerParticipantId);
    // assert that less than 2 requests happen in the next 300 milliseconds
    semaphore->waitFor(std::chrono::milliseconds(300));
    ASSERT_FALSE(semaphore->waitFor(std::chrono::milliseconds(300)));
}

/**
  * Precondition: A provider is registered and there is at least one subscription for it.
  * Trigger:    A subscription stop message is received
  * Expected:   The PublicationManager stops the publications for this provider
  */
TEST_F(SubscriptionTest, stopMessage_stopsPublications)
{

    // Use a semaphore to count and wait on calls to the mockRequestCaller
    auto semaphore = std::make_shared<Semaphore>(0);
    EXPECT_CALL(*_mockRequestCaller, getLocationMock(_, _)).WillRepeatedly(DoAll(
            Invoke(_mockRequestCaller.get(), &MockTestRequestCaller::invokeLocationOnSuccessFct),
            ReleaseSemaphore(semaphore)));

    _dispatcher->addRequestCaller(_providerParticipantId, _mockRequestCaller);
    std::string attributeName = "Location";
    auto subscriptionQos =
            std::make_shared<OnChangeWithKeepAliveSubscriptionQos>(1200, // validity_ms
                                                                   1000, // publication ttl
                                                                   10,   // minInterval_ms
                                                                   500,  // maxInterval_ms
                                                                   1100  // alertInterval_ms
                                                                   );
    std::string subscriptionId = "SubscriptionID";
    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscriptionId(subscriptionId);
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(subscriptionQos);

    MutableMessage mutableMessage = _messageFactory.createSubscriptionRequest(
            _proxyParticipantId, _providerParticipantId, _qos, subscriptionRequest, _isLocalMessage);
    // first received message with subscription request
    _dispatcher->receive(mutableMessage.getImmutableMessage());

    // wait for two requests from the subscription
    ASSERT_TRUE(semaphore->waitFor(std::chrono::seconds(1)));
    ASSERT_TRUE(semaphore->waitFor(std::chrono::seconds(1)));

    SubscriptionStop subscriptionStop;
    subscriptionStop.setSubscriptionId(subscriptionRequest.getSubscriptionId());
    // receive a subscription stop message
    mutableMessage = _messageFactory.createSubscriptionStop(
            _proxyParticipantId, _providerParticipantId, _qos, subscriptionStop);
    _dispatcher->receive(mutableMessage.getImmutableMessage());

    ASSERT_FALSE(semaphore->waitFor(std::chrono::seconds(1)));
}
