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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "../../libjoynrclustercontroller/access-control/LocalDomainAccessStore.h"
#include "../../libjoynrclustercontroller/access-control/LocalDomainAccessController.h"
#include "../../libjoynrclustercontroller/access-control/AccessController.h"

#include "joynr/CallContext.h"
#include "joynr/CallContextStorage.h"
#include "joynr/CapabilityUtils.h"
#include "joynr/LocalCapabilitiesDirectory.h"
#include "joynr/ClusterControllerDirectories.h"
#include "joynr/ClusterControllerSettings.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/Logger.h"
#include "joynr/LibjoynrSettings.h"
#include "joynr/types/Version.h"
#include "joynr/Semaphore.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/Settings.h"

#include "tests/JoynrTest.h"
#include "tests/mock/MockCallback.h"
#include "tests/mock/MockLocalCapabilitiesDirectoryCallback.h"
#include "tests/mock/MockMessageRouter.h"
#include "tests/mock/MockCapabilitiesClient.h"
#include "tests/mock/MockAccessController.h"

using ::testing::Property;
using ::testing::WhenDynamicCastTo;
using ::testing::ElementsAre;
using namespace ::testing;
using namespace joynr;

class LocalCapabilitiesDirectoryTest : public ::testing::Test
{
public:
    LocalCapabilitiesDirectoryTest()
            : settings(),
              clusterControllerSettings(settings),
              purgeExpiredDiscoveryEntriesIntervalMs(100),
              capabilitiesClient(std::make_shared<MockCapabilitiesClient>()),
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
              callback(),
              defaultOnSuccess([]() {}),
              defaultOnError([](const joynr::exceptions::ProviderRuntimeException&) {}),
              defaultProviderVersion(26, 05)
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
                capabilitiesClient,
                LOCAL_ADDRESS,
                mockMessageRouter,
                singleThreadedIOService->getIOService(),
                clusterControllerId);
        localCapabilitiesDirectory->init();
    }

    ~LocalCapabilitiesDirectoryTest()
    {
        singleThreadedIOService->stop();
        localCapabilitiesDirectory.reset();

        joynr::test::util::removeFileInCurrentDirectory(".*\\.settings");
        joynr::test::util::removeFileInCurrentDirectory(".*\\.persist");
    }

    void SetUp()
    {

        // TODO the participantId should be provided by the provider
        dummyParticipantId1 = util::createUuid();
        dummyParticipantId2 = util::createUuid();
        dummyParticipantId3 = util::createUuid();
        callback = std::make_shared<MockLocalCapabilitiesDirectoryCallback>();
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
    }

    void fakeLookupZeroResultsForInterfaceAddress(
            const std::vector<std::string>& domains,
            const std::string& interfaceName,
            const std::int64_t messagingTtl,
            std::function<void(const std::vector<types::GlobalDiscoveryEntry>& capabilities)>
                    onSuccess,
            std::function<void(const exceptions::JoynrRuntimeException& error)> onError)
    {
        std::ignore = domains;
        std::ignore = interfaceName;
        std::ignore = messagingTtl;
        std::ignore = onError;
        std::vector<types::GlobalDiscoveryEntry> result;
        onSuccess(result);
    }

    void fakeLookupZeroResults(
            const std::string& participantId,
            std::function<void(const std::vector<types::GlobalDiscoveryEntry>& capabilities)>
                    onSuccess,
            std::function<void(const exceptions::JoynrRuntimeException& error)> onError)
    {
        std::ignore = participantId;
        std::ignore = onError;
        std::vector<types::GlobalDiscoveryEntry> result;
        onSuccess(result);
    }

    void fakeCapabilitiesClientLookupWithError (
            const std::vector<std::string>& domains,
            const std::string& interfaceName,
            const std::int64_t messagingTtl,
            std::function<void(const std::vector<types::GlobalDiscoveryEntry>& capabilities)>
                    onSuccess,
            std::function<void(const exceptions::JoynrRuntimeException& error)> onError)
    {
        std::ignore = domains;
        std::ignore = interfaceName;
        std::ignore = messagingTtl;
        std::ignore = onSuccess;
        exceptions::DiscoveryException fakeDiscoveryException("fakeDiscoveryException");
        onError(fakeDiscoveryException);
    }

    void fakeCapabilitiesClientAddWithError (
            const joynr::types::GlobalDiscoveryEntry& entry,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError) {
        std::ignore = entry;
        std::ignore = onSuccess;
        exceptions::DiscoveryException fakeDiscoveryException("fakeDiscoveryException");
        onError(fakeDiscoveryException);
    }

    void fakeCapabilitiesClientAddSuccess (
            const joynr::types::GlobalDiscoveryEntry& entry,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError) {
        std::ignore = entry;
        std::ignore = onError;
        onSuccess();
    }

    void fakeLookupWithResults(
            const std::vector<std::string>& domains,
            const std::string& interfaceName,
            const std::int64_t messagingTtl,
            std::function<void(const std::vector<types::GlobalDiscoveryEntry>& capabilities)>
                    onSuccess,
            std::function<void(const exceptions::JoynrRuntimeException& error)> onError)
    {
        std::ignore = domains;
        std::ignore = interfaceName;
        std::ignore = messagingTtl;
        std::ignore = onError;
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

    void fakeLookupByParticipantIdWithResults(
            const std::string& participantId,
            std::function<void(const std::vector<types::GlobalDiscoveryEntry>& discoveryEntries)>
                    onSuccess,
            std::function<void(const exceptions::JoynrRuntimeException& error)> onError)
    {
        std::ignore = onError;
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

    void fakeLookupWithThreeResults(
            const std::string& participantId,
            std::function<void(const std::vector<types::GlobalDiscoveryEntry>& discoveryEntries)>
                    onSuccess,
            std::function<void(const exceptions::JoynrRuntimeException& error)> onError)
    {
        std::ignore = participantId;
        std::ignore = onError;
        types::ProviderQos qos;
        std::vector<types::GlobalDiscoveryEntry> discoveryEntryList;
        // each participantId can register for only one domain and only one interface.
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
                                                                 DOMAIN_2_NAME,
                                                                 INTERFACE_2_NAME,
                                                                 dummyParticipantId2,
                                                                 qos,
                                                                 LASTSEEN_MS,
                                                                 EXPIRYDATE_MS,
                                                                 PUBLIC_KEY_ID,
                                                                 EXTERNAL_ADDRESS));
        discoveryEntryList.push_back(types::GlobalDiscoveryEntry(defaultProviderVersion,
                                                                 DOMAIN_3_NAME,
                                                                 INTERFACE_3_NAME,
                                                                 dummyParticipantId3,
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

protected:
    Settings settings;
    ClusterControllerSettings clusterControllerSettings;
    const int purgeExpiredDiscoveryEntriesIntervalMs;
    std::shared_ptr<MockCapabilitiesClient> capabilitiesClient;
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
    std::shared_ptr<MockLocalCapabilitiesDirectoryCallback> callback;
    std::function<void()> defaultOnSuccess;
    std::function<void(const joynr::exceptions::ProviderRuntimeException&)> defaultOnError;
    joynr::types::Version defaultProviderVersion;

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

    void registerReceivedCapabilities(const std::string& addressType,
                                      const std::string& serializedAddress);

    ADD_LOGGER(LocalCapabilitiesDirectoryTest)

private:
    DISALLOW_COPY_AND_ASSIGN(LocalCapabilitiesDirectoryTest);
};

const std::string LocalCapabilitiesDirectoryTest::INTERFACE_1_NAME("myInterfaceA");
const std::string LocalCapabilitiesDirectoryTest::INTERFACE_2_NAME("myInterfaceB");
const std::string LocalCapabilitiesDirectoryTest::INTERFACE_3_NAME("myInterfaceC");
const std::string LocalCapabilitiesDirectoryTest::DOMAIN_1_NAME("domainA");
const std::string LocalCapabilitiesDirectoryTest::DOMAIN_2_NAME("domainB");
const std::string LocalCapabilitiesDirectoryTest::DOMAIN_3_NAME("domainB");
const std::string LocalCapabilitiesDirectoryTest::LOCAL_ADDRESS("localAddress");
const std::string LocalCapabilitiesDirectoryTest::EXTERNAL_ADDRESS(
        joynr::serializer::serializeToJson(joynr::system::RoutingTypes::MqttAddress()));
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

TEST_F(LocalCapabilitiesDirectoryTest, addGloballyDelegatesToCapabilitiesClient)
{
    EXPECT_CALL(*capabilitiesClient, add(An<const types::GlobalDiscoveryEntry&>(), _, _)).Times(1);
    joynr::types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantId1,
                                       types::ProviderQos(),
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);
    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultOnError);
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

    EXPECT_CALL(*capabilitiesClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(
                            AnConvertedGlobalDiscoveryEntry(entry1)),
                    _,
                    _)).Times(1);
    EXPECT_CALL(*capabilitiesClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(
                            AnConvertedGlobalDiscoveryEntry(entry2)),
                    _,
                    _)).Times(1);

    localCapabilitiesDirectory->add(entry1, defaultOnSuccess, defaultOnError);

    localCapabilitiesDirectory->add(entry2, defaultOnSuccess, defaultOnError);

    Mock::VerifyAndClearExpectations(capabilitiesClient.get());

    EXPECT_CALL(*capabilitiesClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(
                            AnConvertedGlobalDiscoveryEntry(entry1)),
                    _,
                    _)).Times(1);
    EXPECT_CALL(*capabilitiesClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(
                            AnConvertedGlobalDiscoveryEntry(entry2)),
                    _,
                    _)).Times(1);

    bool onSuccessCalled = false;
    localCapabilitiesDirectory->triggerGlobalProviderReregistration(
            [&onSuccessCalled]() { onSuccessCalled = true; },
            [](const joynr::exceptions::ProviderRuntimeException&) { FAIL(); });

    Mock::VerifyAndClearExpectations(capabilitiesClient.get());
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

    EXPECT_CALL(*capabilitiesClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(
                            AnConvertedGlobalDiscoveryEntry(entry1)),
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

    Mock::VerifyAndClearExpectations(capabilitiesClient.get());

    EXPECT_CALL(*capabilitiesClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(
                            AnConvertedGlobalDiscoveryEntry(entry1)),
                    _,
                    _)).Times(1);

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

    EXPECT_CALL(*capabilitiesClient,
                add(Matcher<const std::vector<joynr::types::GlobalDiscoveryEntry>&>(_), _, _))
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
            *capabilitiesClient,
            lookup(dummyParticipantId1,
                   A<std::function<void(
                           const std::vector<joynr::types::GlobalDiscoveryEntry>& capabilities)>>(),
                   A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(0);
    EXPECT_CALL(*capabilitiesClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(_), _, _)).Times(1);

    joynr::types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantId1,
                                       types::ProviderQos(),
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);
    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultOnError);

    localCapabilitiesDirectory->lookup(dummyParticipantId1, callback);
    EXPECT_EQ(1, callback->getResults(TIMEOUT).size());
}

