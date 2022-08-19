/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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

#include "tests/utils/Gtest.h"
#include "tests/utils/Gmock.h"

#include "joynr/Future.h"
#include "joynr/MessagingSettings.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/Settings.h"

#include "joynr/tests/testProvider.h"
#include "joynr/tests/testProxy.h"
#include "joynr/types/ProviderQos.h"
#include "tests/JoynrTest.h"
#include "tests/mock/MockTestProvider.h"
#include "tests/mock/TestJoynrClusterControllerRuntime.h"
#include "tests/utils/PtrUtils.h"

using namespace ::testing;

using namespace joynr;

class End2End2CcWithEmptyGbidTest : public Test
{
public:
    End2End2CcWithEmptyGbidTest() :
        domain("cppEnd2End2CcWithEmptyGbidTest_Domain_" + util::createUuid()),
        runtime1WithEmptyGbid(),
        runtimeWithNonEmptyGbid()
    {
        const Settings libjoynrSettings1("test-resources/libjoynrSystemIntegration1.settings");
        const Settings libjoynrSettings2("test-resources/libjoynrSystemIntegration2.settings");
        const std::string settingsFile1 = "test-resources/MqttSystemIntegrationTest1.settings";
        const std::string settingsFile2 = "test-resources/MqttSystemIntegrationTest2.settings";
        auto settings1 = std::make_unique<Settings>(settingsFile1);
        auto settings2 = std::make_unique<Settings>(settingsFile2);
        Settings::merge(libjoynrSettings1, *settings1, false);
        Settings::merge(libjoynrSettings2, *settings2, false);
        MessagingSettings mSettings1(*settings1);
        MessagingSettings mSettings2(*settings2);
        mSettings1.setGbid("");
        assert(mSettings1.getGbid().empty());
        assert(!mSettings2.getGbid().empty());

        runtime1WithEmptyGbid = std::make_shared<TestJoynrClusterControllerRuntime>(
                std::move(settings1), failOnFatalRuntimeError);
        runtime1WithEmptyGbid->init();
        runtimeWithNonEmptyGbid = std::make_shared<TestJoynrClusterControllerRuntime>(
                std::move(settings2), failOnFatalRuntimeError);
        runtimeWithNonEmptyGbid->init();

        discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
        discoveryQos.setDiscoveryTimeoutMs(5000);
    }

    ~End2End2CcWithEmptyGbidTest() = default;

    // Sets up the test fixture.
    void SetUp() override
    {
        runtime1WithEmptyGbid->start();
        runtimeWithNonEmptyGbid->start();
    }

    // Tears down the test fixture.
    void TearDown() override
    {
        runtime1WithEmptyGbid->shutdown();
        runtimeWithNonEmptyGbid->shutdown();
        test::util::resetAndWaitUntilDestroyed(runtime1WithEmptyGbid);
        test::util::resetAndWaitUntilDestroyed(runtimeWithNonEmptyGbid);

        // Delete persisted files
        test::util::removeAllCreatedSettingsAndPersistencyFiles();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

protected:
    void callRpcMethodAndGetExpectedResult(
            std::shared_ptr<TestJoynrClusterControllerRuntime> consumerRuntime,
            std::shared_ptr<TestJoynrClusterControllerRuntime> providerRuntime)
    {
        auto mockProvider = std::make_shared<MockTestProvider>();

        types::ProviderQos providerQos;
        std::chrono::milliseconds millisSinceEpoch =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch());
        providerQos.setPriority(millisSinceEpoch.count());
        providerQos.setScope(joynr::types::ProviderScope::GLOBAL);
        const std::string providerParticipantId = providerRuntime->registerProvider<tests::testProvider>(
                    domain, mockProvider, providerQos, true, true);

        std::shared_ptr<ProxyBuilder<tests::testProxy>> proxyBuilder =
                consumerRuntime->createProxyBuilder<tests::testProxy>(domain);

        if (discoveryQos.getArbitrationStrategy() == DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT) {
            discoveryQos.addCustomParameter("fixedParticipantId", providerParticipantId);
        }
        std::uint64_t qosRoundTripTTL = 30000;
        std::shared_ptr<tests::testProxy> testProxy =
                proxyBuilder->setMessagingQos(MessagingQos(qosRoundTripTTL))
                        ->setDiscoveryQos(discoveryQos)
                        ->build();

        const std::vector<std::int32_t> ints = {32, 10};
        std::shared_ptr<Future<std::int32_t>> future(testProxy->sumIntsAsync(ints));
        ASSERT_NO_THROW(future->wait(30000));
        const std::int32_t expectedValue = 42;
        std::int32_t actualValue;
        future->get(actualValue);
        EXPECT_EQ(expectedValue, actualValue);

        providerRuntime->unregisterProvider(providerParticipantId);
    }

    std::string domain;
    std::shared_ptr<TestJoynrClusterControllerRuntime> runtime1WithEmptyGbid;
    std::shared_ptr<TestJoynrClusterControllerRuntime> runtimeWithNonEmptyGbid;
    joynr::DiscoveryQos discoveryQos;

private:
    DISALLOW_COPY_AND_ASSIGN(End2End2CcWithEmptyGbidTest);
};

TEST_F(End2End2CcWithEmptyGbidTest, consumerCcWithEmptyGbid_providerCcWithNonEmptyGbid_rpcCallSuccessfulAfterLookupByDomainInterface)
{
    callRpcMethodAndGetExpectedResult(runtime1WithEmptyGbid, runtimeWithNonEmptyGbid);
}

TEST_F(End2End2CcWithEmptyGbidTest, consumerCcWithNonEmptyGbid_providerCcWithEmptyGbid_rpcCallSuccessfulAfterLookupByDomainInterface)
{
    callRpcMethodAndGetExpectedResult(runtimeWithNonEmptyGbid, runtime1WithEmptyGbid);
}

TEST_F(End2End2CcWithEmptyGbidTest, consumerCcWithEmptyGbid_providerCcWithNonEmptyGbid_rpcCallSuccessfulAfterLookupByParticipantId)
{
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);
    callRpcMethodAndGetExpectedResult(runtime1WithEmptyGbid, runtimeWithNonEmptyGbid);
}

TEST_F(End2End2CcWithEmptyGbidTest, consumerCcWithNonEmptyGbid_providerCcWithEmptyGbid_rpcCallSuccessfulAfterLookupByParticipantId)
{
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);
    callRpcMethodAndGetExpectedResult(runtimeWithNonEmptyGbid, runtime1WithEmptyGbid);
}
