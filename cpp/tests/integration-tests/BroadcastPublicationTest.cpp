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
        request(NULL),
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
        request = new BroadcastSubscriptionRequest();

        request->setSubscribeToName("locationUpdateSelective");
        request->setSubscriptionId(subscriptionId);

        auto subscriptionQos =
                QSharedPointer<OnChangeSubscriptionQos>(new OnChangeWithKeepAliveSubscriptionQos(
                    80, // validity_ms
                    100, // minInterval_ms
                    200, // maxInterval_ms
                    80 // alertInterval_ms
        ));
        request->setQos(subscriptionQos);
        request->setFilterParameters(filterParameters);

        requestCaller->registerBroadcastListener(
                    "locationUpdateSelective",
                    subscriptionBroadcastListener);

        publicationManager->add(
                    proxyParticipantId,
                    providerParticipantId,
                    requestCaller,
                    request,
                    publicationSender);

        publicationManager->addBroadcastFilter(filter1);
        publicationManager->addBroadcastFilter(filter2);
    }

    void TearDown(){
        delete publicationSender;

        // The filter objects' destructors aren't executed at the end of the test, because gmock seems
        // to still hold a reference internally. This leads to gmock reporting an error about leaked
        // mock objects when leaving the scope of the test.
        // --> Delete the pointers manually.

        delete filter1.data();
        filter1.clear();

        delete filter2.data();
        filter2.clear();
    }

protected:
    types::GpsLocation gpsLocation1;
    double speed1;

    QString providerParticipantId;
    QString proxyParticipantId;
    QString subscriptionId;
    PublicationManager* publicationManager;
    MockPublicationSender* publicationSender;
    BroadcastSubscriptionRequest* request;
    SubscriptionBroadcastListener* subscriptionBroadcastListener;

    QSharedPointer<MockTestProvider> provider;
    QSharedPointer<RequestCaller> requestCaller;
    TestLocationUpdateSelectiveBroadcastFilterParameters filterParameters;
    QSharedPointer<MockLocationUpdatedSelectiveFilter> filter1;
    QSharedPointer<MockLocationUpdatedSelectiveFilter> filter2;

private:
    DISALLOW_COPY_AND_ASSIGN(BroadcastPublicationTest);
};

/**
  * Trigger:    An event occurs.
  * Expected:   The registered filter objects are called correctly.
  */
TEST_F(BroadcastPublicationTest, call_BroadcastFilterOnEventTriggered) {

    // It's only guaranteed that all filters are executed when they return true
    // (When not returning true, filter chain execution is interrupted)
    ON_CALL(*filter1, filter(Eq(gpsLocation1), Eq(filterParameters))).WillByDefault(Return(true));
    ON_CALL(*filter2, filter(Eq(gpsLocation1), Eq(filterParameters))).WillByDefault(Return(true));

    EXPECT_CALL(*filter1, filter(Eq(gpsLocation1), Eq(filterParameters)));
    EXPECT_CALL(*filter2, filter(Eq(gpsLocation1), Eq(filterParameters)));

    provider->locationUpdateSelectiveEventOccured(gpsLocation1);
}
