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
#include <gtest/gtest.h>
#include <gmock/gmock.h>
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
class BroadcastPublicationTest : public ::testing::Test {
public:
    BroadcastPublicationTest() :
        singleThreadedIOService(std::make_shared<SingleThreadedIOService>()),
        gpsLocation1(1.1, 2.2, 3.3, types::Localisation::GpsFixEnum::MODE2D, 0.0, 0.0, 0.0, 0.0, 444, 444, 444),
        speed1(100),
        providerParticipantId("providerParticipantId"),
        proxyParticipantId("proxyParticipantId"),
        subscriptionId("subscriptionId"),
        mockMessageRouter(std::make_shared<MockMessageRouter>(singleThreadedIOService->getIOService())),
        messageSender(std::make_shared<MessageSender>(mockMessageRouter, nullptr)),
        publicationManager(std::make_shared<PublicationManager>(singleThreadedIOService->getIOService(), messageSender)),
        publicationSender(std::make_shared<MockPublicationSender>()),
        request(),
        subscriptionBroadcastListener(std::make_shared<UnicastBroadcastListener>(subscriptionId, publicationManager)),
        multicastBroadcastListener(std::make_shared<MulticastBroadcastListener>(providerParticipantId, publicationManager)),
        provider(std::make_shared<MockTestProvider>()),
        requestCaller(std::make_shared<testRequestCaller>(provider)),
        filter1(std::make_shared<MockLocationUpdatedSelectiveFilter>()),
        filter2(std::make_shared<MockLocationUpdatedSelectiveFilter>())
    {
        singleThreadedIOService->start();
    }

    ~BroadcastPublicationTest()
    {
        publicationManager->shutdown();
        singleThreadedIOService->stop();
    }

    void SetUp(){
        //remove stored subscriptions
        std::remove(LibjoynrSettings::DEFAULT_BROADCASTSUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME().c_str());

        request.setSubscribeToName("locationUpdateSelective");
        request.setSubscriptionId(subscriptionId);

        auto qos = std::make_shared<OnChangeSubscriptionQos>(
                    1000, // publication ttl
                    80, // validity_ms
                    100 // minInterval_ms
        );
        request.setQos(qos);
        request.setFilterParameters(filterParameters);

        requestCaller->registerBroadcastListener(
                    "locationUpdateSelective",
                    subscriptionBroadcastListener);

        publicationManager->add(
                    proxyParticipantId,
                    providerParticipantId,
                    requestCaller,
                    request,
                    publicationSender);

        provider->registerBroadcastListener(multicastBroadcastListener);
        provider->addBroadcastFilter(filter1);
        provider->addBroadcastFilter(filter2);
    }

    void TearDown(){
        EXPECT_TRUE(Mock::VerifyAndClearExpectations(filter1.get()));
        EXPECT_TRUE(Mock::VerifyAndClearExpectations(filter2.get()));
        messageSender.reset();
    }

protected:
    std::shared_ptr<SingleThreadedIOService> singleThreadedIOService;
    types::Localisation::GpsLocation gpsLocation1;
    double speed1;

    std::string providerParticipantId;
    std::string proxyParticipantId;
    std::string subscriptionId;
    std::shared_ptr<MockMessageRouter> mockMessageRouter;
    std::shared_ptr<IMessageSender> messageSender;
    std::shared_ptr<PublicationManager> publicationManager;
    std::shared_ptr<MockPublicationSender> publicationSender;
    BroadcastSubscriptionRequest request;
    std::shared_ptr<UnicastBroadcastListener> subscriptionBroadcastListener;
    std::shared_ptr<MulticastBroadcastListener> multicastBroadcastListener;

    std::shared_ptr<MockTestProvider> provider;
    std::shared_ptr<RequestCaller> requestCaller;
    TestLocationUpdateSelectiveBroadcastFilterParameters filterParameters;
    std::shared_ptr<MockLocationUpdatedSelectiveFilter> filter1;
    std::shared_ptr<MockLocationUpdatedSelectiveFilter> filter2;

private:
    DISALLOW_COPY_AND_ASSIGN(BroadcastPublicationTest);
};

/**
  * Trigger:    A broadcast occurs.
  * Expected:   The registered filter objects are called correctly.
  */
TEST_F(BroadcastPublicationTest, call_BroadcastFilterOnBroadcastTriggered) {

    // It's only guaranteed that all filters are executed when they return true
    // (When not returning true, filter chain execution is interrupted)
    ON_CALL(*filter1, filter(Eq(gpsLocation1), Eq(filterParameters))).WillByDefault(Return(true));
    ON_CALL(*filter2, filter(Eq(gpsLocation1), Eq(filterParameters))).WillByDefault(Return(true));

    EXPECT_CALL(*filter1, filter(Eq(gpsLocation1), Eq(filterParameters)));
    EXPECT_CALL(*filter2, filter(Eq(gpsLocation1), Eq(filterParameters)));

    provider->fireLocationUpdateSelective(gpsLocation1);
}

/**
  * Trigger:    A broadcast occurs. The filter chain has a positive result.
  * Expected:   A broadcast publication is triggered
  */
TEST_F(BroadcastPublicationTest, sendPublication_FilterChainSuccess) {

    ON_CALL(*filter1, filter(Eq(gpsLocation1), Eq(filterParameters))).WillByDefault(Return(true));
    ON_CALL(*filter2, filter(Eq(gpsLocation1), Eq(filterParameters))).WillByDefault(Return(true));

    EXPECT_CALL(*publicationSender, sendSubscriptionPublicationMock(
                    Eq(providerParticipantId),
                    Eq(proxyParticipantId),
                    _,
                    AllOf(
                        A<const SubscriptionPublication&>(),
                        Property(&SubscriptionPublication::getSubscriptionId, Eq(subscriptionId)))
                    ));

    provider->fireLocationUpdateSelective(gpsLocation1);
}

TEST_F(BroadcastPublicationTest, sendPublication_broadcastwithSingleArrayParam) {

    const std::vector<std::string> singleParam = {"A", "B"};

    using ImmutableMessagePtr = std::shared_ptr<ImmutableMessage>;

    const std::string expectedRecipient = providerParticipantId+"/broadcastWithSingleArrayParameter";
    EXPECT_CALL(*mockMessageRouter, route(
                     AllOf(
                         A<ImmutableMessagePtr>(),
                         MessageHasSender(providerParticipantId),
                         MessageHasRecipient(expectedRecipient)
                        ),
                     _
                     ));

    provider->fireBroadcastWithSingleArrayParameter(singleParam);
}

/**
  * Trigger:    A broadcast occurs. The filter chain has a negative result.
  * Expected:   A broadcast publication is triggered
  */
TEST_F(BroadcastPublicationTest, sendPublication_FilterChainFail) {

    ON_CALL(*filter1, filter(Eq(gpsLocation1), Eq(filterParameters))).WillByDefault(Return(true));
    ON_CALL(*filter2, filter(Eq(gpsLocation1), Eq(filterParameters))).WillByDefault(Return(false));

    EXPECT_CALL(*publicationSender, sendSubscriptionPublicationMock(
                    Eq(providerParticipantId),
                    Eq(proxyParticipantId),
                    _,
                    AllOf(
                        A<const SubscriptionPublication&>(),
                        Property(&SubscriptionPublication::getSubscriptionId, Eq(subscriptionId)))
                    ))
            .Times(Exactly(0));

    provider->fireLocationUpdateSelective(gpsLocation1);
}
