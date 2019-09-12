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

#include <algorithm>
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
                std::dynamic_pointer_cast<const system::RoutingTypes::MqttAddress>(arg);
        if (mqttAddress == nullptr) {
            return false;
        }
        return serializer::serializeToJson(*mqttAddress) == serializedAddress;
    } else if (addressType == "http") {
        auto httpAddress =
                std::dynamic_pointer_cast<const system::RoutingTypes::ChannelAddress>(arg);
        if (httpAddress == nullptr) {
            return false;
        }
        return serializer::serializeToJson(*httpAddress) == serializedAddress;
    } else {
        return false;
    }
}

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
              dummyParticipantIdsVector{util::createUuid(), util::createUuid(), util::createUuid()},
              defaultOnSuccess([]() {}),
              defaultProviderRuntimeExceptionError([](const exceptions::ProviderRuntimeException&) {}),
              unexpectedProviderRuntimeExceptionFunction([](const exceptions::ProviderRuntimeException& exception) {
                  FAIL() << "Got unexpected ProviderRuntimeException: " + exception.getMessage(); }
              ),
              unexpectedOnDiscoveryErrorFunction([](const types::DiscoveryError::Enum& errorEnum) {
                  FAIL() << "Unexpected onError call: " + types::DiscoveryError::getLiteral(errorEnum);}
              ),
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
        discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);
        discoveryQos.setCacheMaxAge(10000);

        // init a capentry recieved from the global capabilities directory
        types::ProviderQos qos;
        qos.setScope(types::ProviderScope::GLOBAL);
        types::DiscoveryEntry globalCapEntry(defaultProviderVersion,
                                             DOMAIN_1_NAME,
                                             INTERFACE_1_NAME,
                                             dummyParticipantIdsVector[2],
                                             qos,
                                             10000,
                                             10000,
                                             PUBLIC_KEY_ID);
        globalCapEntryMap.insert({EXTERNAL_ADDRESSES_VECTOR[0], globalCapEntry});
        entry = types::DiscoveryEntry(defaultProviderVersion,
                                      DOMAIN_1_NAME,
                                      INTERFACE_1_NAME,
                                      dummyParticipantIdsVector[0],
                                      types::ProviderQos(),
                                      lastSeenDateMs,
                                      expiryDateMs,
                                      PUBLIC_KEY_ID);
        expectedGlobalCapEntry = types::GlobalDiscoveryEntry(entry.getProviderVersion(),
                                                             entry.getDomain(),
                                                             entry.getInterfaceName(),
                                                             entry.getParticipantId(),
                                                             entry.getQos(),
                                                             entry.getLastSeenDateMs(),
                                                             entry.getExpiryDateMs(),
                                                             entry.getPublicKeyId(),
                                                             LOCAL_ADDRESS);
    }

    ~LocalCapabilitiesDirectoryTest() override
    {
        singleThreadedIOService->stop();
        localCapabilitiesDirectory.reset();

        test::util::removeFileInCurrentDirectory(".*\\.settings");
        test::util::removeFileInCurrentDirectory(".*\\.persist");
    }

    void fakeLookupNoEntryForParticipant(
            const std::string& participantId,
            const std::vector<std::string>& gbids,
            std::int64_t messagingTtl,
            std::function<void(const std::vector<types::GlobalDiscoveryEntry>& capabilities)>
                    onSuccess,
            std::function<void(const types::DiscoveryError::Enum& errorEnum)> onError,
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

    void fakeCapabilitiesClientLookupWithDiscoveryException(
            const std::vector<std::string>& domains,
            const std::string& interfaceName,
            const std::vector<std::string>& gbids,
            const std::int64_t messagingTtl,
            std::function<void(const std::vector<types::GlobalDiscoveryEntry>& capabilities)>
                    onSuccess,
            std::function<void(const types::DiscoveryError::Enum& errorEnum)> onError,
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

    void fakeCapabilitiesClientAddWithException (
            const types::GlobalDiscoveryEntry& entry,
            const std::vector<std::string>& gbids,
            std::function<void()> onSuccess,
            std::function<void(const types::DiscoveryError::Enum& errorEnum)> onError,
            std::function<void(const exceptions::JoynrRuntimeException& error)> onRuntimeError) {
        std::ignore = entry;
        std::ignore = gbids;
        std::ignore = onSuccess;
        std::ignore = onError;
        exceptions::DiscoveryException fakeDiscoveryException("fakeDiscoveryException");
        onRuntimeError(fakeDiscoveryException);
    }

    void fakeCapabilitiesClientAddSuccess (
            const types::GlobalDiscoveryEntry& entry,
            const std::vector<std::string>& gbids,
            std::function<void()> onSuccess,
            std::function<void(const types::DiscoveryError::Enum& errorEnum)> onError,
            std::function<void(const exceptions::JoynrRuntimeException& error)> onRuntimeError) {
        std::ignore = entry;
        std::ignore = gbids;
        std::ignore = onError;
        std::ignore = onRuntimeError;
        onSuccess();
    }

    void testRemoveUsesSameGbidOrderAsAdd(const std::vector<std::string>& selectedGbids)
    {
        const bool awaitGlobalRegistration = true;
        const std::vector<std::string>& expectedGbids {selectedGbids};
        types::ProviderQos providerQos;
        providerQos.setScope(types::ProviderScope::GLOBAL);
        entry.setQos(providerQos);

        checkAddToGcdClient(expectedGbids);

        localCapabilitiesDirectory->add(
                entry,
                awaitGlobalRegistration,
                selectedGbids,
                createAddOnSuccessFunction(),
                unexpectedOnDiscoveryErrorFunction);
        EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

        EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                    remove(Eq(dummyParticipantIdsVector[0]),
                           Eq(expectedGbids), _, _, _)).Times(1);

        localCapabilitiesDirectory->remove(dummyParticipantIdsVector[0], defaultOnSuccess, defaultProviderRuntimeExceptionError);

        Mock::VerifyAndClearExpectations(globalCapabilitiesDirectoryClient.get());
    }

    std::vector<types::GlobalDiscoveryEntry> getGlobalDiscoveryEntries(const std::uint8_t numEntries)
    {
        if (numEntries > dummyParticipantIdsVector.size()) {
            for (std::uint8_t i = dummyParticipantIdsVector.size(); i <= numEntries; i++) {
                dummyParticipantIdsVector.push_back(util::createUuid());
            }
        }

        std::vector<types::GlobalDiscoveryEntry> globalDiscoveryEntryList;
        for(std::uint8_t i = 0; i < numEntries; i++) {
            globalDiscoveryEntryList.push_back(types::GlobalDiscoveryEntry(defaultProviderVersion,
                                                                     DOMAIN_1_NAME,
                                                                     INTERFACE_1_NAME,
                                                                     dummyParticipantIdsVector[i],
                                                                     types::ProviderQos(),
                                                                     LASTSEEN_MS,
                                                                     EXPIRYDATE_MS,
                                                                     PUBLIC_KEY_ID,
                                                                     EXTERNAL_ADDRESSES_VECTOR[i>2?0:i]));
        }
        return globalDiscoveryEntryList;
    }

    void testLookupByParticipantIdWithGbids_globalOnly_notCached_invokesGCDClient(const std::vector<std::string>& gbidsForLookup,
                                                             const std::vector<std::string>& expectedGbids)
    {
        discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
        EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                    lookup(Eq(dummyParticipantIdsVector[1]),
                    Eq(expectedGbids), Eq(discoveryQos.getDiscoveryTimeout()), _, _, _)).Times(1);
        localCapabilitiesDirectory->lookup(dummyParticipantIdsVector[1],
                                           discoveryQos,
                                           gbidsForLookup,
                                           nullptr,
                                           nullptr);
    }

    void testLookupByParticipantIdWithGbids_globalOnly_cached_doesNotInvokeGcdClient(const std::vector<std::string>& gbids)
    {
        discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
        // caching dummy entry
        types::ProviderQos providerQos;
        providerQos.setScope(types::ProviderScope::GLOBAL);
        entry.setQos(providerQos);
        const types::DiscoveryEntryWithMetaInfo expectedEntry = util::convert(false, entry);

        EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                    add(_, _, _, _, _)).Times(1)
                    .WillOnce(InvokeArgument<2>());

        localCapabilitiesDirectory->add(entry, true, KNOWN_GBIDS,
                                        createAddOnSuccessFunction(), unexpectedOnDiscoveryErrorFunction);

        EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
        Mock::VerifyAndClearExpectations(globalCapabilitiesDirectoryClient.get());

        // Entry is already cached GCD should not be involved
        EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _)).Times(0);
        EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);

        auto onSuccess = [this, expectedEntry] (const types::DiscoveryEntryWithMetaInfo& result) {
            EXPECT_EQ(expectedEntry, result);
            semaphore.notify();
        };

        localCapabilitiesDirectory->lookup(dummyParticipantIdsVector[0],
                                           discoveryQos,
                                           gbids,
                                           onSuccess,
                                           unexpectedOnDiscoveryErrorFunction);
        EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
    }

    void fakeLookupByParticipantIdWithResult(
            const std::string& participantId,
            const std::vector<std::string>& gbids,
            std::int64_t messagingTtl,
            std::function<void(const std::vector<types::GlobalDiscoveryEntry>& discoveryEntries)>
                    onSuccess,
            std::function<void(const types::DiscoveryError::Enum& errorEnum)> onError,
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
                                                                 EXTERNAL_ADDRESSES_VECTOR[0]));
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

    std::function<void(const std::vector<types::DiscoveryEntryWithMetaInfo>&)> createLookupSuccessFunction(
            const int expectedNumberOfEntries,
            std::vector<types::DiscoveryEntryWithMetaInfo>& returnValue)
    {
        return [this, expectedNumberOfEntries, &returnValue] (const std::vector<types::DiscoveryEntryWithMetaInfo>& result) {
            EXPECT_EQ(expectedNumberOfEntries, result.size());
            returnValue = result;
            semaphore.notify();
        };
    }

    std::function<void()> createAddOnSuccessFunction()
    {
        return [this] () {
            semaphore.notify();
        };
    }

    std::function<void(const types::DiscoveryEntryWithMetaInfo&)>
    createLookupParticipantIdSuccessFunction()
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

    std::function<void()> createUnexpectedAddOnSuccessFunction() {
        return [] () {
            FAIL();
        };
    }

    std::function<void(const exceptions::ProviderRuntimeException&)>
    createExpectedProviderRuntimeExceptionFunction(
            const joynr::exceptions::ProviderRuntimeException& expectedRuntimeException) {
        return [this, expectedRuntimeException] (const exceptions::ProviderRuntimeException& exception) {
            EXPECT_EQ(expectedRuntimeException, exception);
            semaphore.notify();
        };
    }

    void testAddToAllIsProperlyRejected(const types::DiscoveryError::Enum& expectedError)
    {
        const bool awaitGlobalRegistration = true;

        EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(AnConvertedGlobalDiscoveryEntry(entry)),
                _, _, _, _)).Times(1)
                .WillOnce(InvokeArgument<3>(expectedError));

        localCapabilitiesDirectory->addToAll(
                entry,
                awaitGlobalRegistration,
                createUnexpectedAddOnSuccessFunction(),
                createExpectedDiscoveryErrorFunction(expectedError));

        EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
    }

    void lookupByDomainInterfaceWithGbidsIsProperlyRejected(types::DiscoveryError::Enum errorEnum)
    {
        discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
        EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                lookup(ElementsAre(DOMAIN_1_NAME), INTERFACE_1_NAME, _, _, _, _, _))
                .Times(1)
                .WillOnce(InvokeArgument<5>(errorEnum));

        localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                           INTERFACE_1_NAME,
                                           discoveryQos,
                                           {},
                                           createUnexpectedLookupSuccessFunction(),
                                           createExpectedDiscoveryErrorFunction(errorEnum));
        EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
    }

    void lookupByParticipantIdWithGbidsIsProperlyRejected(types::DiscoveryError::Enum errorEnum)
    {
        discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
        EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                lookup(Eq(dummyParticipantIdsVector[1]), _, _, _, _, _))
                .Times(1)
                .WillOnce(InvokeArgument<4>(errorEnum));

        localCapabilitiesDirectory->lookup(dummyParticipantIdsVector[1],
                                           discoveryQos,
                                           KNOWN_GBIDS,
                                           createUnexpectedLookupParticipantIdSuccessFunction(),
                                           createExpectedDiscoveryErrorFunction(errorEnum));

        EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
    }

    void testLookupByDomainInterfaceWithGbids_gbidValidationFails(
            const std::vector<std::string>& gbids,
            types::DiscoveryError::Enum errorEnum)
    {
        EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
        EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _)).Times(0);

        localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                           INTERFACE_1_NAME,
                                           discoveryQos,
                                           gbids,
                                           createUnexpectedLookupSuccessFunction(),
                                           createExpectedDiscoveryErrorFunction(errorEnum));
        EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
    }

    void testLookupByParticipantIdWithGbids_gbidValidationFails(
            const std::vector<std::string>& gbids,
            types::DiscoveryError::Enum errorEnum)
    {
        EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _)).Times(0);
        EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);

        localCapabilitiesDirectory->lookup(dummyParticipantIdsVector[1],
                                           discoveryQos,
                                           gbids,
                                           createUnexpectedLookupParticipantIdSuccessFunction(),
                                           createExpectedDiscoveryErrorFunction(errorEnum));

        EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
    }

    void testAddWithGbidsIsProperlyRejected(const types::DiscoveryError::Enum& expectedDiscoveryError) {
        const bool awaitGlobalRegistration = true;
        const std::vector<std::string>& gbids = {KNOWN_GBIDS[0]};
        EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(An<const types::GlobalDiscoveryEntry&>(), _, _, _, _)).Times(1)
                .WillOnce(InvokeArgument<3>(expectedDiscoveryError));

        localCapabilitiesDirectory->add(
                entry,
                awaitGlobalRegistration,
                gbids,
                createUnexpectedAddOnSuccessFunction(),
                createExpectedDiscoveryErrorFunction(expectedDiscoveryError));

        EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
    }

    void testAddIsProperlyRejected(const types::DiscoveryError::Enum& expectedDiscoveryError) {
        const bool awaitGlobalRegistration = true;
        EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(An<const types::GlobalDiscoveryEntry&>(), _, _, _, _)).Times(1)
                .WillOnce(InvokeArgument<3>(expectedDiscoveryError));

        joynr::exceptions::ProviderRuntimeException expectedException(
                fmt::format("Error registering provider {} in default backend: {}",
                            entry.getParticipantId(),
                            types::DiscoveryError::getLiteral(expectedDiscoveryError)));
        localCapabilitiesDirectory->add(
                entry,
                awaitGlobalRegistration,
                createUnexpectedAddOnSuccessFunction(),
                createExpectedProviderRuntimeExceptionFunction(expectedException));

        EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
    }

    void checkAddToGcdClient(const std::vector<std::string>& expectedGbids) {
        EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Eq(expectedGlobalCapEntry),
                Eq(expectedGbids), _, _, _)).Times(1)
                .WillOnce(InvokeArgument<2>());
    }

    void testGbidValidationOnAdd(
            const std::vector<std::string>& gbids, const types::DiscoveryError::Enum& expectedDiscoveryError)
    {
        EXPECT_CALL(*globalCapabilitiesDirectoryClient, add(_, _, _, _, _)).Times(0);
        const bool awaitGlobalRegistration = true;
        localCapabilitiesDirectory->add(
                entry,
                awaitGlobalRegistration,
                gbids,
                createUnexpectedAddOnSuccessFunction(),
                createExpectedDiscoveryErrorFunction(expectedDiscoveryError));

        EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
    }

