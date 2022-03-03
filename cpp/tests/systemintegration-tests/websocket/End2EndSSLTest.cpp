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

#include <mococrw/x509.h>

#include "joynr/ClusterControllerSettings.h"
#include "joynr/Future.h"
#include "joynr/JoynrClusterControllerRuntime.h"
#include "joynr/LibjoynrSettings.h"
#include "joynr/MessagingSettings.h"
#include "joynr/Settings.h"
#include "joynr/Util.h"
#include "joynr/vehicle/GpsProxy.h"

#include "tests/JoynrTest.h"
#include "tests/utils/TestLibJoynrWebSocketRuntime.h"
#include "tests/mock/MockKeychain.h"
#include "tests/mock/MockGpsProvider.h"

using namespace ::testing;
using namespace joynr;

/*********************************************************************************************************
*
* To run this test you must use {joynr}/docker/joynr-base/scripts/docker/gen-certificates.sh with the
*config from
* {joynr}/docker/joynr-base/openssl.conf to generate test certificates in /data/ssl-data as done in
* {joynr}/docker/joynr-base/Dockerfile.
*
* The certificates may not be stored in the git repository for security reasons (even if they are
*only test
* certificates!)
*
**********************************************************************************************************/
class End2EndSSLTest : public TestWithParam<std::tuple<std::string, std::string, bool>>
{
public:
    End2EndSSLTest()
            : domain(), ownerId(), useTls(std::get<2>(GetParam())), ccRuntime(), libJoynrRuntime()
    {
        std::string uuid = util::createUuid();
        domain = "cppEnd2EndSSLTest_Domain_" + uuid;

        keyChain = useTls ? createMockKeychain() : nullptr;
    }

    ~End2EndSSLTest()
    {
        libJoynrRuntime->shutdown();
        libJoynrRuntime.reset();
        ccRuntime->stop();
        ccRuntime->shutdown();
        ccRuntime.reset();

        std::this_thread::sleep_for(std::chrono::milliseconds(550));

        // Delete persisted files
        std::remove(ClusterControllerSettings::
                            DEFAULT_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCE_FILENAME().c_str());
        std::remove(LibjoynrSettings::DEFAULT_PARTICIPANT_IDS_PERSISTENCE_FILENAME().c_str());
    }

protected:
    void startRuntimes()
    {
        auto ccSettings = std::make_unique<Settings>(std::get<0>(GetParam()));
        ccRuntime = std::make_shared<JoynrClusterControllerRuntime>(
                std::move(ccSettings), failOnFatalRuntimeError);
        ccRuntime->init();

        auto libJoynrSettings = std::make_unique<Settings>(std::get<1>(GetParam()));
        libJoynrRuntime = std::make_shared<TestLibJoynrWebSocketRuntime>(
                std::move(libJoynrSettings), keyChain);

        ccRuntime->start();
        ASSERT_TRUE(libJoynrRuntime->connect(std::chrono::milliseconds(2000)));
    }

    std::string createAndRegisterProvider()
    {
        auto mockProvider = std::make_shared<MockGpsProvider>();
        types::ProviderQos providerQos;
        std::chrono::milliseconds millisSinceEpoch =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch());
        providerQos.setPriority(millisSinceEpoch.count());
        providerQos.setScope(joynr::types::ProviderScope::LOCAL);
        providerQos.setSupportsOnChangeSubscriptions(true);
        std::string participantId = ccRuntime->registerProvider<vehicle::GpsProvider>(
                domain, mockProvider, providerQos);

        return participantId;
    }

    std::shared_ptr<vehicle::GpsProxy> buildProxy()
    {
        std::shared_ptr<ProxyBuilder<vehicle::GpsProxy>> gpsProxyBuilder =
                libJoynrRuntime->createProxyBuilder<vehicle::GpsProxy>(domain);
        DiscoveryQos discoveryQos;
        discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
        discoveryQos.setDiscoveryTimeoutMs(3000);

        std::uint64_t qosRoundTripTTL = 40000;
        std::shared_ptr<vehicle::GpsProxy> gpsProxy =
                gpsProxyBuilder->setMessagingQos(MessagingQos(qosRoundTripTTL))
                        ->setDiscoveryQos(discoveryQos)
                        ->build();

        return gpsProxy;
    }

    void testLocalconnectionCallRpcMethodWithInvalidMessageSignature()
    {
        startRuntimes();

        std::string participantId = createAndRegisterProvider();

        std::shared_ptr<vehicle::GpsProxy> gpsProxy = buildProxy();

        std::shared_ptr<Future<int>> gpsFuture(gpsProxy->calculateAvailableSatellitesAsync());
        int actualValue;
        JOYNR_ASSERT_NO_THROW(gpsFuture->get(2000, actualValue));

        gpsFuture = gpsProxy->calculateAvailableSatellitesAsync();
        if (useTls) {
            EXPECT_THROW(gpsFuture->get(2000, actualValue), exceptions::JoynrTimeOutException);
        } else {
            JOYNR_EXPECT_NO_THROW(gpsFuture->get(2000, actualValue));
        }

        ccRuntime->unregisterProvider(participantId);
    }

