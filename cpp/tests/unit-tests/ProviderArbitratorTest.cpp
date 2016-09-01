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
#include <cstdint>
#include <chrono>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <semaphore.h>
#include "joynr/Logger.h"
#include "joynr/ProviderArbitrator.h"
#include "joynr/exceptions/JoynrException.h"
#include "tests/utils/MockObjects.h"

using ::testing::A;
using ::testing::_;
using ::testing::Return;
using ::testing::Eq;
using ::testing::NotNull;
using ::testing::AllOf;
using ::testing::Property;
using ::testing::Invoke;
using ::testing::Unused;
using ::testing::AtLeast;

using namespace joynr;

ACTION_P(ReleaseSemaphore,semaphore)
{
    semaphore->notify();
}

MATCHER_P(discoveryException, msg, "") {
    return arg.getTypeName() == joynr::exceptions::DiscoveryException::TYPE_NAME() && arg.getMessage() == msg;
}
class MockProviderArbitrator : public ProviderArbitrator {
public:
    MockProviderArbitrator(const std::string& domain,
                       const std::string& interfaceName,
                       const joynr::types::Version& interfaceVersion,
                       joynr::system::IDiscoverySync& discoveryProxy,
                       const DiscoveryQos& discoveryQos) : ProviderArbitrator(domain, interfaceName, interfaceVersion, discoveryProxy, discoveryQos){
    };

    MOCK_METHOD0(attemptArbitration, void (void));
    MOCK_METHOD1(filterDiscoveryEntries, std::string (const std::vector<joynr::types::DiscoveryEntry>&));
};

/**
 * Tests correct functionality of ProviderArbitrator
 */

/**
 * @brief Fixture.
 */
class ProviderArbitratorTest : public ::testing::Test {
public:

    ProviderArbitratorTest() :
        mockDiscovery(),
        discoveryTimeoutMs(std::chrono::milliseconds(1000).count()),
        retryIntervalMs(std::chrono::milliseconds(450).count()),
        discoveryQos(),
        mockArbitrationListener(new MockArbitrationListener()),
        semaphore(0)
    {
        discoveryQos.setDiscoveryTimeoutMs(discoveryTimeoutMs);
        discoveryQos.setRetryIntervalMs(retryIntervalMs);
        types::Version providerVersion;
        mockProviderArbitrator = new MockProviderArbitrator("domain", "interfaceName", providerVersion, mockDiscovery, discoveryQos);
    }
    void SetUp() {
        mockProviderArbitrator->setArbitrationListener(mockArbitrationListener);
    }

    void TearDown(){
        mockProviderArbitrator->removeArbitrationListener();
        delete mockArbitrationListener;
        delete mockProviderArbitrator;
    }

    static joynr::Logger logger;
    MockDiscovery mockDiscovery;
    std::int64_t discoveryTimeoutMs;
    std::int64_t retryIntervalMs;
    DiscoveryQos discoveryQos;
    MockProviderArbitrator* mockProviderArbitrator;
    MockArbitrationListener* mockArbitrationListener;
    Semaphore semaphore;
private:
    DISALLOW_COPY_AND_ASSIGN(ProviderArbitratorTest);
};

INIT_LOGGER(ProviderArbitratorTest);

TEST_F(ProviderArbitratorTest, arbitrationTimeout) {
    // Use a semaphore to count and wait on calls to the mock listener

    EXPECT_CALL(*mockArbitrationListener, setArbitrationStatus(Eq(ArbitrationStatus::ArbitrationCanceledForever)))
            .WillRepeatedly(ReleaseSemaphore(&semaphore));
    EXPECT_CALL(*mockArbitrationListener, setArbitrationError(discoveryException("Arbitration could not be finished in time.")));

    auto start = std::chrono::system_clock::now();

    EXPECT_CALL(*mockProviderArbitrator, attemptArbitration()).Times(AtLeast(1));
    mockProviderArbitrator->startArbitration();

    // Wait for timeout
    // Wait for more than discoveryTimeoutMs milliseconds since it might take some time until the timeout is reported
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(discoveryTimeoutMs * 10)));

    auto now = std::chrono::system_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);

    JOYNR_LOG_DEBUG(logger, "Time elapsed for unsuccessful arbitration : {}", elapsed.count());
    ASSERT_GE(elapsed.count(), discoveryTimeoutMs);
}