protected:
    void registerReceivedCapabilities(
            const std::string& addressType,
            const std::string& serializedAddress)
    {
        const std::string& participantId = "TEST_participantId";
        EXPECT_CALL(
                *mockMessageRouter,
                addNextHop(participantId,
                           AllOf(Pointee(A<const system::RoutingTypes::Address>()),
                                 pointerToAddressWithSerializedAddress(addressType, serializedAddress)),
                           _,
                           _,
                           _,
                           _,
                           _)).Times(1);
        EXPECT_CALL(*mockMessageRouter,
                    addNextHop(participantId,
                               AnyOf(Not(Pointee(A<const system::RoutingTypes::Address>())),
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
    std::vector<std::string> dummyParticipantIdsVector;
    types::DiscoveryQos discoveryQos;
    std::unordered_multimap<std::string, types::DiscoveryEntry> globalCapEntryMap;
    types::DiscoveryEntry entry;
    types::GlobalDiscoveryEntry expectedGlobalCapEntry;

    std::function<void()> defaultOnSuccess;
    std::function<void(const exceptions::ProviderRuntimeException&)> defaultProviderRuntimeExceptionError;
    std::function<void(const exceptions::ProviderRuntimeException&)> unexpectedProviderRuntimeExceptionFunction;
    std::function<void(const types::DiscoveryError::Enum& errorEnum)> unexpectedOnDiscoveryErrorFunction;
    types::Version defaultProviderVersion;
    Semaphore semaphore;

    static const std::vector<std::string> KNOWN_GBIDS;
    static const std::string INTERFACE_1_NAME;
    static const std::string DOMAIN_1_NAME;
    static const std::string INTERFACE_2_NAME;
    static const std::string DOMAIN_2_NAME;
    static const std::string INTERFACE_3_NAME;
    static const std::string DOMAIN_3_NAME;
    static const std::string LOCAL_ADDRESS;
    static const std::vector<std::string> EXTERNAL_ADDRESSES_VECTOR;
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
const std::string LocalCapabilitiesDirectoryTest::LOCAL_ADDRESS(
        serializer::serializeToJson(system::RoutingTypes::MqttAddress(KNOWN_GBIDS[0], "localTopic")));
const std::vector<std::string> LocalCapabilitiesDirectoryTest::EXTERNAL_ADDRESSES_VECTOR{
            serializer::serializeToJson(system::RoutingTypes::MqttAddress(KNOWN_GBIDS[0], "externalTopic")),
            serializer::serializeToJson(system::RoutingTypes::MqttAddress(KNOWN_GBIDS[1], "externalTopic")),
            serializer::serializeToJson(system::RoutingTypes::MqttAddress(KNOWN_GBIDS[2], "externalTopic"))};
const std::int64_t LocalCapabilitiesDirectoryTest::LASTSEEN_MS(1000);
const std::int64_t LocalCapabilitiesDirectoryTest::EXPIRYDATE_MS(10000);
const std::string LocalCapabilitiesDirectoryTest::PUBLIC_KEY_ID("publicKeyId");
const int LocalCapabilitiesDirectoryTest::TIMEOUT(2000);

TEST_F(LocalCapabilitiesDirectoryTest, add_global_invokesGcd)
{
    checkAddToGcdClient({KNOWN_GBIDS[0]});

    localCapabilitiesDirectory->add(entry, createAddOnSuccessFunction(), unexpectedProviderRuntimeExceptionFunction);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, addCapabilityWithSingleNonDefaultGbid)
{
    const std::vector<std::string>& gbids{KNOWN_GBIDS[1]};
    const std::vector<std::string>& expectedGbids = gbids;
    const bool awaitGlobalRegistration = true;
    checkAddToGcdClient(expectedGbids);

    localCapabilitiesDirectory->add(
            entry, awaitGlobalRegistration, gbids, createAddOnSuccessFunction(), unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, addWithGbids_global_multipleGbids_invokesGcd)
{
    const std::vector<std::string>& gbids{KNOWN_GBIDS[1], KNOWN_GBIDS[0]};
    const std::vector<std::string>& expectedGbids = gbids;
    const bool awaitGlobalRegistration = true;
    checkAddToGcdClient(expectedGbids);

    localCapabilitiesDirectory->add(
                entry, awaitGlobalRegistration, gbids, createAddOnSuccessFunction(), unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, addWithGbids_global_emptyGbidVector_addsToDefaultBackend)
{
    const std::vector<std::string>& gbids{};
    const std::vector<std::string>& expectedGbids {KNOWN_GBIDS[0]};
    const bool awaitGlobalRegistration = true;
    checkAddToGcdClient(expectedGbids);

    localCapabilitiesDirectory->add(entry,
                                    awaitGlobalRegistration,
                                    gbids,
                                    createAddOnSuccessFunction(),
                                    unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, addToAll_global_invokesGcd)
{
    const std::vector<std::string>& expectedGbids = KNOWN_GBIDS;
    const bool awaitGlobalRegistration = true;
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
             add(Eq(expectedGlobalCapEntry), Eq(expectedGbids), _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<2>());

    localCapabilitiesDirectory->addToAll(
            entry, awaitGlobalRegistration, createAddOnSuccessFunction(), unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, add_local_doesNotInvokeGcd)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);
    entry.setQos(providerQos);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
            add(An<const types::GlobalDiscoveryEntry&>(), _, _, _, _)).Times(0);

    localCapabilitiesDirectory->add(
            entry,
            createAddOnSuccessFunction(),
            unexpectedProviderRuntimeExceptionFunction);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, addGlobalCapSucceeds_NextAddShallAddGlobalAgain)
{
   const bool awaitGlobalRegistration = true;

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
             add(Matcher<const types::GlobalDiscoveryEntry&>(
                     AnConvertedGlobalDiscoveryEntry(entry)), _, _, _, _))
            .WillOnce(InvokeArgument<2>());

    localCapabilitiesDirectory->add(
            entry,
            awaitGlobalRegistration,
            createAddOnSuccessFunction(),
            unexpectedProviderRuntimeExceptionFunction);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    Mock::VerifyAndClearExpectations(globalCapabilitiesDirectoryClient.get());
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
             add(Matcher<const types::GlobalDiscoveryEntry&>(
                     AnConvertedGlobalDiscoveryEntry(entry)), _, _, _, _))
            .WillOnce(InvokeArgument<2>());

    localCapabilitiesDirectory->add(
            entry,
            awaitGlobalRegistration,
            createAddOnSuccessFunction(),
            unexpectedProviderRuntimeExceptionFunction);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddWithGbidsIsProperlyRejected_invalidGbid)
{
    testAddWithGbidsIsProperlyRejected(types::DiscoveryError::INVALID_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddWithGbidsIsProperlyRejected_unknownGbid)
{
    testAddWithGbidsIsProperlyRejected(types::DiscoveryError::UNKNOWN_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddWithGbidsIsProperlyRejected_internalError)
{
    testAddWithGbidsIsProperlyRejected(types::DiscoveryError::INTERNAL_ERROR);
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddIsProperlyRejected_invalidGbid)
{
    testAddIsProperlyRejected(types::DiscoveryError::INVALID_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddIsProperlyRejected_unknownGbid)
{
    testAddIsProperlyRejected(types::DiscoveryError::UNKNOWN_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddIsProperlyRejected_internalError)
{
    testAddIsProperlyRejected(types::DiscoveryError::INTERNAL_ERROR);
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddWithGbids_unknownGbid)
{
    const std::vector<std::string>& gbids{KNOWN_GBIDS[0], "unknownGbid"};
    testGbidValidationOnAdd(gbids, types::DiscoveryError::UNKNOWN_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddWithGbids_invalidGbid_emptyGbid)
{
    const std::vector<std::string>& gbids{""};
    testGbidValidationOnAdd(gbids, types::DiscoveryError::INVALID_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddWithGbids_invalidGbid_duplicateGbid)
{
    const std::vector<std::string>& gbids{KNOWN_GBIDS[1], KNOWN_GBIDS[1]};
    testGbidValidationOnAdd(gbids, types::DiscoveryError::INVALID_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, addSameGbidTwiceInARow)
{
    const bool awaitGlobalRegistration = true;
    const std::vector<std::string>& gbids{KNOWN_GBIDS[0]};
    const std::vector<std::string>& expectedGbids = gbids;

     EXPECT_CALL(*globalCapabilitiesDirectoryClient,
              add(Matcher<const types::GlobalDiscoveryEntry&>(
                      AnConvertedGlobalDiscoveryEntry(entry)), Eq(expectedGbids), _, _, _)).Times(2)
             .WillRepeatedly(InvokeArgument<2>());

     localCapabilitiesDirectory->add(
             entry,
             awaitGlobalRegistration,
             gbids,
             createAddOnSuccessFunction(),
             unexpectedOnDiscoveryErrorFunction);

     EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

     localCapabilitiesDirectory->add(
             entry,
             awaitGlobalRegistration,
             gbids,
             createAddOnSuccessFunction(),
             unexpectedOnDiscoveryErrorFunction);

     EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, addDifferentGbidsAfterEachOther)
{
    const bool awaitGlobalRegistration = true;
    const std::vector<std::string>& gbids1{KNOWN_GBIDS[0]};
    const std::vector<std::string>& expectedGbids1 = gbids1;
    const std::vector<std::string>& gbids2{KNOWN_GBIDS[1]};
    const std::vector<std::string>& expectedGbids2 = gbids2;

    checkAddToGcdClient(expectedGbids1);
    localCapabilitiesDirectory->add(
            entry,
            awaitGlobalRegistration,
            gbids1,
            createAddOnSuccessFunction(),
            unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    checkAddToGcdClient(expectedGbids2);
    localCapabilitiesDirectory->add(
            entry,
            awaitGlobalRegistration,
            gbids2,
            createAddOnSuccessFunction(),
            unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    // provider is now registered for both GBIDs
    localCapabilitiesDirectory->lookup(entry.getParticipantId(),
                                       discoveryQos,
                                       gbids1,
                                       createLookupParticipantIdSuccessFunction(),
                                       unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    localCapabilitiesDirectory->lookup(entry.getParticipantId(),
                                       discoveryQos,
                                       gbids2,
                                       createLookupParticipantIdSuccessFunction(),
                                       unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddKnownLocalEntryDoesNothing)
{
    const bool awaitGlobalRegistration = false;
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);
    entry.setQos(providerQos);
    types::DiscoveryEntry newDiscoveryEntry(entry);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(An<const types::GlobalDiscoveryEntry&>(),
                _, _, _, _)).Times(0);

    localCapabilitiesDirectory->add(
            entry,
            awaitGlobalRegistration,
            defaultOnSuccess,
            defaultProviderRuntimeExceptionError);

    localCapabilitiesDirectory->add(
            newDiscoveryEntry,
            awaitGlobalRegistration,
            defaultOnSuccess,
            defaultProviderRuntimeExceptionError);
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddToAll)
{
    const std::vector<std::string>& expectedGbids = KNOWN_GBIDS;
    const bool awaitGlobalRegistration = true;
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
            add(Matcher<const types::GlobalDiscoveryEntry&>(AnConvertedGlobalDiscoveryEntry(entry)),
            Eq(expectedGbids), _, _, _)).Times(1)
            .WillOnce(InvokeArgument<2>());

    localCapabilitiesDirectory->addToAll(
            entry, awaitGlobalRegistration, createAddOnSuccessFunction(), unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddToAllLocal)
{
    const bool awaitGlobalRegistration = true;
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);
    entry.setQos(providerQos);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
            add(An<const types::GlobalDiscoveryEntry&>(),
            _, _, _, _)).Times(0);

    localCapabilitiesDirectory->addToAll(
            entry,
            awaitGlobalRegistration,
            createAddOnSuccessFunction(),
            unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(2)));
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddToAllIsProperlyRejected_internalError)
{
    testAddToAllIsProperlyRejected(types::DiscoveryError::INTERNAL_ERROR);
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddToAllIsProperlyRejected_invalidGbid)
{
    testAddToAllIsProperlyRejected(types::DiscoveryError::INVALID_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddToAllIsProperlyRejected_unknownGbid)
{
    testAddToAllIsProperlyRejected(types::DiscoveryError::UNKNOWN_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddToAllIsProperlyRejected_exception)
{
    const bool awaitGlobalRegistration = true;

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
            add(Matcher<const types::GlobalDiscoveryEntry&>(AnConvertedGlobalDiscoveryEntry(entry)),
            Eq(KNOWN_GBIDS), _, _, _)).Times(1)
            .WillOnce(Invoke(this, &LocalCapabilitiesDirectoryTest::fakeCapabilitiesClientAddWithException));

    localCapabilitiesDirectory->addToAll(
            entry,
            awaitGlobalRegistration,
            createUnexpectedAddOnSuccessFunction(),
            createExpectedDiscoveryErrorFunction(types::DiscoveryError::INTERNAL_ERROR));

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByDomainInterfaceWithGbids_emptyDomainsVector_throwsProviderRuntimeException)
{
    const std::vector<std::string>& emptyDomains{};

    EXPECT_THROW(localCapabilitiesDirectory->lookup(emptyDomains,
                                           INTERFACE_1_NAME,
                                           discoveryQos,
                                           KNOWN_GBIDS,
                                           createUnexpectedLookupSuccessFunction(),
                                           unexpectedOnDiscoveryErrorFunction),
                                           exceptions::ProviderRuntimeException);
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupByDomainInterfaceWithGbids_globalOnly_noLocalButRemoteCachedEntries_doesNotInvokeGcd_returnsFilteredResult)
{
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
    const std::vector<types::GlobalDiscoveryEntry> discoveryEntryResultList =
            getGlobalDiscoveryEntries(KNOWN_GBIDS.size());
    std::vector<types::DiscoveryEntryWithMetaInfo> expectedEntries;
    for (const auto& entry : discoveryEntryResultList) {
        expectedEntries.push_back(util::convert(false, entry));
    }

    // simulate global capability directory would store three entries.
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
            lookup(ElementsAre(DOMAIN_1_NAME), INTERFACE_1_NAME, _, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<4>(discoveryEntryResultList));

    std::vector<types::DiscoveryEntryWithMetaInfo> result;
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(discoveryEntryResultList.size(), result),
                                       unexpectedOnDiscoveryErrorFunction);

    ASSERT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
    for (const auto& expectedEntry : expectedEntries) {
        ASSERT_TRUE(std::find(result.cbegin(), result.cend(), expectedEntry) != result.cend());
    }

    Mock::VerifyAndClearExpectations(globalCapabilitiesDirectoryClient.get());

    // enries are now in cache, globalCapabilitiesDirectoryClient should not be called.
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _)).Times(0);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);

    result = {};
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       {KNOWN_GBIDS[1]},
                                       createLookupSuccessFunction(1, result),
                                       unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
    EXPECT_TRUE(std::find(result.cbegin(), result.cend(), expectedEntries[1]) != result.cend());

    result = {};
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       {KNOWN_GBIDS[0]},
                                       createLookupSuccessFunction(1, result),
                                       unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
    EXPECT_TRUE(std::find(result.cbegin(), result.cend(), expectedEntries[0]) != result.cend());
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupByDomainInterfaceWithGbids_globalOnly_localGlobalEntries_doesNotInvokeGcd_returnsFilteredResult)
{
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);

    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);
    entry.setQos(providerQos);

    const types::DiscoveryEntryWithMetaInfo expectedEntry = util::convert(false, entry);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
            add(AnConvertedGlobalDiscoveryEntry(entry), Eq(KNOWN_GBIDS), _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<2>());

    localCapabilitiesDirectory->add(entry, true, KNOWN_GBIDS, createAddOnSuccessFunction(), unexpectedOnDiscoveryErrorFunction);

    ASSERT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    // local entry is now registered and cached for all GBIDs with the same participantId
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _)).Times(0);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);

    std::vector<types::DiscoveryEntryWithMetaInfo> result;
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(1, result),
                                       unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
    EXPECT_TRUE(std::find(result.cbegin(), result.cend(), expectedEntry) != result.cend());

    // search in the 3rd backend for global entries. There is only one entry
    result = {};
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       std::vector<std::string>{KNOWN_GBIDS[2]},
                                       createLookupSuccessFunction(1, result),
                                       unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
    EXPECT_TRUE(std::find(result.cbegin(), result.cend(), expectedEntry) != result.cend());
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupByParticipantIdWithGbids_globalOnly_localGlobalEntries_doesNotInvokeGcd_returnsFilteredResult)
{
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);

    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);
    entry.setQos(providerQos);

    const types::DiscoveryEntryWithMetaInfo expectedEntry = util::convert(false, entry);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
            add(AnConvertedGlobalDiscoveryEntry(entry), Eq(KNOWN_GBIDS), _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<2>());

    localCapabilitiesDirectory->add(entry, true, KNOWN_GBIDS, createAddOnSuccessFunction(), unexpectedOnDiscoveryErrorFunction);

    ASSERT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    // local entry is now registered and cached for all GBIDs with the same participantId
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _)).Times(0);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);

    auto onSuccess = [this, expectedEntry] (const types::DiscoveryEntryWithMetaInfo& result) {
        EXPECT_EQ(expectedEntry, result);
        semaphore.notify();
    };

    std::vector<std::string> gbids{KNOWN_GBIDS[1]};
    localCapabilitiesDirectory->lookup(dummyParticipantIdsVector[0],
                                       discoveryQos,
                                       gbids,
                                       onSuccess,
                                       unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    gbids = {KNOWN_GBIDS[0]};
    localCapabilitiesDirectory->lookup(dummyParticipantIdsVector[0],
                                       discoveryQos,
                                       gbids,
                                       onSuccess,
                                       unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    localCapabilitiesDirectory->lookup(dummyParticipantIdsVector[0],
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       onSuccess,
                                       unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByDomainInterfaceWithGbids_globalOnly_multipleGbids_allCached)
{
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
    std::vector<types::GlobalDiscoveryEntry> discoveryEntryResultList =
            getGlobalDiscoveryEntries(6);

    // simulate global capability directory would store five entries
    // globalEntry_withParticipantId0_in_gbid0, globalEntry_withParticipantId1_in_gbid1,
    // globalEntry_withParticipantId2_in_gbid2, globalEntry_withParticipantId3_in_gbid0,
    // globalEntry_withParticipantId4_in_gbid0, globalEntry_withParticipantId5_in_gbid0
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
            lookup(ElementsAre(DOMAIN_1_NAME), INTERFACE_1_NAME, _, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<4>(discoveryEntryResultList));

    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(discoveryEntryResultList.size()),
                                       unexpectedOnDiscoveryErrorFunction);

    ASSERT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    Mock::VerifyAndClearExpectations(globalCapabilitiesDirectoryClient.get());

    // 6 enries are now in cache
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _)).Times(0);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);

    const std::vector<std::string>& multipleGbids{KNOWN_GBIDS[0], KNOWN_GBIDS[2]};
    const std::uint8_t expectedReturnedGlobalEntries = 5;
    std::vector<types::DiscoveryEntryWithMetaInfo> result;
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       multipleGbids,
                                       createLookupSuccessFunction(expectedReturnedGlobalEntries, result),
                                       unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
    auto getPredicate = [] (const std::string& participantId) {
        return [participantId](const types::DiscoveryEntryWithMetaInfo& entry) {
            return entry.getParticipantId() == participantId;
        };
    };
    EXPECT_TRUE(std::find_if(result.cbegin(), result.cend(), getPredicate(dummyParticipantIdsVector[0])) != result.cend());
    EXPECT_TRUE(std::find_if(result.cbegin(), result.cend(), getPredicate(dummyParticipantIdsVector[2])) != result.cend());
    EXPECT_TRUE(std::find_if(result.cbegin(), result.cend(), getPredicate(dummyParticipantIdsVector[3])) != result.cend());
    EXPECT_TRUE(std::find_if(result.cbegin(), result.cend(), getPredicate(dummyParticipantIdsVector[4])) != result.cend());
    EXPECT_TRUE(std::find_if(result.cbegin(), result.cend(), getPredicate(dummyParticipantIdsVector[5])) != result.cend());
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByDomainInterfaceWithGbids_globalOnly_emptyGbidsVector_allCached)
{
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
    std::vector<types::GlobalDiscoveryEntry> discoveryEntryResultList =
            getGlobalDiscoveryEntries(6);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
            lookup(ElementsAre(DOMAIN_1_NAME), INTERFACE_1_NAME, _, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<4>(discoveryEntryResultList));

    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(discoveryEntryResultList.size()),
                                       unexpectedOnDiscoveryErrorFunction);

    ASSERT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    Mock::VerifyAndClearExpectations(globalCapabilitiesDirectoryClient.get());

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _)).Times(0);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);

    const std::vector<std::string>& emptyGbids{};
    const std::uint8_t expectedReturnedGlobalEntries = 6;
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       emptyGbids,
                                       createLookupSuccessFunction(expectedReturnedGlobalEntries),
                                       unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByDomainInterfaceWithGbids_globalOnly_multipleGbids_noneCached)
{
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
    std::vector<types::GlobalDiscoveryEntry> discoveryEntryResultList = getGlobalDiscoveryEntries(2);
    std::vector<std::string> multipleGbids{KNOWN_GBIDS[0], KNOWN_GBIDS[1]};

    // no entries cached, lookup of GlobalCapabilitiesDirectoryClient will be called
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
            lookup(ElementsAre(DOMAIN_1_NAME), INTERFACE_1_NAME, Eq(multipleGbids), _, _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<4>(discoveryEntryResultList));

    const std::uint8_t& expectedReturnedGlobalEntries = 2;
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       multipleGbids,
                                       createLookupSuccessFunction(expectedReturnedGlobalEntries),
                                       unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByDomainInterfaceWithGbids_globalOnly_emptyGbidsVector_noneCached)
{
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
    std::vector<types::GlobalDiscoveryEntry> discoveryEntryResultList = getGlobalDiscoveryEntries(2);

    // no entries cached, lookup of GlobalCapabilitiesDirectoryClient will be called
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
            lookup(ElementsAre(DOMAIN_1_NAME), INTERFACE_1_NAME, Eq(KNOWN_GBIDS), _, _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<4>(discoveryEntryResultList));

    const std::uint8_t expectedReturnedGlobalEntries = 2;
    std::vector<std::string> emptyGbidsVector{};
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       emptyGbidsVector,
                                       createLookupSuccessFunction(expectedReturnedGlobalEntries),
                                       unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByParticipantIdWithGbids_globalOnly_localEntry_invokesGcd_returnsRemoteResultAndNotLocalEntry) {
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);

    std::vector<std::string> gbidsForLookup {KNOWN_GBIDS[2], KNOWN_GBIDS[0]};
    std::vector<std::string> expectedGbids = gbidsForLookup;

    types::GlobalDiscoveryEntry remoteEntry(entry.getProviderVersion(),
                                            entry.getDomain(),
                                            entry.getInterfaceName(),
                                            entry.getParticipantId(),
                                            entry.getQos(),
                                            entry.getLastSeenDateMs() + 42,
                                            entry.getExpiryDateMs() + 10,
                                            entry.getPublicKeyId(),
                                            EXTERNAL_ADDRESSES_VECTOR[0]);
    types::DiscoveryEntryWithMetaInfo expectedEntry = util::convert(false, remoteEntry);

    // register local entry
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);
    entry.setQos(providerQos);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(0);
    localCapabilitiesDirectory->add(
                entry,
                true,
                createAddOnSuccessFunction(),
                unexpectedProviderRuntimeExceptionFunction);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    const std::vector<types::GlobalDiscoveryEntry>& onSuccessResult {remoteEntry};

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(
                    Eq(dummyParticipantIdsVector[0]),
                    Eq(expectedGbids),
                    Eq(discoveryQos.getDiscoveryTimeout()),
                    _,
                    _,
                    _))
                .Times(1).WillOnce(InvokeArgument<3>(onSuccessResult));

    auto onSuccess = [this, expectedEntry]
            (const joynr::types::DiscoveryEntryWithMetaInfo& result) {
        EXPECT_EQ(expectedEntry, result);
        semaphore.notify();
    };

    localCapabilitiesDirectory->lookup(dummyParticipantIdsVector[0],
            discoveryQos,
            gbidsForLookup,
            onSuccess,
            unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByParticipantIdWithGbids_globalOnly_multipleGbids_allCached) {
    std::vector<std::string> gbids{KNOWN_GBIDS[0], KNOWN_GBIDS[2]};
    testLookupByParticipantIdWithGbids_globalOnly_cached_doesNotInvokeGcdClient(gbids);

    gbids = {KNOWN_GBIDS[1], KNOWN_GBIDS[2]};
    testLookupByParticipantIdWithGbids_globalOnly_cached_doesNotInvokeGcdClient(gbids);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByParticipantIdWithGbids_globalOnly_multipleGbids_noneCached) {
    std::vector<std::string> gbidsForLookup{KNOWN_GBIDS[0], KNOWN_GBIDS[2]};
    std::vector<std::string> expectedGbids{gbidsForLookup};
    testLookupByParticipantIdWithGbids_globalOnly_notCached_invokesGCDClient(gbidsForLookup, expectedGbids);

    gbidsForLookup = {KNOWN_GBIDS[1], KNOWN_GBIDS[2]};
    expectedGbids = {gbidsForLookup};
    testLookupByParticipantIdWithGbids_globalOnly_notCached_invokesGCDClient(gbidsForLookup, expectedGbids);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByParticipantIdWithGbids_globalOnly_emptyGbidsVector_allCached) {
    std::vector<std::string> gbidsForLookup{};
    testLookupByParticipantIdWithGbids_globalOnly_cached_doesNotInvokeGcdClient(gbidsForLookup);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByParticipantIdWithGbids_globalOnly_emptyGbidsVector_noneCached) {
    std::vector<std::string> gbidsForLookup{};
    testLookupByParticipantIdWithGbids_globalOnly_notCached_invokesGCDClient(gbidsForLookup, KNOWN_GBIDS);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByDomainInterfaceWithGbidsIsProperlyRejected_exception)
{
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
            lookup(ElementsAre(DOMAIN_1_NAME), INTERFACE_1_NAME, _, _, _, _, _))
            .Times(1)
            .WillOnce(Invoke(this, &LocalCapabilitiesDirectoryTest::
                             fakeCapabilitiesClientLookupWithDiscoveryException));

    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createUnexpectedLookupSuccessFunction(),
                                       createExpectedDiscoveryErrorFunction(
                                           types::DiscoveryError::INTERNAL_ERROR));
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByDomainInterfaceWithGbidsIsProperlyRejected_invalidGbid)
{
    lookupByDomainInterfaceWithGbidsIsProperlyRejected(types::DiscoveryError::Enum::INVALID_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByDomainInterfaceWithGbidsIsProperlyRejected_unknownGbid)
{
    lookupByDomainInterfaceWithGbidsIsProperlyRejected(types::DiscoveryError::Enum::UNKNOWN_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByDomainInterfaceWithGbidsIsProperlyRejected_internalError)
{
    lookupByDomainInterfaceWithGbidsIsProperlyRejected(types::DiscoveryError::INTERNAL_ERROR);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByDomainInterfaceWithGbidsIsProperlyRejected_noEntryForSelectedBackend)
{
    lookupByDomainInterfaceWithGbidsIsProperlyRejected(types::DiscoveryError::NO_ENTRY_FOR_SELECTED_BACKENDS);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByParticipantIdWithGbidsIsProperlyRejected_exception)
{
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
    exceptions::DiscoveryException fakeDiscoveryException("fakeDiscoveryException");
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
            lookup(Eq(dummyParticipantIdsVector[1]), _, Eq(discoveryQos.getDiscoveryTimeout()), _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<5>(fakeDiscoveryException));

    localCapabilitiesDirectory->lookup(dummyParticipantIdsVector[1],
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createUnexpectedLookupParticipantIdSuccessFunction(),
                                       createExpectedDiscoveryErrorFunction(
                                           types::DiscoveryError::INTERNAL_ERROR));

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByParticipantIdWithGbidsIsProperlyRejected_invalidGbid)
{
    lookupByParticipantIdWithGbidsIsProperlyRejected(types::DiscoveryError::INVALID_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByParticipantIdWithGbidsIsProperlyRejected_unknownGbid)
{
    lookupByParticipantIdWithGbidsIsProperlyRejected(types::DiscoveryError::UNKNOWN_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByParticipantIdWithGbidsIsProperlyRejected_internalError)
{
    lookupByParticipantIdWithGbidsIsProperlyRejected(types::DiscoveryError::INTERNAL_ERROR);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByParticipantIdWithGbidsIsProperlyRejected_noEntryForSelectedBackend)
{
    lookupByParticipantIdWithGbidsIsProperlyRejected(types::DiscoveryError::NO_ENTRY_FOR_SELECTED_BACKENDS);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByParticipantIdWithGbidsIsProperlyRejected_noEntryForParticipant)
{
    lookupByParticipantIdWithGbidsIsProperlyRejected(types::DiscoveryError::NO_ENTRY_FOR_PARTICIPANT);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByDomainInterfaceWithGbids_unknownGbids)
{
    const std::vector<std::string>& unknownGbids{"unknownGbid", KNOWN_GBIDS[0]};
    testLookupByDomainInterfaceWithGbids_gbidValidationFails(unknownGbids, types::DiscoveryError::UNKNOWN_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByParticipantIdWithGbids_unknownGbids)
{
    const std::vector<std::string>& unknownGbids{"unknownGbid", KNOWN_GBIDS[0]};
    testLookupByParticipantIdWithGbids_gbidValidationFails(unknownGbids, types::DiscoveryError::UNKNOWN_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByDomainInterfaceWithGbids_invalidGbid_emptyGbid)
{
    const std::vector<std::string>& emptyGbid{""};
    testLookupByDomainInterfaceWithGbids_gbidValidationFails(emptyGbid, types::DiscoveryError::INVALID_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByParticipantIdWithGbids_invalidGbid_emptyGbid)
{
    const std::vector<std::string>& emptyGbid{""};
    testLookupByParticipantIdWithGbids_gbidValidationFails(emptyGbid, types::DiscoveryError::INVALID_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByDomainInterfaceWithGbids_invalidGbid_duplicateGbid)
{
    const std::vector<std::string>& duplicateGbid{KNOWN_GBIDS[1], KNOWN_GBIDS[0], KNOWN_GBIDS[1]};
    testLookupByDomainInterfaceWithGbids_gbidValidationFails(duplicateGbid, types::DiscoveryError::INVALID_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByParticipantIdWithGbids_invalidGbid_duplicateGbid)
{
    const std::vector<std::string>& duplicateGbid{KNOWN_GBIDS[1], KNOWN_GBIDS[0], KNOWN_GBIDS[1]};
    testLookupByParticipantIdWithGbids_gbidValidationFails(duplicateGbid, types::DiscoveryError::INVALID_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByDomainInterface_globalOnly_emptyCache_invokesGcd)
{
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
    const std::vector<types::GlobalDiscoveryEntry>& discoveryEntriesResultList{};
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
            lookup(ElementsAre(DOMAIN_1_NAME), INTERFACE_1_NAME, Eq(KNOWN_GBIDS), _, _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<4>(discoveryEntriesResultList));

    auto onSuccess = [this] (const std::vector<types::DiscoveryEntryWithMetaInfo>& result) {
        EXPECT_EQ(0, result.size());
        semaphore.notify();
    };

    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       onSuccess,
                                       unexpectedProviderRuntimeExceptionFunction);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByDomainInterface_globalOnly_remoteEntryInCache_doesNotInvokeGcd)
{
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
    const std::vector<types::GlobalDiscoveryEntry> discoveryEntriesResultList = getGlobalDiscoveryEntries(2);
    const std::vector<types::GlobalDiscoveryEntry> expectedResult = discoveryEntriesResultList;

    auto onSuccess = [this, &expectedResult]
            (const std::vector<types::DiscoveryEntryWithMetaInfo>& result) {
        EXPECT_EQ(expectedResult.size(), result.size());
        semaphore.notify();
    };

    // add DiscoveryEntries to cache
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
            lookup(ElementsAre(DOMAIN_1_NAME), INTERFACE_1_NAME, Eq(KNOWN_GBIDS), _, _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<4>(discoveryEntriesResultList));

    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       onSuccess,
                                       defaultProviderRuntimeExceptionError);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    Mock::VerifyAndClearExpectations(globalCapabilitiesDirectoryClient.get());

    // do actual lookup
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _)).Times(0);
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       onSuccess,
                                       defaultProviderRuntimeExceptionError);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByDomainInterface_localThenGlobal_emptyCache_invokesGcd)
{
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);
    const std::vector<types::GlobalDiscoveryEntry>& discoveryEntriesResultList{};
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
            lookup(ElementsAre(DOMAIN_1_NAME), INTERFACE_1_NAME, Eq(KNOWN_GBIDS), _, _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<4>(discoveryEntriesResultList));

    auto onSuccess = [this] (const std::vector<types::DiscoveryEntryWithMetaInfo>& result) {
        EXPECT_EQ(0, result.size());
        semaphore.notify();
    };

    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       onSuccess,
                                       defaultProviderRuntimeExceptionError);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByDomainInterface_localThenGlobal_localEntryCached_doesNotInvokeGcd)
{
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _)).Times(0);

    // add local capability
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);
    entry.setQos(providerQos);
    const types::DiscoveryEntryWithMetaInfo expectedEntry = util::convert(true, entry);
    localCapabilitiesDirectory->add(entry, createAddOnSuccessFunction(), unexpectedProviderRuntimeExceptionFunction);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    // do actual lookup
    auto onSuccess = [this, &expectedEntry]
            (const std::vector<types::DiscoveryEntryWithMetaInfo>& result) {
        ASSERT_EQ(1, result.size());
        EXPECT_EQ(expectedEntry, result[0]);
        semaphore.notify();
    };

    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       onSuccess,
                                       defaultProviderRuntimeExceptionError);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByDomainInterface_localThenGlobal_remoteEntryCached_doesNotInvokeGcd)
{
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);
    const std::vector<types::GlobalDiscoveryEntry> discoveryEntriesResultList = getGlobalDiscoveryEntries(1);
    const types::DiscoveryEntryWithMetaInfo expectedResult = util::convert(false, discoveryEntriesResultList[0]);

    auto onSuccess = [this, &expectedResult]
            (const std::vector<types::DiscoveryEntryWithMetaInfo>& result) {
        ASSERT_EQ(1, result.size());
        EXPECT_EQ(expectedResult, result[0]);
        semaphore.notify();
    };

    // add DiscoveryEntries to cache
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
            lookup(ElementsAre(DOMAIN_1_NAME), INTERFACE_1_NAME, Eq(KNOWN_GBIDS), _, _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<4>(discoveryEntriesResultList));

    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       onSuccess,
                                       defaultProviderRuntimeExceptionError);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    Mock::VerifyAndClearExpectations(globalCapabilitiesDirectoryClient.get());

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _)).Times(0);

    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       onSuccess,
                                       defaultProviderRuntimeExceptionError);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByParticipantId_localThenGlobal_localEntry_doesNotInvokeGcdAndReturnsLocalEntry)
{
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);

    // add local capability
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);
    entry.setQos(providerQos);
    const types::DiscoveryEntryWithMetaInfo expectedEntry = util::convert(true, entry);
    localCapabilitiesDirectory->add(entry, createAddOnSuccessFunction(), defaultProviderRuntimeExceptionError);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _)).Times(0);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);

    auto onSuccess = [this, &expectedEntry] (const types::DiscoveryEntryWithMetaInfo& result) {
        EXPECT_EQ(expectedEntry, result);
        semaphore.notify();
    };

    localCapabilitiesDirectory->lookup(dummyParticipantIdsVector[0],
                                       onSuccess,
                                       defaultProviderRuntimeExceptionError);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, removeCapabilities_invokesGcdClient)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);
    types::DiscoveryEntry entry1(defaultProviderVersion,
                                        DOMAIN_1_NAME,
                                        INTERFACE_1_NAME,
                                        dummyParticipantIdsVector[0],
                                        providerQos,
                                        lastSeenDateMs,
                                        expiryDateMs,
                                        PUBLIC_KEY_ID);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(1);

    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultProviderRuntimeExceptionError);

    Mock::VerifyAndClearExpectations(globalCapabilitiesDirectoryClient.get());

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                remove(Eq(dummyParticipantIdsVector[0]),
                       Eq(std::vector<std::string>{KNOWN_GBIDS[0]}), _, _, _)).Times(1);

    localCapabilitiesDirectory->remove(dummyParticipantIdsVector[0],
            defaultOnSuccess, defaultProviderRuntimeExceptionError);
}

