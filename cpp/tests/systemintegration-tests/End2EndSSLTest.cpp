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

#include <memory>
#include <string>
#include <chrono>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/JoynrClusterControllerRuntime.h"
#include "joynr/vehicle/GpsProxy.h"
#include "joynr/Future.h"
#include "joynr/Util.h"
#include "joynr/Settings.h"
#include "joynr/LibjoynrSettings.h"

#include "runtimes/libjoynr-runtime/websocket/LibJoynrWebSocketRuntime.h"

#include "tests/utils/MockObjects.h"
#include "tests/utils/TestLibJoynrWebSocketRuntime.h"

using namespace ::testing;
using namespace joynr;

/*********************************************************************************************************
*
* To run this test you must use {joynr}/docker/joynr-base/scripts/gen-certificates.sh with the config from
* {joynr}/docker/joynr-base/openssl.conf to generate test certificates in /data/ssl-data as done in
* {joynr}/docker/joynr-base/Dockerfile.
* 
* The certificates may not be stored in the git repository for security reasons (even if they are only test
* certificates!)
*
**********************************************************************************************************/
class End2EndSSLTest : public TestWithParam<std::tuple<std::string, std::string>> {
public:
    End2EndSSLTest() : domain()
    {
        auto ccSettings = std::make_unique<Settings>(std::get<0>(GetParam()));
        runtime = std::make_unique<JoynrClusterControllerRuntime>(std::move(ccSettings));

        auto libJoynrSettings = std::make_unique<Settings>(std::get<1>(GetParam()));
        libJoynrRuntime = std::make_unique<TestLibJoynrWebSocketRuntime>(std::move(libJoynrSettings));

        std::string uuid = util::createUuid();
        domain = "cppEnd2EndSSLTest_Domain_" + uuid;
    }

    // Sets up the test fixture.
    void SetUp(){
       runtime->start();
       EXPECT_TRUE(libJoynrRuntime->connect(std::chrono::milliseconds(2000)));
    }

    // Tears down the test fixture.
    void TearDown(){
        bool deleteChannel = true;
        runtime->stop(deleteChannel);

        // Delete persisted files
        std::remove(ClusterControllerSettings::DEFAULT_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCE_FILENAME().c_str());
        std::remove(LibjoynrSettings::DEFAULT_MESSAGE_ROUTER_PERSISTENCE_FILENAME().c_str());
        std::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME().c_str());
        std::remove(LibjoynrSettings::DEFAULT_PARTICIPANT_IDS_PERSISTENCE_FILENAME().c_str());

        std::this_thread::sleep_for(std::chrono::milliseconds(550));
    }

protected:
    std::string domain;
    std::shared_ptr<JoynrClusterControllerRuntime> runtime;
    std::shared_ptr<TestLibJoynrWebSocketRuntime> libJoynrRuntime;

private:
    DISALLOW_COPY_AND_ASSIGN(End2EndSSLTest);
};

TEST_P(End2EndSSLTest, localconnection_call_rpc_method_and_get_expected_result)
{
    // Create a provider
    auto mockProvider = std::make_shared<MockGpsProvider>();
    types::ProviderQos providerQos;
    std::chrono::milliseconds millisSinceEpoch =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch());
    providerQos.setPriority(millisSinceEpoch.count());
    providerQos.setScope(joynr::types::ProviderScope::LOCAL);
    providerQos.setSupportsOnChangeSubscriptions(true);
    runtime->registerProvider<vehicle::GpsProvider>(domain, mockProvider, providerQos);

    // Build a proxy
    std::unique_ptr<ProxyBuilder<vehicle::GpsProxy>> gpsProxyBuilder =
            libJoynrRuntime->createProxyBuilder<vehicle::GpsProxy>(domain);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeoutMs(3000);

    std::int64_t qosRoundTripTTL = 40000;
    std::unique_ptr<vehicle::GpsProxy> gpsProxy = gpsProxyBuilder
            ->setMessagingQos(MessagingQos(qosRoundTripTTL))
            ->setDiscoveryQos(discoveryQos)
            ->build();

    // Call the provider and wait for a result
    std::shared_ptr<Future<int> >gpsFuture (gpsProxy->calculateAvailableSatellitesAsync());
    gpsFuture->wait();

    int expectedValue = 42; //as defined in MockGpsProvider
    int actualValue;
    gpsFuture->get(actualValue);
    EXPECT_EQ(expectedValue, actualValue);
}

INSTANTIATE_TEST_CASE_P(TLS,
    End2EndSSLTest,
    testing::Values(std::make_tuple("test-resources/websocket-cc-tls.settings", "test-resources/websocket-libjoynr-tls.settings"))
);

INSTANTIATE_TEST_CASE_P(NonTLS,
    End2EndSSLTest,
    testing::Values(std::make_tuple("test-resources/websocket-cc-tls.settings", "test-resources/websocket-libjoynr-non-tls.settings"))
);
