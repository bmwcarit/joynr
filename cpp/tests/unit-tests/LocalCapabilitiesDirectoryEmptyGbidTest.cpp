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

#include <algorithm>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "joynr/ClusterControllerSettings.h"
#include "joynr/LocalCapabilitiesDirectory.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/Semaphore.h"
#include "joynr/Settings.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/Util.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "joynr/types/DiscoveryQos.h"
#include "joynr/types/DiscoveryScope.h"
#include "joynr/types/Version.h"

#include "tests/JoynrTest.h"
#include "tests/mock/MockGlobalCapabilitiesDirectoryClient.h"
#include "tests/mock/MockMessageRouter.h"

using namespace ::testing;
using namespace joynr;

MATCHER_P(pointerToMqttAddress, expectedAddress, "")
{
    if (arg == nullptr) {
        std::cout << "NULLPTR_1" << std::endl;
        return false;
    }
    auto mqttAddress =
            std::dynamic_pointer_cast<const system::RoutingTypes::MqttAddress>(arg);
    if (mqttAddress == nullptr) {
        std::cout << "NULLPTR_2" << std::endl;
        return false;
    }
    bool result = expectedAddress.equals(*mqttAddress, 0);
    std::cout << "RESULT: " << std::to_string(result) << std::endl;
    return result;
}

class LocalCapabilitiesDirectoryEmptyGbidTest : public ::testing::Test
{
public:
    LocalCapabilitiesDirectoryEmptyGbidTest()
            : settings(),
              clusterControllerSettings(settings),
              globalCapabilitiesDirectoryClientMock(std::make_shared<MockGlobalCapabilitiesDirectoryClient>()),
              singleThreadedIOService(std::make_shared<SingleThreadedIOService>()),
              messageRouterMock(
                      std::make_shared<MockMessageRouter>(singleThreadedIOService->getIOService())),
              clusterControllerId("clusterControllerId"),
              localCapabilitiesDirectory(),
              semaphore(0),
              discoveryQos(),
              nonEmptyGbids(std::vector<std::string>{"nonEmptyGbid1", "nonEmptyGbid2"}),
              externalAddresses(std::vector<system::RoutingTypes::MqttAddress>{
                  system::RoutingTypes::MqttAddress(nonEmptyGbids[0], "externalTopic1"),
                  system::RoutingTypes::MqttAddress(nonEmptyGbids[1], "externalTopic2")
              }),
              expectedLookupOnSuccessFunction([this](const std::vector<types::DiscoveryEntryWithMetaInfo>&) {
                  semaphore.notify();
              }),
              expectedLookupParticipantIdOnSuccessFunction([this](const types::DiscoveryEntryWithMetaInfo&) {
                  semaphore.notify();
              }),
              unexpectedOnErrorFunction([](const types::DiscoveryError::Enum& errorEnum) {
                  FAIL() << "Unexpected onError call: " + types::DiscoveryError::getLiteral(errorEnum);}
              ),
              unexpectedOnRuntimeErrorFunction([](const exceptions::JoynrRuntimeException& exception) {
                  FAIL() << "Unexpected onRuntimeError call: " + exception.getTypeName() + ":" + exception.getMessage();}
              )
    {
        singleThreadedIOService->start();

        discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);

        types::ProviderQos providerQos;
        types::GlobalDiscoveryEntry gde1(types::Version(23, 7),
                                         DOMAIN_1_NAME,
                                         INTERFACE_1_NAME,
                                         "participantId1",
                                         providerQos,
                                         10000,
                                         10000,
                                         PUBLIC_KEY_ID,
                                         serializer::serializeToJson(externalAddresses[0]));
        types::GlobalDiscoveryEntry gde2(types::Version(23, 7),
                                         DOMAIN_1_NAME,
                                         INTERFACE_1_NAME,
                                         "participantId2",
                                         providerQos,
                                         10000,
                                         10000,
                                         PUBLIC_KEY_ID,
                                         serializer::serializeToJson(externalAddresses[1]));
        gdesWithNonEmptyGbid.push_back(gde1);
        gdesWithNonEmptyGbid.push_back(gde2);
    }

    ~LocalCapabilitiesDirectoryEmptyGbidTest() override
    {
        singleThreadedIOService->stop();
        localCapabilitiesDirectory.reset();

        test::util::removeFileInCurrentDirectory(".*\\.settings");
        test::util::removeFileInCurrentDirectory(".*\\.persist");
    }