TEST_F(LocalCapabilitiesDirectoryTest, testRemove_GcdNotCalledIfParticipantIsNotRegistered)
{
    // There is no discoveryEntry registered. Gcd won't be invoked. Lcd will call onRuntimeError
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                remove(_, _, _, _, _)).Times(0);

    exceptions::ProviderRuntimeException expectedException(
                    fmt::format("Failed to remove participantId: {}. ParticipantId is not "
                                "registered in cluster controller.",
                                dummyParticipantIdsVector[0]));
    localCapabilitiesDirectory->remove(dummyParticipantIdsVector[0],
            defaultOnSuccess, createExpectedProviderRuntimeExceptionFunction(expectedException));

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, testRemoveUsesSameGbidOrderAsAdd)
{
    testRemoveUsesSameGbidOrderAsAdd({KNOWN_GBIDS[0]});
    testRemoveUsesSameGbidOrderAsAdd({KNOWN_GBIDS[1]});
    testRemoveUsesSameGbidOrderAsAdd({KNOWN_GBIDS[0], KNOWN_GBIDS[1] });
    testRemoveUsesSameGbidOrderAsAdd({KNOWN_GBIDS[1], KNOWN_GBIDS[0] });
}

TEST_F(LocalCapabilitiesDirectoryTest, reregisterGlobalCapabilities)
{
    types::DiscoveryEntry entry1(defaultProviderVersion,
                                        DOMAIN_1_NAME,
                                        INTERFACE_1_NAME,
                                        dummyParticipantIdsVector[0],
                                        types::ProviderQos(),
                                        lastSeenDateMs,
                                        expiryDateMs,
                                        PUBLIC_KEY_ID);

    types::DiscoveryEntry entry2(defaultProviderVersion,
                                        DOMAIN_2_NAME,
                                        INTERFACE_2_NAME,
                                        dummyParticipantIdsVector[1],
                                        types::ProviderQos(),
                                        lastSeenDateMs,
                                        expiryDateMs,
                                        PUBLIC_KEY_ID);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(
                            AnConvertedGlobalDiscoveryEntry(entry1)),
                    _,
                    _,
                    _,
                    _)).Times(1);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(
                            AnConvertedGlobalDiscoveryEntry(entry2)),
                    _,
                    _,
                    _,
                    _)).Times(1);

    localCapabilitiesDirectory->add(entry1, defaultOnSuccess, defaultProviderRuntimeExceptionError);

    localCapabilitiesDirectory->add(entry2, defaultOnSuccess, defaultProviderRuntimeExceptionError);

    Mock::VerifyAndClearExpectations(globalCapabilitiesDirectoryClient.get());

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(
                            AnConvertedGlobalDiscoveryEntry(entry1)),
                    _,
                    _,
                    _,
                    _)).Times(1);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(
                            AnConvertedGlobalDiscoveryEntry(entry2)),
                    _,
                    _,
                    _,
                    _)).Times(1);

    bool onSuccessCalled = false;
    localCapabilitiesDirectory->triggerGlobalProviderReregistration(
            [&onSuccessCalled]() { onSuccessCalled = true; },
            [](const exceptions::ProviderRuntimeException&) { FAIL(); });

    Mock::VerifyAndClearExpectations(globalCapabilitiesDirectoryClient.get());
    EXPECT_TRUE(onSuccessCalled);
}

