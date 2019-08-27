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

#include <cstdint>
#include <functional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../../libjoynrclustercontroller/access-control/AccessController.h"
#include "../../libjoynrclustercontroller/access-control/LocalDomainAccessController.h"
#include "../../libjoynrclustercontroller/access-control/LocalDomainAccessStore.h"

#include "joynr/CallContext.h"
#include "joynr/CallContextStorage.h"
#include "joynr/CapabilityUtils.h"
#include "joynr/ClusterControllerDirectories.h"
#include "joynr/ClusterControllerSettings.h"
#include "joynr/LibjoynrSettings.h"
#include "joynr/LocalCapabilitiesDirectory.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/Semaphore.h"
#include "joynr/Settings.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/Util.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "joynr/types/DiscoveryQos.h"
#include "joynr/types/DiscoveryScope.h"
#include "joynr/types/Version.h"

#include "tests/JoynrTest.h"
#include "tests/mock/MockAccessController.h"
#include "tests/mock/MockCallback.h"
#include "tests/mock/MockGlobalCapabilitiesDirectoryClient.h"
#include "tests/mock/MockMessageRouter.h"

using namespace ::testing;
using namespace joynr;

MATCHER_P2(pointerToAddressWithSerializedAddress, addressType, serializedAddress, "")
{
    if (arg == nullptr) {
        return false;
    }
    if (addressType == "mqtt") {
        auto mqttAddress =
                std::dynamic_pointer_cast<const joynr::system::RoutingTypes::MqttAddress>(arg);
        if (mqttAddress == nullptr) {
            return false;
        }
        return joynr::serializer::serializeToJson(*mqttAddress) == serializedAddress;
    } else if (addressType == "http") {
        auto httpAddress =
                std::dynamic_pointer_cast<const joynr::system::RoutingTypes::ChannelAddress>(arg);
        if (httpAddress == nullptr) {
            return false;
        }
        return joynr::serializer::serializeToJson(*httpAddress) == serializedAddress;
    } else {
        return false;
    }
}

class LocalCapabilitiesDirectoryTest : public ::testing::Test
{
public:
    LocalCapabilitiesDirectoryTest()
            : settings(),
              clusterControllerSettings(settings),
              purgeExpiredDiscoveryEntriesIntervalMs(100),
              globalCapabilitiesDirectoryClient(std::make_shared<MockGlobalCapabilitiesDirectoryClient>()),
              singleThreadedIOService(std::make_shared<SingleThreadedIOService>()),
              mockMessageRouter(
                      std::make_shared<MockMessageRouter>(singleThreadedIOService->getIOService())),
              clusterControllerId("clusterControllerId"),
              localCapabilitiesDirectory(),
              lastSeenDateMs(std::chrono::duration_cast<std::chrono::milliseconds>(
                                     std::chrono::system_clock::now().time_since_epoch()).count()),
              expiryDateMs(lastSeenDateMs + 60 * 60 * 1000), // lastSeen + 1h
              dummyParticipantId1(),
              dummyParticipantId2(),
              dummyParticipantId3(),
              defaultOnSuccess([]() {}),
              defaultOnError([](const joynr::exceptions::ProviderRuntimeException&) {}),
              discoveryOnError([](const joynr::types::DiscoveryError::Enum&) {}),
              defaultProviderVersion(26, 05),
              semaphore(0)
    {
        singleThreadedIOService->start();
        clusterControllerSettings.setPurgeExpiredDiscoveryEntriesIntervalMs(
                purgeExpiredDiscoveryEntriesIntervalMs);
        settings.set(ClusterControllerSettings::SETTING_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS(),
                     200);
        settings.set(ClusterControllerSettings::
                             SETTING_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCY_ENABLED(),
                     true);
        localCapabilitiesDirectory = std::make_shared<LocalCapabilitiesDirectory>(
                clusterControllerSettings,
                globalCapabilitiesDirectoryClient,
                LOCAL_ADDRESS,
                mockMessageRouter,
                singleThreadedIOService->getIOService(),
                clusterControllerId,
                KNOWN_GBIDS);
        localCapabilitiesDirectory->init();

        // TODO the participantId should be provided by the provider
        dummyParticipantId1 = util::createUuid();
        dummyParticipantId2 = util::createUuid();
        dummyParticipantId3 = util::createUuid();
        discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL);
        discoveryQos.setCacheMaxAge(10000);

        // init a capentry recieved from the global capabilities directory
        types::ProviderQos qos;
        qos.setScope(types::ProviderScope::GLOBAL);
        types::DiscoveryEntry globalCapEntry(defaultProviderVersion,
                                             DOMAIN_1_NAME,
                                             INTERFACE_1_NAME,
                                             dummyParticipantId3,
                                             qos,
                                             10000,
                                             10000,
                                             PUBLIC_KEY_ID);
        globalCapEntryMap.insert({EXTERNAL_ADDRESS, globalCapEntry});
        entry = joynr::types::DiscoveryEntry(defaultProviderVersion,
                                           DOMAIN_1_NAME,
                                           INTERFACE_1_NAME,
                                           dummyParticipantId1,
                                           types::ProviderQos(),
                                           lastSeenDateMs,
                                           expiryDateMs,
                                           PUBLIC_KEY_ID);
    }

    ~LocalCapabilitiesDirectoryTest() override
    {
        singleThreadedIOService->stop();
        localCapabilitiesDirectory.reset();

        joynr::test::util::removeFileInCurrentDirectory(".*\\.settings");
        joynr::test::util::removeFileInCurrentDirectory(".*\\.persist");
    }

    void fakeLookupZeroResultsForInterfaceAddress(
            const std::vector<std::string>& domains,
            const std::string& interfaceName,
            const std::vector<std::string>& gbids,
            const std::int64_t messagingTtl,
            std::function<void(const std::vector<types::GlobalDiscoveryEntry>& capabilities)>
                    onSuccess,
            std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError,
            std::function<void(const exceptions::JoynrRuntimeException& error)> onRuntimeError)
    {
        std::ignore = domains;
        std::ignore = interfaceName;
        std::ignore = gbids;
        std::ignore = messagingTtl;
        std::ignore = onError;
        std::ignore = onRuntimeError;
        std::vector<types::GlobalDiscoveryEntry> result;
        onSuccess(result);
    }

    void fakeLookupNoEntryForParticipant(
            const std::string& participantId,
            const std::vector<std::string>& gbids,
            std::int64_t messagingTtl,
            std::function<void(const std::vector<types::GlobalDiscoveryEntry>& capabilities)>
                    onSuccess,
            std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError,
            std::function<void(const exceptions::JoynrRuntimeException& error)> onRuntimeError)
    {
        std::ignore = participantId;
        std::ignore = gbids;
        std::ignore = messagingTtl;
        std::ignore = onSuccess;
        std::ignore = onRuntimeError;
        std::vector<types::GlobalDiscoveryEntry> result;
        onError(types::DiscoveryError::NO_ENTRY_FOR_PARTICIPANT);
    }

    void fakeCapabilitiesClientLookupWithError (
            const std::vector<std::string>& domains,
            const std::string& interfaceName,
            const std::vector<std::string>& gbids,
            const std::int64_t messagingTtl,
            std::function<void(const std::vector<types::GlobalDiscoveryEntry>& capabilities)>
                    onSuccess,
            std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError,
            std::function<void(const exceptions::JoynrRuntimeException& error)> onRuntimeError)
    {
        std::ignore = domains;
        std::ignore = interfaceName;
        std::ignore = gbids;
        std::ignore = messagingTtl;
        std::ignore = onSuccess;
        std::ignore = onError;
        exceptions::DiscoveryException fakeDiscoveryException("fakeDiscoveryException");
        onRuntimeError(fakeDiscoveryException);
    }

    void fakeCapabilitiesClientAddWithError (
            const joynr::types::GlobalDiscoveryEntry& entry,
            const std::vector<std::string>& gbids,
            std::function<void()> onSuccess,
            std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError) {
        std::ignore = entry;
        std::ignore = gbids;
        std::ignore = onSuccess;
        std::ignore = onError;
        exceptions::DiscoveryException fakeDiscoveryException("fakeDiscoveryException");
        onRuntimeError(fakeDiscoveryException);
    }

    void fakeCapabilitiesClientAddSuccess (
            const joynr::types::GlobalDiscoveryEntry& entry,
            const std::vector<std::string>& gbids,
            std::function<void()> onSuccess,
            std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError) {
        std::ignore = entry;
        std::ignore = gbids;
        std::ignore = onError;
        std::ignore = onRuntimeError;
        onSuccess();
    }

    void fakeLookupWithResults(
            const std::vector<std::string>& domains,
            const std::string& interfaceName,
            const std::vector<std::string>& gbids,
            const std::int64_t messagingTtl,
            std::function<void(const std::vector<types::GlobalDiscoveryEntry>& capabilities)>
                    onSuccess,
            std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError,
            std::function<void(const exceptions::JoynrRuntimeException& error)> onRuntimeError)
    {
        std::ignore = domains;
        std::ignore = interfaceName;
        std::ignore = gbids;
        std::ignore = messagingTtl;
        std::ignore = onError;
        std::ignore = onRuntimeError;
        types::ProviderQos qos;
        std::vector<types::GlobalDiscoveryEntry> discoveryEntryList;
        discoveryEntryList.push_back(types::GlobalDiscoveryEntry(defaultProviderVersion,
                                                                 DOMAIN_1_NAME,
                                                                 INTERFACE_1_NAME,
                                                                 dummyParticipantId1,
                                                                 qos,
                                                                 LASTSEEN_MS,
                                                                 EXPIRYDATE_MS,
                                                                 PUBLIC_KEY_ID,
                                                                 EXTERNAL_ADDRESS));
        discoveryEntryList.push_back(types::GlobalDiscoveryEntry(defaultProviderVersion,
                                                                 DOMAIN_1_NAME,
                                                                 INTERFACE_1_NAME,
                                                                 dummyParticipantId2,
                                                                 qos,
                                                                 LASTSEEN_MS,
                                                                 EXPIRYDATE_MS,
                                                                 PUBLIC_KEY_ID,
                                                                 EXTERNAL_ADDRESS));
        onSuccess(discoveryEntryList);
    }

    void fakeLookupByParticipantIdWithResult(
            const std::string& participantId,
            const std::vector<std::string>& gbids,
            std::int64_t messagingTtl,
            std::function<void(const std::vector<types::GlobalDiscoveryEntry>& discoveryEntries)>
                    onSuccess,
            std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError,
            std::function<void(const exceptions::JoynrRuntimeException& error)> onRuntimeError)
    {
        std::ignore = gbids;
        std::ignore = messagingTtl;
        std::ignore = onError;
        std::ignore = onRuntimeError;
        types::ProviderQos qos;
        std::vector<types::GlobalDiscoveryEntry> discoveryEntryList;
        discoveryEntryList.push_back(types::GlobalDiscoveryEntry(defaultProviderVersion,
                                                                 DOMAIN_1_NAME,
                                                                 INTERFACE_1_NAME,
                                                                 participantId,
                                                                 qos,
                                                                 LASTSEEN_MS,
                                                                 EXPIRYDATE_MS,
                                                                 PUBLIC_KEY_ID,
                                                                 EXTERNAL_ADDRESS));
        onSuccess(discoveryEntryList);
    }

    void simulateTimeout()
    {
        throw exceptions::JoynrTimeOutException("Simulating timeout");
    }

    std::function<void(const std::vector<types::DiscoveryEntryWithMetaInfo>&)> createLookupSuccessFunction(
            const int expectedNumberOfEntries)
    {
        return [this, expectedNumberOfEntries] (const std::vector<types::DiscoveryEntryWithMetaInfo>& result) {
            EXPECT_EQ(expectedNumberOfEntries, result.size());
            semaphore.notify();
        };
    }

    std::function<void()> createAddOnSuccessFunction()
    {
        return [this] () {
            semaphore.notify();
        };
    }

    std::function<void(const types::DiscoveryEntryWithMetaInfo&)> createLookupParticipantIdSuccessFunction()
    {
        return [this] (const types::DiscoveryEntryWithMetaInfo& result) {
            std::ignore = result;
            semaphore.notify();
        };
    }

    std::function<void(const std::vector<types::DiscoveryEntryWithMetaInfo>&)>
    createUnexpectedLookupSuccessFunction()
    {
        return [this] (const std::vector<types::DiscoveryEntryWithMetaInfo>& result) {
            FAIL() << "Got result: " + (result.empty() ? "EMPTY" : result.at(0).toString());
        };
    }

    std::function<void(const types::DiscoveryEntryWithMetaInfo&)>
    createUnexpectedLookupParticipantIdSuccessFunction()
    {
        return [] (const types::DiscoveryEntryWithMetaInfo& result) {
            FAIL() << "Got result: " + result.toString();
        };
    }

    std::function<void(const types::DiscoveryError::Enum&)> createExpectedDiscoveryErrorFunction(
            const types::DiscoveryError::Enum& expectedError)
    {
        return [this, expectedError] (const types::DiscoveryError::Enum& error) {
            EXPECT_EQ(expectedError, error);
            semaphore.notify();
        };
    }

    std::function<void(const joynr::exceptions::ProviderRuntimeException&)> createExpectedProviderRuntimeExceptionFunction(
            const joynr::exceptions::ProviderRuntimeException& expectedRuntimeException) {
        return [this, expectedRuntimeException] (const joynr::exceptions::ProviderRuntimeException& exception) {
            semaphore.notify();
        };
    }

    std::function<void(const types::DiscoveryError::Enum&)> createUnexpectedDiscoveryErrorFunction()
    {
        return [] (const types::DiscoveryError::Enum& discoveryError) {
            FAIL() << "Got DiscoveryError: " + types::DiscoveryError::getLiteral(discoveryError);
        };
    }

    void testAddWithGbidsIsProperlyRejected(const types::DiscoveryError::Enum& expectedDiscoveryError) {
        const bool& awaitGlobalRegistration = true;
        const std::vector<std::string> gbids = {KNOWN_GBIDS[0]};
        EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(An<const types::GlobalDiscoveryEntry&>(), _, _, _, _)).Times(1)
                .WillOnce(InvokeArgument<3>(expectedDiscoveryError));

        localCapabilitiesDirectory->add(
                entry,
                awaitGlobalRegistration,
                gbids,
                defaultOnSuccess,
                createExpectedDiscoveryErrorFunction(expectedDiscoveryError));

        EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
    }

    void testAddIsProperlyRejected(const types::DiscoveryError::Enum& expectedDiscoveryError) {
        const bool& awaitGlobalRegistration = true;
        EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(An<const types::GlobalDiscoveryEntry&>(), _, _, _, _)).Times(1)
                .WillOnce(InvokeArgument<3>(expectedDiscoveryError));

        localCapabilitiesDirectory->add(
                entry,
                awaitGlobalRegistration,
                defaultOnSuccess,
                createExpectedProviderRuntimeExceptionFunction(joynr::exceptions::ProviderRuntimeException()));

        EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
    }

    void testAddWithDiscoveryError(
            const std::vector<std::string>& gbids, const types::DiscoveryError::Enum& expectedDiscoveryError)
    {
        const bool& awaitGlobalRegistration = true;
        EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(An<const types::GlobalDiscoveryEntry&>(), _, _, _, _)).Times(1)
                .WillOnce(InvokeArgument<3>(expectedDiscoveryError));

        localCapabilitiesDirectory->add(
                entry,
                awaitGlobalRegistration,
                defaultOnSuccess,
                createExpectedProviderRuntimeExceptionFunction(joynr::exceptions::ProviderRuntimeException()));

        EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
    }