TEST_F(LocalCapabilitiesDirectoryTest, addLocallyDoesNotCallCapabilitiesClient)
{
    EXPECT_CALL(*capabilitiesClient,
                lookup(_,
                       A<std::function<void(const std::vector<joynr::types::GlobalDiscoveryEntry>&
                                                    discoveryEntries)>>(),
                       A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(0);
    EXPECT_CALL(*capabilitiesClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(_), _, _)).Times(0);
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

    localCapabilitiesDirectory->lookup(dummyParticipantId1, callback);
    EXPECT_EQ(1, callback->getResults(TIMEOUT).size());
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupForInterfaceAddressReturnsCachedValues)
{

    // simulate global capability directory would store two entries.
    EXPECT_CALL(*capabilitiesClient, lookup(ElementsAre(DOMAIN_1_NAME), INTERFACE_1_NAME, _, _, _))
            .Times(1)
            .WillOnce(Invoke(this, &LocalCapabilitiesDirectoryTest::fakeLookupWithResults));

    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME}, INTERFACE_1_NAME, callback, discoveryQos);
    callback->clearResults();
    // enries are now in cache, capabilitiesClient should not be called.
    EXPECT_CALL(*capabilitiesClient,
                lookup(_,
                       _,
                       _,
                       A<std::function<void(const std::vector<joynr::types::GlobalDiscoveryEntry>&
                                                    discoveryEntries)>>(),
                       A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(0);
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME}, INTERFACE_1_NAME, callback, discoveryQos);
    EXPECT_EQ(2, callback->getResults(TIMEOUT).size());
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupForInterfaceAddressDelegatesToCapabilitiesClient)
{
    // simulate global capability directory would store two entries.
    EXPECT_CALL(*capabilitiesClient, lookup(ElementsAre(DOMAIN_1_NAME), INTERFACE_1_NAME, _, _, _))
            .Times(1)
            .WillOnce(Invoke(this, &LocalCapabilitiesDirectoryTest::fakeLookupWithResults));

    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME}, INTERFACE_1_NAME, callback, discoveryQos);
    std::vector<types::DiscoveryEntryWithMetaInfo> capabilities = callback->getResults(TIMEOUT);

    EXPECT_EQ(2, capabilities.size());

    // check that the results contain the two channel ids
    bool firstParticipantIdFound = false;
    bool secondParticipantIdFound = false;
    for (std::uint16_t i = 0; i < capabilities.size(); i++) {
        types::DiscoveryEntryWithMetaInfo entry = capabilities.at(i);
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
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupForParticipantIdReturnsCachedValues)
{

    EXPECT_CALL(
            *capabilitiesClient,
            lookup(_,
                   A<std::function<void(
                           const std::vector<types::GlobalDiscoveryEntry>& discoveryEntries)>>(),
                   A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(1)
            .WillOnce(Invoke(
                    this, &LocalCapabilitiesDirectoryTest::fakeLookupByParticipantIdWithResults));

    localCapabilitiesDirectory->lookup(dummyParticipantId1, callback);
    std::vector<types::DiscoveryEntryWithMetaInfo> capabilities = callback->getResults(TIMEOUT);
    EXPECT_EQ(1, capabilities.size());
    callback->clearResults();
    EXPECT_CALL(
            *capabilitiesClient,
            lookup(_,
                   A<std::function<void(
                           const std::vector<types::GlobalDiscoveryEntry>& discoveryEntries)>>(),
                   A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(0);
    localCapabilitiesDirectory->lookup(dummyParticipantId1, callback);
    capabilities = callback->getResults(TIMEOUT);
    EXPECT_EQ(1, capabilities.size());
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupForParticipantIdReturnsNoCapability)
{
    EXPECT_CALL(
            *capabilitiesClient,
            lookup(_,
                   A<std::function<void(
                           const std::vector<types::GlobalDiscoveryEntry>& discoveryEntries)>>(),
                   A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(1)
            .WillOnce(Invoke(this, &LocalCapabilitiesDirectoryTest::fakeLookupZeroResults));

    localCapabilitiesDirectory->lookup(dummyParticipantId1, callback);
    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> capabilities =
            callback->getResults(TIMEOUT);
    EXPECT_EQ(0, capabilities.size());
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupForParticipantIdDelegatesToCapabilitiesClient)
{

    EXPECT_CALL(
            *capabilitiesClient,
            lookup(dummyParticipantId1,
                   A<std::function<void(
                           const std::vector<types::GlobalDiscoveryEntry>& discoveryEntries)>>(),
                   A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(1)
            .WillOnce(Invoke(this, &LocalCapabilitiesDirectoryTest::fakeLookupWithThreeResults));

    localCapabilitiesDirectory->lookup(dummyParticipantId1, callback);
    std::vector<types::DiscoveryEntryWithMetaInfo> capabilities = callback->getResults(TIMEOUT);

    EXPECT_EQ(3, capabilities.size());
    bool interfaceAddress1Found = false;
    bool interfaceAddress2Found = false;
    bool interfaceAddress3Found = false;
    for (std::uint16_t i = 0; i < capabilities.size(); i++) {
        types::DiscoveryEntry entry = capabilities.at(i);
        if ((entry.getDomain() == DOMAIN_1_NAME) &&
            (entry.getInterfaceName() == INTERFACE_1_NAME)) {
            interfaceAddress1Found = true;
        } else if ((entry.getDomain() == DOMAIN_2_NAME) &&
                   (entry.getInterfaceName() == INTERFACE_2_NAME)) {
            interfaceAddress2Found = true;
        } else if ((entry.getDomain() == DOMAIN_3_NAME) &&
                   (entry.getInterfaceName() == INTERFACE_3_NAME)) {
            interfaceAddress3Found = true;
        }
    }

    EXPECT_TRUE(interfaceAddress1Found);
    EXPECT_TRUE(interfaceAddress2Found);
    EXPECT_TRUE(interfaceAddress3Found);
}

TEST_F(LocalCapabilitiesDirectoryTest, clearRemovesEntries)
{

    EXPECT_CALL(
            *capabilitiesClient,
            lookup(_,
                   A<std::function<void(
                           const std::vector<types::GlobalDiscoveryEntry>& discoveryEntries)>>(),
                   A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(1)
            .WillOnce(Invoke(
                    this, &LocalCapabilitiesDirectoryTest::fakeLookupByParticipantIdWithResults));

    localCapabilitiesDirectory->lookup(dummyParticipantId1, callback);

    // remove all entries in the cache
    localCapabilitiesDirectory->clear();
    // retrieving capabilities will force a call to the backend as the cache is empty
    EXPECT_CALL(
            *capabilitiesClient,
            lookup(_,
                   A<std::function<void(
                           const std::vector<types::GlobalDiscoveryEntry>& discoveryEntries)>>(),
                   A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(1);
    localCapabilitiesDirectory->lookup(dummyParticipantId1, callback);
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
        EXPECT_CALL(*capabilitiesClient, add(globalDiscoveryEntryInfo1, _, _)).Times(1);
        EXPECT_CALL(*capabilitiesClient, add(globalDiscoveryEntryInfo2, _, _)).Times(1);
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
            *capabilitiesClient,
            lookup(_,
                   A<std::function<void(
                           const std::vector<types::GlobalDiscoveryEntry>& discoveryEntries)>>(),
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
    localCapabilitiesDirectory->lookup(dummyParticipantId1, callback);
    EXPECT_EQ(1, callback->getResults(10).size());
    callback->clearResults();

    localCapabilitiesDirectory->remove(dummyParticipantId1);

    EXPECT_CALL(
            *capabilitiesClient,
            lookup(_,
                   A<std::function<void(
                           const std::vector<types::GlobalDiscoveryEntry>& discoveryEntries)>>(),
                   A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(this, &LocalCapabilitiesDirectoryTest::simulateTimeout));
    // JoynrTimeOutException timeoutException;
    EXPECT_THROW(localCapabilitiesDirectory->lookup(dummyParticipantId1, callback),
                 exceptions::JoynrTimeOutException);
    EXPECT_EQ(0, callback->getResults(10).size());
}

TEST_F(LocalCapabilitiesDirectoryTest, registerLocalCapability_lookupLocal)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);

    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::LOCAL_ONLY);

    EXPECT_CALL(*capabilitiesClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(_), _, _)).Times(0);

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

    EXPECT_CALL(*capabilitiesClient, lookup(_, _, _, _, _)).Times(0);
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME}, INTERFACE_1_NAME, callback, discoveryQos);
    EXPECT_EQ(1, callback->getResults(10).size());
    callback->clearResults();

    EXPECT_CALL(*capabilitiesClient, remove(dummyParticipantId1)).Times(0);
    localCapabilitiesDirectory->remove(dummyParticipantId1);

    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME}, INTERFACE_1_NAME, callback, discoveryQos);
    EXPECT_EQ(0, callback->getResults(10).size());
    callback->clearResults();
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
    EXPECT_CALL(*capabilitiesClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(_), _, _)).Times(0);
    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultOnError);
    localCapabilitiesDirectory->registerReceivedCapabilities(std::move(globalCapEntryMap));

    EXPECT_CALL(*capabilitiesClient, lookup(_, _, _, _, _)).Times(0);
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME}, INTERFACE_1_NAME, callback, discoveryQos);
    EXPECT_EQ(1, callback->getResults(10).size());
    callback->clearResults();

    EXPECT_CALL(*capabilitiesClient, remove(dummyParticipantId1)).Times(0);
    localCapabilitiesDirectory->remove(dummyParticipantId1);

    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME}, INTERFACE_1_NAME, callback, discoveryQos);
    EXPECT_EQ(1, callback->getResults(10).size());
    callback->clearResults();

    // disable cache
    discoveryQos.setCacheMaxAge(0);
    EXPECT_CALL(*capabilitiesClient, lookup(_, _, _, _, _)).Times(1).WillOnce(
            InvokeWithoutArgs(this, &LocalCapabilitiesDirectoryTest::simulateTimeout));
    EXPECT_THROW(localCapabilitiesDirectory->lookup(
                         {DOMAIN_1_NAME}, INTERFACE_1_NAME, callback, discoveryQos),
                 exceptions::JoynrTimeOutException);
    EXPECT_EQ(0, callback->getResults(10).size());
    callback->clearResults();
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
    EXPECT_CALL(*capabilitiesClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(_), _, _)).Times(0);
    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultOnError);
    // localCapabilitiesDirectory->registerReceivedCapabilities(globalCapEntryMap);

    EXPECT_CALL(*capabilitiesClient, lookup(_, _, _, _, _))
            .Times(2)
            //            .WillOnce(InvokeWithoutArgs(this,
            //            &LocalCapabilitiesDirectoryTest::simulateTimeout));
            .WillRepeatedly(Invoke(
                    this,
                    &LocalCapabilitiesDirectoryTest::fakeLookupZeroResultsForInterfaceAddress));
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME}, INTERFACE_1_NAME, callback, discoveryQos);
    EXPECT_EQ(1, callback->getResults(10).size());
    callback->clearResults();

    EXPECT_CALL(*capabilitiesClient, remove(dummyParticipantId1)).Times(0);
    localCapabilitiesDirectory->remove(dummyParticipantId1);

    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME}, INTERFACE_1_NAME, callback, discoveryQos);
    EXPECT_EQ(0, callback->getResults(10).size());
    callback->clearResults();

    // disable cache
    discoveryQos.setCacheMaxAge(0);
    EXPECT_CALL(*capabilitiesClient, lookup(_, _, _, _, _)).Times(1).WillOnce(
            Invoke(this, &LocalCapabilitiesDirectoryTest::fakeLookupWithResults));
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME}, INTERFACE_1_NAME, callback, discoveryQos);
    EXPECT_EQ(2, callback->getResults(10).size());
    callback->clearResults();

    EXPECT_CALL(*capabilitiesClient, lookup(_, _, _, _, _)).Times(0);
    localCapabilitiesDirectory->clear();

    discoveryQos.setCacheMaxAge(4000);
    localCapabilitiesDirectory->registerReceivedCapabilities(std::move(globalCapEntryMap));
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME}, INTERFACE_1_NAME, callback, discoveryQos);
    EXPECT_EQ(1, callback->getResults(10).size());
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupLocalThenGlobal_GlobalPendingLocalEntryAdded_ReturnsLocalEntry)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);

    Semaphore semaphore(0);
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
    EXPECT_CALL(*capabilitiesClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(_), _, _)).Times(0);

    EXPECT_CALL(*capabilitiesClient, lookup(_, _, _, _, _)).Times(1).WillRepeatedly(
            DoAll(AcquireSemaphore(&semaphore),
                  Invoke(this, &LocalCapabilitiesDirectoryTest::fakeLookupWithResults)));

    auto thread = std::thread([&]() {
        localCapabilitiesDirectory->lookup(
                {DOMAIN_1_NAME, DOMAIN_2_NAME}, INTERFACE_1_NAME, callback, discoveryQos);
    });
    EXPECT_EQ(0, callback->getResults(100).size());

    EXPECT_TRUE(localCapabilitiesDirectory->hasPendingLookups());

    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultOnError);

    EXPECT_EQ(1, callback->getResults(10).size());
    EXPECT_TRUE(localCapabilitiesDirectory->hasPendingLookups());

    // capabilitiesClient.lookup should return;
    semaphore.notify();
    thread.join();

    EXPECT_FALSE(localCapabilitiesDirectory->hasPendingLookups());
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupLocalThenGlobal_GlobalSucceedsNoLocalEntries_ReturnsGlobalEntries)
{
    Semaphore semaphore(0);
    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryTimeout(5000);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL);

    EXPECT_CALL(*capabilitiesClient, lookup(_, _, _, _, _)).Times(1).WillRepeatedly(
            DoAll(AcquireSemaphore(&semaphore),
                  Invoke(this, &LocalCapabilitiesDirectoryTest::fakeLookupWithResults)));

    auto thread = std::thread([&]() {
        localCapabilitiesDirectory->lookup(
                {DOMAIN_1_NAME, DOMAIN_2_NAME}, INTERFACE_1_NAME, callback, discoveryQos);
    });

    EXPECT_EQ(0, callback->getResults(100).size());
    EXPECT_TRUE(localCapabilitiesDirectory->hasPendingLookups());

    // capabilitiesClient.lookup should return;
    semaphore.notify();
    thread.join();

    EXPECT_EQ(2, callback->getResults(100).size());
    EXPECT_FALSE(localCapabilitiesDirectory->hasPendingLookups());
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupLocalThenGlobal_GlobalFailsNoLocalEntries_ReturnsNoEntries)
{
    Semaphore semaphore(0);
    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryTimeout(5000);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL);

    EXPECT_CALL(*capabilitiesClient, lookup(_, _, _, _, _)).Times(1).WillRepeatedly(
            DoAll(AcquireSemaphore(&semaphore),
                  Invoke(this, &LocalCapabilitiesDirectoryTest::fakeCapabilitiesClientLookupWithError)));

    auto thread = std::thread([&]() {
        localCapabilitiesDirectory->lookup(
                {DOMAIN_1_NAME, DOMAIN_2_NAME}, INTERFACE_1_NAME, callback, discoveryQos);
    });

    EXPECT_EQ(0, callback->getResults(100).size());
    EXPECT_TRUE(localCapabilitiesDirectory->hasPendingLookups());

    // capabilitiesClient.lookup should return;
    semaphore.notify();
    thread.join();

    EXPECT_EQ(0, callback->getResults(100).size());
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
    EXPECT_CALL(*capabilitiesClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(_), _, _)).Times(0);

    EXPECT_CALL(*capabilitiesClient, lookup(_, _, _, _, _)).Times(0);

    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultOnError);
    localCapabilitiesDirectory->lookup(
            {DOMAIN_1_NAME, DOMAIN_2_NAME}, INTERFACE_1_NAME, callback, discoveryQos);

    EXPECT_EQ(1, callback->getResults(10).size());
    EXPECT_FALSE(localCapabilitiesDirectory->hasPendingLookups());
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupLocalAndGlobal_GlobalSucceedsNoLocalEntries_ReturnsGlobalEntries)
{
    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryTimeout(5000);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::LOCAL_AND_GLOBAL);

    EXPECT_CALL(*capabilitiesClient, lookup(_, _, _, _, _)).Times(1).WillRepeatedly(
            Invoke(this, &LocalCapabilitiesDirectoryTest::fakeLookupWithResults));

    localCapabilitiesDirectory->lookup(
            {DOMAIN_1_NAME, DOMAIN_2_NAME}, INTERFACE_1_NAME, callback, discoveryQos);

    EXPECT_EQ(2, callback->getResults(100).size());
    EXPECT_FALSE(localCapabilitiesDirectory->hasPendingLookups());
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupLocalAndGlobal_GlobalFailsNoLocalEntries_ReturnsNoEntries)
{
    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryTimeout(5000);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::LOCAL_AND_GLOBAL);

    EXPECT_CALL(*capabilitiesClient, lookup(_, _, _, _, _)).Times(1).WillRepeatedly(
            Invoke(this, &LocalCapabilitiesDirectoryTest::fakeCapabilitiesClientLookupWithError));

    localCapabilitiesDirectory->lookup(
            {DOMAIN_1_NAME, DOMAIN_2_NAME}, INTERFACE_1_NAME, callback, discoveryQos);

    EXPECT_EQ(0, callback->getResults(100).size());
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
    EXPECT_CALL(*capabilitiesClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(_), _, _)).Times(0);

    EXPECT_CALL(*capabilitiesClient, lookup(_, _, _, _, _)).Times(1).WillRepeatedly(
            Invoke(this, &LocalCapabilitiesDirectoryTest::fakeLookupWithResults));

    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultOnError);
    localCapabilitiesDirectory->lookup(
            {DOMAIN_1_NAME, DOMAIN_2_NAME}, INTERFACE_1_NAME, callback, discoveryQos);

    EXPECT_EQ(3, callback->getResults(10).size());
    EXPECT_FALSE(localCapabilitiesDirectory->hasPendingLookups());
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupLocalAndGlobal_GlobalFailsLocalEntries_ReturnsLocalEntries)
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
    EXPECT_CALL(*capabilitiesClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(_), _, _)).Times(0);

    EXPECT_CALL(*capabilitiesClient, lookup(_, _, _, _, _)).Times(1).WillRepeatedly(
            Invoke(this, &LocalCapabilitiesDirectoryTest::fakeCapabilitiesClientLookupWithError));

    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultOnError);
    localCapabilitiesDirectory->lookup(
            {DOMAIN_1_NAME, DOMAIN_2_NAME}, INTERFACE_1_NAME, callback, discoveryQos);

    EXPECT_EQ(0, callback->getResults(10).size());
    EXPECT_FALSE(localCapabilitiesDirectory->hasPendingLookups());
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupGlobalOnly_GlobalSucceedsNoLocalEntries_ReturnsGlobalEntries)
{
    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryTimeout(5000);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::GLOBAL_ONLY);

    EXPECT_CALL(*capabilitiesClient, lookup(_, _, _, _, _)).Times(1).WillRepeatedly(
            Invoke(this, &LocalCapabilitiesDirectoryTest::fakeLookupWithResults));

    localCapabilitiesDirectory->lookup(
            {DOMAIN_1_NAME, DOMAIN_2_NAME}, INTERFACE_1_NAME, callback, discoveryQos);

    EXPECT_EQ(2, callback->getResults(100).size());
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

    EXPECT_CALL(*capabilitiesClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(_), _, _)).Times(3);

    localCapabilitiesDirectory->add(entry1, defaultOnSuccess, defaultOnError);

    localCapabilitiesDirectory->add(entry2, defaultOnSuccess, defaultOnError);

    localCapabilitiesDirectory->add(entry3, defaultOnSuccess, defaultOnError);

    // Check Cached Global Discovery Entries
    EXPECT_CALL(*capabilitiesClient, lookup(_, _, _, _, _)).Times(0);

    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryTimeout(10000);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::GLOBAL_ONLY);

    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME}, INTERFACE_1_NAME, callback, discoveryQos);

    EXPECT_EQ(2, callback->getResults(100).size());
    callback->clearResults();

    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME}, INTERFACE_3_NAME, callback, discoveryQos);
    EXPECT_EQ(1, callback->getResults(100).size());
    callback->clearResults();

    // wait for cleanup timer to run
    std::this_thread::sleep_for(
            std::chrono::milliseconds(purgeExpiredDiscoveryEntriesIntervalMs * 2));

    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME}, INTERFACE_1_NAME, callback, discoveryQos);

    EXPECT_EQ(1, callback->getResults(100).size());
    callback->clearResults();

    Mock::VerifyAndClearExpectations(capabilitiesClient.get());
    EXPECT_CALL(*capabilitiesClient, lookup(_, _, _, _, _)).Times(1);
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME}, INTERFACE_3_NAME, callback, discoveryQos);
    EXPECT_EQ(0, callback->getResults(100).size());
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupGlobalOnly_GlobalFailsNoLocalEntries_ReturnsNoEntries)
{
    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryTimeout(5000);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::GLOBAL_ONLY);

    EXPECT_CALL(*capabilitiesClient, lookup(_, _, _, _, _)).Times(1).WillRepeatedly(
            Invoke(this, &LocalCapabilitiesDirectoryTest::fakeCapabilitiesClientLookupWithError));

    localCapabilitiesDirectory->lookup(
            {DOMAIN_1_NAME, DOMAIN_2_NAME}, INTERFACE_1_NAME, callback, discoveryQos);

    EXPECT_EQ(0, callback->getResults(100).size());
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
    EXPECT_CALL(*capabilitiesClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(_), _, _)).Times(0);

    EXPECT_CALL(*capabilitiesClient, lookup(_, _, _, _, _)).Times(1).WillRepeatedly(
            Invoke(this, &LocalCapabilitiesDirectoryTest::fakeLookupWithResults));

    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultOnError);
    localCapabilitiesDirectory->lookup(
            {DOMAIN_1_NAME, DOMAIN_2_NAME}, INTERFACE_1_NAME, callback, discoveryQos);

    EXPECT_EQ(2, callback->getResults(10).size());
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
    EXPECT_CALL(*capabilitiesClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(_), _, _)).Times(0);

    EXPECT_CALL(*capabilitiesClient, lookup(_, _, _, _, _)).Times(1).WillRepeatedly(
            Invoke(this, &LocalCapabilitiesDirectoryTest::fakeCapabilitiesClientLookupWithError));

    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultOnError);
    localCapabilitiesDirectory->lookup(
            {DOMAIN_1_NAME, DOMAIN_2_NAME}, INTERFACE_1_NAME, callback, discoveryQos);

    EXPECT_EQ(0, callback->getResults(10).size());
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

    EXPECT_CALL(*capabilitiesClient,
                lookup(_,
                       A<std::function<void(const std::vector<joynr::types::GlobalDiscoveryEntry>&
                                                    discoveryEntries)>>(),
                       A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(0);

    std::vector<std::string> domains = {
            multipleDomainName1, multipleDomainName2, multipleDomainName3};

    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::LOCAL_ONLY);
    localCapabilitiesDirectory->lookup(domains, INTERFACE_1_NAME, callback, discoveryQos);

    EXPECT_EQ(4, callback->getResults(TIMEOUT).size());
}

