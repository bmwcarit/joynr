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
#include <cstdint>
#include <fstream>
#include <memory>
#include <string>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/ClusterControllerSettings.h"
#include "joynr/MessagingSettings.h"
#include "joynr/Settings.h"
#include "joynr/JoynrClusterControllerRuntime.h"

#include "joynr/tests/testProxy.h"
#include "tests/mock/MockSubscriptionListener.h"
#include "tests/mock/MockTestProvider.h"

#include "tests/utils/PtrUtils.h"
#include "tests/JoynrTest.h"

using namespace ::testing;
using namespace joynr;

class End2EndAccessControlTest : public testing::Test
{
public:
    End2EndAccessControlTest()
            : runtimeAcON(),
              runtimeACRetry(),
              runtimeAcOFF(),
              testProvider(nullptr),
              testProxy(nullptr),
              providerQos(),
              domain("End2EndAccessControlTest"),
              providerParticipantId(),
              semaphore(0),
              AC_ENTRIES_FILE("CCAccessControl.entries"),
              MESSAGINGQOS_TTL(10000)
    {
    }

    void registerProviderAndBuildProxy(std::shared_ptr<JoynrRuntime> providerRuntime,
                                       std::shared_ptr<JoynrRuntime> proxyRuntime)
    {
        testProvider = std::make_shared<MockTestProvider>();
        providerParticipantId = providerRuntime->registerProvider<tests::testProvider>(
                domain, testProvider, providerQos, true, true);

        // Create a proxy on proxyRuntime
        auto testProxyBuilder = proxyRuntime->createProxyBuilder<tests::testProxy>(domain);
        DiscoveryQos discoveryQos;
        discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
        discoveryQos.setDiscoveryTimeoutMs(1000);

        testProxy = testProxyBuilder->setMessagingQos(MessagingQos(MESSAGINGQOS_TTL))
                            ->setDiscoveryQos(discoveryQos)
                            ->build();
    }

    void initSeparateCcRuntimesForProxyAndProvider(std::string fileWithACCentries)
    {
        // copy access entry file to bin folder for the test so that runtimes will find and load the
        // file
        joynr::test::util::copyTestResourceToCurrentDirectory(fileWithACCentries, AC_ENTRIES_FILE);

        auto settings1 = std::make_unique<Settings>(
                "test-resources/CCSettingsWithAccessControlEnabled.settings");
        auto settings2 = std::make_unique<Settings>(
                "test-resources/CCSettingsWithAccessControlDisabled.settings");
        settings1->set(ClusterControllerSettings::SETTING_ACL_ENTRIES_DIRECTORY(), ".");

        runtimeAcON = JoynrRuntime::createRuntime(std::move(settings1), failOnFatalRuntimeError);
        runtimeAcOFF = JoynrRuntime::createRuntime(std::move(settings2), failOnFatalRuntimeError);

        // Create a provider on runtimeAcON and a proxy on runtimeAcOFF
        providerQos.setScope(joynr::types::ProviderScope::GLOBAL);
        this->registerProviderAndBuildProxy(runtimeAcON, runtimeAcOFF);
    }

    void initSingleCcRuntimeWithACRetryInterval(std::string fileWithACCentries)
    {
        // copy access entry file to bin folder for the test so that runtimes will find and load the
        // file
        joynr::test::util::copyTestResourceToCurrentDirectory(fileWithACCentries, AC_ENTRIES_FILE);

        auto settings = std::make_unique<Settings>(
                "test-resources/CCSettingsWithAccessControlEnabled.settings");
        settings->set(ClusterControllerSettings::SETTING_ACL_ENTRIES_DIRECTORY(), ".");

        settings->set(MessagingSettings::SETTING_SEND_MSG_RETRY_INTERVAL(), MESSAGINGQOS_TTL / 10);

        runtimeACRetry = JoynrRuntime::createRuntime(std::move(settings), failOnFatalRuntimeError);

        // Create a provider on runtimeACRetry and a proxy on the same runtimeACRetry
        providerQos.setScope(joynr::types::ProviderScope::LOCAL);
        this->registerProviderAndBuildProxy(runtimeACRetry, runtimeACRetry);
    }