protected:
    void registerReceivedCapabilities(
            const std::string& addressType,
            const std::string& serializedAddress)
    {
        const std::string participantId = "TEST_participantId";
        EXPECT_CALL(
                *mockMessageRouter,
                addNextHop(participantId,
                           AllOf(Pointee(A<const joynr::system::RoutingTypes::Address>()),
                                 pointerToAddressWithSerializedAddress(addressType, serializedAddress)),
                           _,
                           _,
                           _,
                           _,
                           _)).Times(1);
        EXPECT_CALL(*mockMessageRouter,
                    addNextHop(participantId,
                               AnyOf(Not(Pointee(A<const joynr::system::RoutingTypes::Address>())),
                                     Not(pointerToAddressWithSerializedAddress(
                                             addressType, serializedAddress))),
                               _,
                               _,
                               _,
                               _,
                               _)).Times(0);

        std::unordered_multimap<std::string, types::DiscoveryEntry> capabilitiesMap;
        types::DiscoveryEntry capEntry;
        capEntry.setParticipantId(participantId);
        capabilitiesMap.insert({serializedAddress, capEntry});
        localCapabilitiesDirectory->registerReceivedCapabilities(std::move(capabilitiesMap));
    }

    Settings settings;
    ClusterControllerSettings clusterControllerSettings;
    const int purgeExpiredDiscoveryEntriesIntervalMs;
    std::shared_ptr<MockGlobalCapabilitiesDirectoryClient> globalCapabilitiesDirectoryClient;
    std::shared_ptr<SingleThreadedIOService> singleThreadedIOService;
    std::shared_ptr<MockMessageRouter> mockMessageRouter;
    std::string clusterControllerId;
    std::shared_ptr<LocalCapabilitiesDirectory> localCapabilitiesDirectory;
    std::int64_t lastSeenDateMs;
    std::int64_t expiryDateMs;
    std::string dummyParticipantId1;
    std::string dummyParticipantId2;
    std::string dummyParticipantId3;
    joynr::types::DiscoveryQos discoveryQos;
    std::unordered_multimap<std::string, types::DiscoveryEntry> globalCapEntryMap;
    joynr::types::DiscoveryEntry entry;

    std::function<void()> defaultOnSuccess;
    std::function<void(const joynr::exceptions::ProviderRuntimeException&)> defaultOnError;
    std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> discoveryOnError;
    joynr::types::Version defaultProviderVersion;
    joynr::Semaphore semaphore;

    static const std::vector<std::string> KNOWN_GBIDS;
    static const std::string INTERFACE_1_NAME;
    static const std::string DOMAIN_1_NAME;
    static const std::string INTERFACE_2_NAME;
    static const std::string DOMAIN_2_NAME;
    static const std::string INTERFACE_3_NAME;
    static const std::string DOMAIN_3_NAME;
    static const std::string LOCAL_ADDRESS;
    static const std::string EXTERNAL_ADDRESS;
    static const std::int64_t LASTSEEN_MS;
    static const std::int64_t EXPIRYDATE_MS;
    static const std::string PUBLIC_KEY_ID;
    static const int TIMEOUT;

private:
    DISALLOW_COPY_AND_ASSIGN(LocalCapabilitiesDirectoryTest);
};

const std::vector<std::string> LocalCapabilitiesDirectoryTest::KNOWN_GBIDS{"testGbid1", "testGbid2", "testGbid3"};
const std::string LocalCapabilitiesDirectoryTest::INTERFACE_1_NAME("myInterfaceA");
const std::string LocalCapabilitiesDirectoryTest::INTERFACE_2_NAME("myInterfaceB");
const std::string LocalCapabilitiesDirectoryTest::INTERFACE_3_NAME("myInterfaceC");
const std::string LocalCapabilitiesDirectoryTest::DOMAIN_1_NAME("domainA");
const std::string LocalCapabilitiesDirectoryTest::DOMAIN_2_NAME("domainB");
const std::string LocalCapabilitiesDirectoryTest::DOMAIN_3_NAME("domainB");
const std::string LocalCapabilitiesDirectoryTest::LOCAL_ADDRESS("localAddress");
const std::string LocalCapabilitiesDirectoryTest::EXTERNAL_ADDRESS(
        joynr::serializer::serializeToJson(joynr::system::RoutingTypes::MqttAddress(KNOWN_GBIDS[0], "topic")));
const std::int64_t LocalCapabilitiesDirectoryTest::LASTSEEN_MS(1000);
const std::int64_t LocalCapabilitiesDirectoryTest::EXPIRYDATE_MS(10000);
const std::string LocalCapabilitiesDirectoryTest::PUBLIC_KEY_ID("publicKeyId");
const int LocalCapabilitiesDirectoryTest::TIMEOUT(2000);

MATCHER_P(AnConvertedGlobalDiscoveryEntry, other, "")
{
    return other.getDomain() == arg.getDomain() &&
           other.getInterfaceName() == arg.getInterfaceName() &&
           other.getParticipantId() == arg.getParticipantId() && other.getQos() == arg.getQos() &&
           other.getLastSeenDateMs() == arg.getLastSeenDateMs() &&
           other.getProviderVersion() == arg.getProviderVersion() &&
           other.getExpiryDateMs() == arg.getExpiryDateMs() &&
           other.getPublicKeyId() == arg.getPublicKeyId();
}

