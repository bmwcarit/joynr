/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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

#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "libjoynrclustercontroller/capabilities-client/CapabilitiesClient.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/infrastructure/GlobalCapabilitiesDirectoryProxy.h"
#include "joynr/types/GlobalDiscoveryEntry.h"
#include "joynr/DiscoveryQos.h"

#include "tests/mock/MockProxyBuilder.h"

using namespace joynr;

class CapabilitiesClientTestFixture : public ::testing::Test {
public:
    CapabilitiesClientTestFixture():
        capClient(std::make_unique<CapabilitiesClient>()),
        proxyBuilder(std::make_unique<MockProxyBuilder<infrastructure::GlobalCapabilitiesDirectoryProxy>>())
    {}

protected:
    std::unique_ptr<ICapabilitiesClient> capClient;
    std::unique_ptr<MockProxyBuilder<infrastructure::GlobalCapabilitiesDirectoryProxy>> proxyBuilder;
};

TEST_F(CapabilitiesClientTestFixture, callWithoutSetProxyBuilder)
{
    // prepare test data
    joynr::types::GlobalDiscoveryEntry aCapabilitiesInformation;
    std::vector<std::string> participantIds = {"aParticipantIDinAvector"};
    auto onSuccess = [](const std::vector<joynr::types::GlobalDiscoveryEntry>& result) {
        std::ignore = result;
    };

    // Attempt to call add, remove or lookup without settting ProxyBuilder should
    // trigger an assertion
    using ::testing::_;
    EXPECT_CALL(*(proxyBuilder.get()), build()).Times(0);
    EXPECT_CALL(*(proxyBuilder.get()), setCached(_)).Times(0);
    EXPECT_CALL(*(proxyBuilder.get()), setMessagingQos(_)).Times(0);
    EXPECT_CALL(*(proxyBuilder.get()), setDiscoveryQos(_)).Times(0);

    EXPECT_DEATH(capClient->add(aCapabilitiesInformation,
                                [](){},
                                [](const joynr::exceptions::JoynrRuntimeException& /*exception*/){}),
                 "Assertion.*");
    EXPECT_DEATH(capClient->remove("aParticipantID"), "Assertion.*");
    EXPECT_DEATH(capClient->remove(participantIds), "Assertion.*");
    EXPECT_DEATH(capClient->lookup({"domain"}, "interface", 0), "Assertion.*");
    EXPECT_DEATH(capClient->lookup({"domain"}, "interface", 0, onSuccess), "Assertion.*");
    EXPECT_DEATH(capClient->lookup("aParticipantID", onSuccess), "Assertion.*");
}