TEST_F(LocalCapabilitiesDirectoryTest,
       doNotReregisterDiscoveryEntriesFromGlobalCapabilitiesDirectory)
{
    types::DiscoveryEntry entry1(defaultProviderVersion,
                                        DOMAIN_1_NAME,
                                        INTERFACE_1_NAME,
                                        dummyParticipantIdsVector[0],
                                        types::ProviderQos(),
                                        lastSeenDateMs,
                                        expiryDateMs,
                                        PUBLIC_KEY_ID);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(
                            AnConvertedGlobalDiscoveryEntry(entry1)),
                    _,
                    _,
                    _,
                    _)).Times(1);

    localCapabilitiesDirectory->add(entry1, defaultOnSuccess, defaultProviderRuntimeExceptionError);

    types::DiscoveryEntry entry2(defaultProviderVersion,
                                        DOMAIN_2_NAME,
                                        INTERFACE_2_NAME,
                                        dummyParticipantIdsVector[1],
                                        types::ProviderQos(),
                                        lastSeenDateMs,
                                        expiryDateMs,
                                        PUBLIC_KEY_ID);

    localCapabilitiesDirectory->registerReceivedCapabilities({{EXTERNAL_ADDRESSES_VECTOR[0], entry2}});

    Mock::VerifyAndClearExpectations(globalCapabilitiesDirectoryClient.get());

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(
                            AnConvertedGlobalDiscoveryEntry(entry1)),
                    _,
                    _,
                    _,
                    _)).Times(1);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(
                            AnConvertedGlobalDiscoveryEntry(entry2)),
                    _,
                    _,
                    _,
                    _)).Times(0);

    bool onSuccessCalled = false;
    localCapabilitiesDirectory->triggerGlobalProviderReregistration(
            [&onSuccessCalled]() { onSuccessCalled = true; },
            [](const exceptions::ProviderRuntimeException&) { FAIL(); });

    EXPECT_TRUE(onSuccessCalled);
}