TEST_F(LocalCapabilitiesDirectoryTest, registerLocalCapability_lookupGlobalOnly)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);

    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::GLOBAL_ONLY);

    EXPECT_CALL(*capabilitiesClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(_), _, _)).Times(0);

    joynr::types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantId1,
                                       providerQos,
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);
    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultOnError);

    EXPECT_CALL(*capabilitiesClient, lookup(_, _, _, _, _)).Times(1).WillOnce(
            InvokeWithoutArgs(this, &LocalCapabilitiesDirectoryTest::simulateTimeout));
    // JoynrTimeOutException timeoutException;
    EXPECT_THROW(localCapabilitiesDirectory->lookup(
                         {DOMAIN_1_NAME}, INTERFACE_1_NAME, callback, discoveryQos),
                 exceptions::JoynrTimeOutException);
    EXPECT_EQ(0, callback->getResults(10).size());
    callback->clearResults();

    // register the external capability
    localCapabilitiesDirectory->registerReceivedCapabilities(std::move(globalCapEntryMap));
    // get the global entry
    EXPECT_CALL(*capabilitiesClient, lookup(_, _, _, _, _)).Times(0);
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME}, INTERFACE_1_NAME, callback, discoveryQos);
    EXPECT_EQ(1, callback->getResults(10).size());
    callback->clearResults();

    EXPECT_CALL(*capabilitiesClient, remove(dummyParticipantId1)).Times(0);
    localCapabilitiesDirectory->remove(dummyParticipantId1);

    // disable cache
    discoveryQos.setCacheMaxAge(0);
    EXPECT_CALL(*capabilitiesClient, lookup(_, _, _, _, _)).Times(1).WillOnce(
            InvokeWithoutArgs(this, &LocalCapabilitiesDirectoryTest::simulateTimeout));
    EXPECT_THROW(localCapabilitiesDirectory->lookup(
                         {DOMAIN_1_NAME}, INTERFACE_1_NAME, callback, discoveryQos),
                 exceptions::JoynrTimeOutException);
    EXPECT_EQ(0, callback->getResults(10).size());
    callback->clearResults();
}