private:
    std::shared_ptr<MockKeychain> createMockKeychain()
    {
        const std::string privateKeyPassword("");

        std::shared_ptr<const mococrw::X509Certificate> certificate =
                std::make_shared<const mococrw::X509Certificate>(
                        mococrw::X509Certificate::fromPEMFile(
                                "/data/ssl-data/certs/client.cert.pem"));
        std::shared_ptr<const mococrw::X509Certificate> caCertificate =
                std::make_shared<const mococrw::X509Certificate>(
                        mococrw::X509Certificate::fromPEMFile("/data/ssl-data/certs/ca.cert.pem"));
        std::shared_ptr<const mococrw::AsymmetricPrivateKey> privateKey =
                std::make_shared<const mococrw::AsymmetricPrivateKey>(
                        mococrw::AsymmetricPrivateKey::readPrivateKeyFromPEM(
                                joynr::util::loadStringFromFile(
                                        "/data/ssl-data/private/client.key.pem"),
                                privateKeyPassword));

        ownerId = certificate->getSubjectDistinguishedName().commonName();

        std::shared_ptr<MockKeychain> keyChain = std::make_shared<MockKeychain>();
        ON_CALL(*keyChain, getTlsCertificate()).WillByDefault(Return(certificate));
        ON_CALL(*keyChain, getTlsKey()).WillByDefault(Return(privateKey));
        ON_CALL(*keyChain, getTlsRootCertificate()).WillByDefault(Return(caCertificate));

        return keyChain;
    }

protected:
    std::string domain;
    std::string ownerId;
    const bool useTls;
    std::shared_ptr<MockKeychain> keyChain;
    std::shared_ptr<JoynrClusterControllerRuntime> ccRuntime;
    std::shared_ptr<TestLibJoynrWebSocketRuntime> libJoynrRuntime;

private:
    DISALLOW_COPY_AND_ASSIGN(End2EndSSLTest);
};

TEST_P(End2EndSSLTest, localconnection_call_rpc_method_and_get_expected_result)
{
    if (useTls) {
        ON_CALL(*keyChain, getOwnerId()).WillByDefault(Return(ownerId));
    }
    startRuntimes();

    std::string participantId = createAndRegisterProvider();

    std::shared_ptr<vehicle::GpsProxy> gpsProxy = buildProxy();

    // Call the provider and wait for a result
    std::shared_ptr<Future<int>> gpsFuture(gpsProxy->calculateAvailableSatellitesAsync());

    int expectedValue = 42; // as defined in MockGpsProvider
    int actualValue;
    gpsFuture->get(2000, actualValue);
    EXPECT_EQ(expectedValue, actualValue);

    ccRuntime->unregisterProvider(participantId);
}

TEST_P(End2EndSSLTest, localconnection_call_rpc_method_without_message_signature)
{
    if (useTls) {
        InSequence s;
        // calls to getOwnerId:
        // 1 getGlobalAddress
        // 2 getReplyToAddress
        // 3 addNextHop for routing proxy
        // 4 addNextHop for discovery proxy

        // 5 resolve next hop for discovery provider
        // 6 lookup provider
        // 7 addNextHop for proxy

        // 8 call provider method
        EXPECT_CALL(*keyChain, getOwnerId()).Times(8).WillRepeatedly(Return(ownerId));
        // call provider method
        EXPECT_CALL(*keyChain, getOwnerId()).Times(1).WillOnce(Return(""));
    }
    testLocalconnectionCallRpcMethodWithInvalidMessageSignature();
}

TEST_P(End2EndSSLTest, localconnection_call_rpc_method_with_invalid_message_signature)
{
    if (useTls) {
        InSequence s;
        // calls to getOwnerId:
        // 1 getGlobalAddress
        // 2 getReplyToAddress
        // 3 addNextHop for routing proxy
        // 4 addNextHop for discovery proxy

        // 5 resolve next hop for discovery provider
        // 6 lookup provider
        // 7 addNextHop for proxy

        // 8 call provider method
        EXPECT_CALL(*keyChain, getOwnerId()).Times(8).WillRepeatedly(Return(ownerId));
        // call provider method
        EXPECT_CALL(*keyChain, getOwnerId()).WillOnce(Return("invalid signature"));
    }
    testLocalconnectionCallRpcMethodWithInvalidMessageSignature();
}

using namespace std::string_literals;

INSTANTIATE_TEST_CASE_P(
        TLS,
        End2EndSSLTest,
        testing::Values(std::make_tuple("test-resources/websocket-cc-tls.settings"s,
                                        "test-resources/websocket-libjoynr-tls.settings"s,
                                        true)));

INSTANTIATE_TEST_CASE_P(
        NonTLS,
        End2EndSSLTest,
        testing::Values(std::make_tuple("test-resources/websocket-cc-tls.settings"s,
                                        "test-resources/websocket-libjoynr-non-tls.settings"s,
                                        false)));