TEST_F(LocalCapabilitiesDirectoryTest,
       reregisterGlobalCapabilities_BackendNotCalledIfNoGlobalProvidersArePresent)
{
    types::ProviderQos localProviderQos({}, 1, types::ProviderScope::LOCAL, false);
    types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantIdsVector[0],
                                       localProviderQos,
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);

    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultProviderRuntimeExceptionError);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _))
            .Times(0);

    bool onSuccessCalled = false;
    localCapabilitiesDirectory->triggerGlobalProviderReregistration(
            [&onSuccessCalled]() { onSuccessCalled = true; },
            [](const exceptions::ProviderRuntimeException&) { FAIL(); });

    EXPECT_TRUE(onSuccessCalled);
}

TEST_F(LocalCapabilitiesDirectoryTest, addAddsToCache)
{
    EXPECT_CALL(
            *globalCapabilitiesDirectoryClient,
            lookup(dummyParticipantIdsVector[0],
                   _,
                   _,
                   A<std::function<void(
                           const std::vector<types::GlobalDiscoveryEntry>& capabilities)>>(),
                   _,
                   A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(0);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(1);

    types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantIdsVector[0],
                                       types::ProviderQos(),
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);
    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultProviderRuntimeExceptionError);

    const types::DiscoveryQos discoveryQos;
    localCapabilitiesDirectory->lookup(dummyParticipantIdsVector[0],
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupParticipantIdSuccessFunction(),
                                       unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, addLocallyDoesNotCallCapabilitiesClient)
{
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                lookup(_,
                       _,
                       _,
                       A<std::function<void(const std::vector<types::GlobalDiscoveryEntry>&
                                                    discoveryEntries)>>(),
                       _,
                       A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(0);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(0);
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);

    types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantIdsVector[0],
                                       providerQos,
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);
    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultProviderRuntimeExceptionError);

    const types::DiscoveryQos discoveryQos;
    localCapabilitiesDirectory->lookup(dummyParticipantIdsVector[0],
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupParticipantIdSuccessFunction(),
                                       unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupForInterfaceAddressReturnsCachedValues)
{
    // simulate global capability directory would store two entries.
    const std::vector<types::GlobalDiscoveryEntry>& onSuccessResult = getGlobalDiscoveryEntries(2);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(ElementsAre(DOMAIN_1_NAME), INTERFACE_1_NAME, _, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<4>(onSuccessResult));

    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(2),
                                       unexpectedOnDiscoveryErrorFunction);
    ASSERT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
    // enries are now in cache, globalCapabilitiesDirectoryClient should not be called.
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                lookup(_,
                       _,
                       _,
                       _,
                       A<std::function<void(const std::vector<types::GlobalDiscoveryEntry>&
                                                    discoveryEntries)>>(),
                       _,
                       A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(0);
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(2),
                                       unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupForInterfaceAddressDelegatesToCapabilitiesClient)
{
    // simulate global capability directory would store two entries.
    const std::vector<types::GlobalDiscoveryEntry>& onSuccessResult = getGlobalDiscoveryEntries(2);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(ElementsAre(DOMAIN_1_NAME), INTERFACE_1_NAME, _, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<4>(onSuccessResult));

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
            if (participantId == dummyParticipantIdsVector[0]) {
                firstParticipantIdFound = true;
            } else if (participantId == dummyParticipantIdsVector[1]) {
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
                                       unexpectedOnDiscoveryErrorFunction);

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
    localCapabilitiesDirectory->lookup(dummyParticipantIdsVector[0],
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupParticipantIdSuccessFunction(),
                                       unexpectedOnDiscoveryErrorFunction);
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
    localCapabilitiesDirectory->lookup(dummyParticipantIdsVector[0],
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupParticipantIdSuccessFunction(),
                                       unexpectedOnDiscoveryErrorFunction);
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
    localCapabilitiesDirectory->lookup(dummyParticipantIdsVector[0],
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
            lookup(dummyParticipantIdsVector[0],
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
    localCapabilitiesDirectory->lookup(dummyParticipantIdsVector[0],
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupParticipantIdSuccessFunction(),
                                       unexpectedOnDiscoveryErrorFunction);
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
    localCapabilitiesDirectory->lookup(dummyParticipantIdsVector[0],
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupParticipantIdSuccessFunction(),
                                       unexpectedOnDiscoveryErrorFunction);
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
    localCapabilitiesDirectory->lookup(dummyParticipantIdsVector[0],
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createUnexpectedLookupParticipantIdSuccessFunction(),
                                       unexpectedOnDiscoveryErrorFunction);
}

TEST_F(LocalCapabilitiesDirectoryTest, registerMultipleGlobalCapabilitiesCheckIfTheyAreMerged)
{

    types::ProviderQos qos;

    types::GlobalDiscoveryEntry globalDiscoveryEntryInfo1(defaultProviderVersion,
                                                          DOMAIN_1_NAME,
                                                          INTERFACE_1_NAME,
                                                          dummyParticipantIdsVector[0],
                                                          qos,
                                                          lastSeenDateMs,
                                                          expiryDateMs,
                                                          PUBLIC_KEY_ID,
                                                          LOCAL_ADDRESS);

    types::GlobalDiscoveryEntry globalDiscoveryEntryInfo2(defaultProviderVersion,
                                                          DOMAIN_2_NAME,
                                                          INTERFACE_1_NAME,
                                                          dummyParticipantIdsVector[1],
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

    types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantIdsVector[0],
                                       qos,
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);
    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultProviderRuntimeExceptionError);
    types::DiscoveryEntry entry2(defaultProviderVersion,
                                        DOMAIN_2_NAME,
                                        INTERFACE_1_NAME,
                                        dummyParticipantIdsVector[1],
                                        qos,
                                        lastSeenDateMs,
                                        expiryDateMs,
                                        PUBLIC_KEY_ID);
    localCapabilitiesDirectory->add(entry2, defaultOnSuccess, defaultProviderRuntimeExceptionError);
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

    types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantIdsVector[0],
                                       qos,
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);
    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultProviderRuntimeExceptionError);

    types::DiscoveryQos discoveryQos;
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);
    localCapabilitiesDirectory->lookup(dummyParticipantIdsVector[0],
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupParticipantIdSuccessFunction(),
                                       unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    localCapabilitiesDirectory->remove(dummyParticipantIdsVector[0], nullptr, nullptr);

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
    EXPECT_THROW(localCapabilitiesDirectory->lookup(dummyParticipantIdsVector[0],
                                                    discoveryQos,
                                                    KNOWN_GBIDS,
                                                    createLookupParticipantIdSuccessFunction(),
                                                    unexpectedOnDiscoveryErrorFunction),
                 exceptions::JoynrTimeOutException);
}