TEST_F(LocalCapabilitiesDirectoryTest, addCapability)
{
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
            add(Matcher<const types::GlobalDiscoveryEntry&>(AnConvertedGlobalDiscoveryEntry(entry)),
            _, _, _, _)).Times(1)
            .WillOnce(InvokeArgument<2>());

    localCapabilitiesDirectory->add(entry, createAddOnSuccessFunction(), defaultOnError);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, addCapabilityWithSingleNonDefaultGbid) {
    const std::vector<std::string> gbids{KNOWN_GBIDS[1]};
    const std::vector<std::string> expectedGbids = gbids;
    const bool& awaitGlobalRegistration = true;
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
            add(Matcher<const types::GlobalDiscoveryEntry&>(AnConvertedGlobalDiscoveryEntry(entry)),
            Eq(expectedGbids), _, _, _)).Times(1)
            .WillOnce(InvokeArgument<2>());

    localCapabilitiesDirectory->add(
            entry, awaitGlobalRegistration, gbids, createAddOnSuccessFunction(), discoveryOnError);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, addCapabilityWithMultipleGbids) {
    const std::vector<std::string> gbids{KNOWN_GBIDS[1], KNOWN_GBIDS[0]};
    const std::vector<std::string> expectedGbids = gbids;
    const bool& awaitGlobalRegistration = true;
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
            add(Matcher<const types::GlobalDiscoveryEntry&>(AnConvertedGlobalDiscoveryEntry(entry)),
            Eq(expectedGbids), _, _, _)).Times(1)
            .WillOnce(InvokeArgument<2>());

    localCapabilitiesDirectory->add(
                entry, awaitGlobalRegistration, gbids, createAddOnSuccessFunction(), discoveryOnError);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, addCapabilityWithEmptyGbidArray_addToDefaultBackend) {
    const std::vector<std::string> gbids{};
    const std::vector<std::string> expectedGbids {KNOWN_GBIDS[0]};
    const bool& awaitGlobalRegistration = true;
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
            add(Matcher<const types::GlobalDiscoveryEntry&>(AnConvertedGlobalDiscoveryEntry(entry)),
            Eq(expectedGbids), _, _, _)).Times(1)
            .WillOnce(InvokeArgument<2>());

    localCapabilitiesDirectory->add(entry,
                                    awaitGlobalRegistration,
                                    gbids,
                                    createAddOnSuccessFunction(),
                                    discoveryOnError);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, addCapabilityWithAddToAll) {
    const std::vector<std::string> expectedGbids = KNOWN_GBIDS;
    const bool& awaitGlobalRegistration = true;
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
            add(Matcher<const types::GlobalDiscoveryEntry&>(AnConvertedGlobalDiscoveryEntry(entry)),
            Eq(expectedGbids), _, _, _)).Times(1)
            .WillOnce(InvokeArgument<2>());

    localCapabilitiesDirectory->addToAll(
            entry, awaitGlobalRegistration, createAddOnSuccessFunction(), discoveryOnError);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, addCapabilityWithGbidsWithEmptyStringEntry) {
    const std::vector<std::string> gbids{KNOWN_GBIDS[0], ""};
    const bool& awaitGlobalRegistration = true;
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
            add(An<const joynr::types::GlobalDiscoveryEntry&>(), _, _, _, _)).Times(0);

    localCapabilitiesDirectory->add(
            entry,
            awaitGlobalRegistration,
            gbids,
            defaultOnSuccess,
            createExpectedDiscoveryErrorFunction(types::DiscoveryError::INVALID_GBID));

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, addCapabilityWithGbidsWithDuplicateStringEntries) {
    const std::vector<std::string> gbids{KNOWN_GBIDS[0], KNOWN_GBIDS[0]};
    const bool& awaitGlobalRegistration = true;
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
            add(An<const joynr::types::GlobalDiscoveryEntry&>(), _, _, _, _)).Times(0);

    localCapabilitiesDirectory->add(
            entry,
            awaitGlobalRegistration,
            gbids,
            defaultOnSuccess,
            createExpectedDiscoveryErrorFunction(types::DiscoveryError::INVALID_GBID));

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, addCapabilityWithUnknownGbidEntry) {
    const std::vector<std::string> gbids{KNOWN_GBIDS[0], "unknownGbid"};
    const bool& awaitGlobalRegistration = true;
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
            add(An<const joynr::types::GlobalDiscoveryEntry&>(), _, _, _, _)).Times(0);

    localCapabilitiesDirectory->add(
            entry,
            awaitGlobalRegistration,
            gbids,
            defaultOnSuccess,
            createExpectedDiscoveryErrorFunction(types::DiscoveryError::UNKNOWN_GBID));

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, addLocalOnlyCapability) {
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);
    joynr::types::DiscoveryEntry localEntry(defaultProviderVersion,
                                        DOMAIN_1_NAME,
                                        INTERFACE_1_NAME,
                                        dummyParticipantId1,
                                        providerQos,
                                        lastSeenDateMs,
                                        expiryDateMs,
                                        PUBLIC_KEY_ID);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
            add(An<const types::GlobalDiscoveryEntry&>(), _, _, _, _)).Times(0);

    localCapabilitiesDirectory->add(
            localEntry,
            createAddOnSuccessFunction(),
            defaultOnError);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, addGlobalCapSucceeds_NextAddShallAddGlobalAgain) {
   const bool& awaitGlobalRegistration = true;

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
             add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(
                     AnConvertedGlobalDiscoveryEntry(entry)), _, _, _, _)).Times(2)
            .WillRepeatedly(InvokeArgument<2>());

    localCapabilitiesDirectory->add(
            entry,
            awaitGlobalRegistration,
            createAddOnSuccessFunction(),
            defaultOnError);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    joynr::types::DiscoveryEntry entry2(defaultProviderVersion,
                                        DOMAIN_1_NAME,
                                        INTERFACE_1_NAME,
                                        dummyParticipantId1,
                                        types::ProviderQos(),
                                        lastSeenDateMs,
                                        expiryDateMs,
                                        PUBLIC_KEY_ID);

    localCapabilitiesDirectory->add(
            entry2,
            awaitGlobalRegistration,
            createAddOnSuccessFunction(),
            defaultOnError);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddWithGbidsIsProperlyRejected_invalidGbid) {
    testAddWithGbidsIsProperlyRejected(joynr::types::DiscoveryError::INVALID_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddWithGbidsIsProperlyRejected_unknownGbid) {
    testAddWithGbidsIsProperlyRejected(joynr::types::DiscoveryError::UNKNOWN_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddWithGbidsIsProperlyRejected_internalError) {
    testAddWithGbidsIsProperlyRejected(joynr::types::DiscoveryError::INTERNAL_ERROR);
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddIsProperlyRejected_invalidGbid) {
    testAddIsProperlyRejected(joynr::types::DiscoveryError::INVALID_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddIsProperlyRejected_unknownGbid) {
    testAddIsProperlyRejected(joynr::types::DiscoveryError::UNKNOWN_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddIsProperlyRejected_internalError) {
    testAddIsProperlyRejected(joynr::types::DiscoveryError::INTERNAL_ERROR);
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddWithGbids_unknownGbid) {
    const std::vector<std::string> gbids{KNOWN_GBIDS[0], "unknownGbid"};
    testAddWithDiscoveryError(gbids, joynr::types::DiscoveryError::UNKNOWN_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddWithGbids_invalidGbid_emptyGbid) {
    const std::vector<std::string> gbids{};
    testAddWithDiscoveryError(gbids, joynr::types::DiscoveryError::INVALID_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddWithGbids_invalidGbid_duplicateGbid) {
    const std::vector<std::string> gbids{KNOWN_GBIDS[1], KNOWN_GBIDS[1]};
    testAddWithDiscoveryError(gbids, joynr::types::DiscoveryError::INVALID_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, addSameGbidTwiceInARow) {
    const bool& awaitGlobalRegistration = true;
    const std::vector<std::string> gbids{KNOWN_GBIDS[0]};

     EXPECT_CALL(*globalCapabilitiesDirectoryClient,
              add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(
                      AnConvertedGlobalDiscoveryEntry(entry)), _, _, _, _)).Times(2)
             .WillRepeatedly(InvokeArgument<2>());

     localCapabilitiesDirectory->add(
             entry,
             awaitGlobalRegistration,
             gbids,
             createAddOnSuccessFunction(),
             discoveryOnError);

     EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

     localCapabilitiesDirectory->add(
             entry,
             awaitGlobalRegistration,
             gbids,
             createAddOnSuccessFunction(),
             discoveryOnError);

     EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, addDifferentGbidsAfterEachOther) {
    const bool& awaitGlobalRegistration = true;
    const std::vector<std::string> gbids1{KNOWN_GBIDS[0]};
    const std::vector<std::string> expectedGbids1 = gbids1;
    const std::vector<std::string> gbids2{KNOWN_GBIDS[0]};
    const std::vector<std::string> expectedGbids2 = gbids2;

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
            add(Matcher<const types::GlobalDiscoveryEntry&>(AnConvertedGlobalDiscoveryEntry(entry)),
            Eq(expectedGbids1), _, _, _)).Times(1)
            .WillOnce(InvokeArgument<2>());

    localCapabilitiesDirectory->add(
            entry,
            awaitGlobalRegistration,
            gbids1,
            createAddOnSuccessFunction(),
            discoveryOnError);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    joynr::types::DiscoveryEntry entry2(defaultProviderVersion,
                                        DOMAIN_2_NAME,
                                        INTERFACE_2_NAME,
                                        dummyParticipantId2,
                                        types::ProviderQos(),
                                        lastSeenDateMs,
                                        expiryDateMs,
                                        PUBLIC_KEY_ID);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
             add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(AnConvertedGlobalDiscoveryEntry(entry2)),
             Eq(expectedGbids2), _, _, _)).Times(1)
            .WillOnce(InvokeArgument<2>());

    localCapabilitiesDirectory->add(
            entry2,
            awaitGlobalRegistration,
            gbids2,
            createAddOnSuccessFunction(),
            discoveryOnError);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddKnownLocalEntryDoesNothing) {
    const bool& awaitGlobalRegistration = false;
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);
    joynr::types::DiscoveryEntry localEntry(defaultProviderVersion,
                                        DOMAIN_1_NAME,
                                        INTERFACE_1_NAME,
                                        dummyParticipantId1,
                                        providerQos,
                                        lastSeenDateMs,
                                        expiryDateMs,
                                        PUBLIC_KEY_ID);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(An<const joynr::types::GlobalDiscoveryEntry&>(),
                _, _, _, _)).Times(0);

    localCapabilitiesDirectory->add(
            localEntry,
            awaitGlobalRegistration,
            defaultOnSuccess,
            defaultOnError);

    joynr::types::DiscoveryEntry newDiscoveryEntry(localEntry);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(An<const joynr::types::GlobalDiscoveryEntry&>(),
                _, _, _, _)).Times(0);

    localCapabilitiesDirectory->add(
            newDiscoveryEntry,
            awaitGlobalRegistration,
            defaultOnSuccess,
            defaultOnError);
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddKnownLocalEntryWithDifferentExpiryDateAddsAgain) {
    const bool& awaitGlobalRegistration = false;
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);
    joynr::types::DiscoveryEntry localEntry(defaultProviderVersion,
                                        DOMAIN_1_NAME,
                                        INTERFACE_1_NAME,
                                        dummyParticipantId1,
                                        providerQos,
                                        lastSeenDateMs,
                                        expiryDateMs,
                                        PUBLIC_KEY_ID);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(An<const joynr::types::GlobalDiscoveryEntry&>(),
                _, _, _, _)).Times(0);

    localCapabilitiesDirectory->add(
            localEntry,
            awaitGlobalRegistration,
            KNOWN_GBIDS,
            createAddOnSuccessFunction(),
            discoveryOnError);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(2)));

    joynr::types::DiscoveryEntry newDiscoveryEntry(localEntry);
    newDiscoveryEntry.setExpiryDateMs(localEntry.getExpiryDateMs()+1);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(An<const joynr::types::GlobalDiscoveryEntry&>(),
                _, _, _, _)).Times(0);

    localCapabilitiesDirectory->add(
            newDiscoveryEntry,
            awaitGlobalRegistration,
            KNOWN_GBIDS,
            createAddOnSuccessFunction(),
            discoveryOnError);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(2)));
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddToAll) {
    const std::vector<std::string> expectedGbids = KNOWN_GBIDS;
    const bool& awaitGlobalRegistration = true;
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
            add(Matcher<const types::GlobalDiscoveryEntry&>(AnConvertedGlobalDiscoveryEntry(entry)),
            Eq(expectedGbids), _, _, _)).Times(1)
            .WillOnce(InvokeArgument<2>());

    localCapabilitiesDirectory->addToAll(
            entry, awaitGlobalRegistration, createAddOnSuccessFunction(), discoveryOnError);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddToAllLocal) {
    const bool& awaitGlobalRegistration = true;
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);
    joynr::types::DiscoveryEntry localEntry(defaultProviderVersion,
                                        DOMAIN_1_NAME,
                                        INTERFACE_1_NAME,
                                        dummyParticipantId1,
                                        providerQos,
                                        lastSeenDateMs,
                                        expiryDateMs,
                                        PUBLIC_KEY_ID);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
            add(An<const types::GlobalDiscoveryEntry&>(),
            _, _, _, _)).Times(0);

    localCapabilitiesDirectory->addToAll(
            localEntry,
            awaitGlobalRegistration,
            createAddOnSuccessFunction(),
            discoveryOnError);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(2)));
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddToAllIsProperlyRejected_internalError) {
    const bool& awaitGlobalRegistration = true;

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
            add(Matcher<const types::GlobalDiscoveryEntry&>(AnConvertedGlobalDiscoveryEntry(entry)),
            _, _, _, _)).Times(1)
            .WillOnce(InvokeArgument<3>(types::DiscoveryError::INTERNAL_ERROR));

    localCapabilitiesDirectory->addToAll(
            entry,
            awaitGlobalRegistration,
            defaultOnSuccess,
            createExpectedDiscoveryErrorFunction(types::DiscoveryError::INTERNAL_ERROR));

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddToAllIsProperlyRejected_invalidGbid) {
    const bool& awaitGlobalRegistration = true;

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
            add(Matcher<const types::GlobalDiscoveryEntry&>(AnConvertedGlobalDiscoveryEntry(entry)),
            _, _, _, _)).Times(1)
            .WillOnce(InvokeArgument<3>(types::DiscoveryError::INVALID_GBID));

    localCapabilitiesDirectory->addToAll(
            entry,
            awaitGlobalRegistration,
            defaultOnSuccess,
            createExpectedDiscoveryErrorFunction(types::DiscoveryError::INVALID_GBID));

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddToAllIsProperlyRejected_unknownGbid) {
    const bool& awaitGlobalRegistration = true;

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
            add(Matcher<const types::GlobalDiscoveryEntry&>(AnConvertedGlobalDiscoveryEntry(entry)),
            _, _, _, _)).Times(1)
            .WillOnce(InvokeArgument<3>(types::DiscoveryError::UNKNOWN_GBID));

    localCapabilitiesDirectory->addToAll(
            entry,
            awaitGlobalRegistration,
            defaultOnSuccess,
            createExpectedDiscoveryErrorFunction(types::DiscoveryError::UNKNOWN_GBID));

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, reregisterGlobalCapabilities)
{
    joynr::types::DiscoveryEntry entry1(defaultProviderVersion,
                                        DOMAIN_1_NAME,
                                        INTERFACE_1_NAME,
                                        dummyParticipantId1,
                                        types::ProviderQos(),
                                        lastSeenDateMs,
                                        expiryDateMs,
                                        PUBLIC_KEY_ID);

    joynr::types::DiscoveryEntry entry2(defaultProviderVersion,
                                        DOMAIN_2_NAME,
                                        INTERFACE_2_NAME,
                                        dummyParticipantId2,
                                        types::ProviderQos(),
                                        lastSeenDateMs,
                                        expiryDateMs,
                                        PUBLIC_KEY_ID);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(
                            AnConvertedGlobalDiscoveryEntry(entry1)),
                    _,
                    _,
                    _,
                    _)).Times(1);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(
                            AnConvertedGlobalDiscoveryEntry(entry2)),
                    _,
                    _,
                    _,
                    _)).Times(1);

    localCapabilitiesDirectory->add(entry1, defaultOnSuccess, defaultOnError);

    localCapabilitiesDirectory->add(entry2, defaultOnSuccess, defaultOnError);

    Mock::VerifyAndClearExpectations(globalCapabilitiesDirectoryClient.get());

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(
                            AnConvertedGlobalDiscoveryEntry(entry1)),
                    _,
                    _,
                    _,
                    _)).Times(1);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(
                            AnConvertedGlobalDiscoveryEntry(entry2)),
                    _,
                    _,
                    _,
                    _)).Times(1);

    bool onSuccessCalled = false;
    localCapabilitiesDirectory->triggerGlobalProviderReregistration(
            [&onSuccessCalled]() { onSuccessCalled = true; },
            [](const joynr::exceptions::ProviderRuntimeException&) { FAIL(); });

    Mock::VerifyAndClearExpectations(globalCapabilitiesDirectoryClient.get());
    EXPECT_TRUE(onSuccessCalled);
}

