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

#include "joynr/CapabilitiesStorage.h"
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
            : _settings(),
              _clusterControllerSettings(_settings),
              _globalCapabilitiesDirectoryClientMock(std::make_shared<MockGlobalCapabilitiesDirectoryClient>()),
              _singleThreadedIOService(std::make_shared<SingleThreadedIOService>()),
              _messageRouterMock(
                      std::make_shared<MockMessageRouter>(_singleThreadedIOService->getIOService())),
              _clusterControllerId("clusterControllerId"),
              _locallyRegisteredCapabilities(std::make_shared<capabilities::Storage>()),
              _globalLookupCache(std::make_shared<capabilities::CachingStorage>()),
              _localCapabilitiesDirectory(),
              _semaphore(0),
              _discoveryQos(),
              _nonEmptyGbids(std::vector<std::string>{"nonEmptyGbid1", "nonEmptyGbid2"}),
              _externalAddresses(std::vector<system::RoutingTypes::MqttAddress>{
                  system::RoutingTypes::MqttAddress(_nonEmptyGbids[0], "externalTopic1"),
                  system::RoutingTypes::MqttAddress(_nonEmptyGbids[1], "externalTopic2")
              }),
              _expectedLookupOnSuccessFunction([this](const std::vector<types::DiscoveryEntryWithMetaInfo>&) {
                  _semaphore.notify();
              }),
              _expectedLookupParticipantIdOnSuccessFunction([this](const types::DiscoveryEntryWithMetaInfo&) {
                  _semaphore.notify();
              }),
              _unexpectedOnErrorFunction([](const types::DiscoveryError::Enum& errorEnum) {
                  FAIL() << "Unexpected onError call: " + types::DiscoveryError::getLiteral(errorEnum);}
              ),
              _unexpectedOnRuntimeErrorFunction([](const exceptions::JoynrRuntimeException& exception) {
                  FAIL() << "Unexpected onRuntimeError call: " + exception.getTypeName() + ":" + exception.getMessage();}
              )
    {
        _singleThreadedIOService->start();

        _discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);

        types::ProviderQos providerQos;
        types::GlobalDiscoveryEntry gde1(types::Version(23, 7),
                                         _DOMAIN_1_NAME,
                                         _INTERFACE_1_NAME,
                                         "participantId1",
                                         providerQos,
                                         10000,
                                         10000,
                                         _PUBLIC_KEY_ID,
                                         serializer::serializeToJson(_externalAddresses[0]));
        types::GlobalDiscoveryEntry gde2(types::Version(23, 7),
                                         _DOMAIN_1_NAME,
                                         _INTERFACE_1_NAME,
                                         "participantId2",
                                         providerQos,
                                         10000,
                                         10000,
                                         _PUBLIC_KEY_ID,
                                         serializer::serializeToJson(_externalAddresses[1]));
        _gdesWithNonEmptyGbid.push_back(gde1);
        _gdesWithNonEmptyGbid.push_back(gde2);
    }

    ~LocalCapabilitiesDirectoryEmptyGbidTest() override
    {
        _singleThreadedIOService->stop();
        _localCapabilitiesDirectory.reset();

        test::util::removeFileInCurrentDirectory(".*\\.settings");
        test::util::removeFileInCurrentDirectory(".*\\.persist");
    }