TEST_F(LocalCapabilitiesDirectoryTest, registerLocalCapability_lookupLocal)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);

    types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_ONLY);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(0);

    types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantIdsVector[0],
                                       providerQos,
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);
    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultProviderRuntimeExceptionError);
    localCapabilitiesDirectory->registerReceivedCapabilities(std::move(globalCapEntryMap));

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(1),
                                       unexpectedOnDiscoveryErrorFunction);
    ASSERT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, remove(dummyParticipantIdsVector[0], _, _, _, _)).Times(0);
    localCapabilitiesDirectory->remove(dummyParticipantIdsVector[0], nullptr, nullptr);

    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(0),
                                       unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, registerLocalCapability_lookupLocalThenGlobal)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);

    types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);

    types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantIdsVector[0],
                                       providerQos,
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(0);
    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultProviderRuntimeExceptionError);
    localCapabilitiesDirectory->registerReceivedCapabilities(std::move(globalCapEntryMap));

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(1),
                                       unexpectedOnDiscoveryErrorFunction);
    ASSERT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, remove(dummyParticipantIdsVector[0], _, _, _, _)).Times(0);
    localCapabilitiesDirectory->remove(dummyParticipantIdsVector[0], nullptr, nullptr);

    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(1),
                                       unexpectedOnDiscoveryErrorFunction);
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
                     unexpectedOnDiscoveryErrorFunction),
                 exceptions::JoynrTimeOutException);
}