TEST_F(LocalCapabilitiesDirectoryTest,
       doNotReregisterDiscoveryEntriesFromGlobalCapabilitiesDirectory)
{
    joynr::types::DiscoveryEntry entry1(defaultProviderVersion,
                                        DOMAIN_1_NAME,
                                        INTERFACE_1_NAME,
                                        dummyParticipantId1,
                                        types::ProviderQos(),
                                        lastSeenDateMs,
                                        expiryDateMs,
                                        PUBLIC_KEY_ID);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(
                            AnConvertedGlobalDiscoveryEntry(entry1)),
                    _,
                    _,
                    _,
                    _)).Times(1);

    localCapabilitiesDirectory->add(entry1, defaultOnSuccess, defaultOnError);

    joynr::types::DiscoveryEntry entry2(defaultProviderVersion,
                                        DOMAIN_2_NAME,
                                        INTERFACE_2_NAME,
                                        dummyParticipantId2,
                                        types::ProviderQos(),
                                        lastSeenDateMs,
                                        expiryDateMs,
                                        PUBLIC_KEY_ID);

    localCapabilitiesDirectory->registerReceivedCapabilities({{EXTERNAL_ADDRESS, entry2}});

    Mock::VerifyAndClearExpectations(globalCapabilitiesDirectoryClient.get());

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(
                            AnConvertedGlobalDiscoveryEntry(entry1)),
                    _,
                    _,
                    _,
                    _)).Times(1);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(
                            AnConvertedGlobalDiscoveryEntry(entry2)),
                    _,
                    _,
                    _,
                    _)).Times(0);

    bool onSuccessCalled = false;
    localCapabilitiesDirectory->triggerGlobalProviderReregistration(
            [&onSuccessCalled]() { onSuccessCalled = true; },
            [](const joynr::exceptions::ProviderRuntimeException&) { FAIL(); });

    EXPECT_TRUE(onSuccessCalled);
}

TEST_F(LocalCapabilitiesDirectoryTest,
       reregisterGlobalCapabilities_BackendNotCalledIfNoGlobalProvidersArePresent)
{
    types::ProviderQos localProviderQos({}, 1, joynr::types::ProviderScope::LOCAL, false);
    joynr::types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantId1,
                                       localProviderQos,
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);

    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultOnError);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(_), _, _, _, _))
            .Times(0);

    bool onSuccessCalled = false;
    localCapabilitiesDirectory->triggerGlobalProviderReregistration(
            [&onSuccessCalled]() { onSuccessCalled = true; },
            [](const joynr::exceptions::ProviderRuntimeException&) { FAIL(); });

    EXPECT_TRUE(onSuccessCalled);
}

TEST_F(LocalCapabilitiesDirectoryTest, addAddsToCache)
{
    EXPECT_CALL(
            *globalCapabilitiesDirectoryClient,
            lookup(dummyParticipantId1,
                   _,
                   _,
                   A<std::function<void(
                           const std::vector<joynr::types::GlobalDiscoveryEntry>& capabilities)>>(),
                   _,
                   A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(0);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(1);

    joynr::types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantId1,
                                       types::ProviderQos(),
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);
    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultOnError);

    const types::DiscoveryQos discoveryQos;
    localCapabilitiesDirectory->lookup(dummyParticipantId1,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupParticipantIdSuccessFunction(),
                                       createUnexpectedDiscoveryErrorFunction());
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, addLocallyDoesNotCallCapabilitiesClient)
{
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                lookup(_,
                       _,
                       _,
                       A<std::function<void(const std::vector<joynr::types::GlobalDiscoveryEntry>&
                                                    discoveryEntries)>>(),
                       _,
                       A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(0);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(0);
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);

    joynr::types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantId1,
                                       providerQos,
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);
    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultOnError);

    const types::DiscoveryQos discoveryQos;
    localCapabilitiesDirectory->lookup(dummyParticipantId1,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupParticipantIdSuccessFunction(),
                                       createUnexpectedDiscoveryErrorFunction());
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupForInterfaceAddressReturnsCachedValues)
{

    // simulate global capability directory would store two entries.
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(ElementsAre(DOMAIN_1_NAME), INTERFACE_1_NAME, _, _, _, _, _))
            .Times(1)
            .WillOnce(Invoke(this, &LocalCapabilitiesDirectoryTest::fakeLookupWithResults));

    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(2),
                                       createUnexpectedDiscoveryErrorFunction());
    ASSERT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
    // enries are now in cache, globalCapabilitiesDirectoryClient should not be called.
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                lookup(_,
                       _,
                       _,
                       _,
                       A<std::function<void(const std::vector<joynr::types::GlobalDiscoveryEntry>&
                                                    discoveryEntries)>>(),
                       _,
                       A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(0);
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(2),
                                       createUnexpectedDiscoveryErrorFunction());
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupForInterfaceAddressDelegatesToCapabilitiesClient)
{
    // simulate global capability directory would store two entries.
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(ElementsAre(DOMAIN_1_NAME), INTERFACE_1_NAME, _, _, _, _, _))
            .Times(1)
            .WillOnce(Invoke(this, &LocalCapabilitiesDirectoryTest::fakeLookupWithResults));

    auto onSuccess = [this] (const std::vector<types::DiscoveryEntryWithMetaInfo>& result) {
        EXPECT_EQ(2, result.size());

        // check that the results contain the two channel ids
        bool firstParticipantIdFound = false;
        bool secondParticipantIdFound = false;
        for (std::uint16_t i = 0; i < result.size(); i++) {
            types::DiscoveryEntryWithMetaInfo entry = result.at(i);
            EXPECT_EQ(DOMAIN_1_NAME, entry.getDomain());
            EXPECT_EQ(INTERFACE_1_NAME, entry.getInterfaceName());
            std::string participantId = entry.getParticipantId();
            if (participantId == dummyParticipantId1) {
                firstParticipantIdFound = true;
            } else if (participantId == dummyParticipantId2) {
                secondParticipantIdFound = true;
            }
        }

        EXPECT_TRUE(firstParticipantIdFound);
        EXPECT_TRUE(secondParticipantIdFound);
        semaphore.notify();
    };

    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       onSuccess,
                                       createUnexpectedDiscoveryErrorFunction());

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupForParticipantIdReturnsCachedValues)
{
    EXPECT_CALL(
            *globalCapabilitiesDirectoryClient,
            lookup(_,
                   _,
                   _,
                   A<std::function<void(
                           const std::vector<types::GlobalDiscoveryEntry>& discoveryEntries)>>(),
                   _,
                   A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(1)
            .WillOnce(Invoke(
                    this, &LocalCapabilitiesDirectoryTest::fakeLookupByParticipantIdWithResult));

    types::DiscoveryQos discoveryQos;
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);
    localCapabilitiesDirectory->lookup(dummyParticipantId1,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupParticipantIdSuccessFunction(),
                                       createUnexpectedDiscoveryErrorFunction());
    ASSERT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    EXPECT_CALL(
            *globalCapabilitiesDirectoryClient,
            lookup(_,
                   _,
                   _,
                   A<std::function<void(
                           const std::vector<types::GlobalDiscoveryEntry>& discoveryEntries)>>(),
                   _,
                   A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(0);
    localCapabilitiesDirectory->lookup(dummyParticipantId1,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupParticipantIdSuccessFunction(),
                                       createUnexpectedDiscoveryErrorFunction());
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupForParticipantIdReturnsNoCapability)
{
    EXPECT_CALL(
            *globalCapabilitiesDirectoryClient,
            lookup(_,
                   _,
                   _,
                   A<std::function<void(
                           const std::vector<types::GlobalDiscoveryEntry>& discoveryEntries)>>(),
                   _,
                   A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(1)
            .WillOnce(Invoke(this, &LocalCapabilitiesDirectoryTest::fakeLookupNoEntryForParticipant));

    types::DiscoveryQos discoveryQos;
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);
    localCapabilitiesDirectory->lookup(dummyParticipantId1,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createUnexpectedLookupParticipantIdSuccessFunction(),
                                       createExpectedDiscoveryErrorFunction(types::DiscoveryError::NO_ENTRY_FOR_PARTICIPANT));
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupForParticipantIdDelegatesToCapabilitiesClient)
{

    EXPECT_CALL(
            *globalCapabilitiesDirectoryClient,
            lookup(dummyParticipantId1,
                   _,
                   _,
                   A<std::function<void(
                           const std::vector<types::GlobalDiscoveryEntry>& discoveryEntries)>>(),
                   _,
                   A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(1)
            .WillOnce(Invoke(this, &LocalCapabilitiesDirectoryTest::fakeLookupByParticipantIdWithResult));

    types::DiscoveryQos discoveryQos;
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);
    localCapabilitiesDirectory->lookup(dummyParticipantId1,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupParticipantIdSuccessFunction(),
                                       createUnexpectedDiscoveryErrorFunction());
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, clearRemovesEntries)
{
    EXPECT_CALL(
            *globalCapabilitiesDirectoryClient,
            lookup(_,
                   _,
                   _,
                   A<std::function<void(
                           const std::vector<types::GlobalDiscoveryEntry>& discoveryEntries)>>(),
                   _,
                   A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(1)
            .WillOnce(Invoke(
                    this, &LocalCapabilitiesDirectoryTest::fakeLookupByParticipantIdWithResult));

    types::DiscoveryQos discoveryQos;
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);
    localCapabilitiesDirectory->lookup(dummyParticipantId1,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupParticipantIdSuccessFunction(),
                                       createUnexpectedDiscoveryErrorFunction());
    ASSERT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    // remove all entries in the cache
    localCapabilitiesDirectory->clear();
    // retrieving capabilities will force a call to the backend as the cache is empty
    EXPECT_CALL(
            *globalCapabilitiesDirectoryClient,
            lookup(_,
                   _,
                   _,
                   A<std::function<void(
                           const std::vector<types::GlobalDiscoveryEntry>& discoveryEntries)>>(),
                   _,
                   A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(1);
    localCapabilitiesDirectory->lookup(dummyParticipantId1,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createUnexpectedLookupParticipantIdSuccessFunction(),
                                       createUnexpectedDiscoveryErrorFunction());
}

TEST_F(LocalCapabilitiesDirectoryTest, registerMultipleGlobalCapabilitiesCheckIfTheyAreMerged)
{

    types::ProviderQos qos;

    types::GlobalDiscoveryEntry globalDiscoveryEntryInfo1(defaultProviderVersion,
                                                          DOMAIN_1_NAME,
                                                          INTERFACE_1_NAME,
                                                          dummyParticipantId1,
                                                          qos,
                                                          lastSeenDateMs,
                                                          expiryDateMs,
                                                          PUBLIC_KEY_ID,
                                                          LOCAL_ADDRESS);

    types::GlobalDiscoveryEntry globalDiscoveryEntryInfo2(defaultProviderVersion,
                                                          DOMAIN_2_NAME,
                                                          INTERFACE_1_NAME,
                                                          dummyParticipantId2,
                                                          qos,
                                                          lastSeenDateMs,
                                                          expiryDateMs,
                                                          PUBLIC_KEY_ID,
                                                          LOCAL_ADDRESS);

    {
        InSequence inSequence;
        EXPECT_CALL(*globalCapabilitiesDirectoryClient, add(globalDiscoveryEntryInfo1, _, _, _, _)).Times(1);
        EXPECT_CALL(*globalCapabilitiesDirectoryClient, add(globalDiscoveryEntryInfo2, _, _, _, _)).Times(1);
    }

    joynr::types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantId1,
                                       qos,
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);
    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultOnError);
    joynr::types::DiscoveryEntry entry2(defaultProviderVersion,
                                        DOMAIN_2_NAME,
                                        INTERFACE_1_NAME,
                                        dummyParticipantId2,
                                        qos,
                                        lastSeenDateMs,
                                        expiryDateMs,
                                        PUBLIC_KEY_ID);
    localCapabilitiesDirectory->add(entry2, defaultOnSuccess, defaultOnError);
}

TEST_F(LocalCapabilitiesDirectoryTest, removeLocalCapabilityByParticipantId)
{
    types::ProviderQos qos;
    EXPECT_CALL(
            *globalCapabilitiesDirectoryClient,
            lookup(_,
                   _,
                   _,
                   A<std::function<void(
                           const std::vector<types::GlobalDiscoveryEntry>& discoveryEntries)>>(),
                   _,
                   A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(0);

    joynr::types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantId1,
                                       qos,
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);
    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultOnError);

    types::DiscoveryQos discoveryQos;
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);
    localCapabilitiesDirectory->lookup(dummyParticipantId1,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupParticipantIdSuccessFunction(),
                                       createUnexpectedDiscoveryErrorFunction());
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    localCapabilitiesDirectory->remove(dummyParticipantId1, nullptr, nullptr);

    EXPECT_CALL(
            *globalCapabilitiesDirectoryClient,
            lookup(_,
                   _,
                   _,
                   A<std::function<void(
                           const std::vector<types::GlobalDiscoveryEntry>& discoveryEntries)>>(),
                   _,
                   A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(this, &LocalCapabilitiesDirectoryTest::simulateTimeout));
    // JoynrTimeOutException timeoutException;
    EXPECT_THROW(localCapabilitiesDirectory->lookup(dummyParticipantId1,
                                                    discoveryQos,
                                                    KNOWN_GBIDS,
                                                    createLookupParticipantIdSuccessFunction(),
                                                    createUnexpectedDiscoveryErrorFunction()),
                 exceptions::JoynrTimeOutException);
}

TEST_F(LocalCapabilitiesDirectoryTest, registerLocalCapability_lookupLocal)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);

    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::LOCAL_ONLY);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(0);

    joynr::types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantId1,
                                       providerQos,
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);
    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultOnError);
    localCapabilitiesDirectory->registerReceivedCapabilities(std::move(globalCapEntryMap));

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(1),
                                       createUnexpectedDiscoveryErrorFunction());
    ASSERT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, remove(dummyParticipantId1, _, _, _, _)).Times(0);
    localCapabilitiesDirectory->remove(dummyParticipantId1, nullptr, nullptr);

    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(0),
                                       createUnexpectedDiscoveryErrorFunction());
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, registerLocalCapability_lookupLocalThenGlobal)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);

    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL);

    joynr::types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantId1,
                                       providerQos,
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(0);
    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultOnError);
    localCapabilitiesDirectory->registerReceivedCapabilities(std::move(globalCapEntryMap));

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(1),
                                       createUnexpectedDiscoveryErrorFunction());
    ASSERT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, remove(dummyParticipantId1, _, _, _, _)).Times(0);
    localCapabilitiesDirectory->remove(dummyParticipantId1, nullptr, nullptr);

    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(1),
                                       createUnexpectedDiscoveryErrorFunction());
    ASSERT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    // disable cache
    discoveryQos.setCacheMaxAge(0);
    // wait some time to be sure that the cached entry is not used
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillOnce(
            InvokeWithoutArgs(this, &LocalCapabilitiesDirectoryTest::simulateTimeout));
    EXPECT_THROW(localCapabilitiesDirectory->lookup(
                     {DOMAIN_1_NAME},
                     INTERFACE_1_NAME,
                     discoveryQos,
                     KNOWN_GBIDS,
                     createLookupSuccessFunction(1),
                     createUnexpectedDiscoveryErrorFunction()),
                 exceptions::JoynrTimeOutException);
}