protected:
    std::shared_ptr<LocalCapabilitiesDirectory> getLcd(const std::vector<std::string>& knownGbids)
    {
        const std::string localAddress =
                serializer::serializeToJson(system::RoutingTypes::MqttAddress(knownGbids[0], "localTopic"));
        const std::int64_t defaultExpiryDateMs = 60 * 60 * 1000;
        localCapabilitiesDirectory.reset();
        localCapabilitiesDirectory = std::make_shared<LocalCapabilitiesDirectory>(
                clusterControllerSettings,
                globalCapabilitiesDirectoryClientMock,
                localAddress,
                messageRouterMock,
                singleThreadedIOService->getIOService(),
                clusterControllerId,
                knownGbids,
                defaultExpiryDateMs);
        localCapabilitiesDirectory->init();
        return localCapabilitiesDirectory;
    }

    void checkGdeGbidReplacement()
    {
        ASSERT_TRUE(gdesWithNonEmptyGbid.size() > 0 && externalAddresses.size() >= gdesWithNonEmptyGbid.size());
        for (size_t i = 0; i < gdesWithNonEmptyGbid.size(); i++) {
            auto address = externalAddresses[i];
            EXPECT_CALL(*messageRouterMock, addNextHop(Eq(gdesWithNonEmptyGbid[i].getParticipantId()),
                                                       pointerToMqttAddress(address),
                                                       _,
                                                       _,
                                                       _,
                                                       _,
                                                       _))
                    .Times(0);
            system::RoutingTypes::MqttAddress expectedAddress(address);
            expectedAddress.setBrokerUri("");
            EXPECT_CALL(*messageRouterMock, addNextHop(Eq(gdesWithNonEmptyGbid[i].getParticipantId()),
                                                       pointerToMqttAddress(expectedAddress),
                                                       _,
                                                       _,
                                                       _,
                                                       _,
                                                       _));
        }
    }

    void checkGdeGbidsAreNotChanged()
    {
        ASSERT_TRUE(gdesWithNonEmptyGbid.size() > 0 && externalAddresses.size() >= gdesWithNonEmptyGbid.size());
        for (size_t i = 0; i < gdesWithNonEmptyGbid.size(); i++) {
            auto expectedAddress = externalAddresses[i];
            EXPECT_TRUE(!expectedAddress.getBrokerUri().empty());
            EXPECT_CALL(*messageRouterMock, addNextHop(Eq(gdesWithNonEmptyGbid[i].getParticipantId()),
                                                       pointerToMqttAddress(expectedAddress),
                                                       _,
                                                       _,
                                                       _,
                                                       _,
                                                       _));
        }
    }

    Settings settings;
    ClusterControllerSettings clusterControllerSettings;
    std::shared_ptr<MockGlobalCapabilitiesDirectoryClient> globalCapabilitiesDirectoryClientMock;
    std::shared_ptr<SingleThreadedIOService> singleThreadedIOService;
    std::shared_ptr<MockMessageRouter> messageRouterMock;
    std::string clusterControllerId;
    std::shared_ptr<LocalCapabilitiesDirectory> localCapabilitiesDirectory;

    Semaphore semaphore;
    types::DiscoveryQos discoveryQos;

    const std::vector<std::string> nonEmptyGbids;
    const std::vector<system::RoutingTypes::MqttAddress> externalAddresses;
    std::vector<types::GlobalDiscoveryEntry> gdesWithNonEmptyGbid;

    std::function<void(const std::vector<types::DiscoveryEntryWithMetaInfo>&)> expectedLookupOnSuccessFunction;
    std::function<void(const types::DiscoveryEntryWithMetaInfo&)> expectedLookupParticipantIdOnSuccessFunction;
    std::function<void(const types::DiscoveryError::Enum&)> unexpectedOnErrorFunction;
    std::function<void(const exceptions::JoynrRuntimeException&)> unexpectedOnRuntimeErrorFunction;

    static const std::string INTERFACE_1_NAME;
    static const std::string DOMAIN_1_NAME;
    static const std::string PUBLIC_KEY_ID;

private:
    DISALLOW_COPY_AND_ASSIGN(LocalCapabilitiesDirectoryEmptyGbidTest);
};

const std::string LocalCapabilitiesDirectoryEmptyGbidTest::INTERFACE_1_NAME("myInterface1");
const std::string LocalCapabilitiesDirectoryEmptyGbidTest::DOMAIN_1_NAME("domain1");
const std::string LocalCapabilitiesDirectoryEmptyGbidTest::PUBLIC_KEY_ID("publicKeyId");