TEST_F(LocalCapabilitiesDirectoryTest, registerLocalCapability_lookupLocalAndGlobal)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);

    types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryTimeout(5000);
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_AND_GLOBAL);

    types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantIdsVector[0],
                                       providerQos,
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(0);
    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultProviderRuntimeExceptionError);
    // localCapabilitiesDirectory->registerReceivedCapabilities(globalCapEntryMap);

    const std::vector<types::GlobalDiscoveryEntry>& onSuccessZeroResult{};
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _))
            .Times(2)
            //            .WillOnce(InvokeWithoutArgs(this,
            //            &LocalCapabilitiesDirectoryTest::simulateTimeout));
            .WillRepeatedly(InvokeArgument<4>(onSuccessZeroResult));
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(1),
                                       unexpectedOnDiscoveryErrorFunction);
    ASSERT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, remove(dummyParticipantIdsVector[0], _, _, _, _)).Times(0);
    localCapabilitiesDirectory->remove(dummyParticipantIdsVector[0], nullptr, nullptr);

    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(0),
                                       unexpectedOnDiscoveryErrorFunction);
    ASSERT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    // disable cache
    discoveryQos.setCacheMaxAge(0);
    // wait some time to be sure that the cached entry is not used
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    const std::vector<types::GlobalDiscoveryEntry>& onSuccessResult = getGlobalDiscoveryEntries(2);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillOnce(
            InvokeArgument<4>(onSuccessResult));
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(2),
                                       unexpectedOnDiscoveryErrorFunction);
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
                                       unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupLocalThenGlobal_GlobalPendingLocalEntryAdded_ReturnsLocalEntry)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);

    Semaphore gcdSemaphore(0);
    types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryTimeout(5000);
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);

    types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantIdsVector[0],
                                       providerQos,
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(0);

    const std::vector<types::GlobalDiscoveryEntry>& onSuccessResult = getGlobalDiscoveryEntries(2);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillRepeatedly(
            DoAll(AcquireSemaphore(&gcdSemaphore),
                  InvokeArgument<4>(onSuccessResult)));

    auto thread = std::thread([&]() {
        localCapabilitiesDirectory->lookup(
                    {DOMAIN_1_NAME, DOMAIN_2_NAME},
                    INTERFACE_1_NAME,
                    discoveryQos,
                    KNOWN_GBIDS,
                    createLookupSuccessFunction(1),
                    unexpectedOnDiscoveryErrorFunction);
    });

    EXPECT_FALSE(semaphore.waitFor(std::chrono::milliseconds(100)));
    EXPECT_TRUE(localCapabilitiesDirectory->hasPendingLookups());

    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultProviderRuntimeExceptionError);

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
    types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryTimeout(5000);
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);

    const std::vector<types::GlobalDiscoveryEntry>& onSuccessResult = getGlobalDiscoveryEntries(2);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillRepeatedly(
            DoAll(AcquireSemaphore(&gcdSemaphore),
                  InvokeArgument<4>(onSuccessResult)));

    auto thread = std::thread([&]() {
        localCapabilitiesDirectory->lookup(
                    {DOMAIN_1_NAME, DOMAIN_2_NAME},
                    INTERFACE_1_NAME,
                    discoveryQos,
                    KNOWN_GBIDS,
                    createLookupSuccessFunction(2),
                    unexpectedOnDiscoveryErrorFunction);
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
    types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryTimeout(5000);
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillRepeatedly(
            DoAll(AcquireSemaphore(&gcdSemaphore),
                  Invoke(this, &LocalCapabilitiesDirectoryTest::fakeCapabilitiesClientLookupWithDiscoveryException)));

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

    types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryTimeout(5000);
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);

    types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantIdsVector[0],
                                       providerQos,
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(0);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);

    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultProviderRuntimeExceptionError);
    localCapabilitiesDirectory->lookup(
                {DOMAIN_1_NAME, DOMAIN_2_NAME},
                INTERFACE_1_NAME,
                discoveryQos,
                KNOWN_GBIDS,
                createLookupSuccessFunction(1),
                unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
    EXPECT_FALSE(localCapabilitiesDirectory->hasPendingLookups());
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupLocalAndGlobal_GlobalSucceedsNoLocalEntries_ReturnsGlobalEntries)
{
    types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryTimeout(5000);
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_AND_GLOBAL);

    const std::vector<types::GlobalDiscoveryEntry>& onSuccessResult = getGlobalDiscoveryEntries(2);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillRepeatedly(
            InvokeArgument<4>(onSuccessResult));

    localCapabilitiesDirectory->lookup(
                {DOMAIN_1_NAME, DOMAIN_2_NAME},
                INTERFACE_1_NAME,
                discoveryQos,
                KNOWN_GBIDS,
                createLookupSuccessFunction(2),
                unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
    EXPECT_FALSE(localCapabilitiesDirectory->hasPendingLookups());
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupLocalAndGlobal_GlobalFailsNoLocalEntries_ReturnsNoEntries)
{
    types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryTimeout(5000);
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_AND_GLOBAL);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillRepeatedly(
            Invoke(this, &LocalCapabilitiesDirectoryTest::fakeCapabilitiesClientLookupWithDiscoveryException));

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

    types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryTimeout(5000);
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_AND_GLOBAL);

    types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantIdsVector[2],
                                       providerQos,
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(0);

    const std::vector<types::GlobalDiscoveryEntry>& onSuccessResult = getGlobalDiscoveryEntries(2);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillRepeatedly(
            InvokeArgument<4>(onSuccessResult));

    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultProviderRuntimeExceptionError);
    localCapabilitiesDirectory->lookup(
                {DOMAIN_1_NAME, DOMAIN_2_NAME},
                INTERFACE_1_NAME,
                discoveryQos,
                KNOWN_GBIDS,
                createLookupSuccessFunction(3),
                unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
    EXPECT_FALSE(localCapabilitiesDirectory->hasPendingLookups());
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupLocalAndGlobal_GlobalFailsLocalEntries_ReturnsNoEntries)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);

    types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryTimeout(5000);
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_AND_GLOBAL);

    types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantIdsVector[0],
                                       providerQos,
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(0);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillRepeatedly(
            Invoke(this, &LocalCapabilitiesDirectoryTest::fakeCapabilitiesClientLookupWithDiscoveryException));

    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultProviderRuntimeExceptionError);
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
    types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryTimeout(5000);
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);

    const std::vector<types::GlobalDiscoveryEntry>& onSuccessResult = getGlobalDiscoveryEntries(2);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillRepeatedly(
            InvokeArgument<4>(onSuccessResult));

    localCapabilitiesDirectory->lookup(
                {DOMAIN_1_NAME, DOMAIN_2_NAME},
                INTERFACE_1_NAME,
                discoveryQos,
                KNOWN_GBIDS,
                createLookupSuccessFunction(2),
                unexpectedOnDiscoveryErrorFunction);

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

    types::DiscoveryEntry entry1(defaultProviderVersion,
                                        DOMAIN_1_NAME,
                                        INTERFACE_1_NAME,
                                        dummyParticipantIdsVector[0],
                                        providerQos,
                                        lastSeenDateMs,
                                        longerExpiryDateMs,
                                        PUBLIC_KEY_ID);

    types::DiscoveryEntry entry2(defaultProviderVersion,
                                        DOMAIN_1_NAME,
                                        INTERFACE_1_NAME,
                                        dummyParticipantIdsVector[1],
                                        providerQos,
                                        lastSeenDateMs,
                                        expiryDateMs,
                                        PUBLIC_KEY_ID);

    types::DiscoveryEntry entry3(defaultProviderVersion,
                                        DOMAIN_1_NAME,
                                        INTERFACE_3_NAME,
                                        dummyParticipantIdsVector[2],
                                        providerQos,
                                        lastSeenDateMs,
                                        expiryDateMs,
                                        PUBLIC_KEY_ID);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(3);

    localCapabilitiesDirectory->add(entry1, defaultOnSuccess, defaultProviderRuntimeExceptionError);

    localCapabilitiesDirectory->add(entry2, defaultOnSuccess, defaultProviderRuntimeExceptionError);

    localCapabilitiesDirectory->add(entry3, defaultOnSuccess, defaultProviderRuntimeExceptionError);

    // Check Cached Global Discovery Entries
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);

    types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryTimeout(10000);
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);

    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(2),
                                       unexpectedOnDiscoveryErrorFunction);

    ASSERT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_3_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(1),
                                       unexpectedOnDiscoveryErrorFunction);
    ASSERT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    // wait for cleanup timer to run
    std::this_thread::sleep_for(
            std::chrono::milliseconds(purgeExpiredDiscoveryEntriesIntervalMs * 2));

    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(1),
                                       unexpectedOnDiscoveryErrorFunction);

    ASSERT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    Mock::VerifyAndClearExpectations(globalCapabilitiesDirectoryClient.get());

    const std::vector<types::GlobalDiscoveryEntry>& onSuccessZeroResult{};
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<4>(onSuccessZeroResult));
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_3_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(0),
                                       unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupGlobalOnly_GlobalFailsNoLocalEntries_ReturnsNoEntries)
{
    types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryTimeout(5000);
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillRepeatedly(
            Invoke(this, &LocalCapabilitiesDirectoryTest::fakeCapabilitiesClientLookupWithDiscoveryException));

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

    types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryTimeout(5000);
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);

    types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantIdsVector[0],
                                       providerQos,
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(0);

    const std::vector<types::GlobalDiscoveryEntry>& onSuccessResult = getGlobalDiscoveryEntries(2);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillRepeatedly(
            InvokeArgument<4>(onSuccessResult));

    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultProviderRuntimeExceptionError);
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME, DOMAIN_2_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(2),
                                       unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
    EXPECT_FALSE(localCapabilitiesDirectory->hasPendingLookups());
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupGlobalOnly_GlobalFailsLocalEntries_ReturnsNoEntries)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);

    types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryTimeout(5000);
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);

    types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantIdsVector[0],
                                       providerQos,
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(0);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillRepeatedly(
            Invoke(this, &LocalCapabilitiesDirectoryTest::fakeCapabilitiesClientLookupWithDiscoveryException));

    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultProviderRuntimeExceptionError);
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

    types::DiscoveryEntry entry1(defaultProviderVersion,
                                        multipleDomainName1,
                                        INTERFACE_1_NAME,
                                        multipleDomainName1ParticipantId,
                                        providerQos,
                                        lastSeenDateMs,
                                        expiryDateMs,
                                        PUBLIC_KEY_ID);
    types::DiscoveryEntry entry2(defaultProviderVersion,
                                        multipleDomainName2,
                                        INTERFACE_1_NAME,
                                        multipleDomainName2ParticipantId,
                                        providerQos,
                                        lastSeenDateMs,
                                        expiryDateMs,
                                        PUBLIC_KEY_ID);
    types::DiscoveryEntry entry31(defaultProviderVersion,
                                         multipleDomainName3,
                                         INTERFACE_1_NAME,
                                         multipleDomainName3ParticipantId1,
                                         providerQos,
                                         lastSeenDateMs,
                                         expiryDateMs,
                                         PUBLIC_KEY_ID);
    types::DiscoveryEntry entry32(defaultProviderVersion,
                                         multipleDomainName3,
                                         INTERFACE_1_NAME,
                                         multipleDomainName3ParticipantId2,
                                         providerQos,
                                         lastSeenDateMs,
                                         expiryDateMs,
                                         PUBLIC_KEY_ID);
    localCapabilitiesDirectory->add(entry1, defaultOnSuccess, defaultProviderRuntimeExceptionError);
    localCapabilitiesDirectory->add(entry2, defaultOnSuccess, defaultProviderRuntimeExceptionError);
    localCapabilitiesDirectory->add(entry31, defaultOnSuccess, defaultProviderRuntimeExceptionError);
    localCapabilitiesDirectory->add(entry32, defaultOnSuccess, defaultProviderRuntimeExceptionError);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                lookup(_,
                       _,
                       _,
                       A<std::function<void(const std::vector<types::GlobalDiscoveryEntry>&
                                                    discoveryEntries)>>(),
                       _,
                       A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(0);

    std::vector<std::string> domains = {
            multipleDomainName1, multipleDomainName2, multipleDomainName3};

    types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_ONLY);
    localCapabilitiesDirectory->lookup(domains,
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(4),
                                       unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, registerLocalCapability_lookupGlobalOnly)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);

    types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(0);

    types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantIdsVector[0],
                                       providerQos,
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);
    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultProviderRuntimeExceptionError);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillOnce(
            InvokeWithoutArgs(this, &LocalCapabilitiesDirectoryTest::simulateTimeout));
    // JoynrTimeOutException timeoutException;
    EXPECT_THROW(localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                                    INTERFACE_1_NAME,
                                                    discoveryQos,
                                                    KNOWN_GBIDS,
                                                    createUnexpectedLookupSuccessFunction(),
                                                    unexpectedOnDiscoveryErrorFunction),
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
                                       unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, remove(dummyParticipantIdsVector[0], _, _, _, _)).Times(0);
    localCapabilitiesDirectory->remove(dummyParticipantIdsVector[0], nullptr, nullptr);

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
                                                    unexpectedOnDiscoveryErrorFunction),
                 exceptions::JoynrTimeOutException);
}

TEST_F(LocalCapabilitiesDirectoryTest, registerGlobalCapability_lookupLocal)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);

    types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_ONLY);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(1);

    types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantIdsVector[0],
                                       providerQos,
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);
    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultProviderRuntimeExceptionError);
    localCapabilitiesDirectory->registerReceivedCapabilities(std::move(globalCapEntryMap));

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(1),
                                       unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, remove(dummyParticipantIdsVector[0], _, _, _, _)).Times(1);
    localCapabilitiesDirectory->remove(dummyParticipantIdsVector[0], nullptr, nullptr);
}

TEST_F(LocalCapabilitiesDirectoryTest, registerGlobalCapability_lookupLocalThenGlobal)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);

    types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(100);
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(1);

    types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantIdsVector[0],
                                       providerQos,
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);
    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultProviderRuntimeExceptionError);
    localCapabilitiesDirectory->registerReceivedCapabilities(std::move(globalCapEntryMap));

    // get the local entry
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(1),
                                       unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, remove(dummyParticipantIdsVector[0], _, _, _, _)).Times(1);
    localCapabilitiesDirectory->remove(dummyParticipantIdsVector[0], nullptr, nullptr);

    // get the global entry
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(1),
                                       unexpectedOnDiscoveryErrorFunction);
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
                     unexpectedOnDiscoveryErrorFunction),
                 exceptions::JoynrTimeOutException);
}

TEST_F(LocalCapabilitiesDirectoryTest, registerCachedGlobalCapability_lookupGlobalOnly)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);

    types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(100);
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);

    // JoynrTimeOutException timeoutException;
    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(1);

    types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantIdsVector[0],
                                       providerQos,
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);
    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultProviderRuntimeExceptionError);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(1),
                                       unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    // recieve a global entry
    localCapabilitiesDirectory->registerReceivedCapabilities(std::move(globalCapEntryMap));
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(2),
                                       unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, remove(dummyParticipantIdsVector[0], _, _, _, _)).Times(1);
    localCapabilitiesDirectory->remove(dummyParticipantIdsVector[0], nullptr, nullptr);
}

TEST_F(LocalCapabilitiesDirectoryTest, registerReceivedCapabilites_registerMqttAddress)
{
    const std::string& addressType = "mqtt";
    const std::string& topic = "mqtt_TEST_channelId";
    system::RoutingTypes::MqttAddress mqttAddress("brokerUri", topic);
    registerReceivedCapabilities(addressType, serializer::serializeToJson(mqttAddress));
}

TEST_F(LocalCapabilitiesDirectoryTest, registerReceivedCapabilites_registerHttpAddress)
{
    const std::string& addressType = "http";
    const std::string& channelID = "TEST_channelId";
    const std::string& endPointUrl = "TEST_endPointUrl";
    const system::RoutingTypes::ChannelAddress channelAddress(endPointUrl, channelID);
    registerReceivedCapabilities(addressType, serializer::serializeToJson(channelAddress));
}