TEST_F(LocalCapabilitiesDirectoryTest, registerLocalCapability_lookupLocalAndGlobal)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);

    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryTimeout(5000);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::LOCAL_AND_GLOBAL);

    joynr::types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantId1,
                                       providerQos,
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(0);
    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultOnError);
    // localCapabilitiesDirectory->registerReceivedCapabilities(globalCapEntryMap);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _))
            .Times(2)
            //            .WillOnce(InvokeWithoutArgs(this,
            //            &LocalCapabilitiesDirectoryTest::simulateTimeout));
            .WillRepeatedly(Invoke(
                    this,
                    &LocalCapabilitiesDirectoryTest::fakeLookupZeroResultsForInterfaceAddress));
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(1),
                                       createUnexpectedDiscoveryErrorFunction());
    ASSERT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, remove(dummyParticipantId1, _, _, _, _)).Times(0);
    localCapabilitiesDirectory->remove(dummyParticipantId1, nullptr, nullptr);

    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(0),
                                       createUnexpectedDiscoveryErrorFunction());
    ASSERT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    // disable cache
    discoveryQos.setCacheMaxAge(0);
    // wait some time to be sure that the cached entry is not used
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillOnce(
            Invoke(this, &LocalCapabilitiesDirectoryTest::fakeLookupWithResults));
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(2),
                                       createUnexpectedDiscoveryErrorFunction());
    ASSERT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
    localCapabilitiesDirectory->clear();

    discoveryQos.setCacheMaxAge(4000);
    localCapabilitiesDirectory->registerReceivedCapabilities(std::move(globalCapEntryMap));
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(1),
                                       createUnexpectedDiscoveryErrorFunction());
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupLocalThenGlobal_GlobalPendingLocalEntryAdded_ReturnsLocalEntry)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);

    Semaphore gcdSemaphore(0);
    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryTimeout(5000);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL);

    joynr::types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantId1,
                                       providerQos,
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(0);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillRepeatedly(
            DoAll(AcquireSemaphore(&gcdSemaphore),
                  Invoke(this, &LocalCapabilitiesDirectoryTest::fakeLookupWithResults)));

    auto thread = std::thread([&]() {
        localCapabilitiesDirectory->lookup(
                    {DOMAIN_1_NAME, DOMAIN_2_NAME},
                    INTERFACE_1_NAME,
                    discoveryQos,
                    KNOWN_GBIDS,
                    createLookupSuccessFunction(1),
                    createUnexpectedDiscoveryErrorFunction());
    });

    EXPECT_FALSE(semaphore.waitFor(std::chrono::milliseconds(100)));
    EXPECT_TRUE(localCapabilitiesDirectory->hasPendingLookups());

    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultOnError);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
    EXPECT_TRUE(localCapabilitiesDirectory->hasPendingLookups());

    // globalCapabilitiesDirectoryClient.lookup should return;
    gcdSemaphore.notify();
    thread.join();

    EXPECT_FALSE(localCapabilitiesDirectory->hasPendingLookups());
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupLocalThenGlobal_GlobalSucceedsNoLocalEntries_ReturnsGlobalEntries)
{
    Semaphore gcdSemaphore(0);
    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryTimeout(5000);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillRepeatedly(
            DoAll(AcquireSemaphore(&gcdSemaphore),
                  Invoke(this, &LocalCapabilitiesDirectoryTest::fakeLookupWithResults)));

    auto thread = std::thread([&]() {
        localCapabilitiesDirectory->lookup(
                    {DOMAIN_1_NAME, DOMAIN_2_NAME},
                    INTERFACE_1_NAME,
                    discoveryQos,
                    KNOWN_GBIDS,
                    createLookupSuccessFunction(2),
                    createUnexpectedDiscoveryErrorFunction());
    });

    EXPECT_FALSE(semaphore.waitFor(std::chrono::milliseconds(100)));
    EXPECT_TRUE(localCapabilitiesDirectory->hasPendingLookups());

    // globalCapabilitiesDirectoryClient.lookup should return;
    gcdSemaphore.notify();
    thread.join();

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
    EXPECT_FALSE(localCapabilitiesDirectory->hasPendingLookups());
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupLocalThenGlobal_GlobalFailsNoLocalEntries_ReturnsNoEntries)
{
    Semaphore gcdSemaphore(0);
    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryTimeout(5000);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillRepeatedly(
            DoAll(AcquireSemaphore(&gcdSemaphore),
                  Invoke(this, &LocalCapabilitiesDirectoryTest::fakeCapabilitiesClientLookupWithError)));

    auto thread = std::thread([&]() {
        localCapabilitiesDirectory->lookup(
                    {DOMAIN_1_NAME, DOMAIN_2_NAME},
                    INTERFACE_1_NAME,
                    discoveryQos,
                    KNOWN_GBIDS,
                    createUnexpectedLookupSuccessFunction(),
                    createExpectedDiscoveryErrorFunction(types::DiscoveryError::INTERNAL_ERROR));
    });

    EXPECT_FALSE(semaphore.waitFor(std::chrono::milliseconds(100)));
    EXPECT_TRUE(localCapabilitiesDirectory->hasPendingLookups());

    // globalCapabilitiesDirectoryClient.lookup should return;
    gcdSemaphore.notify();
    thread.join();

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
    EXPECT_FALSE(localCapabilitiesDirectory->hasPendingLookups());
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupLocalThenGlobal_LocalEntriesNoGlobalLookup_ReturnsLocalEntry)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);

    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryTimeout(5000);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL);

    joynr::types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantId1,
                                       providerQos,
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(0);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);

    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultOnError);
    localCapabilitiesDirectory->lookup(
                {DOMAIN_1_NAME, DOMAIN_2_NAME},
                INTERFACE_1_NAME,
                discoveryQos,
                KNOWN_GBIDS,
                createLookupSuccessFunction(1),
                createUnexpectedDiscoveryErrorFunction());

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
    EXPECT_FALSE(localCapabilitiesDirectory->hasPendingLookups());
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupLocalAndGlobal_GlobalSucceedsNoLocalEntries_ReturnsGlobalEntries)
{
    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryTimeout(5000);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::LOCAL_AND_GLOBAL);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillRepeatedly(
            Invoke(this, &LocalCapabilitiesDirectoryTest::fakeLookupWithResults));

    localCapabilitiesDirectory->lookup(
                {DOMAIN_1_NAME, DOMAIN_2_NAME},
                INTERFACE_1_NAME,
                discoveryQos,
                KNOWN_GBIDS,
                createLookupSuccessFunction(2),
                createUnexpectedDiscoveryErrorFunction());

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
    EXPECT_FALSE(localCapabilitiesDirectory->hasPendingLookups());
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupLocalAndGlobal_GlobalFailsNoLocalEntries_ReturnsNoEntries)
{
    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryTimeout(5000);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::LOCAL_AND_GLOBAL);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillRepeatedly(
            Invoke(this, &LocalCapabilitiesDirectoryTest::fakeCapabilitiesClientLookupWithError));

    localCapabilitiesDirectory->lookup(
                {DOMAIN_1_NAME, DOMAIN_2_NAME},
                INTERFACE_1_NAME,
                discoveryQos,
                KNOWN_GBIDS,
                createUnexpectedLookupSuccessFunction(),
                createExpectedDiscoveryErrorFunction(types::DiscoveryError::INTERNAL_ERROR));

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
    EXPECT_FALSE(localCapabilitiesDirectory->hasPendingLookups());
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupLocalAndGlobal_GlobalSucceedsLocalEntries_ReturnsLocalAndGlobalEntries)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);

    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryTimeout(5000);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::LOCAL_AND_GLOBAL);

    joynr::types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantId3,
                                       providerQos,
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(0);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillRepeatedly(
            Invoke(this, &LocalCapabilitiesDirectoryTest::fakeLookupWithResults));

    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultOnError);
    localCapabilitiesDirectory->lookup(
                {DOMAIN_1_NAME, DOMAIN_2_NAME},
                INTERFACE_1_NAME,
                discoveryQos,
                KNOWN_GBIDS,
                createLookupSuccessFunction(3),
                createUnexpectedDiscoveryErrorFunction());

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
    EXPECT_FALSE(localCapabilitiesDirectory->hasPendingLookups());
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupLocalAndGlobal_GlobalFailsLocalEntries_ReturnsNoEntries)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);

    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryTimeout(5000);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::LOCAL_AND_GLOBAL);

    joynr::types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantId1,
                                       providerQos,
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(0);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillRepeatedly(
            Invoke(this, &LocalCapabilitiesDirectoryTest::fakeCapabilitiesClientLookupWithError));

    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultOnError);
    localCapabilitiesDirectory->lookup(
                {DOMAIN_1_NAME, DOMAIN_2_NAME},
                INTERFACE_1_NAME,
                discoveryQos,
                KNOWN_GBIDS,
                createUnexpectedLookupSuccessFunction(),
                createExpectedDiscoveryErrorFunction(types::DiscoveryError::INTERNAL_ERROR));

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
    EXPECT_FALSE(localCapabilitiesDirectory->hasPendingLookups());
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupGlobalOnly_GlobalSucceedsNoLocalEntries_ReturnsGlobalEntries)
{
    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryTimeout(5000);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::GLOBAL_ONLY);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillRepeatedly(
            Invoke(this, &LocalCapabilitiesDirectoryTest::fakeLookupWithResults));

    localCapabilitiesDirectory->lookup(
                {DOMAIN_1_NAME, DOMAIN_2_NAME},
                INTERFACE_1_NAME,
                discoveryQos,
                KNOWN_GBIDS,
                createLookupSuccessFunction(2),
                createUnexpectedDiscoveryErrorFunction());

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
    EXPECT_FALSE(localCapabilitiesDirectory->hasPendingLookups());
}

