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
#include <memory>
#include "tests/utils/MockObjects.h"
#include <QString>
#include "joynr/LibjoynrSettings.h"
#include "joynr/OnChangeWithKeepAliveSubscriptionQos.h"
#include "joynr/tests/TestLocationUpdateSelectiveBroadcastFilterParameters.h"

#include "joynr/types/GpsLocation.h"
#include "libjoynr/subscription/SubscriptionBroadcastListener.h"

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
        gpsLocation1(1.1, 2.2, 3.3, types::GpsFixEnum::MODE2D, 0.0, 0.0, 0.0, 0.0, 444, 444, 444),
        speed1(100),
        providerParticipantId("providerParticipantId"),
        proxyParticipantId("proxyParticipantId"),
        subscriptionId("subscriptionId"),
        publicationManager(NULL),
        publicationSender(NULL),
        request(),
        provider(new MockTestProvider),
        requestCaller(new testRequestCaller(provider)),
        filter1(new MockLocationUpdatedSelectiveFilter),
        filter2(new MockLocationUpdatedSelectiveFilter)
    {
    }

    void SetUp(){
        //remove stored subscriptions
        QFile::remove(LibjoynrSettings::DEFAULT_BROADCASTSUBSCRIPTIONREQUEST_STORAGE_FILENAME());
        publicationManager = new PublicationManager();
        subscriptionBroadcastListener =
                new SubscriptionBroadcastListener(subscriptionId, *publicationManager);
        publicationSender = new MockPublicationSender();

        request.setSubscribeToName("locationUpdateSelective");
        request.setSubscriptionId(subscriptionId);

        auto subscriptionQos =
                QSharedPointer<OnChangeSubscriptionQos>(new OnChangeWithKeepAliveSubscriptionQos(
                    80, // validity_ms
                    100, // minInterval_ms
                    200, // maxInterval_ms
                    80 // alertInterval_ms
        ));
        request.setQos(subscriptionQos);
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

        provider->addBroadcastFilter(filter1);
        provider->addBroadcastFilter(filter2);
    }

    void TearDown(){
        delete publicationManager;
        delete publicationSender;
        delete subscriptionBroadcastListener;
        EXPECT_TRUE(Mock::VerifyAndClearExpectations(filter1.data()));
        EXPECT_TRUE(Mock::VerifyAndClearExpectations(filter2.data()));
    }

protected:
    types::GpsLocation gpsLocation1;
    double speed1;

    QString providerParticipantId;
    QString proxyParticipantId;
    QString subscriptionId;
    PublicationManager* publicationManager;
    MockPublicationSender* publicationSender;
    BroadcastSubscriptionRequest request;
    SubscriptionBroadcastListener* subscriptionBroadcastListener;

    std::shared_ptr<MockTestProvider> provider;
    QSharedPointer<RequestCaller> requestCaller;
    TestLocationUpdateSelectiveBroadcastFilterParameters filterParameters;
    QSharedPointer<MockLocationUpdatedSelectiveFilter> filter1;
    QSharedPointer<MockLocationUpdatedSelectiveFilter> filter2;

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

    EXPECT_CALL(*publicationSender, sendSubscriptionPublication(
                    Eq(providerParticipantId),
                    Eq(proxyParticipantId),
                    _,
                    AllOf(
                        A<SubscriptionPublication>(),
                        Property(&SubscriptionPublication::getSubscriptionId, Eq(subscriptionId)))
                    ));

    provider->fireLocationUpdateSelective(gpsLocation1);
}

/**
  * Trigger:    A broadcast occurs. The filter chain has a negative result.
  * Expected:   A broadcast publication is triggered
  */
TEST_F(BroadcastPublicationTest, sendPublication_FilterChainFail) {

    ON_CALL(*filter1, filter(Eq(gpsLocation1), Eq(filterParameters))).WillByDefault(Return(true));
    ON_CALL(*filter2, filter(Eq(gpsLocation1), Eq(filterParameters))).WillByDefault(Return(false));

    EXPECT_CALL(*publicationSender, sendSubscriptionPublication(
                    Eq(providerParticipantId),
                    Eq(proxyParticipantId),
                    _,
                    AllOf(
                        A<SubscriptionPublication>(),
                        Property(&SubscriptionPublication::getSubscriptionId, Eq(subscriptionId)))
                    ))
            .Times(Exactly(0));

    provider->fireLocationUpdateSelective(gpsLocation1);
}