TEST_F(LocalCapabilitiesDirectoryTest, registerGlobalCapability_lookupLocal)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);

    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::LOCAL_ONLY);

    EXPECT_CALL(*capabilitiesClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(_), _, _)).Times(1);

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

    EXPECT_CALL(*capabilitiesClient, lookup(_, _, _, _, _)).Times(0);
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME}, INTERFACE_1_NAME, callback, discoveryQos);
    EXPECT_EQ(1, callback->getResults(10).size());
    callback->clearResults();

    EXPECT_CALL(*capabilitiesClient, remove(dummyParticipantId1)).Times(1);
    localCapabilitiesDirectory->remove(dummyParticipantId1);
}

TEST_F(LocalCapabilitiesDirectoryTest, registerGlobalCapability_lookupLocalThenGlobal)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);

    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(100);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL);

    EXPECT_CALL(*capabilitiesClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(_), _, _)).Times(1);

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
    EXPECT_CALL(*capabilitiesClient, lookup(_, _, _, _, _)).Times(0);
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME}, INTERFACE_1_NAME, callback, discoveryQos);
    EXPECT_EQ(1, callback->getResults(10).size());
    callback->clearResults();

    EXPECT_CALL(*capabilitiesClient, remove(dummyParticipantId1)).Times(1);
    localCapabilitiesDirectory->remove(dummyParticipantId1);

    // get the global entry
    EXPECT_CALL(*capabilitiesClient, lookup(_, _, _, _, _)).Times(0);
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME}, INTERFACE_1_NAME, callback, discoveryQos);
    EXPECT_EQ(1, callback->getResults(10).size());
    callback->clearResults();

    // wait for cleanup timer to run
    std::this_thread::sleep_for(
            std::chrono::milliseconds(purgeExpiredDiscoveryEntriesIntervalMs * 2));

    // get the global, but timeout occured
    EXPECT_CALL(*capabilitiesClient, lookup(_, _, _, _, _)).Times(1).WillOnce(
            InvokeWithoutArgs(this, &LocalCapabilitiesDirectoryTest::simulateTimeout));
    EXPECT_THROW(localCapabilitiesDirectory->lookup(
                         {DOMAIN_1_NAME}, INTERFACE_1_NAME, callback, discoveryQos),
                 exceptions::JoynrTimeOutException);

    EXPECT_EQ(0, callback->getResults(10).size());
}