TEST_F(LocalCapabilitiesDirectoryEmptyGbidTest,
       lookupDomainInterface_onlyEmptyGbidIsKnown_gcdReturnsNonEmptyGbid_gbidOfGcdResultIsReplaced)
{
    const std::vector<std::string> knownGbids {""};
    auto lcd = getLcd(knownGbids);

    EXPECT_CALL(*globalCapabilitiesDirectoryClientMock, lookup(_, // domains
                                                               _, // interfaceName
                                                               Eq(knownGbids), // gbids
                                                               _, // messagingTtl
                                                               _, // onSuccess
                                                               _, // onError
                                                               _)) // onRuntimeError
            .WillOnce(InvokeArgument<4>(gdesWithNonEmptyGbid));

    checkGdeGbidReplacement();

    lcd->lookup({DOMAIN_1_NAME},
                INTERFACE_1_NAME,
                discoveryQos,
                expectedLookupOnSuccessFunction,
                unexpectedOnRuntimeErrorFunction);
    ASSERT_TRUE(semaphore.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(LocalCapabilitiesDirectoryEmptyGbidTest,
       lookupParticipantId_onlyEmptyGbidIsKnown_gcdReturnsNonEmptyGbid_gbidOfGcdResultIsReplaced)
{
    const std::vector<std::string> knownGbids {""};
    auto lcd = getLcd(knownGbids);

    gdesWithNonEmptyGbid.pop_back();
    ASSERT_TRUE(gdesWithNonEmptyGbid.size() == 1); // lookup by participantId returns only 1 DiscoveryEntry

    EXPECT_CALL(*globalCapabilitiesDirectoryClientMock, lookup(_, // participantId
                                                               Eq(knownGbids), // gbids
                                                               _, // messagingTtl
                                                               _, // onSuccess
                                                               _, // onError
                                                               _)) // onRuntimeError
            .WillOnce(InvokeArgument<3>(gdesWithNonEmptyGbid));

    checkGdeGbidReplacement();

    lcd->lookup(gdesWithNonEmptyGbid[0].getParticipantId(),
                expectedLookupParticipantIdOnSuccessFunction,
                unexpectedOnRuntimeErrorFunction);
    ASSERT_TRUE(semaphore.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(LocalCapabilitiesDirectoryEmptyGbidTest,
       lookupDomainInterfaceWithGbids_emptyGbidArray_onlyEmptyGbidIsKnown_gcdReturnsNonEmptyGbid_gbidOfGcdResultIsReplaced)
{
    const std::vector<std::string> knownGbids {""};
    auto lcd = getLcd(knownGbids);

    EXPECT_CALL(*globalCapabilitiesDirectoryClientMock, lookup(_, // domains
                                                               _, // interfaceName
                                                               Eq(knownGbids), // gbids
                                                               _, // messagingTtl
                                                               _, // onSuccess
                                                               _, // onError
                                                               _)) // onRuntimeError
            .WillOnce(InvokeArgument<4>(gdesWithNonEmptyGbid));

    checkGdeGbidReplacement();

    lcd->lookup({DOMAIN_1_NAME},
                INTERFACE_1_NAME,
                discoveryQos,
                std::vector<std::string>(),
                expectedLookupOnSuccessFunction,
                unexpectedOnErrorFunction);
    ASSERT_TRUE(semaphore.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(LocalCapabilitiesDirectoryEmptyGbidTest,
       lookupParticipantIdWithGbids_emptyGbidArray_onlyEmptyGbidIsKnown_gcdReturnsNonEmptyGbid_gbidOfGcdResultIsReplaced)
{
    const std::vector<std::string> knownGbids {""};
    auto lcd = getLcd(knownGbids);

    gdesWithNonEmptyGbid.pop_back();
    ASSERT_TRUE(gdesWithNonEmptyGbid.size() == 1); // lookup by participantId returns only 1 DiscoveryEntry

    EXPECT_CALL(*globalCapabilitiesDirectoryClientMock, lookup(_, // participantId
                                                               Eq(knownGbids), // gbids
                                                               _, // messagingTtl
                                                               _, // onSuccess
                                                               _, // onError
                                                               _)) // onRuntimeError
            .WillOnce(InvokeArgument<3>(gdesWithNonEmptyGbid));

    checkGdeGbidReplacement();

    lcd->lookup(gdesWithNonEmptyGbid[0].getParticipantId(),
                discoveryQos,
                std::vector<std::string>(),
                expectedLookupParticipantIdOnSuccessFunction,
                unexpectedOnErrorFunction);
    ASSERT_TRUE(semaphore.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(LocalCapabilitiesDirectoryEmptyGbidTest,
       lookupDomainInterfaceWithGbids_emptyGbid_onlyEmptyGbidIsKnown_emptyGbidStillInvalid)
{
    const std::vector<std::string> knownGbids {""};
    auto lcd = getLcd(knownGbids);


    EXPECT_CALL(*globalCapabilitiesDirectoryClientMock, lookup(_, // domains
                                                               _, // interfaceName
                                                               _, // gbids
                                                               _, // messagingTtl
                                                               _, // onSuccess
                                                               _, // onError
                                                               _)) // onRuntimeError
            .Times(0);

    EXPECT_CALL(*messageRouterMock, addNextHop(_,
                                               _,
                                               _,
                                               _,
                                               _,
                                               _,
                                               _))
            .Times(0);

    lcd->lookup({DOMAIN_1_NAME},
                INTERFACE_1_NAME,
                discoveryQos,
                std::vector<std::string> {""},
                [](const std::vector<types::DiscoveryEntryWithMetaInfo>&) {
                    FAIL() << "Unexpected onSuccess call";
                },
                [this](const types::DiscoveryError::Enum& errorEnum) {
                    EXPECT_EQ(types::DiscoveryError::INVALID_GBID, errorEnum);
                    semaphore.notify();
                });
    ASSERT_TRUE(semaphore.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(LocalCapabilitiesDirectoryEmptyGbidTest,
       lookupParticipantIdWithGbids_emptyGbid_onlyEmptyGbidIsKnown_emptyGbidStillInvalid)
{
    const std::vector<std::string> knownGbids {""};
    auto lcd = getLcd(knownGbids);

    EXPECT_CALL(*globalCapabilitiesDirectoryClientMock, lookup(_, // participantId
                                                               _, // gbids
                                                               _, // messagingTtl
                                                               _, // onSuccess
                                                               _, // onError
                                                               _)) // onRuntimeError
            .Times(0);

    EXPECT_CALL(*messageRouterMock, addNextHop(_,
                                               _,
                                               _,
                                               _,
                                               _,
                                               _,
                                               _))
            .Times(0);

    lcd->lookup(gdesWithNonEmptyGbid[0].getParticipantId(),
                discoveryQos,
                std::vector<std::string> {""},
                [](const types::DiscoveryEntryWithMetaInfo&) {
                    FAIL() << "Unexpected onSuccess call";
                },
                [this](const types::DiscoveryError::Enum& errorEnum) {
                    EXPECT_EQ(types::DiscoveryError::INVALID_GBID, errorEnum);
                    semaphore.notify();
                });
    ASSERT_TRUE(semaphore.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(LocalCapabilitiesDirectoryEmptyGbidTest,
       lookupDomainInterface_nonEmptyGbidsAreKnown_gcdReturnsNonEmptyGbid_gbidOfGcdResultIsNotChanged)
{
    const std::vector<std::string> knownGbids {"gbid1", "gbid2"};
    auto lcd = getLcd(knownGbids);

    EXPECT_CALL(*globalCapabilitiesDirectoryClientMock, lookup(_, // domains
                                                               _, // interfaceName
                                                               Eq(knownGbids), // gbids
                                                               _, // messagingTtl
                                                               _, // onSuccess
                                                               _, // onError
                                                               _)) // onRuntimeError
            .WillOnce(InvokeArgument<4>(gdesWithNonEmptyGbid));

    checkGdeGbidsAreNotChanged();

    lcd->lookup({DOMAIN_1_NAME},
                INTERFACE_1_NAME,
                discoveryQos,
                expectedLookupOnSuccessFunction,
                unexpectedOnRuntimeErrorFunction);
    ASSERT_TRUE(semaphore.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(LocalCapabilitiesDirectoryEmptyGbidTest,
       lookupParticipantId_nonEmptyGbidsAreKnown_gcdReturnsNonEmptyGbid_gbidOfGcdResultIsNotChanged)
{
    const std::vector<std::string> knownGbids {"gbid1", "gbid2"};
    auto lcd = getLcd(knownGbids);

    gdesWithNonEmptyGbid.pop_back();
    ASSERT_TRUE(gdesWithNonEmptyGbid.size() == 1); // lookup by participantId returns only 1 DiscoveryEntry

    EXPECT_CALL(*globalCapabilitiesDirectoryClientMock, lookup(_, // participantId
                                                               Eq(knownGbids), // gbids
                                                               _, // messagingTtl
                                                               _, // onSuccess
                                                               _, // onError
                                                               _)) // onRuntimeError
            .WillOnce(InvokeArgument<3>(gdesWithNonEmptyGbid));

    checkGdeGbidsAreNotChanged();

    lcd->lookup(gdesWithNonEmptyGbid[0].getParticipantId(),
                expectedLookupParticipantIdOnSuccessFunction,
                unexpectedOnRuntimeErrorFunction);
    ASSERT_TRUE(semaphore.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(LocalCapabilitiesDirectoryEmptyGbidTest,
       lookupDomainInterfaceWithGbids_emptyGbidArray_nonEmptyGbidsAreKnown_gcdReturnsNonEmptyGbid_gbidOfGcdResultIsNotChanged)
{
    const std::vector<std::string> knownGbids {"gbid1", "gbid2"};
    auto lcd = getLcd(knownGbids);

    EXPECT_CALL(*globalCapabilitiesDirectoryClientMock, lookup(_, // domains
                                                               _, // interfaceName
                                                               Eq(knownGbids), // gbids
                                                               _, // messagingTtl
                                                               _, // onSuccess
                                                               _, // onError
                                                               _)) // onRuntimeError
            .WillOnce(InvokeArgument<4>(gdesWithNonEmptyGbid));

    checkGdeGbidsAreNotChanged();

    lcd->lookup({DOMAIN_1_NAME},
                INTERFACE_1_NAME,
                discoveryQos,
                std::vector<std::string>(),
                expectedLookupOnSuccessFunction,
                unexpectedOnErrorFunction);
    ASSERT_TRUE(semaphore.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(LocalCapabilitiesDirectoryEmptyGbidTest,
       lookupParticipantIdWithGbids_emptyGbidArray_nonEmptyGbidsAreKnown_gcdReturnsNonEmptyGbid_gbidOfGcdResultIsNotChanged)
{
    const std::vector<std::string> knownGbids {"gbid1", "gbid2"};
    auto lcd = getLcd(knownGbids);

    gdesWithNonEmptyGbid.pop_back();
    ASSERT_TRUE(gdesWithNonEmptyGbid.size() == 1); // lookup by participantId returns only 1 DiscoveryEntry

    EXPECT_CALL(*globalCapabilitiesDirectoryClientMock, lookup(_, // participantId
                                                               Eq(knownGbids), // gbids
                                                               _, // messagingTtl
                                                               _, // onSuccess
                                                               _, // onError
                                                               _)) // onRuntimeError
            .WillOnce(InvokeArgument<3>(gdesWithNonEmptyGbid));

    checkGdeGbidsAreNotChanged();

    lcd->lookup(gdesWithNonEmptyGbid[0].getParticipantId(),
                discoveryQos,
                std::vector<std::string>(),
                expectedLookupParticipantIdOnSuccessFunction,
                unexpectedOnErrorFunction);
    ASSERT_TRUE(semaphore.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(LocalCapabilitiesDirectoryEmptyGbidTest,
       lookupParticipantIdWithGbids_emptyGbidArray_nonEmptyAndEmptyGbidsAreKnown_gcdReturnsNonEmptyGbid_gbidOfGcdResultIsNotChanged)
{
    const std::vector<std::string> knownGbids {"", "gbid2"};
    auto lcd = getLcd(knownGbids);

    gdesWithNonEmptyGbid.pop_back();
    ASSERT_TRUE(gdesWithNonEmptyGbid.size() == 1); // lookup by participantId returns only 1 DiscoveryEntry

    EXPECT_CALL(*globalCapabilitiesDirectoryClientMock, lookup(_, // participantId
                                                               Eq(knownGbids), // gbids
                                                               _, // messagingTtl
                                                               _, // onSuccess
                                                               _, // onError
                                                               _)) // onRuntimeError
            .WillOnce(InvokeArgument<3>(gdesWithNonEmptyGbid));

    checkGdeGbidsAreNotChanged();

    lcd->lookup(gdesWithNonEmptyGbid[0].getParticipantId(),
                discoveryQos,
                std::vector<std::string>(),
                expectedLookupParticipantIdOnSuccessFunction,
                unexpectedOnErrorFunction);
    ASSERT_TRUE(semaphore.waitFor(std::chrono::milliseconds(1000)));
}