TEST_F(LocalCapabilitiesDirectoryTest, removeGlobalExpiredEntires_ReturnNonExpiredGlobalEntries)
{
    // add a few entries
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);

    expiryDateMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::system_clock::now().time_since_epoch()).count() +
                   purgeExpiredDiscoveryEntriesIntervalMs;

    std::int64_t longerExpiryDateMs =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count() +
            purgeExpiredDiscoveryEntriesIntervalMs * 10;

    joynr::types::DiscoveryEntry entry1(defaultProviderVersion,
                                        DOMAIN_1_NAME,
                                        INTERFACE_1_NAME,
                                        dummyParticipantId1,
                                        providerQos,
                                        lastSeenDateMs,
                                        longerExpiryDateMs,
                                        PUBLIC_KEY_ID);

    joynr::types::DiscoveryEntry entry2(defaultProviderVersion,
                                        DOMAIN_1_NAME,
                                        INTERFACE_1_NAME,
                                        dummyParticipantId2,
                                        providerQos,
                                        lastSeenDateMs,
                                        expiryDateMs,
                                        PUBLIC_KEY_ID);

    joynr::types::DiscoveryEntry entry3(defaultProviderVersion,
                                        DOMAIN_1_NAME,
                                        INTERFACE_3_NAME,
                                        dummyParticipantId3,
                                        providerQos,
                                        lastSeenDateMs,
                                        expiryDateMs,
                                        PUBLIC_KEY_ID);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(3);

    localCapabilitiesDirectory->add(entry1, defaultOnSuccess, defaultOnError);

    localCapabilitiesDirectory->add(entry2, defaultOnSuccess, defaultOnError);

    localCapabilitiesDirectory->add(entry3, defaultOnSuccess, defaultOnError);

    // Check Cached Global Discovery Entries
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);

    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryTimeout(10000);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::GLOBAL_ONLY);

    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(2),
                                       createUnexpectedDiscoveryErrorFunction());

    ASSERT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_3_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(1),
                                       createUnexpectedDiscoveryErrorFunction());
    ASSERT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    // wait for cleanup timer to run
    std::this_thread::sleep_for(
            std::chrono::milliseconds(purgeExpiredDiscoveryEntriesIntervalMs * 2));

    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(1),
                                       createUnexpectedDiscoveryErrorFunction());

    ASSERT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    Mock::VerifyAndClearExpectations(globalCapabilitiesDirectoryClient.get());
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _))
            .Times(1)
            .WillOnce(Invoke(this, &LocalCapabilitiesDirectoryTest::fakeLookupZeroResultsForInterfaceAddress));
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_3_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(0),
                                       createUnexpectedDiscoveryErrorFunction());
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupGlobalOnly_GlobalFailsNoLocalEntries_ReturnsNoEntries)
{
    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryTimeout(5000);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::GLOBAL_ONLY);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillRepeatedly(
            Invoke(this, &LocalCapabilitiesDirectoryTest::fakeCapabilitiesClientLookupWithError));

    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME, DOMAIN_2_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createUnexpectedLookupSuccessFunction(),
                                       createExpectedDiscoveryErrorFunction(types::DiscoveryError::INTERNAL_ERROR));
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
    EXPECT_FALSE(localCapabilitiesDirectory->hasPendingLookups());
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupGlobalOnly_GlobalSucceedsLocalEntries_ReturnsGlobalEntries)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);

    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryTimeout(5000);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::GLOBAL_ONLY);

    joynr::types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantId1,
                                       providerQos,
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(0);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillRepeatedly(
            Invoke(this, &LocalCapabilitiesDirectoryTest::fakeLookupWithResults));

    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultOnError);
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME, DOMAIN_2_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(2),
                                       createUnexpectedDiscoveryErrorFunction());

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
    EXPECT_FALSE(localCapabilitiesDirectory->hasPendingLookups());
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupGlobalOnly_GlobalFailsLocalEntries_ReturnsNoEntries)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);

    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryTimeout(5000);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::GLOBAL_ONLY);

    joynr::types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantId1,
                                       providerQos,
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(0);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillRepeatedly(
            Invoke(this, &LocalCapabilitiesDirectoryTest::fakeCapabilitiesClientLookupWithError));

    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultOnError);
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME, DOMAIN_2_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createUnexpectedLookupSuccessFunction(),
                                       createExpectedDiscoveryErrorFunction(types::DiscoveryError::INTERNAL_ERROR));

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
    EXPECT_FALSE(localCapabilitiesDirectory->hasPendingLookups());
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupMultipeDomainsReturnsResultForMultipleDomains)
{
    std::string multipleDomainName1 = "multipleDomainName1";
    std::string multipleDomainName2 = "multipleDomainName2";
    std::string multipleDomainName3 = "multipleDomainName3";
    std::string multipleDomainName1ParticipantId = util::createUuid();
    std::string multipleDomainName2ParticipantId = util::createUuid();
    std::string multipleDomainName3ParticipantId1 = util::createUuid();
    std::string multipleDomainName3ParticipantId2 = util::createUuid();
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);

    joynr::types::DiscoveryEntry entry1(defaultProviderVersion,
                                        multipleDomainName1,
                                        INTERFACE_1_NAME,
                                        multipleDomainName1ParticipantId,
                                        providerQos,
                                        lastSeenDateMs,
                                        expiryDateMs,
                                        PUBLIC_KEY_ID);
    joynr::types::DiscoveryEntry entry2(defaultProviderVersion,
                                        multipleDomainName2,
                                        INTERFACE_1_NAME,
                                        multipleDomainName2ParticipantId,
                                        providerQos,
                                        lastSeenDateMs,
                                        expiryDateMs,
                                        PUBLIC_KEY_ID);
    joynr::types::DiscoveryEntry entry31(defaultProviderVersion,
                                         multipleDomainName3,
                                         INTERFACE_1_NAME,
                                         multipleDomainName3ParticipantId1,
                                         providerQos,
                                         lastSeenDateMs,
                                         expiryDateMs,
                                         PUBLIC_KEY_ID);
    joynr::types::DiscoveryEntry entry32(defaultProviderVersion,
                                         multipleDomainName3,
                                         INTERFACE_1_NAME,
                                         multipleDomainName3ParticipantId2,
                                         providerQos,
                                         lastSeenDateMs,
                                         expiryDateMs,
                                         PUBLIC_KEY_ID);
    localCapabilitiesDirectory->add(entry1, defaultOnSuccess, defaultOnError);
    localCapabilitiesDirectory->add(entry2, defaultOnSuccess, defaultOnError);
    localCapabilitiesDirectory->add(entry31, defaultOnSuccess, defaultOnError);
    localCapabilitiesDirectory->add(entry32, defaultOnSuccess, defaultOnError);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                lookup(_,
                       _,
                       _,
                       A<std::function<void(const std::vector<joynr::types::GlobalDiscoveryEntry>&
                                                    discoveryEntries)>>(),
                       _,
                       A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(0);

    std::vector<std::string> domains = {
            multipleDomainName1, multipleDomainName2, multipleDomainName3};

    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::LOCAL_ONLY);
    localCapabilitiesDirectory->lookup(domains,
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(4),
                                       createUnexpectedDiscoveryErrorFunction());
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, registerLocalCapability_lookupGlobalOnly)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);

    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::GLOBAL_ONLY);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(0);

    joynr::types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantId1,
                                       providerQos,
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);
    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultOnError);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillOnce(
            InvokeWithoutArgs(this, &LocalCapabilitiesDirectoryTest::simulateTimeout));
    // JoynrTimeOutException timeoutException;
    EXPECT_THROW(localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                                    INTERFACE_1_NAME,
                                                    discoveryQos,
                                                    KNOWN_GBIDS,
                                                    createUnexpectedLookupSuccessFunction(),
                                                    createUnexpectedDiscoveryErrorFunction()),
                 exceptions::JoynrTimeOutException);

    // register the external capability
    localCapabilitiesDirectory->registerReceivedCapabilities(std::move(globalCapEntryMap));
    // get the global entry
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(1),
                                       createUnexpectedDiscoveryErrorFunction());
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, remove(dummyParticipantId1, _, _, _, _)).Times(0);
    localCapabilitiesDirectory->remove(dummyParticipantId1, nullptr, nullptr);

    // disable cache
    discoveryQos.setCacheMaxAge(0);
    // wait some time to be sure that the cached entry is not used
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillOnce(
            InvokeWithoutArgs(this, &LocalCapabilitiesDirectoryTest::simulateTimeout));
    EXPECT_THROW(localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                                    INTERFACE_1_NAME,
                                                    discoveryQos,
                                                    KNOWN_GBIDS,
                                                    createUnexpectedLookupSuccessFunction(),
                                                    createUnexpectedDiscoveryErrorFunction()),
                 exceptions::JoynrTimeOutException);
}