TEST_F(LocalCapabilitiesDirectoryTest, persistencyTest)
{
    // Attempt loading (action usually performed by cluster-controller runtime)
    localCapabilitiesDirectory->loadPersistedFile();

    // add few entries
    const std::string& DOMAIN_NAME = "LocalCapabilitiesDirectorySerializerTest_Domain";
    const std::string& INTERFACE_NAME = "LocalCapabilitiesDirectorySerializerTest_InterfaceName";

    std::vector<std::string> participantIds{
            util::createUuid(), util::createUuid(), util::createUuid()};

    types::ProviderQos localProviderQos;
    localProviderQos.setScope(types::ProviderScope::LOCAL);
    types::ProviderQos globalProviderQos;
    globalProviderQos.setScope(types::ProviderScope::GLOBAL);

    const types::DiscoveryEntry entry1(defaultProviderVersion,
                                              DOMAIN_NAME,
                                              INTERFACE_NAME,
                                              participantIds[0],
                                              localProviderQos,
                                              lastSeenDateMs,
                                              expiryDateMs,
                                              PUBLIC_KEY_ID);
    const types::DiscoveryEntry entry2(defaultProviderVersion,
                                              DOMAIN_NAME,
                                              INTERFACE_NAME,
                                              participantIds[1],
                                              globalProviderQos,
                                              lastSeenDateMs,
                                              expiryDateMs,
                                              PUBLIC_KEY_ID);
    const types::DiscoveryEntry entry3(defaultProviderVersion,
                                              DOMAIN_NAME,
                                              INTERFACE_NAME,
                                              participantIds[2],
                                              globalProviderQos,
                                              lastSeenDateMs,
                                              expiryDateMs,
                                              PUBLIC_KEY_ID);

    localCapabilitiesDirectory->add(entry1, defaultOnSuccess, defaultProviderRuntimeExceptionError);
    localCapabilitiesDirectory->add(entry2, defaultOnSuccess, defaultProviderRuntimeExceptionError);
    localCapabilitiesDirectory->add(entry3,
                                    false,
                                    {KNOWN_GBIDS[1], KNOWN_GBIDS[2]},
                                    defaultOnSuccess,
                                    unexpectedOnDiscoveryErrorFunction);

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
                                            unexpectedOnDiscoveryErrorFunction);
        EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
    }

    auto cachedGlobalDiscoveryEntries = localCapabilitiesDirectory2->getCachedGlobalDiscoveryEntries();
    EXPECT_EQ(2, cachedGlobalDiscoveryEntries.size());
    EXPECT_EQ(entry2, cachedGlobalDiscoveryEntries[0]);
    EXPECT_EQ(entry3, cachedGlobalDiscoveryEntries[1]);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _))
            .WillRepeatedly(InvokeArgument<4>(types::DiscoveryError::INTERNAL_ERROR));
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);

    // check entry2 is registered only in backend 0
    localCapabilitiesDirectory2->lookup(participantIds[1],
                                        discoveryQos,
                                        {KNOWN_GBIDS[0]},
                                        createLookupParticipantIdSuccessFunction(),
                                        unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    localCapabilitiesDirectory2->lookup(participantIds[1],
                                        discoveryQos,
                                        {KNOWN_GBIDS[1], KNOWN_GBIDS[2]},
                                        createUnexpectedLookupParticipantIdSuccessFunction(),
                                        createExpectedDiscoveryErrorFunction(types::DiscoveryError::INTERNAL_ERROR));
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    // check entry3 is registered only in backend 1 and backend 2
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
                                        unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    localCapabilitiesDirectory2->lookup(participantIds[2],
                                        discoveryQos,
                                        {KNOWN_GBIDS[2]},
                                        createLookupParticipantIdSuccessFunction(),
                                        unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, loadCapabilitiesFromFile)
{
    const std::string& fileName = "test-resources/ListOfCapabilitiesToInject.json";
    localCapabilitiesDirectory->injectGlobalCapabilitiesFromFile(fileName);

    // Verify that all entries present in the file have indeed been loaded
    types::DiscoveryQos discoveryQos;
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);
    localCapabilitiesDirectory->lookup("notReachableInterface_Schroedinger",
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupParticipantIdSuccessFunction(),
                                       unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    localCapabilitiesDirectory->lookup("notReachableInterface_Heisenberg",
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupParticipantIdSuccessFunction(),
                                       unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, throwExceptionOnEmptyDomainsVector)
{
    const std::vector<std::string>& zeroDomains = {};
    const std::vector<std::string>& twoDomains = {DOMAIN_1_NAME, DOMAIN_2_NAME};
    MockCallback<std::vector<types::DiscoveryEntryWithMetaInfo>> mockCallback;
    auto onSuccess = std::bind(
            &MockCallback<std::vector<types::DiscoveryEntryWithMetaInfo>>::onSuccess,
            &mockCallback,
            std::placeholders::_1);
    auto onError =
            std::bind(&MockCallback<std::vector<types::DiscoveryEntryWithMetaInfo>>::onError,
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

    types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantIdsVector[0],
                                       providerQos,
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);
    const types::DiscoveryEntryWithMetaInfo expectedEntry = util::convert(true, entry);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(1);

    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultProviderRuntimeExceptionError);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);

    auto onSuccess = [this, expectedEntry] (const std::vector<types::DiscoveryEntryWithMetaInfo>& results) {
        EXPECT_EQ(1, results.size());
        const types::DiscoveryEntryWithMetaInfo& result = results.at(0);
        EXPECT_EQ(expectedEntry, result);
        semaphore.notify();
    };

    types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryTimeout(5000);
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_AND_GLOBAL);
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       onSuccess,
                                       unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, localAndGlobalDoesNotReturnDuplicateEntriesCacheDisabled)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);
    types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantIdsVector[0],
                                       providerQos,
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);
    const types::DiscoveryEntryWithMetaInfo& expectedEntry = util::convert(true, entry);
    const types::GlobalDiscoveryEntry& globalEntry =
            types::GlobalDiscoveryEntry(entry.getProviderVersion(),
                                               entry.getDomain(),
                                               entry.getInterfaceName(),
                                               entry.getParticipantId(),
                                               entry.getQos(),
                                               entry.getLastSeenDateMs(),
                                               entry.getExpiryDateMs(),
                                               entry.getPublicKeyId(),
                                               EXTERNAL_ADDRESSES_VECTOR[0]);
    const std::vector<types::GlobalDiscoveryEntry>& globalEntryVec = {globalEntry};

    EXPECT_CALL(*globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(1);

    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultProviderRuntimeExceptionError);

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillOnce(
            InvokeArgument<4>(globalEntryVec));

    types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(0);
    discoveryQos.setDiscoveryTimeout(5000);
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_AND_GLOBAL);

    // wait some time to be sure that the cached entry is not used
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    auto onSuccess = [this, expectedEntry] (const std::vector<types::DiscoveryEntryWithMetaInfo>& results) {
        EXPECT_EQ(1, results.size());
        const types::DiscoveryEntryWithMetaInfo& result = results.at(0);
        EXPECT_EQ(expectedEntry, result);
        semaphore.notify();
    };
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       onSuccess,
                                       unexpectedOnDiscoveryErrorFunction);
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
    qos.setScope(types::ProviderScope::GLOBAL);
    const types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantIdsVector[0],
                                       qos,
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);

    // trigger a failure on the first call and a success on the second

    // 1st call
    Semaphore gcdSemaphore(0);
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, add(An<const types::GlobalDiscoveryEntry&>(), _, _, _, _))
            .WillOnce(DoAll(
                          Invoke(this, &LocalCapabilitiesDirectoryTest::fakeCapabilitiesClientAddWithException),
                          ReleaseSemaphore(&gcdSemaphore)
                      )); // invoke onError
    localCapabilitiesDirectory->add(entry, true, defaultOnSuccess, defaultProviderRuntimeExceptionError);

    // wait for it...
    EXPECT_TRUE(gcdSemaphore.waitFor(std::chrono::seconds(1)));

    // 2nd call
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, add(An<const types::GlobalDiscoveryEntry&>(), _, _, _, _))
            .WillOnce(DoAll(
                          Invoke(this, &LocalCapabilitiesDirectoryTest::fakeCapabilitiesClientAddSuccess),
                          ReleaseSemaphore(&gcdSemaphore)
                      )); // invoke onSuccess
    localCapabilitiesDirectory->add(entry, true, defaultOnSuccess, defaultProviderRuntimeExceptionError);

    // wait for it...
    EXPECT_TRUE(gcdSemaphore.waitFor(std::chrono::seconds(1)));

    // do a lookup to make sure the entry still exists
    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(1),
                                       unexpectedOnDiscoveryErrorFunction);
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

    types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantIdsVector[0],
                                       types::ProviderQos(),
                                       lastSeenDateMs,
                                       expiryDateMs,
                                       PUBLIC_KEY_ID);

    try {
        localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultProviderRuntimeExceptionError);
    } catch (const exceptions::ProviderRuntimeException&) {
    }

    const types::DiscoveryQos discoveryQos;
    if (!this->ENABLE_ACCESS_CONTROL || this->HAS_PERMISSION) {
        localCapabilitiesDirectory->lookup(dummyParticipantIdsVector[0],
                                           discoveryQos,
                                           KNOWN_GBIDS,
                                           createLookupParticipantIdSuccessFunction(),
                                           unexpectedOnDiscoveryErrorFunction);
    } else {
        localCapabilitiesDirectory->lookup(dummyParticipantIdsVector[0],
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
        auto localDomainAccessStore = std::make_shared<LocalDomainAccessStore>(
                "test-resources/LDAS_checkPermissionToAdd.json");
        auto localDomainAccessController =
                std::make_shared<LocalDomainAccessController>(localDomainAccessStore, true);
        accessController = std::make_shared<AccessController>(
                localCapabilitiesDirectory, localDomainAccessController, KNOWN_GBIDS);
        localCapabilitiesDirectory->setAccessController(util::as_weak_ptr(accessController));

        localDomainAccessStore->logContent();
    }

protected:
    std::shared_ptr<IAccessController> accessController;
};

TEST_F(LocalCapabilitiesDirectoryACTest, checkPermissionToAdd)
{
    types::DiscoveryEntry OK_entry(defaultProviderVersion,
                                          "domain-1234",
                                          "my/favourite/interface/Name",
                                          dummyParticipantIdsVector[0],
                                          types::ProviderQos(),
                                          lastSeenDateMs,
                                          expiryDateMs,
                                          PUBLIC_KEY_ID);

    types::DiscoveryEntry NOT_OK_entry_1(
            defaultProviderVersion,
            "domain-1234",                // domain is OK
            "my/favourite/interface/Nam", // interfaceName is a substring of the allowed one
            dummyParticipantIdsVector[0],
            types::ProviderQos(),
            lastSeenDateMs,
            expiryDateMs,
            PUBLIC_KEY_ID);

    types::DiscoveryEntry NOT_OK_entry_2(
            defaultProviderVersion,
            "domain-123",                  // domain is a substring of the allowed one
            "my/favourite/interface/Name", // interfaceName is OK
            dummyParticipantIdsVector[0],
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
                     [](const exceptions::ProviderRuntimeException& ex) {
                         FAIL() << ex.getMessage();
                     });

    EXPECT_THROW(localCapabilitiesDirectory->add(
                     NOT_OK_entry_1,
                     []() { FAIL(); },
                     [](const exceptions::ProviderRuntimeException& ex) {
                         FAIL() << ex.getMessage();
                     }),
                 exceptions::ProviderRuntimeException);

    EXPECT_THROW(localCapabilitiesDirectory->add(
                     NOT_OK_entry_2,
                     []() { FAIL(); },
                     [](const exceptions::ProviderRuntimeException& ex) {
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

    types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_ONLY);

    EXPECT_CALL(
            *globalCapabilitiesDirectoryClient, add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _))
            .Times(GetParam() == types::ProviderScope::LOCAL ? 0 : 1);

    types::DiscoveryEntry entry(defaultProviderVersion,
                                       DOMAIN_1_NAME,
                                       INTERFACE_1_NAME,
                                       dummyParticipantIdsVector[0],
                                       providerQos,
                                       lastSeenDateMs,
                                       10,
                                       PUBLIC_KEY_ID);
    localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultProviderRuntimeExceptionError);
    localCapabilitiesDirectory->registerReceivedCapabilities(std::move(globalCapEntryMap));

    EXPECT_CALL(*globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(1),
                                       unexpectedOnDiscoveryErrorFunction);
    ASSERT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));

    // wait for cleanup timer to run
    std::this_thread::sleep_for(
            std::chrono::milliseconds(purgeExpiredDiscoveryEntriesIntervalMs * 2));

    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(0),
                                       unexpectedOnDiscoveryErrorFunction);
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
                    add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _))
                .Times(numberOfDuplicatedEntriesToAdd)
                .WillRepeatedly(Invoke(this, &LocalCapabilitiesDirectoryTest::fakeCapabilitiesClientAddSuccess));
    }

    for (int i = 0; i < numberOfDuplicatedEntriesToAdd; ++i) {
        // change expiryDate and lastSeen so that entries are not exactly equal
        lastSeenDateMs++;
        expiryDateMs++;

        types::DiscoveryEntry entry(defaultProviderVersion,
                                           DOMAIN_1_NAME,
                                           INTERFACE_1_NAME,
                                           dummyParticipantIdsVector[0],
                                           providerQos,
                                           lastSeenDateMs,
                                           expiryDateMs,
                                           PUBLIC_KEY_ID);
            localCapabilitiesDirectory->add(entry, defaultOnSuccess, defaultProviderRuntimeExceptionError);
    }

    localCapabilitiesDirectory->lookup({DOMAIN_1_NAME},
                                       INTERFACE_1_NAME,
                                       discoveryQos,
                                       KNOWN_GBIDS,
                                       createLookupSuccessFunction(1),
                                       unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(TIMEOUT)));
}

INSTANTIATE_TEST_CASE_P(changeProviderScope,
                        LocalCapabilitiesDirectoryWithProviderScope,
                        ::testing::Values(types::ProviderScope::LOCAL,
                                          types::ProviderScope::GLOBAL));
