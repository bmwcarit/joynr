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
#include "joynr/PrivateCopyAssign.h"
#include "tests/utils/Gtest.h"
#include "tests/utils/Gmock.h"
#include <memory>
#include "joynr/LibjoynrSettings.h"
#include "joynr/MessageSender.h"
#include "joynr/OnChangeWithKeepAliveSubscriptionQos.h"
#include "joynr/tests/TestLocationUpdateSelectiveBroadcastFilterParameters.h"
#include "joynr/SingleThreadedIOService.h"

#include "joynr/UnicastBroadcastListener.h"
#include "joynr/MulticastBroadcastListener.h"

#include "tests/JoynrTest.h"
#include "tests/mock/MockMessageRouter.h"
#include "tests/mock/MockPublicationSender.h"
#include "tests/mock/MockLocationUpdatedSelectiveFilter.h"
#include "tests/mock/MockSubscriptionListener.h"
#include "tests/mock/MockTestProvider.h"

using namespace ::testing;
using ::testing::InSequence;

using namespace joynr;
using namespace joynr::tests;

/**
  * Is an integration test. Tests from Provider -> PublicationManager
  */
class BroadcastPublicationTest : public ::testing::Test
{
public:
    BroadcastPublicationTest()
            : _singleThreadedIOService(std::make_shared<SingleThreadedIOService>()),
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
              _speed1(100),
              _providerParticipantId("providerParticipantId"),
              _proxyParticipantId("proxyParticipantId"),
              _subscriptionId("subscriptionId"),
              _mockMessageRouter(
                      std::make_shared<MockMessageRouter>(_singleThreadedIOService->getIOService())),
              _messageSender(std::make_shared<MessageSender>(_mockMessageRouter, nullptr)),
              _publicationManager(
                      std::make_shared<PublicationManager>(_singleThreadedIOService->getIOService(),
                                                           _messageSender)),
              _publicationSender(std::make_shared<MockPublicationSender>()),
              _request(),
              _subscriptionBroadcastListener(
                      std::make_shared<UnicastBroadcastListener>(_subscriptionId,
                                                                 _publicationManager)),
              _multicastBroadcastListener(
                      std::make_shared<MulticastBroadcastListener>(_providerParticipantId,
                                                                   _publicationManager)),
              _provider(std::make_shared<MockTestProvider>()),
              _requestCaller(std::make_shared<testRequestCaller>(_provider)),
              _filterParameters(),
              _filter1(std::make_shared<MockLocationUpdatedSelectiveFilter>()),
              _filter2(std::make_shared<MockLocationUpdatedSelectiveFilter>())
    {
        _singleThreadedIOService->start();
    }

    ~BroadcastPublicationTest()
    {
        _publicationManager->shutdown();
        _singleThreadedIOService->stop();
    }

    void SetUp()
    {
        _request.setSubscribeToName("locationUpdateSelective");
        _request.setSubscriptionId(_subscriptionId);

        auto qos = std::make_shared<OnChangeSubscriptionQos>(1000, // publication ttl
                                                             80,   // validity_ms
                                                             100   // minInterval_ms
                                                             );
        _request.setQos(qos);
        _request.setFilterParameters(_filterParameters);

        _requestCaller->registerBroadcastListener(
                "locationUpdateSelective", _subscriptionBroadcastListener);

        _publicationManager->add(_proxyParticipantId,
                                _providerParticipantId,
                                _requestCaller,
                                _request,
                                _publicationSender);

        _provider->registerBroadcastListener(_multicastBroadcastListener);
        _provider->addBroadcastFilter(_filter1);
        _provider->addBroadcastFilter(_filter2);
    }

    void TearDown()
    {
        EXPECT_TRUE(Mock::VerifyAndClearExpectations(_filter1.get()));
        EXPECT_TRUE(Mock::VerifyAndClearExpectations(_filter2.get()));
        _messageSender.reset();
    }

protected:
    std::shared_ptr<SingleThreadedIOService> _singleThreadedIOService;
    types::Localisation::GpsLocation _gpsLocation1;
    double _speed1;

