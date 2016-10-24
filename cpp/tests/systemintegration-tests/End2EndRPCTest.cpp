/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
#include <memory>
#include <string>
#include <cstdint>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "runtimes/cluster-controller-runtime/JoynrClusterControllerRuntime.h"
#include "tests/utils/MockObjects.h"
#include "joynr/tests/testProvider.h"
#include "joynr/tests/testProxy.h"
#include "joynr/vehicle/GpsProxy.h"
#include "joynr/types/ProviderQos.h"
#include "joynr/Future.h"
#include "joynr/OnChangeWithKeepAliveSubscriptionQos.h"
#include "joynr/LibjoynrSettings.h"
#include "joynr/PrivateCopyAssign.h"

using namespace ::testing;


using namespace joynr;

class End2EndRPCTest : public TestWithParam< std::string >{
public:
    std::string domain;
    JoynrClusterControllerRuntime* runtime;
    std::shared_ptr<vehicle::GpsProvider> gpsProvider;

    End2EndRPCTest() :
        domain(),
        runtime(nullptr)
    {
        runtime = new JoynrClusterControllerRuntime(
                    std::make_unique<Settings>(GetParam())
        );
        domain = "cppEnd2EndRPCTest_Domain_" + util::createUuid();
    }
    // Sets up the test fixture.
    void SetUp(){
       runtime->start();
    }

    // Tears down the test fixture.
    void TearDown(){
        bool deleteChannel = true;
        runtime->stop(deleteChannel);

        // Delete persisted files
        std::remove(LibjoynrSettings::DEFAULT_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCE_FILENAME().c_str());
        std::remove(LibjoynrSettings::DEFAULT_MESSAGE_ROUTER_PERSISTENCE_FILENAME().c_str());
        std::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME().c_str());
        std::remove(LibjoynrSettings::DEFAULT_PARTICIPANT_IDS_PERSISTENCE_FILENAME().c_str());

        std::this_thread::sleep_for(std::chrono::milliseconds(550));
    }

    ~End2EndRPCTest(){
        delete runtime;
    }
private:
    DISALLOW_COPY_AND_ASSIGN(End2EndRPCTest);
};

// leadsm to assert failure in GpsInProcessConnector line 185: not yet implemented in connector
TEST_P(End2EndRPCTest, call_rpc_method_and_get_expected_result)
{

    auto mockProvider = std::make_shared<MockGpsProvider>();

    runtime->registerProvider<vehicle::GpsProvider>(domain, mockProvider);
    std::this_thread::sleep_for(std::chrono::milliseconds(550));

    ProxyBuilder<vehicle::GpsProxy>* gpsProxyBuilder = runtime->createProxyBuilder<vehicle::GpsProxy>(domain);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeoutMs(1000);

    std::int64_t qosRoundTripTTL = 40000;
    std::unique_ptr<vehicle::GpsProxy> gpsProxy = gpsProxyBuilder
            ->setMessagingQos(MessagingQos(qosRoundTripTTL))
            ->setCached(false)
            ->setDiscoveryQos(discoveryQos)
            ->build();
    std::shared_ptr<Future<int> >gpsFuture (gpsProxy->calculateAvailableSatellitesAsync());
    gpsFuture->wait();
    int expectedValue = 42; //as defined in MockGpsProvider
    int actualValue;
    gpsFuture->get(actualValue);
    EXPECT_EQ(expectedValue, actualValue);
    //TODO CA: shared pointer for proxy builder?
    delete gpsProxyBuilder;
    // This is not yet implemented in CapabilitiesClient
    // runtime->unregisterProvider("Fake_ParticipantId_vehicle/gpsDummyProvider");
}

TEST_P(End2EndRPCTest, call_void_operation)
{
    auto mockProvider = std::make_shared<MockTestProvider>();

    runtime->registerProvider<tests::testProvider>(domain, mockProvider);
    std::this_thread::sleep_for(std::chrono::milliseconds(550));

    ProxyBuilder<tests::testProxy>* testProxyBuilder = runtime->createProxyBuilder<tests::testProxy>(domain);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeoutMs(1000);

    std::int64_t qosRoundTripTTL = 40000;
    std::unique_ptr<tests::testProxy> testProxy = testProxyBuilder
            ->setMessagingQos(MessagingQos(qosRoundTripTTL))
            ->setCached(false)
            ->setDiscoveryQos(discoveryQos)
            ->build();
    testProxy->voidOperation();
//    EXPECT_EQ(expectedValue, gpsFuture->getValue());
    //TODO CA: shared pointer for proxy builder?
    delete testProxyBuilder;
    // This is not yet implemented in CapabilitiesClient
    // runtime->unregisterProvider("Fake_ParticipantId_vehicle/gpsDummyProvider");
}

// tests in process subscription
TEST_P(End2EndRPCTest, _call_subscribeTo_and_get_expected_result)
{
    auto mockProvider = std::make_shared<MockTestProvider>();
    runtime->registerProvider<tests::testProvider>(domain, mockProvider);

    std::this_thread::sleep_for(std::chrono::milliseconds(550));

    ProxyBuilder<tests::testProxy>* testProxyBuilder =
            runtime->createProxyBuilder<tests::testProxy>(domain);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeoutMs(1000);

    std::int64_t qosRoundTripTTL = 40000;
    std::unique_ptr<tests::testProxy> testProxy = testProxyBuilder
            ->setMessagingQos(MessagingQos(qosRoundTripTTL))
            ->setCached(false)
            ->setDiscoveryQos(discoveryQos)
            ->build();

    MockGpsSubscriptionListener* mockListener = new MockGpsSubscriptionListener();
    std::shared_ptr<ISubscriptionListener<types::Localisation::GpsLocation> > subscriptionListener(
                mockListener);

    EXPECT_CALL(*mockListener, onReceive(A<const types::Localisation::GpsLocation&>()))
            .Times(AtLeast(2));

    auto subscriptionQos = std::make_shared<OnChangeWithKeepAliveSubscriptionQos>(
                800, // validity_ms
                100, // minInterval_ms
                200, // maxInterval_ms
                1000 // alertInterval_ms
    );
    testProxy->subscribeToLocation(subscriptionListener, subscriptionQos);
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    //TODO CA: shared pointer for proxy builder?
    delete testProxyBuilder;
    // This is not yet implemented in CapabilitiesClient
    // runtime->unregisterProvider("Fake_ParticipantId_vehicle/gpsDummyProvider");
}

INSTANTIATE_TEST_CASE_P(Http,
        End2EndRPCTest,
        testing::Values(
            "test-resources/HttpSystemIntegrationTest1.settings"
        )
);

INSTANTIATE_TEST_CASE_P(MqttWithHttpBackend,
        End2EndRPCTest,
        testing::Values(
            "test-resources/MqttWithHttpBackendSystemIntegrationTest1.settings"
        )
);