TEST_F(LocalCapabilitiesDirectoryTest, registerCachedGlobalCapability_lookupGlobalOnly)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);

    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(100);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::GLOBAL_ONLY);

    // JoynrTimeOutException timeoutException;
    EXPECT_CALL(*capabilitiesClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(_), _, _)).Times(1);

    joynr::types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantId1,
                                       providerQos,
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);
    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultOnError);

    EXPECT_CALL(*capabilitiesClient, lookup(_, _, _, _, _)).Times(0);
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME}, INTERFACE_1_NAME, callback, discoveryQos);
    EXPECT_EQ(1, callback->getResults(10).size());
    callback->clearResults();

    // recieve a global entry
    localCapabilitiesDirectory->registerReceivedCapabilities(std::move(globalCapEntryMap));
    EXPECT_CALL(*capabilitiesClient, lookup(_, _, _, _, _)).Times(0);
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME}, INTERFACE_1_NAME, callback, discoveryQos);
    EXPECT_EQ(2, callback->getResults(10).size());

    EXPECT_CALL(*capabilitiesClient, remove(dummyParticipantId1)).Times(1);
    localCapabilitiesDirectory->remove(dummyParticipantId1);
}

// TODO test remove global capability

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

void LocalCapabilitiesDirectoryTest::registerReceivedCapabilities(
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
                           _,
                           _)).Times(0);

    std::unordered_multimap<std::string, types::DiscoveryEntry> capabilitiesMap;
    types::DiscoveryEntry capEntry;
    capEntry.setParticipantId(participantId);
    capabilitiesMap.insert({serializedAddress, capEntry});
    localCapabilitiesDirectory->registerReceivedCapabilities(std::move(capabilitiesMap));
}

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

    joynr::types::DiscoveryEntry entry1(defaultProviderVersion,
                                        DOMAIN_NAME,
                                        INTERFACE_NAME,
                                        participantIds[0],
                                        localProviderQos,
                                        lastSeenDateMs,
                                        expiryDateMs,
                                        PUBLIC_KEY_ID);
    joynr::types::DiscoveryEntry entry2(defaultProviderVersion,
                                        DOMAIN_NAME,
                                        INTERFACE_NAME,
                                        participantIds[1],
                                        globalProviderQos,
                                        lastSeenDateMs,
                                        expiryDateMs,
                                        PUBLIC_KEY_ID);
    joynr::types::DiscoveryEntry entry3(defaultProviderVersion,
                                        DOMAIN_NAME,
                                        INTERFACE_NAME,
                                        participantIds[2],
                                        globalProviderQos,
                                        lastSeenDateMs,
                                        expiryDateMs,
                                        PUBLIC_KEY_ID);

    localCapabilitiesDirectory->add(entry1, defaultOnSuccess, defaultOnError);
    localCapabilitiesDirectory->add(entry2, defaultOnSuccess, defaultOnError);
    localCapabilitiesDirectory->add(entry3, defaultOnSuccess, defaultOnError);

    // create a new object
    auto localCapabilitiesDirectory2 =
            std::make_shared<LocalCapabilitiesDirectory>(clusterControllerSettings,
                                                         capabilitiesClient,
                                                         LOCAL_ADDRESS,
                                                         mockMessageRouter,
                                                         singleThreadedIOService->getIOService(),
                                                         "clusterControllerId");
    localCapabilitiesDirectory2->init();

    // load persistency
    localCapabilitiesDirectory2->loadPersistedFile();

    // check all entries are there
    for (auto& partecipantID : participantIds) {
        localCapabilitiesDirectory2->lookup(partecipantID, callback);
        EXPECT_EQ(1, callback->getResults(1000).size());
        callback->clearResults();
    }

    auto globalDiscoveryEntries = localCapabilitiesDirectory2->getCachedGlobalDiscoveryEntries();
    EXPECT_EQ(2, globalDiscoveryEntries.size());
    EXPECT_EQ(entry2, globalDiscoveryEntries[0]);
    EXPECT_EQ(entry3, globalDiscoveryEntries[1]);
}