    std::string _providerParticipantId;
    std::string _proxyParticipantId;
    std::string _subscriptionId;
    std::shared_ptr<MockMessageRouter> _mockMessageRouter;
    std::shared_ptr<IMessageSender> _messageSender;
    std::shared_ptr<PublicationManager> _publicationManager;
    std::shared_ptr<MockPublicationSender> _publicationSender;
    BroadcastSubscriptionRequest _request;
    std::shared_ptr<UnicastBroadcastListener> _subscriptionBroadcastListener;
    std::shared_ptr<MulticastBroadcastListener> _multicastBroadcastListener;

    std::shared_ptr<MockTestProvider> _provider;
    std::shared_ptr<RequestCaller> _requestCaller;
    TestLocationUpdateSelectiveBroadcastFilterParameters _filterParameters;
    std::shared_ptr<MockLocationUpdatedSelectiveFilter> _filter1;
    std::shared_ptr<MockLocationUpdatedSelectiveFilter> _filter2;

private:
    DISALLOW_COPY_AND_ASSIGN(BroadcastPublicationTest);
};

/**
  * Trigger:    A broadcast occurs.
  * Expected:   The registered filter objects are called correctly.
  */
TEST_F(BroadcastPublicationTest, call_BroadcastFilterOnBroadcastTriggered)
{

    // It's only guaranteed that all filters are executed when they return true
    // (When not returning true, filter chain execution is interrupted)
    ON_CALL(*_filter1, filter(Eq(_gpsLocation1), Eq(_filterParameters))).WillByDefault(Return(true));
    ON_CALL(*_filter2, filter(Eq(_gpsLocation1), Eq(_filterParameters))).WillByDefault(Return(true));

    EXPECT_CALL(*_filter1, filter(Eq(_gpsLocation1), Eq(_filterParameters)));
    EXPECT_CALL(*_filter2, filter(Eq(_gpsLocation1), Eq(_filterParameters)));

    _provider->fireLocationUpdateSelective(_gpsLocation1);
}

/**
  * Trigger:    A broadcast occurs. The filter chain has a positive result.
  * Expected:   A broadcast publication is triggered
  */
TEST_F(BroadcastPublicationTest, sendPublication_FilterChainSuccess)
{

    ON_CALL(*_filter1, filter(Eq(_gpsLocation1), Eq(_filterParameters))).WillByDefault(Return(true));
    ON_CALL(*_filter2, filter(Eq(_gpsLocation1), Eq(_filterParameters))).WillByDefault(Return(true));

    EXPECT_CALL(*_publicationSender,
                sendSubscriptionPublicationMock(
                        Eq(_providerParticipantId),
                        Eq(_proxyParticipantId),
                        _,
                        AllOf(A<const SubscriptionPublication&>(),
                              Property(&SubscriptionPublication::getSubscriptionId,
                                       Eq(_subscriptionId)))));

    _provider->fireLocationUpdateSelective(_gpsLocation1);
}

TEST_F(BroadcastPublicationTest, sendPublication_broadcastwithSingleArrayParam)
{

    const std::vector<std::string> singleParam = {"A", "B"};

    using ImmutableMessagePtr = std::shared_ptr<ImmutableMessage>;

    const std::string expectedRecipient =
            _providerParticipantId + "/broadcastWithSingleArrayParameter";
    EXPECT_CALL(*_mockMessageRouter,
                route(AllOf(A<ImmutableMessagePtr>(),
                            MessageHasSender(_providerParticipantId),
                            MessageHasRecipient(expectedRecipient)),
                      _));

    _provider->fireBroadcastWithSingleArrayParameter(singleParam);
}

/**
  * Trigger:    A broadcast occurs. The filter chain has a negative result.
  * Expected:   A broadcast publication is triggered
  */
TEST_F(BroadcastPublicationTest, sendPublication_FilterChainFail)
{

    ON_CALL(*_filter1, filter(Eq(_gpsLocation1), Eq(_filterParameters))).WillByDefault(Return(true));
    ON_CALL(*_filter2, filter(Eq(_gpsLocation1), Eq(_filterParameters))).WillByDefault(Return(false));

    EXPECT_CALL(*_publicationSender,
                sendSubscriptionPublicationMock(
                        Eq(_providerParticipantId),
                        Eq(_proxyParticipantId),
                        _,
                        AllOf(A<const SubscriptionPublication&>(),
                              Property(&SubscriptionPublication::getSubscriptionId,
                                       Eq(_subscriptionId))))).Times(Exactly(0));

    _provider->fireLocationUpdateSelective(_gpsLocation1);
}