TEST_F(LocalCapabilitiesDirectoryTest, registerGlobalCapability_lookupLocal)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);

    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::LOCAL_ONLY);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(1);

    joynr::types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantId1,
                                       providerQos,
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);
    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultOnError);
    localCapabilitiesDirectory->registerReceivedCapabilities(std::move(globalCapEntryMap));

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(1),
                                       createUnexpectedDiscoveryErrorFunction());
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, remove(dummyParticipantId1, _, _, _, _)).Times(1);
    localCapabilitiesDirectory->remove(dummyParticipantId1, nullptr, nullptr);
}

TEST_F(LocalCapabilitiesDirectoryTest, registerGlobalCapability_lookupLocalThenGlobal)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);

    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(100);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(1);

    joynr::types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantId1,
                                       providerQos,
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);
    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultOnError);
    localCapabilitiesDirectory->registerReceivedCapabilities(std::move(globalCapEntryMap));

    // get the local entry
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(1),
                                       createUnexpectedDiscoveryErrorFunction());
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, remove(dummyParticipantId1, _, _, _, _)).Times(1);
    localCapabilitiesDirectory->remove(dummyParticipantId1, nullptr, nullptr);

    // get the global entry
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(1),
                                       createUnexpectedDiscoveryErrorFunction());
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    // wait for cleanup timer to run
    std::this_thread::sleep_for(
            std::chrono::milliseconds(purgeExpiredDiscoveryEntriesIntervalMs * 2));

    // get the global, but timeout occured
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillOnce(
            InvokeWithoutArgs(this, &LocalCapabilitiesDirectoryTest::simulateTimeout));
    EXPECT_THROW(localCapabilitiesDirectory->lookup(
                     {DOMAIN_1_NAME},
                     INTERFACE_1_NAME,
                     discoveryQos,
                     KNOWN_GBIDS,
                     createUnexpectedLookupSuccessFunction(),
                     createUnexpectedDiscoveryErrorFunction()),
                 exceptions::JoynrTimeOutException);
}

TEST_F(LocalCapabilitiesDirectoryTest, registerCachedGlobalCapability_lookupGlobalOnly)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);

    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(100);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::GLOBAL_ONLY);

    // JoynrTimeOutException timeoutException;
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(1);

    joynr::types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantId1,
                                       providerQos,
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);
    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultOnError);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(1),
                                       createUnexpectedDiscoveryErrorFunction());
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    // recieve a global entry
    localCapabilitiesDirectory->registerReceivedCapabilities(std::move(globalCapEntryMap));
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(2),
                                       createUnexpectedDiscoveryErrorFunction());
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, remove(dummyParticipantId1, _, _, _, _)).Times(1);
    localCapabilitiesDirectory->remove(dummyParticipantId1, nullptr, nullptr);
}

// TODO test remove global capability
TEST_F(LocalCapabilitiesDirectoryTest, registerReceivedCapabilites_registerMqttAddress)
{
    const std::string addressType = "mqtt";
    const std::string topic = "mqtt_TEST_channelId";
    system::RoutingTypes::MqttAddress mqttAddress("brokerUri", topic);
    registerReceivedCapabilities(addressType, joynr::serializer::serializeToJson(mqttAddress));
}

TEST_F(LocalCapabilitiesDirectoryTest, registerReceivedCapabilites_registerHttpAddress)
{
    const std::string addressType = "http";
    const std::string channelID = "TEST_channelId";
    const std::string endPointUrl = "TEST_endPointUrl";
    const system::RoutingTypes::ChannelAddress channelAddress(endPointUrl, channelID);
    registerReceivedCapabilities(addressType, joynr::serializer::serializeToJson(channelAddress));
}

TEST_F(LocalCapabilitiesDirectoryTest, persistencyTest)
{
    // Attempt loading (action usually performed by cluster-controller runtime)
    localCapabilitiesDirectory->loadPersistedFile();

    // add few entries
    const std::string DOMAIN_NAME = "LocalCapabilitiesDirectorySerializerTest_Domain";
    const std::string INTERFACE_NAME = "LocalCapabilitiesDirectorySerializerTest_InterfaceName";

    std::vector<std::string> participantIds{
            util::createUuid(), util::createUuid(), util::createUuid()};

    types::ProviderQos localProviderQos;
    localProviderQos.setScope(types::ProviderScope::LOCAL);
    types::ProviderQos globalProviderQos;
    globalProviderQos.setScope(types::ProviderScope::GLOBAL);

    const joynr::types::DiscoveryEntry entry1(defaultProviderVersion,
                                              DOMAIN_NAME,
                                              INTERFACE_NAME,
                                              participantIds[0],
                                              localProviderQos,
                                              lastSeenDateMs,
                                              expiryDateMs,
                                              PUBLIC_KEY_ID);
    const joynr::types::DiscoveryEntry entry2(defaultProviderVersion,
                                              DOMAIN_NAME,
                                              INTERFACE_NAME,
                                              participantIds[1],
                                              globalProviderQos,
                                              lastSeenDateMs,
                                              expiryDateMs,
                                              PUBLIC_KEY_ID);
    const joynr::types::DiscoveryEntry entry3(defaultProviderVersion,
                                              DOMAIN_NAME,
                                              INTERFACE_NAME,
                                              participantIds[2],
                                              globalProviderQos,
                                              lastSeenDateMs,
                                              expiryDateMs,
                                              PUBLIC_KEY_ID);

    localCapabilitiesDirectory->add(entry1, defaultOnSuccess, defaultOnError);
    localCapabilitiesDirectory->add(entry2, defaultOnSuccess, defaultOnError);
    localCapabilitiesDirectory->add(entry3,
                                    false,
                                    {KNOWN_GBIDS[1], KNOWN_GBIDS[2]},
                                    defaultOnSuccess,
                                    createUnexpectedDiscoveryErrorFunction());

    // create a new object
    auto localCapabilitiesDirectory2 =
            std::make_shared<LocalCapabilitiesDirectory>(clusterControllerSettings,
                                                         globalCapabilitiesDirectoryClient,
                                                         LOCAL_ADDRESS,
                                                         mockMessageRouter,
                                                         singleThreadedIOService->getIOService(),
                                                         "clusterControllerId",
                                                         KNOWN_GBIDS);
    localCapabilitiesDirectory2->init();

    // load persistency
    localCapabilitiesDirectory2->loadPersistedFile();

    // check all entries are there
    types::DiscoveryQos discoveryQos;
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_ONLY);
    for (auto& participantID : participantIds) {
        localCapabilitiesDirectory2->lookup(participantID,
                                            discoveryQos,
                                            KNOWN_GBIDS,
                                            createLookupParticipantIdSuccessFunction(),
                                            createUnexpectedDiscoveryErrorFunction());
        EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
    }

    auto cachedGlobalDiscoveryEntries = localCapabilitiesDirectory2->getCachedGlobalDiscoveryEntries();
    EXPECT_EQ(2, cachedGlobalDiscoveryEntries.size());
    EXPECT_EQ(entry2, cachedGlobalDiscoveryEntries[0]);
    EXPECT_EQ(entry3, cachedGlobalDiscoveryEntries[1]);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _))
            .WillRepeatedly(InvokeArgument<4>(types::DiscoveryError::INTERNAL_ERROR));
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);

    // check entry2 is registered only in first backend
    localCapabilitiesDirectory2->lookup(participantIds[1],
                                        discoveryQos,
                                        {KNOWN_GBIDS[0]},
                                        createLookupParticipantIdSuccessFunction(),
                                        createUnexpectedDiscoveryErrorFunction());
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    localCapabilitiesDirectory2->lookup(participantIds[1],
                                        discoveryQos,
                                        {KNOWN_GBIDS[1], KNOWN_GBIDS[2]},
                                        createUnexpectedLookupParticipantIdSuccessFunction(),
                                        createExpectedDiscoveryErrorFunction(types::DiscoveryError::INTERNAL_ERROR));
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    // check entry3 is registered only in backend 2 and backend 3
    localCapabilitiesDirectory2->lookup(participantIds[2],
                                        discoveryQos,
                                        {KNOWN_GBIDS[0]},
                                        createUnexpectedLookupParticipantIdSuccessFunction(),
                                        createExpectedDiscoveryErrorFunction(types::DiscoveryError::INTERNAL_ERROR));
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    localCapabilitiesDirectory2->lookup(participantIds[2],
                                        discoveryQos,
                                        {KNOWN_GBIDS[1]},
                                        createLookupParticipantIdSuccessFunction(),
                                        createUnexpectedDiscoveryErrorFunction());
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    localCapabilitiesDirectory2->lookup(participantIds[2],
                                        discoveryQos,
                                        {KNOWN_GBIDS[2]},
                                        createLookupParticipantIdSuccessFunction(),
                                        createUnexpectedDiscoveryErrorFunction());
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, loadCapabilitiesFromFile)
{
    const std::string fileName = "test-resources/ListOfCapabilitiesToInject.json";
    localCapabilitiesDirectory->injectGlobalCapabilitiesFromFile(fileName);

    // Verify that all entries present in the file have indeed been loaded
    types::DiscoveryQos discoveryQos;
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);
    localCapabilitiesDirectory->lookup("notReachableInterface_Schroedinger",
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupParticipantIdSuccessFunction(),
                                       createUnexpectedDiscoveryErrorFunction());
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    localCapabilitiesDirectory->lookup("notReachableInterface_Heisenberg",
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupParticipantIdSuccessFunction(),
                                       createUnexpectedDiscoveryErrorFunction());
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, throwExceptionOnEmptyDomainsVector)
{
    const std::vector<std::string> zeroDomains = {};
    const std::vector<std::string> twoDomains = {DOMAIN_1_NAME, DOMAIN_2_NAME};
    MockCallback<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>> mockCallback;
    auto onSuccess = std::bind(
            &MockCallback<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>::onSuccess,
            &mockCallback,
            std::placeholders::_1);
    auto onError =
            std::bind(&MockCallback<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>::onError,
                      &mockCallback,
                      std::placeholders::_1);

    EXPECT_CALL(mockCallback, onError(_)).Times(0);
    EXPECT_CALL(mockCallback, onSuccess(_)).Times(0);
    EXPECT_THROW(localCapabilitiesDirectory->lookup(
                     zeroDomains,
                     INTERFACE_1_NAME,
                     discoveryQos,
                     onSuccess,
                     onError),
                 exceptions::ProviderRuntimeException);

    EXPECT_CALL(mockCallback, onError(_)).Times(0);
    EXPECT_CALL(mockCallback, onSuccess(_)).Times(0);
    EXPECT_NO_THROW(localCapabilitiesDirectory->lookup(
                     twoDomains,
                     INTERFACE_1_NAME,
                     discoveryQos,
                     onSuccess,
                     onError));
}