TEST_F(LocalCapabilitiesDirectoryTest, loadCapabilitiesFromFile)
{
    const std::string fileName = "test-resources/ListOfCapabilitiesToInject.json";
    localCapabilitiesDirectory->injectGlobalCapabilitiesFromFile(fileName);

    // Verify that all entries present in the file have indeed been loaded
    localCapabilitiesDirectory->lookup("notReachableInterface_Schroedinger", callback);
    EXPECT_EQ(1, callback->getResults(TIMEOUT).size());
    callback->clearResults();

    localCapabilitiesDirectory->lookup("notReachableInterface_Heisenberg", callback);
    EXPECT_EQ(1, callback->getResults(TIMEOUT).size());
    callback->clearResults();
}

TEST_F(LocalCapabilitiesDirectoryTest, throwExceptionOnMultiProxy)
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

    EXPECT_CALL(mockCallback, onError(_)).Times(1);
    EXPECT_CALL(mockCallback, onSuccess(_)).Times(0);
    localCapabilitiesDirectory->lookup(
            zeroDomains, INTERFACE_1_NAME, discoveryQos, onSuccess, onError);

    EXPECT_CALL(mockCallback, onError(_)).Times(1);
    EXPECT_CALL(mockCallback, onSuccess(_)).Times(0);
    localCapabilitiesDirectory->lookup(
            twoDomains, INTERFACE_1_NAME, discoveryQos, onSuccess, onError);
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

    EXPECT_CALL(*capabilitiesClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(_), _, _)).Times(1);

    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultOnError);

    EXPECT_CALL(*capabilitiesClient, lookup(_, _, _, _, _)).Times(0);

    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryTimeout(5000);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::LOCAL_AND_GLOBAL);
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME}, INTERFACE_1_NAME, callback, discoveryQos);

    // set max timeout to get results to 10 ms
    auto results = callback->getResults(10);
    EXPECT_EQ(1, results.size());
    const joynr::types::DiscoveryEntryWithMetaInfo result = results.at(0);

    EXPECT_EQ(expectedEntry, result);
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

    EXPECT_CALL(*capabilitiesClient,
                add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(_), _, _)).Times(1);

    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultOnError);

    EXPECT_CALL(*capabilitiesClient, lookup(_, _, _, _, _)).Times(1).WillOnce(
            InvokeArgument<3>(globalEntryVec));

    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(0);
    discoveryQos.setDiscoveryTimeout(5000);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::LOCAL_AND_GLOBAL);

    // wait some time to be sure that the cached entry is not used
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME}, INTERFACE_1_NAME, callback, discoveryQos);

    // set max timeout to get results to 10 ms
    auto results = callback->getResults(10);
    EXPECT_EQ(1, results.size());
    const joynr::types::DiscoveryEntryWithMetaInfo result = results.at(0);

    EXPECT_EQ(expectedEntry, result);
}

