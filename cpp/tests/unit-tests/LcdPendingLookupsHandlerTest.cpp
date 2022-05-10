/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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
#include <thread>

#include "tests/utils/Gmock.h"
#include "tests/utils/Gtest.h"

#include "joynr/InterfaceAddress.h"
#include "joynr/LcdPendingLookupsHandler.h"
#include "joynr/LocalCapabilitiesDirectory.h"
#include "joynr/types/DiscoveryEntry.h"
#include "joynr/types/DiscoveryScope.h"
#include "joynr/types/DiscoveryQos.h"
#include "joynr/types/ProviderQos.h"
#include "joynr/types/Version.h"

using namespace ::testing;
using namespace joynr;


class LcdPendingLookupsHandlerTest : public ::testing::Test
{
public:

    LcdPendingLookupsHandlerTest() :
            _providerVersion(30, 06),
            _qos(),
            _discoveryEntry(_providerVersion,
                            "domain1",
                            "interface1",
                            "participant1",
                            _qos,
                            10000,
                            10000,
                            "test"),
            _semaphore(0){
        _qos.setScope(types::ProviderScope::LOCAL);
        _discoveryEntry.setQos(_qos);

    }

protected:
    LcdPendingLookupsHandler _lcdPendingLookupsHandler;
    types::Version _providerVersion;
    types::ProviderQos _qos;
    types::DiscoveryEntry _discoveryEntry;
    Semaphore _semaphore;

};

TEST_F(LcdPendingLookupsHandlerTest, test_callPendingLookups)
{
    std::vector<types::DiscoveryEntry> localCapabilities;
    localCapabilities.push_back(_discoveryEntry);

    InterfaceAddress interfaceAddress(
            _discoveryEntry.getDomain(), _discoveryEntry.getInterfaceName());
    std::vector<InterfaceAddress> interfaceAddresses;
    interfaceAddresses.push_back(interfaceAddress);

    std::function<void(const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>&)> onSuccess =
            [this] (const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>& result) {
                ASSERT_EQ(1, result.size());
                ASSERT_EQ(_discoveryEntry.getParticipantId(), result.at(0).getParticipantId());
                _semaphore.notify();
    };
    std::function<void(const types::DiscoveryError::Enum&)> onError =
            [] (const types::DiscoveryError::Enum& errorEnum) {
                FAIL() << "Unexpected onError call: " + types::DiscoveryError::getLiteral(errorEnum);
    };

    auto localCapabilitiesCallback =
            std::make_shared<LocalCapabilitiesCallback>(std::move(onSuccess), std::move(onError));

    //We start with no pending Lookups
    ASSERT_FALSE( _lcdPendingLookupsHandler.hasPendingLookups());
    //Then we register a pending lookup
    _lcdPendingLookupsHandler.registerPendingLookup(interfaceAddresses, localCapabilitiesCallback);
    //Now we have a pending lookup
    ASSERT_TRUE( _lcdPendingLookupsHandler.hasPendingLookups());
    //Verify that it has not been called, first with an invalid discoveryQos
    types::DiscoveryQos discoveryQos;
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_ONLY);
    ASSERT_FALSE(_lcdPendingLookupsHandler.isCallbackCalled(interfaceAddresses, localCapabilitiesCallback, discoveryQos));
    //Now with the valid DiscoveryQos (it still hasn't been called)
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);
    ASSERT_FALSE(_lcdPendingLookupsHandler.isCallbackCalled(interfaceAddresses, localCapabilitiesCallback, discoveryQos));
    //Call the pending lookup
    _lcdPendingLookupsHandler.callPendingLookups(interfaceAddress, localCapabilities);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(1000)));
    //Verify that the call is removed after use
    ASSERT_TRUE(_lcdPendingLookupsHandler.isCallbackCalled(interfaceAddresses, localCapabilitiesCallback, discoveryQos));
}

TEST_F(LcdPendingLookupsHandlerTest, test_callbackCalled)
{
    std::vector<types::DiscoveryEntry> localCapabilities;
    localCapabilities.push_back(_discoveryEntry);

    InterfaceAddress interfaceAddress(
            _discoveryEntry.getDomain(), _discoveryEntry.getInterfaceName());
    std::vector<InterfaceAddress> interfaceAddresses;
    interfaceAddresses.push_back(interfaceAddress);

    std::function<void(const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>&)> onSuccess =
            [this] (const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>& result) {
                ASSERT_EQ(1, result.size());
                ASSERT_EQ(_discoveryEntry.getParticipantId(), result.at(0).getParticipantId());
                _semaphore.notify();
    };
    std::function<void(const types::DiscoveryError::Enum&)> onError =
            [] (const types::DiscoveryError::Enum& errorEnum) {
                FAIL() << "Unexpected onError call: " + types::DiscoveryError::getLiteral(errorEnum);
    };

    auto localCapabilitiesCallback =
            std::make_shared<LocalCapabilitiesCallback>(std::move(onSuccess), std::move(onError));

    //We start with no pending Lookups
    ASSERT_FALSE( _lcdPendingLookupsHandler.hasPendingLookups());
    //Then we register a pending lookup
    _lcdPendingLookupsHandler.registerPendingLookup(interfaceAddresses, localCapabilitiesCallback);
    //Now we have a pending lookup
    ASSERT_TRUE( _lcdPendingLookupsHandler.hasPendingLookups());
    //Verify that it has not been called, first with an invalid discoveryQos
    types::DiscoveryQos discoveryQos;
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_ONLY);
    ASSERT_FALSE(_lcdPendingLookupsHandler.isCallbackCalled(interfaceAddresses, localCapabilitiesCallback, discoveryQos));
    //Now with the valid DiscoveryQos (it still hasn't been called)
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);
    ASSERT_FALSE(_lcdPendingLookupsHandler.isCallbackCalled(interfaceAddresses, localCapabilitiesCallback, discoveryQos));
    //Execute callbackCalled
    _lcdPendingLookupsHandler.callbackCalled(interfaceAddresses, localCapabilitiesCallback);
    //In this testcase, the callback is supposed to be called outside of the functions of this class.
    //This is why we expect it not to be called in this test.
    EXPECT_FALSE(_semaphore.waitFor(std::chrono::milliseconds(1000)));
    //Verify that the call is removed after use
    ASSERT_TRUE(_lcdPendingLookupsHandler.isCallbackCalled(interfaceAddresses, localCapabilitiesCallback, discoveryQos));
}