    ~End2EndAccessControlTest() override
    {
        if(runtimeAcON) {
            runtimeAcON->unregisterProvider(providerParticipantId);
        }
        if(runtimeACRetry) {
            runtimeACRetry->unregisterProvider(providerParticipantId);
        }
        test::util::resetAndWaitUntilDestroyed(runtimeAcON);
        test::util::resetAndWaitUntilDestroyed(runtimeACRetry);
        test::util::resetAndWaitUntilDestroyed(runtimeAcOFF);

        // Delete test specific files
        test::util::removeAllCreatedSettingsAndPersistencyFiles();
    }

protected:
    std::shared_ptr<JoynrRuntime> runtimeAcON;
    std::shared_ptr<JoynrRuntime> runtimeACRetry;
    std::shared_ptr<JoynrRuntime> runtimeAcOFF;

    std::shared_ptr<MockTestProvider> testProvider;
    std::shared_ptr<tests::testProxy> testProxy;
    types::ProviderQos providerQos;

    std::string domain;
    std::string providerParticipantId;
    joynr::Semaphore semaphore;

    const std::string AC_ENTRIES_FILE;
    const std::uint64_t MESSAGINGQOS_TTL;
};

TEST_F(End2EndAccessControlTest, queuedMsgsForUnavailableProviderAreAcCheckedWhenProviderIsRegisteredAgain_permissionYes)
{
    // consumer has permission
    // in this method provider is registered and proxy is built
    initSingleCcRuntimeWithACRetryInterval("AccessControlYesPermission.entries");
    runtimeACRetry->unregisterProvider(providerParticipantId);

    EXPECT_CALL(*testProvider, methodWithNoInputParametersMock(_, _)).Times(0);

    joynr::MessagingQos messagingQoS(MESSAGINGQOS_TTL);
    testProxy->methodWithNoInputParametersAsync(nullptr, nullptr, messagingQoS);

    // This sleep to give the proxy call chance to reach the provider
    std::this_thread::sleep_for(std::chrono::milliseconds(MESSAGINGQOS_TTL / 2));
    Mock::VerifyAndClearExpectations(testProvider.get());

    EXPECT_CALL(*testProvider, methodWithNoInputParametersMock(_, _)).Times(1).WillOnce(
                ReleaseSemaphore(&semaphore));

    // registering the provider again, queued messages should be delivered
    providerParticipantId = runtimeACRetry->registerProvider<tests::testProvider>(
            domain, testProvider, providerQos, true, true);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(MESSAGINGQOS_TTL)));
}

TEST_F(End2EndAccessControlTest, queuedMsgsForUnavailableProviderAreAcCheckedWhenProviderIsRegisteredAgain_permissionNo)
{
    // consumer has no permission
    // in this method provider is registered and proxy is built
    initSingleCcRuntimeWithACRetryInterval("AccessControlNoPermission.entries");
    runtimeACRetry->unregisterProvider(providerParticipantId);

    EXPECT_CALL(*testProvider, methodWithNoInputParametersMock(_, _)).Times(0);

    auto onError = [&] (const joynr::exceptions::JoynrRuntimeException&) {
        semaphore.notify();
    };

    joynr::MessagingQos messagingQoS(static_cast<std::uint64_t>(MESSAGINGQOS_TTL));
    testProxy->methodWithNoInputParametersAsync(nullptr, onError, messagingQoS);

    // Sleep some time to give the proxy call chance to reach the cluster controller and get queued
    std::this_thread::sleep_for(std::chrono::milliseconds(MESSAGINGQOS_TTL / 2));

    // registering the provider again, queued messages should be delivered
    providerParticipantId = runtimeACRetry->registerProvider<tests::testProvider>(
            domain, testProvider, providerQos, true, true);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(MESSAGINGQOS_TTL)));
}

TEST_F(End2EndAccessControlTest, proxyDoesNotHavePermission)
{
    initSeparateCcRuntimesForProxyAndProvider("AccessControlNoPermission.entries");
    // If AccessControl is active, the proxy cannot call methodWithNoInputParameters (see
    // AC_ENTRIES_FILE file)
    EXPECT_CALL(*testProvider, methodWithNoInputParametersMock(_, _)).Times(0);

    std::int32_t outputParameter = 0;

    // side effect of calling a method without access permission
    EXPECT_THROW(testProxy->methodWithNoInputParameters(outputParameter),
                 exceptions::JoynrTimeOutException);
}

TEST_F(End2EndAccessControlTest, proxyDoesHavePermission)
{
    initSeparateCcRuntimesForProxyAndProvider("AccessControlYesPermission.entries");
    EXPECT_CALL(*testProvider, methodWithNoInputParametersMock(_, _)).Times(1).WillOnce(
            ReleaseSemaphore(&semaphore));
    testProxy->methodWithNoInputParametersAsync();
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(MESSAGINGQOS_TTL)));
}