TEST_F(LocalCapabilitiesDirectoryTest, callTouchPeriodically)
{
    EXPECT_CALL(*capabilitiesClient, touch(_, _, _)).Times(0);
    Mock::VerifyAndClearExpectations(capabilitiesClient.get());
    Semaphore semaphore(0);
    EXPECT_CALL(*capabilitiesClient, touch(Eq(clusterControllerId), _, _)).Times(2).WillRepeatedly(
            ReleaseSemaphore(&semaphore));
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(250)));
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(250)));
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
    joynr::Semaphore semaphore(0);
    EXPECT_CALL(*capabilitiesClient, add(An<const types::GlobalDiscoveryEntry&>(), _, _))
            .WillOnce(DoAll(
                          Invoke(this, &LocalCapabilitiesDirectoryTest::fakeCapabilitiesClientAddWithError),
                          ReleaseSemaphore(&semaphore)
                      )); // invoke onError
    localCapabilitiesDirectory->add(entry, true, defaultOnSuccess, defaultOnError);

    // wait for it...
    EXPECT_TRUE(semaphore.waitFor(std::chrono::seconds(1)));

    // 2nd call
    EXPECT_CALL(*capabilitiesClient, add(An<const types::GlobalDiscoveryEntry&>(), _, _))
            .WillOnce(DoAll(
                          Invoke(this, &LocalCapabilitiesDirectoryTest::fakeCapabilitiesClientAddSuccess),
                          ReleaseSemaphore(&semaphore)
                      )); // invoke onSuccess
    localCapabilitiesDirectory->add(entry, true, defaultOnSuccess, defaultOnError);

    // wait for it...
    EXPECT_TRUE(semaphore.waitFor(std::chrono::seconds(1)));

    // do a lookup to make sure the entry still exists
    EXPECT_CALL(*capabilitiesClient, lookup(_, _, _, _, _)).Times(0);
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME}, INTERFACE_1_NAME, callback, discoveryQos);
    EXPECT_EQ(1, callback->getResults(100).size());
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

    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultOnError);

    localCapabilitiesDirectory->lookup(dummyParticipantId1, callback);
    const int numberOfEntriesInLCD = !this->ENABLE_ACCESS_CONTROL || this->HAS_PERMISSION ? 1 : 0;
    EXPECT_EQ(numberOfEntriesInLCD, callback->getResults(TIMEOUT).size());
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
                localCapabilitiesDirectory, localDomainAccessController);
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

    localCapabilitiesDirectory->add(OK_entry,
                                    []() { SUCCEED() << "OK"; },
                                    [](const joynr::exceptions::ProviderRuntimeException& ex) {
                                        FAIL() << ex.getMessage();
                                    });

    localCapabilitiesDirectory->add(NOT_OK_entry_1,
                                    []() { FAIL(); },
                                    [](const joynr::exceptions::ProviderRuntimeException& ex) {
                                        SUCCEED() << ex.getMessage();
                                    });

    localCapabilitiesDirectory->add(NOT_OK_entry_2,
                                    []() { FAIL(); },
                                    [](const joynr::exceptions::ProviderRuntimeException& ex) {
                                        SUCCEED() << ex.getMessage();
                                    });

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
            *capabilitiesClient, add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(_), _, _))
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

    EXPECT_CALL(*capabilitiesClient, lookup(_, _, _, _, _)).Times(0);
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME}, INTERFACE_1_NAME, callback, discoveryQos);
    EXPECT_EQ(1, callback->getResults(10).size());
    callback->clearResults();

    // wait for cleanup timer to run
    std::this_thread::sleep_for(
            std::chrono::milliseconds(purgeExpiredDiscoveryEntriesIntervalMs * 2));

    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME}, INTERFACE_1_NAME, callback, discoveryQos);
    EXPECT_EQ(0, callback->getResults(10).size());
    callback->clearResults();
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
        EXPECT_CALL(*capabilitiesClient,
                    add(Matcher<const joynr::types::GlobalDiscoveryEntry&>(_), _, _))
                .Times(numberOfDuplicatedEntriesToAdd)
                .WillRepeatedly(Invoke(this, &LocalCapabilitiesDirectoryTest::fakeCapabilitiesClientAddWithError)); // invoke onError
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

    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME}, INTERFACE_1_NAME, callback, discoveryQos);
    std::vector<types::DiscoveryEntryWithMetaInfo> capabilities = callback->getResults(100);

    EXPECT_EQ(1, capabilities.size());
}

INSTANTIATE_TEST_CASE_P(changeProviderScope,
                        LocalCapabilitiesDirectoryWithProviderScope,
                        ::testing::Values(types::ProviderScope::LOCAL,
                                          types::ProviderScope::GLOBAL));
