/*
 * #%L
 * %%
 * Copyright (C) 2023 BMW Car IT GmbH
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

#include "tests/unit-tests/lcd/LocalCapabilitiesDirectoryAbstract.h"

class LocalCapabilitiesDirectoryACTest : public AbstractLocalCapabilitiesDirectoryTest
{
public:
    LocalCapabilitiesDirectoryACTest()
    {
        _clusterControllerSettings.setEnableAccessController(true);
        auto localDomainAccessStore = std::make_shared<LocalDomainAccessStore>(
                "test-resources/LDAS_checkPermissionToAdd.json");
        finalizeTestSetupAfterMockExpectationsAreDone();
        _accessController = std::make_shared<AccessController>(
                _localCapabilitiesDirectory, localDomainAccessStore);
        _localCapabilitiesDirectory->setAccessController(util::as_weak_ptr(_accessController));

        localDomainAccessStore->logContent();
    }

protected:
    std::shared_ptr<IAccessController> _accessController;
};

TEST_F(LocalCapabilitiesDirectoryACTest, checkPermissionToAdd)
{
    types::DiscoveryEntry OK_entry(_defaultProviderVersion,
                                   "domain-1234",
                                   "my/favourite/interface/Name",
                                   _dummyParticipantIdsVector[0],
                                   types::ProviderQos(),
                                   _lastSeenDateMs,
                                   _lastSeenDateMs + _defaultExpiryIntervalMs,
                                   _PUBLIC_KEY_ID);

    types::DiscoveryEntry NOT_OK_entry_1(
            _defaultProviderVersion,
            "domain-1234",                // domain is OK
            "my/favourite/interface/Nam", // interfaceName is a substring of the allowed one
            _dummyParticipantIdsVector[0],
            types::ProviderQos(),
            _lastSeenDateMs,
            _lastSeenDateMs + _defaultExpiryIntervalMs,
            _PUBLIC_KEY_ID);

    types::DiscoveryEntry NOT_OK_entry_2(_defaultProviderVersion,
                                         "domain-123", // domain is a substring of the allowed one
                                         "my/favourite/interface/Name", // interfaceName is OK
                                         _dummyParticipantIdsVector[0],
                                         types::ProviderQos(),
                                         _lastSeenDateMs,
                                         _lastSeenDateMs + _defaultExpiryIntervalMs,
                                         _PUBLIC_KEY_ID);

    std::string principal = "testUser";
    CallContext callContext;
    callContext.setPrincipal(principal);
    CallContextStorage::set(std::move(callContext));

    _localCapabilitiesDirectory->add(
            OK_entry,
            []() { SUCCEED() << "OK"; },
            [](const exceptions::ProviderRuntimeException& ex) { FAIL() << ex.getMessage(); });

    EXPECT_THROW(_localCapabilitiesDirectory->add(
                         NOT_OK_entry_1,
                         []() { FAIL(); },
                         [](const exceptions::ProviderRuntimeException& ex) {
                             FAIL() << ex.getMessage();
                         }),
                 exceptions::ProviderRuntimeException);

    EXPECT_THROW(_localCapabilitiesDirectory->add(
                         NOT_OK_entry_2,
                         []() { FAIL(); },
                         [](const exceptions::ProviderRuntimeException& ex) {
                             FAIL() << ex.getMessage();
                         }),
                 exceptions::ProviderRuntimeException);

    CallContextStorage::invalidate();
}

class LocalCapabilitiesDirectoryACMockTest
        : public AbstractLocalCapabilitiesDirectoryTest,
          public ::testing::WithParamInterface<std::tuple<bool, bool>>
{
public:
    LocalCapabilitiesDirectoryACMockTest()
            : _ENABLE_ACCESS_CONTROL(std::get<0>(GetParam())),
              _HAS_PERMISSION(std::get<1>(GetParam()))
    {
        _clusterControllerSettings.setEnableAccessController(_ENABLE_ACCESS_CONTROL);
    }

    void initializeMockLocalCapabilitiesDirectoryStore()
    {
        _mockLocalCapabilitiesDirectoryStore =
                std::make_shared<MockLocalCapabilitiesDirectoryStore>(
                        _mockGlobalLookupCache, _mockLocallyRegisteredCapabilities);
    }

    std::function<void(const types::DiscoveryEntryWithMetaInfo&)>
    createLookupParticipantIdSuccessFunction()
    {
        return [this](const types::DiscoveryEntryWithMetaInfo& result) {
            std::ignore = result;
            _semaphore->notify();
        };
    }

    std::function<void(const types::DiscoveryEntryWithMetaInfo&)>
    createUnexpectedLookupParticipantIdSuccessFunction()
    {
        return [](const types::DiscoveryEntryWithMetaInfo& result) {
            FAIL() << "Got result: " + result.toString();
        };
    }

    std::function<void(const types::DiscoveryError::Enum&)> createExpectedDiscoveryErrorFunction(
            const types::DiscoveryError::Enum& expectedError)
    {
        return [this, expectedError](const types::DiscoveryError::Enum& error) {
            EXPECT_EQ(expectedError, error);
            _semaphore->notify();
        };
    }

protected:
    const bool _ENABLE_ACCESS_CONTROL;
    const bool _HAS_PERMISSION;
};

TEST_P(LocalCapabilitiesDirectoryACMockTest, checkPermissionToRegisterWithMock)
{
    auto mockAccessController = std::make_shared<MockAccessController>();
    ON_CALL(*mockAccessController, hasProviderPermission(_, _, _, _))
            .WillByDefault(Return(this->_HAS_PERMISSION));

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->setAccessController(util::as_weak_ptr(mockAccessController));

    types::DiscoveryEntry entry(_defaultProviderVersion,
                                _DOMAIN_1_NAME,
                                _INTERFACE_1_NAME,
                                _dummyParticipantIdsVector[0],
                                types::ProviderQos(),
                                _lastSeenDateMs,
                                _lastSeenDateMs + _defaultExpiryIntervalMs,
                                _PUBLIC_KEY_ID);

    try {
        _localCapabilitiesDirectory->add(
                entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);
    } catch (const exceptions::ProviderRuntimeException&) {
    }

    const types::DiscoveryQos localDiscoveryQos;
    if (!this->_ENABLE_ACCESS_CONTROL || this->_HAS_PERMISSION) {
        _localCapabilitiesDirectory->lookup(_dummyParticipantIdsVector[0],
                                            localDiscoveryQos,
                                            _KNOWN_GBIDS,
                                            createLookupParticipantIdSuccessFunction(),
                                            _unexpectedOnDiscoveryErrorFunction);
    } else {
        _localCapabilitiesDirectory->lookup(
                _dummyParticipantIdsVector[0],
                localDiscoveryQos,
                _KNOWN_GBIDS,
                createUnexpectedLookupParticipantIdSuccessFunction(),
                createExpectedDiscoveryErrorFunction(
                        types::DiscoveryError::NO_ENTRY_FOR_PARTICIPANT));
    }
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

std::tuple<bool, bool> const LCDWithAC_UseCases[] = {
        // Access controller enabled/disabled: tuple[0]
        // Emulation of "Has and not have" permission: tuple[1]
        make_tuple(false, false), make_tuple(false, true), make_tuple(true, false),
        make_tuple(true, true)};

INSTANTIATE_TEST_SUITE_P(WithAC,
                         LocalCapabilitiesDirectoryACMockTest,
                         ::testing::ValuesIn(LCDWithAC_UseCases));