TEST_F(LocalCapabilitiesDirectoryTest, localAndGlobalDoesNotReturnDuplicateEntriesCacheEnabled)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);

    joynr::types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantId1,
                                       providerQos,
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);
    const joynr::types::DiscoveryEntryWithMetaInfo expectedEntry = util::convert(true, entry);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(1);

    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultOnError);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);

    auto onSuccess = [this, expectedEntry] (const std::vector<types::DiscoveryEntryWithMetaInfo>& results) {
        EXPECT_EQ(1, results.size());
        const joynr::types::DiscoveryEntryWithMetaInfo result = results.at(0);
        EXPECT_EQ(expectedEntry, result);
        semaphore.notify();
    };

    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryTimeout(5000);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::LOCAL_AND_GLOBAL);
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       onSuccess,
                                       createUnexpectedDiscoveryErrorFunction());
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, localAndGlobalDoesNotReturnDuplicateEntriesCacheDisabled)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);
    joynr::types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantId1,
                                       providerQos,
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);
    const joynr::types::DiscoveryEntryWithMetaInfo expectedEntry = util::convert(true, entry);
    const joynr::types::GlobalDiscoveryEntry globalEntry =
            joynr::types::GlobalDiscoveryEntry(entry.getProviderVersion(),
                                               entry.getDomain(),
                                               entry.getInterfaceName(),
                                               entry.getParticipantId(),
                                               entry.getQos(),
                                               entry.getLastSeenDateMs(),
                                               entry.getExpiryDateMs(),
                                               entry.getPublicKeyId(),
                                               EXTERNAL_ADDRESS);
    const std::vector<types::GlobalDiscoveryEntry> globalEntryVec = {globalEntry};

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(1);

    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultOnError);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillOnce(
            InvokeArgument<4>(globalEntryVec));

    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(0);
    discoveryQos.setDiscoveryTimeout(5000);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::LOCAL_AND_GLOBAL);

    // wait some time to be sure that the cached entry is not used
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    auto onSuccess = [this, expectedEntry] (const std::vector<types::DiscoveryEntryWithMetaInfo>& results) {
        EXPECT_EQ(1, results.size());
        const joynr::types::DiscoveryEntryWithMetaInfo result = results.at(0);
        EXPECT_EQ(expectedEntry, result);
        semaphore.notify();
    };
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       onSuccess,
                                       createUnexpectedDiscoveryErrorFunction());
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, callTouchPeriodically)
{
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, touch(_, _, _)).Times(0);
    Mock::VerifyAndClearExpectations(globalCapabilitiesDirectoryClient.get());
    Semaphore gcdSemaphore(0);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, touch(Eq(clusterControllerId), _, _)).Times(2).WillRepeatedly(
            ReleaseSemaphore(&gcdSemaphore));
    EXPECT_TRUE(gcdSemaphore.waitFor(std::chrono::milliseconds(250)));
    EXPECT_TRUE(gcdSemaphore.waitFor(std::chrono::milliseconds(250)));
}

TEST_F(LocalCapabilitiesDirectoryTest, addMultipleTimesSameProviderAwaitForGlobal)
{
    types::ProviderQos qos;
    qos.setScope(joynr::types::ProviderScope::GLOBAL);
    const joynr::types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantId1,
                                       qos,
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);

    // trigger a failure on the first call and a success on the second

    // 1st call
    joynr::Semaphore gcdSemaphore(0);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, add(An<const types::GlobalDiscoveryEntry&>(), _, _, _, _))
            .WillOnce(DoAll(
                          Invoke(this, &LocalCapabilitiesDirectoryTest::fakeCapabilitiesClientAddWithError),
                          ReleaseSemaphore(&gcdSemaphore)
                      )); // invoke onError
    localCapabilitiesDirectory->add(entry, true, defaultOnSuccess, defaultOnError);

    // wait for it...
    EXPECT_TRUE(gcdSemaphore.waitFor(std::chrono::seconds(1)));

    // 2nd call
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, add(An<const types::GlobalDiscoveryEntry&>(), _, _, _, _))
            .WillOnce(DoAll(
                          Invoke(this, &LocalCapabilitiesDirectoryTest::fakeCapabilitiesClientAddSuccess),
                          ReleaseSemaphore(&gcdSemaphore)
                      )); // invoke onSuccess
    localCapabilitiesDirectory->add(entry, true, defaultOnSuccess, defaultOnError);

    // wait for it...
    EXPECT_TRUE(gcdSemaphore.waitFor(std::chrono::seconds(1)));

    // do a lookup to make sure the entry still exists
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(1),
                                       createUnexpectedDiscoveryErrorFunction());
}

class LocalCapabilitiesDirectoryACMockTest
        : public LocalCapabilitiesDirectoryTest,
          public ::testing::WithParamInterface<std::tuple<bool, bool>>
{
public:
    LocalCapabilitiesDirectoryACMockTest()
            : ENABLE_ACCESS_CONTROL(std::get<0>(GetParam())),
              HAS_PERMISSION(std::get<1>(GetParam()))
    {
        clusterControllerSettings.setEnableAccessController(ENABLE_ACCESS_CONTROL);
    }

protected:
    const bool ENABLE_ACCESS_CONTROL;
    const bool HAS_PERMISSION;
};

TEST_P(LocalCapabilitiesDirectoryACMockTest, checkPermissionToRegisterWithMock)
{
    auto mockAccessController = std::make_shared<MockAccessController>();
    ON_CALL(*mockAccessController, hasProviderPermission(_, _, _, _))
            .WillByDefault(Return(this->HAS_PERMISSION));

    localCapabilitiesDirectory->setAccessController(util::as_weak_ptr(mockAccessController));

    joynr::types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantId1,
                                       types::ProviderQos(),
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);

    try {
        localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultOnError);
    } catch (const exceptions::ProviderRuntimeException&) {
    }

    const types::DiscoveryQos discoveryQos;
    if (!this->ENABLE_ACCESS_CONTROL || this->HAS_PERMISSION) {
        localCapabilitiesDirectory->lookup(dummyParticipantId1,
                                           discoveryQos,
                                           KNOWN_GBIDS,
                                           createLookupParticipantIdSuccessFunction(),
                                           createUnexpectedDiscoveryErrorFunction());
    } else {
        localCapabilitiesDirectory->lookup(dummyParticipantId1,
                                           discoveryQos,
                                           KNOWN_GBIDS,
                                           createUnexpectedLookupParticipantIdSuccessFunction(),
                                           createExpectedDiscoveryErrorFunction(types::DiscoveryError::NO_ENTRY_FOR_PARTICIPANT));
    }
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

std::tuple<bool, bool> const LCDWithAC_UseCases[] = {
        // Access controller enabled/disabled: tuple[0]
        // Emulation of "Has and not have" permission: tuple[1]
        make_tuple(false, false),
        make_tuple(false, true),
        make_tuple(true, false),
        make_tuple(true, true)};

INSTANTIATE_TEST_CASE_P(WithAC,
                        LocalCapabilitiesDirectoryACMockTest,
                        ::testing::ValuesIn(LCDWithAC_UseCases));

class LocalCapabilitiesDirectoryACTest : public LocalCapabilitiesDirectoryTest
{
public:
    LocalCapabilitiesDirectoryACTest()
    {
        clusterControllerSettings.setEnableAccessController(true);
        auto localDomainAccessStore = std::make_shared<joynr::LocalDomainAccessStore>(
                "test-resources/LDAS_checkPermissionToAdd.json");
        auto localDomainAccessController =
                std::make_shared<joynr::LocalDomainAccessController>(localDomainAccessStore, true);
        accessController = std::make_shared<joynr::AccessController>(
                localCapabilitiesDirectory, localDomainAccessController, KNOWN_GBIDS);
        localCapabilitiesDirectory->setAccessController(util::as_weak_ptr(accessController));

        localDomainAccessStore->logContent();
    }

protected:
    std::shared_ptr<IAccessController> accessController;
};

TEST_F(LocalCapabilitiesDirectoryACTest, checkPermissionToAdd)
{
    joynr::types::DiscoveryEntry OK_entry(defaultProviderVersion,
                                          "domain-1234",
                                          "my/favourite/interface/Name",
                                          dummyParticipantId1,
                                          types::ProviderQos(),
                                          lastSeenDateMs,
                                          expiryDateMs,
                                          PUBLIC_KEY_ID);

    joynr::types::DiscoveryEntry NOT_OK_entry_1(
            defaultProviderVersion,
            "domain-1234",                // domain is OK
            "my/favourite/interface/Nam", // interfaceName is a substring of the allowed one
            dummyParticipantId1,
            types::ProviderQos(),
            lastSeenDateMs,
            expiryDateMs,
            PUBLIC_KEY_ID);

    joynr::types::DiscoveryEntry NOT_OK_entry_2(
            defaultProviderVersion,
            "domain-123",                  // domain is a substring of the allowed one
            "my/favourite/interface/Name", // interfaceName is OK
            dummyParticipantId1,
            types::ProviderQos(),
            lastSeenDateMs,
            expiryDateMs,
            PUBLIC_KEY_ID);

    std::string principal = "testUser";
    CallContext callContext;
    callContext.setPrincipal(principal);
    CallContextStorage::set(std::move(callContext));

    localCapabilitiesDirectory->add(
                     OK_entry,
                     []() { SUCCEED() << "OK"; },
                     [](const joynr::exceptions::ProviderRuntimeException& ex) {
                         FAIL() << ex.getMessage();
                     });

    EXPECT_THROW(localCapabilitiesDirectory->add(
                     NOT_OK_entry_1,
                     []() { FAIL(); },
                     [](const joynr::exceptions::ProviderRuntimeException& ex) {
                         FAIL() << ex.getMessage();
                     }),
                 exceptions::ProviderRuntimeException);

    EXPECT_THROW(localCapabilitiesDirectory->add(
                     NOT_OK_entry_2,
                     []() { FAIL(); },
                     [](const joynr::exceptions::ProviderRuntimeException& ex) {
                         FAIL() << ex.getMessage();
                     }),
                 exceptions::ProviderRuntimeException);

    CallContextStorage::invalidate();
}

class LocalCapabilitiesDirectoryWithProviderScope
        : public LocalCapabilitiesDirectoryTest,
          public ::testing::WithParamInterface<types::ProviderScope::Enum>
{
};

TEST_P(LocalCapabilitiesDirectoryWithProviderScope, purgeTimedOutEntries)
{
    types::ProviderQos providerQos;
    providerQos.setScope(GetParam());

    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::LOCAL_ONLY);

    EXPECT_CALL(
            *globalCapabilitiesDirectoryClient, add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(_), _, _, _, _))
            .Times(GetParam() == joynr::types::ProviderScope::LOCAL ? 0 : 1);

    joynr::types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantId1,
                                       providerQos,
                                       lastSeenDateMs,
                                       10,
                                       PUBLIC_KEY_ID);
    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultOnError);
    localCapabilitiesDirectory->registerReceivedCapabilities(std::move(globalCapEntryMap));

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(1),
                                       createUnexpectedDiscoveryErrorFunction());
    ASSERT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    // wait for cleanup timer to run
    std::this_thread::sleep_for(
            std::chrono::milliseconds(purgeExpiredDiscoveryEntriesIntervalMs * 2));

    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(0),
                                       createUnexpectedDiscoveryErrorFunction());
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_P(LocalCapabilitiesDirectoryWithProviderScope,
       registerCapabilitiesMultipleTimesDoesNotDuplicate)
{
    types::ProviderQos providerQos;
    providerQos.setScope(GetParam());

    const int numberOfDuplicatedEntriesToAdd = 3;
    const bool testingGlobalScope = GetParam() == types::ProviderScope::GLOBAL;
    if (testingGlobalScope) {
        // simulate capabilities client cannot connect to global directory
        EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                    add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(_), _, _, _, _))
                .Times(numberOfDuplicatedEntriesToAdd)
                .WillRepeatedly(Invoke(this, &LocalCapabilitiesDirectoryTest::fakeCapabilitiesClientAddSuccess));
    }

    for (int i = 0; i < numberOfDuplicatedEntriesToAdd; ++i) {
        // change expiryDate and lastSeen so that entries are not exactly equal
        lastSeenDateMs++;
        expiryDateMs++;

        joynr::types::DiscoveryEntry entry(defaultProviderVersion,
                                           DOMAIN_1_NAME,
                                           INTERFACE_1_NAME,
                                           dummyParticipantId1,
                                           providerQos,
                                           lastSeenDateMs,
                                           expiryDateMs,
                                           PUBLIC_KEY_ID);
            localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultOnError);
    }

    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(1),
                                       createUnexpectedDiscoveryErrorFunction());
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

INSTANTIATE_TEST_CASE_P(changeProviderScope,
                        LocalCapabilitiesDirectoryWithProviderScope,
                        ::testing::Values(types::ProviderScope::LOCAL,
                                          types::ProviderScope::GLOBAL));