protected:
    std::shared_ptr<LocalCapabilitiesDirectory> getLcd(const std::vector<std::string>& knownGbids)
    {
        const std::string localAddress =
                serializer::serializeToJson(system::RoutingTypes::MqttAddress(knownGbids[0], "localTopic"));
        const std::int64_t defaultExpiryDateMs = 60 * 60 * 1000;
        _localCapabilitiesDirectory.reset();
        _localCapabilitiesDirectory = std::make_shared<LocalCapabilitiesDirectory>(
                _clusterControllerSettings,
                _globalCapabilitiesDirectoryClientMock,
                _locallyRegisteredCapabilities,
                _globalLookupCache,
                localAddress,
                _messageRouterMock,
                _singleThreadedIOService->getIOService(),
                _clusterControllerId,
                knownGbids,
                defaultExpiryDateMs);
        _localCapabilitiesDirectory->init();
        return _localCapabilitiesDirectory;
    }

    void checkGdeGbidReplacement()
    {
        ASSERT_TRUE(_gdesWithNonEmptyGbid.size() > 0 && _externalAddresses.size() >= _gdesWithNonEmptyGbid.size());
        for (size_t i = 0; i < _gdesWithNonEmptyGbid.size(); i++) {
            auto address = _externalAddresses[i];
            EXPECT_CALL(*_messageRouterMock, addNextHop(Eq(_gdesWithNonEmptyGbid[i].getParticipantId()),
                                                       pointerToMqttAddress(address),
                                                       _,
                                                       _,
                                                       _,
                                                       _,
                                                       _))
                    .Times(0);
            system::RoutingTypes::MqttAddress expectedAddress(address);
            expectedAddress.setBrokerUri("");
            EXPECT_CALL(*_messageRouterMock, addNextHop(Eq(_gdesWithNonEmptyGbid[i].getParticipantId()),
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
        ASSERT_TRUE(_gdesWithNonEmptyGbid.size() > 0 && _externalAddresses.size() >= _gdesWithNonEmptyGbid.size());
        for (size_t i = 0; i < _gdesWithNonEmptyGbid.size(); i++) {
            auto expectedAddress = _externalAddresses[i];
            EXPECT_TRUE(!expectedAddress.getBrokerUri().empty());
            EXPECT_CALL(*_messageRouterMock, addNextHop(Eq(_gdesWithNonEmptyGbid[i].getParticipantId()),
                                                       pointerToMqttAddress(expectedAddress),
                                                       _,
                                                       _,
                                                       _,
                                                       _,
                                                       _));
        }
    }

    Settings _settings;
    ClusterControllerSettings _clusterControllerSettings;
    std::shared_ptr<MockGlobalCapabilitiesDirectoryClient> _globalCapabilitiesDirectoryClientMock;
    std::shared_ptr<SingleThreadedIOService> _singleThreadedIOService;
    std::shared_ptr<MockMessageRouter> _messageRouterMock;
    std::string _clusterControllerId;
    std::shared_ptr<capabilities::Storage> _locallyRegisteredCapabilities;
    std::shared_ptr<capabilities::CachingStorage> _globalLookupCache;
    std::shared_ptr<LocalCapabilitiesDirectory> _localCapabilitiesDirectory;

    Semaphore _semaphore;
    types::DiscoveryQos _discoveryQos;

    const std::vector<std::string> _nonEmptyGbids;
    const std::vector<system::RoutingTypes::MqttAddress> _externalAddresses;
    std::vector<types::GlobalDiscoveryEntry> _gdesWithNonEmptyGbid;

    std::function<void(const std::vector<types::DiscoveryEntryWithMetaInfo>&)> _expectedLookupOnSuccessFunction;
    std::function<void(const types::DiscoveryEntryWithMetaInfo&)> _expectedLookupParticipantIdOnSuccessFunction;
    std::function<void(const types::DiscoveryError::Enum&)> _unexpectedOnErrorFunction;
    std::function<void(const exceptions::JoynrRuntimeException&)> _unexpectedOnRuntimeErrorFunction;

    static const std::string _INTERFACE_1_NAME;
    static const std::string _DOMAIN_1_NAME;
    static const std::string _PUBLIC_KEY_ID;

private:
    DISALLOW_COPY_AND_ASSIGN(LocalCapabilitiesDirectoryEmptyGbidTest);
};

const std::string LocalCapabilitiesDirectoryEmptyGbidTest::_INTERFACE_1_NAME("myInterface1");
const std::string LocalCapabilitiesDirectoryEmptyGbidTest::_DOMAIN_1_NAME("domain1");
const std::string LocalCapabilitiesDirectoryEmptyGbidTest::_PUBLIC_KEY_ID("publicKeyId");

TEST_F(LocalCapabilitiesDirectoryEmptyGbidTest,
       lookupDomainInterface_onlyEmptyGbidIsKnown_gcdReturnsNonEmptyGbid_gbidOfGcdResultIsReplaced)
{
    const std::vector<std::string> knownGbids {""};
    auto lcd = getLcd(knownGbids);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClientMock, lookup(_, // domains
                                                               _, // interfaceName
                                                               Eq(knownGbids), // gbids
                                                               _, // messagingTtl
                                                               _, // onSuccess
                                                               _, // onError
                                                               _)) // onRuntimeError
            .WillOnce(InvokeArgument<4>(_gdesWithNonEmptyGbid));

    checkGdeGbidReplacement();

    lcd->lookup({_DOMAIN_1_NAME},
                _INTERFACE_1_NAME,
                _discoveryQos,
                _expectedLookupOnSuccessFunction,
                _unexpectedOnRuntimeErrorFunction);
    ASSERT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(LocalCapabilitiesDirectoryEmptyGbidTest,
       lookupParticipantId_onlyEmptyGbidIsKnown_gcdReturnsNonEmptyGbid_gbidOfGcdResultIsReplaced)
{
    const std::vector<std::string> knownGbids {""};
    auto lcd = getLcd(knownGbids);

    _gdesWithNonEmptyGbid.pop_back();
    ASSERT_TRUE(_gdesWithNonEmptyGbid.size() == 1); // lookup by participantId returns only 1 DiscoveryEntry

    EXPECT_CALL(*_globalCapabilitiesDirectoryClientMock, lookup(_, // participantId
                                                               Eq(knownGbids), // gbids
                                                               _, // messagingTtl
                                                               _, // onSuccess
                                                               _, // onError
                                                               _)) // onRuntimeError
            .WillOnce(InvokeArgument<3>(_gdesWithNonEmptyGbid));

    checkGdeGbidReplacement();

    lcd->lookup(_gdesWithNonEmptyGbid[0].getParticipantId(),
                _expectedLookupParticipantIdOnSuccessFunction,
                _unexpectedOnRuntimeErrorFunction);
    ASSERT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(LocalCapabilitiesDirectoryEmptyGbidTest,
       lookupDomainInterfaceWithGbids_emptyGbidArray_onlyEmptyGbidIsKnown_gcdReturnsNonEmptyGbid_gbidOfGcdResultIsReplaced)
{
    const std::vector<std::string> knownGbids {""};
    auto lcd = getLcd(knownGbids);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClientMock, lookup(_, // domains
                                                               _, // interfaceName
                                                               Eq(knownGbids), // gbids
                                                               _, // messagingTtl
                                                               _, // onSuccess
                                                               _, // onError
                                                               _)) // onRuntimeError
            .WillOnce(InvokeArgument<4>(_gdesWithNonEmptyGbid));

    checkGdeGbidReplacement();

    lcd->lookup({_DOMAIN_1_NAME},
                _INTERFACE_1_NAME,
                _discoveryQos,
                std::vector<std::string>(),
                _expectedLookupOnSuccessFunction,
                _unexpectedOnErrorFunction);
    ASSERT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(LocalCapabilitiesDirectoryEmptyGbidTest,
       lookupParticipantIdWithGbids_emptyGbidArray_onlyEmptyGbidIsKnown_gcdReturnsNonEmptyGbid_gbidOfGcdResultIsReplaced)
{
    const std::vector<std::string> knownGbids {""};
    auto lcd = getLcd(knownGbids);

    _gdesWithNonEmptyGbid.pop_back();
    ASSERT_TRUE(_gdesWithNonEmptyGbid.size() == 1); // lookup by participantId returns only 1 DiscoveryEntry

    EXPECT_CALL(*_globalCapabilitiesDirectoryClientMock, lookup(_, // participantId
                                                               Eq(knownGbids), // gbids
                                                               _, // messagingTtl
                                                               _, // onSuccess
                                                               _, // onError
                                                               _)) // onRuntimeError
            .WillOnce(InvokeArgument<3>(_gdesWithNonEmptyGbid));

    checkGdeGbidReplacement();

    lcd->lookup(_gdesWithNonEmptyGbid[0].getParticipantId(),
                _discoveryQos,
                std::vector<std::string>(),
                _expectedLookupParticipantIdOnSuccessFunction,
                _unexpectedOnErrorFunction);
    ASSERT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(LocalCapabilitiesDirectoryEmptyGbidTest,
       lookupDomainInterfaceWithGbids_emptyGbid_onlyEmptyGbidIsKnown_emptyGbidStillInvalid)
{
    const std::vector<std::string> knownGbids {""};
    auto lcd = getLcd(knownGbids);


    EXPECT_CALL(*_globalCapabilitiesDirectoryClientMock, lookup(_, // domains
                                                               _, // interfaceName
                                                               _, // gbids
                                                               _, // messagingTtl
                                                               _, // onSuccess
                                                               _, // onError
                                                               _)) // onRuntimeError
            .Times(0);

    EXPECT_CALL(*_messageRouterMock, addNextHop(_,
                                               _,
                                               _,
                                               _,
                                               _,
                                               _,
                                               _))
            .Times(0);

    lcd->lookup({_DOMAIN_1_NAME},
                _INTERFACE_1_NAME,
                _discoveryQos,
                std::vector<std::string> {""},
                [](const std::vector<types::DiscoveryEntryWithMetaInfo>&) {
                    FAIL() << "Unexpected onSuccess call";
                },
                [this](const types::DiscoveryError::Enum& errorEnum) {
                    EXPECT_EQ(types::DiscoveryError::INVALID_GBID, errorEnum);
                    _semaphore.notify();
                });
    ASSERT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(LocalCapabilitiesDirectoryEmptyGbidTest,
       lookupParticipantIdWithGbids_emptyGbid_onlyEmptyGbidIsKnown_emptyGbidStillInvalid)
{
    const std::vector<std::string> knownGbids {""};
    auto lcd = getLcd(knownGbids);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClientMock, lookup(_, // participantId
                                                               _, // gbids
                                                               _, // messagingTtl
                                                               _, // onSuccess
                                                               _, // onError
                                                               _)) // onRuntimeError
            .Times(0);

    EXPECT_CALL(*_messageRouterMock, addNextHop(_,
                                               _,
                                               _,
                                               _,
                                               _,
                                               _,
                                               _))
            .Times(0);

    lcd->lookup(_gdesWithNonEmptyGbid[0].getParticipantId(),
                _discoveryQos,
                std::vector<std::string> {""},
                [](const types::DiscoveryEntryWithMetaInfo&) {
                    FAIL() << "Unexpected onSuccess call";
                },
                [this](const types::DiscoveryError::Enum& errorEnum) {
                    EXPECT_EQ(types::DiscoveryError::INVALID_GBID, errorEnum);
                    _semaphore.notify();
                });
    ASSERT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(LocalCapabilitiesDirectoryEmptyGbidTest,
       lookupDomainInterface_nonEmptyGbidsAreKnown_gcdReturnsNonEmptyGbid_gbidOfGcdResultIsNotChanged)
{
    const std::vector<std::string> knownGbids {"gbid1", "gbid2"};
    auto lcd = getLcd(knownGbids);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClientMock, lookup(_, // domains
                                                               _, // interfaceName
                                                               Eq(knownGbids), // gbids
                                                               _, // messagingTtl
                                                               _, // onSuccess
                                                               _, // onError
                                                               _)) // onRuntimeError
            .WillOnce(InvokeArgument<4>(_gdesWithNonEmptyGbid));

    checkGdeGbidsAreNotChanged();

    lcd->lookup({_DOMAIN_1_NAME},
                _INTERFACE_1_NAME,
                _discoveryQos,
                _expectedLookupOnSuccessFunction,
                _unexpectedOnRuntimeErrorFunction);
    ASSERT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(LocalCapabilitiesDirectoryEmptyGbidTest,
       lookupParticipantId_nonEmptyGbidsAreKnown_gcdReturnsNonEmptyGbid_gbidOfGcdResultIsNotChanged)
{
    const std::vector<std::string> knownGbids {"gbid1", "gbid2"};
    auto lcd = getLcd(knownGbids);

    _gdesWithNonEmptyGbid.pop_back();
    ASSERT_TRUE(_gdesWithNonEmptyGbid.size() == 1); // lookup by participantId returns only 1 DiscoveryEntry

    EXPECT_CALL(*_globalCapabilitiesDirectoryClientMock, lookup(_, // participantId
                                                               Eq(knownGbids), // gbids
                                                               _, // messagingTtl
                                                               _, // onSuccess
                                                               _, // onError
                                                               _)) // onRuntimeError
            .WillOnce(InvokeArgument<3>(_gdesWithNonEmptyGbid));

    checkGdeGbidsAreNotChanged();

    lcd->lookup(_gdesWithNonEmptyGbid[0].getParticipantId(),
                _expectedLookupParticipantIdOnSuccessFunction,
                _unexpectedOnRuntimeErrorFunction);
    ASSERT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(LocalCapabilitiesDirectoryEmptyGbidTest,
       lookupDomainInterfaceWithGbids_emptyGbidArray_nonEmptyGbidsAreKnown_gcdReturnsNonEmptyGbid_gbidOfGcdResultIsNotChanged)
{
    const std::vector<std::string> knownGbids {"gbid1", "gbid2"};
    auto lcd = getLcd(knownGbids);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClientMock, lookup(_, // domains
                                                               _, // interfaceName
                                                               Eq(knownGbids), // gbids
                                                               _, // messagingTtl
                                                               _, // onSuccess
                                                               _, // onError
                                                               _)) // onRuntimeError
            .WillOnce(InvokeArgument<4>(_gdesWithNonEmptyGbid));

    checkGdeGbidsAreNotChanged();

    lcd->lookup({_DOMAIN_1_NAME},
                _INTERFACE_1_NAME,
                _discoveryQos,
                std::vector<std::string>(),
                _expectedLookupOnSuccessFunction,
                _unexpectedOnErrorFunction);
    ASSERT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(LocalCapabilitiesDirectoryEmptyGbidTest,
       lookupParticipantIdWithGbids_emptyGbidArray_nonEmptyGbidsAreKnown_gcdReturnsNonEmptyGbid_gbidOfGcdResultIsNotChanged)
{
    const std::vector<std::string> knownGbids {"gbid1", "gbid2"};
    auto lcd = getLcd(knownGbids);

    _gdesWithNonEmptyGbid.pop_back();
    ASSERT_TRUE(_gdesWithNonEmptyGbid.size() == 1); // lookup by participantId returns only 1 DiscoveryEntry

    EXPECT_CALL(*_globalCapabilitiesDirectoryClientMock, lookup(_, // participantId
                                                               Eq(knownGbids), // gbids
                                                               _, // messagingTtl
                                                               _, // onSuccess
                                                               _, // onError
                                                               _)) // onRuntimeError
            .WillOnce(InvokeArgument<3>(_gdesWithNonEmptyGbid));

    checkGdeGbidsAreNotChanged();

    lcd->lookup(_gdesWithNonEmptyGbid[0].getParticipantId(),
                _discoveryQos,
                std::vector<std::string>(),
                _expectedLookupParticipantIdOnSuccessFunction,
                _unexpectedOnErrorFunction);
    ASSERT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(LocalCapabilitiesDirectoryEmptyGbidTest,
       lookupParticipantIdWithGbids_emptyGbidArray_nonEmptyAndEmptyGbidsAreKnown_gcdReturnsNonEmptyGbid_gbidOfGcdResultIsNotChanged)
{
    const std::vector<std::string> knownGbids {"", "gbid2"};
    auto lcd = getLcd(knownGbids);

    _gdesWithNonEmptyGbid.pop_back();
    ASSERT_TRUE(_gdesWithNonEmptyGbid.size() == 1); // lookup by participantId returns only 1 DiscoveryEntry

    EXPECT_CALL(*_globalCapabilitiesDirectoryClientMock, lookup(_, // participantId
                                                               Eq(knownGbids), // gbids
                                                               _, // messagingTtl
                                                               _, // onSuccess
                                                               _, // onError
                                                               _)) // onRuntimeError
            .WillOnce(InvokeArgument<3>(_gdesWithNonEmptyGbid));

    checkGdeGbidsAreNotChanged();

    lcd->lookup(_gdesWithNonEmptyGbid[0].getParticipantId(),
                _discoveryQos,
                std::vector<std::string>(),
                _expectedLookupParticipantIdOnSuccessFunction,
                _unexpectedOnErrorFunction);
    ASSERT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(1000)));
}
