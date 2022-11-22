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

#include <string>
#include <tuple>
#include <vector>

#include "tests/utils/Gtest.h"

#include "joynr/CapabilitiesStorage.h"
#include "joynr/ClusterControllerSettings.h"
#include "joynr/ImmutableMessage.h"
#include "joynr/MulticastSubscriptionRequest.h"
#include "joynr/MutableMessage.h"
#include "joynr/MutableMessageFactory.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/Request.h"
#include "joynr/Settings.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/types/DiscoveryEntry.h"
#include "joynr/types/DiscoveryError.h"
#include "joynr/types/DiscoveryQos.h"
#include "joynr/types/Version.h"
#include "libjoynrclustercontroller/access-control/AccessController.h"
#include "libjoynrclustercontroller/access-control/LocalDomainAccessStore.h"

#include "tests/JoynrTest.h"
#include "tests/mock/MockAccessController.h"
#include "tests/mock/MockLocalCapabilitiesDirectory.h"
#include "tests/mock/MockMessageRouter.h"

using namespace ::testing;
using namespace joynr;
using namespace joynr::types;
using namespace joynr::infrastructure;
using namespace joynr::infrastructure::DacTypes;

class MockConsumerPermissionCallback
        : public joynr::IAccessController::IHasConsumerPermissionCallback
{
public:
    MOCK_METHOD1(hasConsumerPermission, void(IAccessController::Enum hasPermission));
};

class MockLocalDomainAccessStore : public LocalDomainAccessStore
{
};

template <typename... Ts>
joynr::Request initOutgoingRequest(std::string methodName,
                                   std::vector<std::string> paramDataTypes,
                                   Ts... paramValues)
{
    Request outgoingRequest;
    outgoingRequest.setMethodName(methodName);
    outgoingRequest.setParamDatatypes(std::move(paramDataTypes));
    outgoingRequest.setParams(std::move(paramValues)...);
    return outgoingRequest;
}

// Mock objects cannot make callbacks themselves but can make calls to methods
// with the same arguments as the mocked method call.
class ConsumerPermissionCallbackMaker
{
public:
    explicit ConsumerPermissionCallbackMaker(Permission::Enum permission) : _permission(permission)
    {
    }

    void consumerPermission(const std::string& userId,
                            const std::string& domain,
                            const std::string& interfaceName,
                            TrustLevel::Enum trustLevel,
                            std::shared_ptr<AccessController::IGetPermissionCallback> callback)
    {
        std::ignore = userId;
        std::ignore = domain;
        std::ignore = interfaceName;
        std::ignore = trustLevel;
        callback->permission(_permission);
    }

    void operationNeeded(const std::string& userId,
                         const std::string& domain,
                         const std::string& interfaceName,
                         TrustLevel::Enum trustLevel,
                         std::shared_ptr<AccessController::IGetPermissionCallback> callback)
    {
        std::ignore = userId;
        std::ignore = domain;
        std::ignore = interfaceName;
        std::ignore = trustLevel;
        callback->operationNeeded();
    }

private:
    Permission::Enum _permission;
};

class MockAccessControllerProtectedScope : public joynr::AccessController
{
public:
    MockAccessControllerProtectedScope(
            std::shared_ptr<joynr::LocalCapabilitiesDirectory> localCapabilitiesDirectory,
            std::shared_ptr<joynr::LocalDomainAccessStore> localDomainAccessStore)
            : AccessController(localCapabilitiesDirectory, localDomainAccessStore)
    {
    }

    MOCK_METHOD5(getConsumerPermission,
                 void(const std::string& userId,
                      const std::string& domain,
                      const std::string& interfaceName,
                      joynr::infrastructure::DacTypes::TrustLevel::Enum trustLevel,
                      std::shared_ptr<joynr::AccessController::IGetPermissionCallback> callback));

    MOCK_METHOD5(getConsumerPermission,
                 joynr::infrastructure::DacTypes::Permission::Enum(
                         const std::string& userId,
                         const std::string& domain,
                         const std::string& interfaceName,
                         const std::string& operation,
                         joynr::infrastructure::DacTypes::TrustLevel::Enum trustLevel));

    MOCK_METHOD5(getProviderPermission,
                 void(const std::string& userId,
                      const std::string& domain,
                      const std::string& interfaceName,
                      joynr::infrastructure::DacTypes::TrustLevel::Enum trustLevel,
                      std::shared_ptr<joynr::AccessController::IGetPermissionCallback> callback));

    MOCK_METHOD4(getProviderPermission,
                 joynr::infrastructure::DacTypes::Permission::Enum(
                         const std::string& uid,
                         const std::string& domain,
                         const std::string& interfacename,
                         joynr::infrastructure::DacTypes::TrustLevel::Enum trustLevel));
};

class AccessControllerTest : public ::testing::Test
{
public:
    AccessControllerTest()
            : _emptySettings(),
              _clusterControllerSettings(_emptySettings),
              _singleThreadedIOService(std::make_shared<SingleThreadedIOService>()),
              _accessControllerCallback(std::make_shared<MockConsumerPermissionCallback>()),
              _messageRouter(std::make_shared<MockMessageRouter>(
                      _singleThreadedIOService->getIOService())),
              _localCapabilitiesDirectoryStore(std::make_shared<LocalCapabilitiesDirectoryStore>()),
              _localCapabilitiesDirectoryMock(std::make_shared<MockLocalCapabilitiesDirectory>(
                      _clusterControllerSettings,
                      _messageRouter,
                      _localCapabilitiesDirectoryStore,
                      _singleThreadedIOService->getIOService(),
                      _defaultExpiryDateMs)),
              _messagingQos(MessagingQos(5000))
    {
        _singleThreadedIOService->start();
    }

    ~AccessControllerTest()
    {
        _localCapabilitiesDirectoryMock->shutdown();
        _singleThreadedIOService->stop();
    }

    void invokeOnSuccessCallbackFct(
            std::string participantId,
            const types::DiscoveryQos& discoveryQos,
            const std::vector<std::string>& gbids,
            std::function<void(const joynr::types::DiscoveryEntryWithMetaInfo&)> onSuccess,
            std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError)
    {
        std::ignore = participantId;
        std::ignore = gbids;
        std::ignore = onError;
        ASSERT_EQ(types::DiscoveryScope::LOCAL_THEN_GLOBAL, discoveryQos.getDiscoveryScope());
        onSuccess(_discoveryEntry);
    }

    void invokeOnErrorCallbackFct(
            std::string participantId,
            const types::DiscoveryQos& discoveryQos,
            const std::vector<std::string>& gbids,
            std::function<void(const joynr::types::DiscoveryEntryWithMetaInfo&)> onSuccess,
            std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError)
    {
        std::ignore = participantId;
        std::ignore = gbids;
        std::ignore = onSuccess;
        ASSERT_EQ(types::DiscoveryScope::LOCAL_THEN_GLOBAL, discoveryQos.getDiscoveryScope());
        onError(types::DiscoveryError::NO_ENTRY_FOR_PARTICIPANT);
    }

    void invokeOnSuccessCallbackFctLocalRecipient(
            std::string participantId,
            const types::DiscoveryQos& discoveryQos,
            const std::vector<std::string>& gbids,
            std::function<void(const joynr::types::DiscoveryEntryWithMetaInfo&)> onSuccess,
            std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError)
    {
        std::ignore = participantId;
        std::ignore = gbids;
        std::ignore = onError;
        ASSERT_EQ(types::DiscoveryScope::LOCAL_ONLY, discoveryQos.getDiscoveryScope());
        onSuccess(_discoveryEntry);
    }

    std::shared_ptr<ImmutableMessage> getImmutableMessage()
    {
        std::shared_ptr<ImmutableMessage> immutableMessage = _mutableMessage.getImmutableMessage();
        immutableMessage->setCreator(_DUMMY_USERID);
        return immutableMessage;
    }

    void SetUp()
    {
        _mutableMessage = _messageFactory.createRequest(_fromParticipantId,
                                                        _toParticipantId,
                                                        _messagingQos,
                                                        initOutgoingRequest(_TEST_OPERATION, {}),
                                                        _isLocalMessage);

        std::int64_t lastSeenDateMs = 0;
        std::int64_t expiryDateMs = 0;
        joynr::types::Version providerVersion(47, 11);
        _discoveryEntry = DiscoveryEntryWithMetaInfo(providerVersion,
                                                     _TEST_DOMAIN,
                                                     _TEST_INTERFACE,
                                                     _toParticipantId,
                                                     types::ProviderQos(),
                                                     lastSeenDateMs,
                                                     expiryDateMs,
                                                     _TEST_PUBLICKEYID,
                                                     false);
    }

    void prepareConsumerTest()
    {
        types::DiscoveryQos discoveryQos;
        discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);
        discoveryQos.setDiscoveryTimeout(60000);
        EXPECT_CALL(
                *_localCapabilitiesDirectoryMock,
                lookup(_toParticipantId,
                       discoveryQos,
                       std::vector<std::string>{},
                       A<std::function<void(const joynr::types::DiscoveryEntryWithMetaInfo&)>>(),
                       A<std::function<void(
                               const joynr::types::DiscoveryError::Enum& errorEnum)>>()))
                .Times(1)
                .WillOnce(Invoke(this, &AccessControllerTest::invokeOnSuccessCallbackFct));
    }

    void prepareConsumerTestLocalRecipient()
    {
        types::DiscoveryQos discoveryQos;
        discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_ONLY);
        discoveryQos.setDiscoveryTimeout(60000);
        EXPECT_CALL(
                *_localCapabilitiesDirectoryMock,
                lookup(_toParticipantId,
                       discoveryQos,
                       std::vector<std::string>{},
                       A<std::function<void(const joynr::types::DiscoveryEntryWithMetaInfo&)>>(),
                       A<std::function<void(
                               const joynr::types::DiscoveryError::Enum& errorEnum)>>()))
                .Times(1)
                .WillOnce(Invoke(
                        this, &AccessControllerTest::invokeOnSuccessCallbackFctLocalRecipient));
    }

    void prepareConsumerTestInRetryErrorCase()
    {
        types::DiscoveryQos discoveryQos;
        discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);
        discoveryQos.setDiscoveryTimeout(60000);
        EXPECT_CALL(
                *_localCapabilitiesDirectoryMock,
                lookup(_toParticipantId,
                       discoveryQos,
                       std::vector<std::string>{},
                       A<std::function<void(const joynr::types::DiscoveryEntryWithMetaInfo&)>>(),
                       A<std::function<void(
                               const joynr::types::DiscoveryError::Enum& errorEnum)>>()))
                .Times(1)
                .WillOnce(Invoke(this, &AccessControllerTest::invokeOnErrorCallbackFct));
    }

    void testPermission(Permission::Enum testPermission, IAccessController::Enum expectedPermission)
    {
        prepareConsumerTest();
        _localCapabilitiesDirectoryMock->init();
        ConsumerPermissionCallbackMaker makeCallback(testPermission);

        auto accessControllerProtectedScopeMock = std::make_shared<MockAccessControllerProtectedScope>(
                _localCapabilitiesDirectoryMock, std::make_unique<LocalDomainAccessStore>());
        EXPECT_CALL(*accessControllerProtectedScopeMock,
                    getConsumerPermission(
                            _DUMMY_USERID, _TEST_DOMAIN, _TEST_INTERFACE, TrustLevel::HIGH, _))
                .WillOnce(Invoke(
                        &makeCallback, &ConsumerPermissionCallbackMaker::consumerPermission));

        EXPECT_CALL(*_accessControllerCallback, hasConsumerPermission(expectedPermission)).Times(1);

        std::shared_ptr<ImmutableMessage> immutableMessage = _mutableMessage.getImmutableMessage();
        immutableMessage->setCreator(_DUMMY_USERID);
        // pass the immutable message to hasConsumerPermission
        accessControllerProtectedScopeMock->hasConsumerPermission(
                immutableMessage,
                std::static_pointer_cast<IAccessController::IHasConsumerPermissionCallback>(
                        _accessControllerCallback),
                false);
    }

    void testPermissionLocalRecipient(Permission::Enum testPermission,
                                      IAccessController::Enum expectedPermission)
    {
        prepareConsumerTestLocalRecipient();
        _localCapabilitiesDirectoryMock->init();
        ConsumerPermissionCallbackMaker makeCallback(testPermission);

        auto accessControllerProtectedScopeMock = std::make_shared<MockAccessControllerProtectedScope>(
                _localCapabilitiesDirectoryMock, std::make_unique<LocalDomainAccessStore>());
        EXPECT_CALL(*accessControllerProtectedScopeMock,
                    getConsumerPermission(
                            _DUMMY_USERID, _TEST_DOMAIN, _TEST_INTERFACE, TrustLevel::HIGH, _))
                .WillOnce(Invoke(
                        &makeCallback, &ConsumerPermissionCallbackMaker::consumerPermission));

        EXPECT_CALL(*_accessControllerCallback, hasConsumerPermission(expectedPermission)).Times(1);

        std::shared_ptr<ImmutableMessage> immutableMessage = _mutableMessage.getImmutableMessage();
        immutableMessage->setCreator(_DUMMY_USERID);
        // pass the immutable message to hasConsumerPermission
        accessControllerProtectedScopeMock->hasConsumerPermission(
                immutableMessage,
                std::static_pointer_cast<IAccessController::IHasConsumerPermissionCallback>(
                        _accessControllerCallback),
                true);
    }

protected:
    Settings _emptySettings;
    ClusterControllerSettings _clusterControllerSettings;
    std::shared_ptr<SingleThreadedIOService> _singleThreadedIOService;
    std::shared_ptr<MockLocalDomainAccessStore> _localDomainAccessStoreMock;
    std::shared_ptr<MockConsumerPermissionCallback> _accessControllerCallback;
    std::shared_ptr<MockMessageRouter> _messageRouter;
    std::shared_ptr<LocalCapabilitiesDirectoryStore> _localCapabilitiesDirectoryStore;
    std::shared_ptr<MockLocalCapabilitiesDirectory> _localCapabilitiesDirectoryMock;
    MutableMessageFactory _messageFactory;
    MutableMessage _mutableMessage;
    MessagingQos _messagingQos;
    const bool _isLocalMessage = true;
    const std::int64_t _defaultExpiryDateMs = 60 * 60 * 1000;
    DiscoveryEntryWithMetaInfo _discoveryEntry;
    static const std::string _fromParticipantId;
    static const std::string _toParticipantId;
    static const std::string _subscriptionId;
    static const std::string _replyToChannelId;
    static const std::string _DUMMY_USERID;
    static const std::string _TEST_DOMAIN;
    static const std::string _TEST_INTERFACE;
    static const std::string _TEST_OPERATION;
    static const std::string _TEST_PUBLICKEYID;

private:
    DISALLOW_COPY_AND_ASSIGN(AccessControllerTest);
};

//----- Constants --------------------------------------------------------------
const std::string AccessControllerTest::_fromParticipantId("sender");
const std::string AccessControllerTest::_toParticipantId("receiver");
const std::string AccessControllerTest::_subscriptionId("testSubscriptionId");
const std::string AccessControllerTest::_replyToChannelId("replyToId");
const std::string AccessControllerTest::_DUMMY_USERID("testUserId");
const std::string AccessControllerTest::_TEST_DOMAIN("testDomain");
const std::string AccessControllerTest::_TEST_INTERFACE("testInterface");
const std::string AccessControllerTest::_TEST_OPERATION("testOperation");
const std::string AccessControllerTest::_TEST_PUBLICKEYID("publicKeyId");

//----- Tests ------------------------------------------------------------------

TEST_F(AccessControllerTest, accessWithInterfaceLevelAccessControl)
{
    prepareConsumerTest();
    _localCapabilitiesDirectoryMock->init();
    ConsumerPermissionCallbackMaker makeCallback(Permission::YES);

    auto accessControllerProtectedScopeMock = std::make_shared<MockAccessControllerProtectedScope>(
            _localCapabilitiesDirectoryMock, std::make_unique<LocalDomainAccessStore>());
    EXPECT_CALL(*accessControllerProtectedScopeMock,
                getConsumerPermission(
                        _DUMMY_USERID, _TEST_DOMAIN, _TEST_INTERFACE, TrustLevel::HIGH, _))
            .Times(1)
            .WillOnce(Invoke(&makeCallback, &ConsumerPermissionCallbackMaker::consumerPermission));

    EXPECT_CALL(*_accessControllerCallback, hasConsumerPermission(IAccessController::Enum::YES))
            .Times(1);

    accessControllerProtectedScopeMock->hasConsumerPermission(
            getImmutableMessage(),
            std::dynamic_pointer_cast<IAccessController::IHasConsumerPermissionCallback>(
                    _accessControllerCallback),
            false);
}

TEST_F(AccessControllerTest, accessWithOperationLevelAccessControl)
{
    prepareConsumerTest();
    _localCapabilitiesDirectoryMock->init();
    ConsumerPermissionCallbackMaker makeCallback(Permission::YES);

    auto accessControllerProtectedScopeMock = std::make_shared<MockAccessControllerProtectedScope>(
            _localCapabilitiesDirectoryMock, std::make_unique<LocalDomainAccessStore>());
    EXPECT_CALL(*accessControllerProtectedScopeMock,
                getConsumerPermission(
                        _DUMMY_USERID, _TEST_DOMAIN, _TEST_INTERFACE, TrustLevel::HIGH, _))
            .Times(1)
            .WillOnce(Invoke(&makeCallback, &ConsumerPermissionCallbackMaker::operationNeeded));

    Permission::Enum permissionYes = Permission::YES;
    DefaultValue<Permission::Enum>::Set(permissionYes);
    EXPECT_CALL(*accessControllerProtectedScopeMock,
                getConsumerPermission(_DUMMY_USERID, _TEST_DOMAIN, _TEST_INTERFACE, _TEST_OPERATION,
                                      TrustLevel::HIGH))
            .WillOnce(Return(permissionYes));

    EXPECT_CALL(*_accessControllerCallback, hasConsumerPermission(IAccessController::Enum::YES))
            .Times(1);

    accessControllerProtectedScopeMock->hasConsumerPermission(
            getImmutableMessage(),
            std::dynamic_pointer_cast<IAccessController::IHasConsumerPermissionCallback>(
                    _accessControllerCallback),
            false);
}

TEST_F(AccessControllerTest, accessWithOperationLevelAccessControlAndFaultyMessage)
{
    prepareConsumerTest();
    _localCapabilitiesDirectoryMock->init();
    ConsumerPermissionCallbackMaker makeCallback(Permission::YES);

    auto accessControllerProtectedScopeMock = std::make_shared<MockAccessControllerProtectedScope>(
            _localCapabilitiesDirectoryMock, std::make_unique<LocalDomainAccessStore>());
    EXPECT_CALL(*accessControllerProtectedScopeMock,
                getConsumerPermission(
                        _DUMMY_USERID, _TEST_DOMAIN, _TEST_INTERFACE, TrustLevel::HIGH, _))
            .Times(1)
            .WillOnce(Invoke(&makeCallback, &ConsumerPermissionCallbackMaker::operationNeeded));

    EXPECT_CALL(*_accessControllerCallback, hasConsumerPermission(IAccessController::Enum::NO))
            .Times(1);

    std::string payload("invalid serialization of Request object");
    _mutableMessage.setPayload(payload);

    accessControllerProtectedScopeMock->hasConsumerPermission(
            getImmutableMessage(),
            std::dynamic_pointer_cast<IAccessController::IHasConsumerPermissionCallback>(
                    _accessControllerCallback),
            false);
}

TEST_F(AccessControllerTest, retryAccessControlCheckIfNoDiscoveryEntry)
{
    prepareConsumerTestInRetryErrorCase();
    _localCapabilitiesDirectoryMock->init();

    auto accessControllerProtectedScopeMock = std::make_shared<MockAccessControllerProtectedScope>(
            _localCapabilitiesDirectoryMock, std::make_unique<LocalDomainAccessStore>());
    EXPECT_CALL(*_accessControllerCallback, hasConsumerPermission(IAccessController::Enum::RETRY))
            .Times(1);

    std::shared_ptr<ImmutableMessage> immutableMessage = _mutableMessage.getImmutableMessage();
    immutableMessage->setCreator(_DUMMY_USERID);
    // pass the immutable message to hasConsumerPermission
    accessControllerProtectedScopeMock->hasConsumerPermission(
            immutableMessage,
            std::static_pointer_cast<IAccessController::IHasConsumerPermissionCallback>(
                    _accessControllerCallback),
            false);
}

TEST_F(AccessControllerTest, hasProviderPermission)
{
    _localCapabilitiesDirectoryMock->init();
    Permission::Enum permissionYes = Permission::YES;
    DefaultValue<Permission::Enum>::Set(permissionYes);

    auto accessControllerProtectedScopeMock = std::make_shared<MockAccessControllerProtectedScope>(
            _localCapabilitiesDirectoryMock, std::make_unique<LocalDomainAccessStore>());
    EXPECT_CALL(*accessControllerProtectedScopeMock, getProviderPermission(_, _, _, _))
            .Times(1)
            .WillOnce(Return(permissionYes));

    bool retval = accessControllerProtectedScopeMock->hasProviderPermission(
            _DUMMY_USERID, TrustLevel::HIGH, _TEST_DOMAIN, _TEST_INTERFACE);
    EXPECT_TRUE(retval);
}

TEST_F(AccessControllerTest, hasNoProviderPermission)
{
    _localCapabilitiesDirectoryMock->init();
    Permission::Enum permissionNo = Permission::NO;
    DefaultValue<Permission::Enum>::Set(permissionNo);

    auto accessControllerProtectedScopeMock = std::make_shared<MockAccessControllerProtectedScope>(
            _localCapabilitiesDirectoryMock, std::make_unique<LocalDomainAccessStore>());
    EXPECT_CALL(*accessControllerProtectedScopeMock, getProviderPermission(_, _, _, _))
            .Times(1)
            .WillOnce(Return(permissionNo));

    bool retval = accessControllerProtectedScopeMock->hasProviderPermission(
            _DUMMY_USERID, TrustLevel::HIGH, _TEST_DOMAIN, _TEST_INTERFACE);
    EXPECT_FALSE(retval);
}

//----- Test Types --------------------------------------------------------------
typedef ::testing::
        Types<SubscriptionRequest, MulticastSubscriptionRequest, BroadcastSubscriptionRequest>
                SubscriptionTypes;

template <typename T>
class AccessControllerSubscriptionTest : public AccessControllerTest
{
public:
    template <typename U = T>
    typename std::enable_if_t<std::is_same<U, SubscriptionRequest>::value> createMutableMessage()
    {
        SubscriptionRequest subscriptionRequest;
        subscriptionRequest.setSubscriptionId(_subscriptionId);
        _mutableMessage = _messageFactory.createSubscriptionRequest(_fromParticipantId,
                                                                    _toParticipantId,
                                                                    _messagingQos,
                                                                    subscriptionRequest,
                                                                    _isLocalMessage);
    }

    template <typename U = T>
    typename std::enable_if_t<std::is_same<U, MulticastSubscriptionRequest>::value>
    createMutableMessage()
    {
        MulticastSubscriptionRequest subscriptionRequest;
        _mutableMessage = _messageFactory.createMulticastSubscriptionRequest(_fromParticipantId,
                                                                             _toParticipantId,
                                                                             _messagingQos,
                                                                             subscriptionRequest,
                                                                             _isLocalMessage);
    }

    template <typename U = T>
    typename std::enable_if_t<std::is_same<U, BroadcastSubscriptionRequest>::value>
    createMutableMessage()
    {
        BroadcastSubscriptionRequest subscriptionRequest;
        _mutableMessage = _messageFactory.createBroadcastSubscriptionRequest(_fromParticipantId,
                                                                             _toParticipantId,
                                                                             _messagingQos,
                                                                             subscriptionRequest,
                                                                             _isLocalMessage);
    }
};

TYPED_TEST_SUITE(AccessControllerSubscriptionTest, SubscriptionTypes, );

TYPED_TEST(AccessControllerSubscriptionTest, hasNoConsumerPermission)
{
    const Permission::Enum permissionNo = Permission::NO;
    const IAccessController::Enum expectedPermissionFalse = IAccessController::Enum::NO;

    this->createMutableMessage();

    this->testPermission(permissionNo, expectedPermissionFalse);
}

TYPED_TEST(AccessControllerSubscriptionTest, hasConsumerPermission)
{
    const Permission::Enum permissionNo = Permission::YES;
    const IAccessController::Enum expectedPermissionFalse = IAccessController::Enum::YES;

    this->createMutableMessage();

    this->testPermission(permissionNo, expectedPermissionFalse);
}

TYPED_TEST(AccessControllerSubscriptionTest, hasNoConsumerPermissionLocalRecipient)
{
    const Permission::Enum permissionNo = Permission::NO;
    const IAccessController::Enum expectedPermissionFalse = IAccessController::Enum::NO;

    this->createMutableMessage();

    this->testPermissionLocalRecipient(permissionNo, expectedPermissionFalse);
}

TYPED_TEST(AccessControllerSubscriptionTest, hasConsumerPermissionLocalRecipient)
{
    const Permission::Enum permissionNo = Permission::YES;
    const IAccessController::Enum expectedPermissionFalse = IAccessController::Enum::YES;

    this->createMutableMessage();

    this->testPermissionLocalRecipient(permissionNo, expectedPermissionFalse);
}

//-----  AccessController WithOrWithoutPersistFile tests
//-------------------------------------------------------------

// Consumer permissions are obtained asynchronously
class PermissionCallback : public AccessController::IGetPermissionCallback
{
public:
    PermissionCallback() : isValid(false), storedPermission(Permission::YES), sem(0)
    {
    }

    ~PermissionCallback() = default;

    void permission(Permission::Enum permission)
    {
        this->storedPermission = permission;
        isValid = true;
        sem.notify();
    }

    void operationNeeded()
    {
        sem.notify(); // isValid stays false
    }

    bool isPermissionAvailable() const
    {
        return isValid;
    }

    Permission::Enum getPermission() const
    {
        return storedPermission;
    }

    // Returns true if the callback was made
    bool expectCallback(int millisecs)
    {
        return sem.waitFor(std::chrono::milliseconds(millisecs));
    }

private:
    bool isValid;
    Permission::Enum storedPermission;
    Semaphore sem;
};

class AccessControllerProtectedScopeWrapper : public AccessController
{
public:
    using AccessController::AccessController;
    using AccessController::getConsumerPermission;
    using AccessController::getProviderPermission;
};

class AccessControllerProtectedScopeTest : public testing::TestWithParam<bool>
{
public:
    AccessControllerProtectedScopeTest() : localDomainAccessStorePtr(nullptr)
    {
    }

    ~AccessControllerProtectedScopeTest() override
    {
        // Delete test specific files
        joynr::test::util::removeFileInCurrentDirectory(".*\\.settings");
        joynr::test::util::removeFileInCurrentDirectory(".*\\.persist");
    }

    void SetUp() override
    {
        std::unique_ptr<LocalDomainAccessStore> localDomainAccessStore;
        if (GetParam()) {
            joynr::test::util::copyTestResourceToCurrentDirectory("AccessStoreTest.persist");
            localDomainAccessStore =
                    std::make_unique<LocalDomainAccessStore>("AccessStoreTest.persist");
        } else {
            localDomainAccessStore = std::make_unique<LocalDomainAccessStore>();
        }
        localDomainAccessStorePtr = localDomainAccessStore.get();
        _accessController = std::make_unique<AccessControllerProtectedScopeWrapper>(
                nullptr, std::move(localDomainAccessStore));

        userDre = DomainRoleEntry(TEST_USER, DOMAINS, Role::OWNER);
        masterAce = MasterAccessControlEntry(
                TEST_USER,       // uid
                TEST_DOMAIN1,    // domain
                TEST_INTERFACE1, // interface name
                TrustLevel::LOW, // default required trust level
                TRUST_LEVELS,    // possible required trust levels
                TrustLevel::LOW, // default required control entry change trust level
                TRUST_LEVELS,    // possible required control entry change trust levels
                TEST_OPERATION1, // operation
                Permission::NO,  // default comsumer permission
                PERMISSIONS      // possible comsumer permissions
        );
        ownerAce = OwnerAccessControlEntry(TEST_USER,       // uid
                                           TEST_DOMAIN1,    // domain
                                           TEST_INTERFACE1, // interface name
                                           TrustLevel::LOW, // required trust level
                                           TrustLevel::LOW, // required ACE change trust level
                                           TEST_OPERATION1, // operation
                                           Permission::YES  // consumer permission
        );
        masterRce = MasterRegistrationControlEntry(
                TEST_USER,       // uid
                TEST_DOMAIN1,    // domain
                TEST_INTERFACE1, // interface name
                TrustLevel::LOW, // default required trust level
                TRUST_LEVELS,    // possible required trust levels
                TrustLevel::LOW, // default required control entry change trust level
                TRUST_LEVELS,    // possible required control entry change trust levels
                Permission::NO,  // default provider permission
                PERMISSIONS      // possible provider permissions
        );
        ownerRce = OwnerRegistrationControlEntry(TEST_USER,       // uid
                                                 TEST_DOMAIN1,    // domain
                                                 TEST_INTERFACE1, // interface name
                                                 TrustLevel::LOW, // required trust level
                                                 TrustLevel::LOW, // required ACE change trust level
                                                 Permission::YES  // provider permission
        );
    }

    static const std::string TEST_USER;
    static const std::string TEST_DOMAIN1;
    static const std::string TEST_INTERFACE1;
    static const std::string TEST_OPERATION1;
    static const std::vector<std::string> DOMAINS;
    static const std::vector<Permission::Enum> PERMISSIONS;
    static const std::vector<TrustLevel::Enum> TRUST_LEVELS;

protected:
    LocalDomainAccessStore* localDomainAccessStorePtr;
    std::unique_ptr<AccessControllerProtectedScopeWrapper> _accessController;

    OwnerAccessControlEntry ownerAce;
    MasterAccessControlEntry masterAce;
    OwnerRegistrationControlEntry ownerRce;
    MasterRegistrationControlEntry masterRce;
    DomainRoleEntry userDre;

private:
    DISALLOW_COPY_AND_ASSIGN(AccessControllerProtectedScopeTest);
};

//----- Constants --------------------------------------------------------------
const std::string AccessControllerProtectedScopeTest::TEST_USER("testUser");
const std::string AccessControllerProtectedScopeTest::TEST_DOMAIN1("domain1");
const std::string AccessControllerProtectedScopeTest::TEST_INTERFACE1("interface1");
const std::string AccessControllerProtectedScopeTest::TEST_OPERATION1("operation1");
const std::vector<std::string> AccessControllerProtectedScopeTest::DOMAINS = {
        AccessControllerProtectedScopeTest::TEST_DOMAIN1};
const std::vector<Permission::Enum> AccessControllerProtectedScopeTest::PERMISSIONS = {
        Permission::NO, Permission::ASK, Permission::YES};
const std::vector<TrustLevel::Enum> AccessControllerProtectedScopeTest::TRUST_LEVELS = {
        TrustLevel::LOW, TrustLevel::MID, TrustLevel::HIGH};

//----- Tests ------------------------------------------------------------------

TEST_P(AccessControllerProtectedScopeTest, testHasRole)
{
    localDomainAccessStorePtr->updateDomainRole(userDre);
    EXPECT_TRUE(_accessController->hasRole(AccessControllerProtectedScopeTest::TEST_USER,
                                           AccessControllerProtectedScopeTest::TEST_DOMAIN1,
                                           Role::OWNER));
}

TEST_P(AccessControllerProtectedScopeTest, testHasRoleWithWildcard)
{
    const std::string wildcard = "*";
    userDre.setDomains({wildcard});
    localDomainAccessStorePtr->updateDomainRole(userDre);
    EXPECT_TRUE(_accessController->hasRole(AccessControllerProtectedScopeTest::TEST_USER,
                                           AccessControllerProtectedScopeTest::TEST_DOMAIN1,
                                           Role::OWNER));
}

TEST_P(AccessControllerProtectedScopeTest, consumerPermission)
{
    localDomainAccessStorePtr->updateOwnerAccessControlEntry(ownerAce);
    EXPECT_EQ(Permission::YES,
              _accessController->getConsumerPermission(
                      AccessControllerProtectedScopeTest::TEST_USER,
                      AccessControllerProtectedScopeTest::TEST_DOMAIN1,
                      AccessControllerProtectedScopeTest::TEST_INTERFACE1,
                      AccessControllerProtectedScopeTest::TEST_OPERATION1,
                      TrustLevel::HIGH));
}

TEST_P(AccessControllerProtectedScopeTest, consumerPermissionInvalidOwnerAce)
{
    localDomainAccessStorePtr->updateOwnerAccessControlEntry(ownerAce);

    // Update the MasterACE so that it does not permit Permission::YES
    std::vector<Permission::Enum> possiblePermissions = {Permission::NO, Permission::ASK};
    masterAce.setDefaultConsumerPermission(Permission::ASK);
    masterAce.setPossibleConsumerPermissions(possiblePermissions);
    localDomainAccessStorePtr->updateMasterAccessControlEntry(masterAce);

    EXPECT_EQ(Permission::NO,
              _accessController->getConsumerPermission(
                      AccessControllerProtectedScopeTest::TEST_USER,
                      AccessControllerProtectedScopeTest::TEST_DOMAIN1,
                      AccessControllerProtectedScopeTest::TEST_INTERFACE1,
                      AccessControllerProtectedScopeTest::TEST_OPERATION1,
                      TrustLevel::HIGH));
}

TEST_P(AccessControllerProtectedScopeTest, consumerPermissionOwnerAceOverrulesMaster)
{
    ownerAce.setRequiredTrustLevel(TrustLevel::MID);
    ownerAce.setConsumerPermission(Permission::ASK);
    localDomainAccessStorePtr->updateOwnerAccessControlEntry(ownerAce);
    localDomainAccessStorePtr->updateMasterAccessControlEntry(masterAce);

    EXPECT_EQ(Permission::ASK,
              _accessController->getConsumerPermission(
                      AccessControllerProtectedScopeTest::TEST_USER,
                      AccessControllerProtectedScopeTest::TEST_DOMAIN1,
                      AccessControllerProtectedScopeTest::TEST_INTERFACE1,
                      AccessControllerProtectedScopeTest::TEST_OPERATION1,
                      TrustLevel::HIGH));
    EXPECT_EQ(Permission::NO,
              _accessController->getConsumerPermission(
                      AccessControllerProtectedScopeTest::TEST_USER,
                      AccessControllerProtectedScopeTest::TEST_DOMAIN1,
                      AccessControllerProtectedScopeTest::TEST_INTERFACE1,
                      AccessControllerProtectedScopeTest::TEST_OPERATION1,
                      TrustLevel::LOW));
}

TEST_P(AccessControllerProtectedScopeTest, consumerPermissionOperationWildcard)
{
    ownerAce.setOperation(access_control::WILDCARD);
    localDomainAccessStorePtr->updateOwnerAccessControlEntry(ownerAce);

    EXPECT_EQ(Permission::YES,
              _accessController->getConsumerPermission(
                      AccessControllerProtectedScopeTest::TEST_USER,
                      AccessControllerProtectedScopeTest::TEST_DOMAIN1,
                      AccessControllerProtectedScopeTest::TEST_INTERFACE1,
                      AccessControllerProtectedScopeTest::TEST_OPERATION1,
                      TrustLevel::HIGH));
}

TEST_P(AccessControllerProtectedScopeTest, consumerPermissionAmbigious)
{
    // Setup the master with a wildcard operation
    masterAce.setOperation(access_control::WILDCARD);
    localDomainAccessStorePtr->updateOwnerAccessControlEntry(ownerAce);
    localDomainAccessStorePtr->updateMasterAccessControlEntry(masterAce);

    // Get the consumer permission (async)
    auto getConsumerPermissionCallback = std::make_shared<PermissionCallback>();

    _accessController->getConsumerPermission(AccessControllerProtectedScopeTest::TEST_USER,
                                             AccessControllerProtectedScopeTest::TEST_DOMAIN1,
                                             AccessControllerProtectedScopeTest::TEST_INTERFACE1,
                                             TrustLevel::HIGH,
                                             getConsumerPermissionCallback);

    EXPECT_TRUE(getConsumerPermissionCallback->expectCallback(1000));

    // The operation is ambigious and interface level permission is not available
    EXPECT_FALSE(getConsumerPermissionCallback->isPermissionAvailable());

    // Operation level permission should work
    EXPECT_EQ(Permission::YES,
              _accessController->getConsumerPermission(
                      AccessControllerProtectedScopeTest::TEST_USER,
                      AccessControllerProtectedScopeTest::TEST_DOMAIN1,
                      AccessControllerProtectedScopeTest::TEST_INTERFACE1,
                      AccessControllerProtectedScopeTest::TEST_OPERATION1,
                      TrustLevel::HIGH));
}

// true: with persist file
// false: without
INSTANTIATE_TEST_SUITE_P(WithOrWithoutPersistFile, AccessControllerProtectedScopeTest, Bool());

TEST(AccessControllerPersistedTest, persistedAcesAreUsed)
{
    // Load persisted ACEs
    joynr::test::util::copyTestResourceToCurrentDirectory("AccessStoreTest.persist");
    auto localDomainAccessStore =
            std::make_unique<LocalDomainAccessStore>("AccessStoreTest.persist");
    auto accessController = std::make_unique<AccessControllerProtectedScopeWrapper>(
            nullptr, std::move(localDomainAccessStore));

    EXPECT_EQ(Permission::NO,
              accessController->getConsumerPermission(
                      AccessControllerProtectedScopeTest::TEST_USER,
                      AccessControllerProtectedScopeTest::TEST_DOMAIN1,
                      AccessControllerProtectedScopeTest::TEST_INTERFACE1,
                      AccessControllerProtectedScopeTest::TEST_OPERATION1,
                      TrustLevel::HIGH));
}

// Registration control entries

TEST_P(AccessControllerProtectedScopeTest, providerPermission)
{
    localDomainAccessStorePtr->updateOwnerRegistrationControlEntry(ownerRce);
    EXPECT_EQ(Permission::YES,
              _accessController->getProviderPermission(
                      AccessControllerProtectedScopeTest::TEST_USER,
                      AccessControllerProtectedScopeTest::TEST_DOMAIN1,
                      AccessControllerProtectedScopeTest::TEST_INTERFACE1,
                      TrustLevel::HIGH));
}

TEST_P(AccessControllerProtectedScopeTest, providerPermissionInvalidOwnerRce)
{
    localDomainAccessStorePtr->updateOwnerRegistrationControlEntry(ownerRce);

    // Update the MasterACE so that it does not permit Permission::YES
    std::vector<Permission::Enum> possiblePermissions = {Permission::NO, Permission::ASK};
    masterRce.setDefaultProviderPermission(Permission::ASK);
    masterRce.setPossibleProviderPermissions(possiblePermissions);
    localDomainAccessStorePtr->updateMasterRegistrationControlEntry(masterRce);

    EXPECT_EQ(Permission::NO,
              _accessController->getProviderPermission(
                      AccessControllerProtectedScopeTest::TEST_USER,
                      AccessControllerProtectedScopeTest::TEST_DOMAIN1,
                      AccessControllerProtectedScopeTest::TEST_INTERFACE1,
                      TrustLevel::HIGH));
}

TEST_P(AccessControllerProtectedScopeTest, providerPermissionOwnerRceOverrulesMaster)
{
    ownerRce.setRequiredTrustLevel(TrustLevel::MID);
    ownerRce.setProviderPermission(Permission::ASK);
    localDomainAccessStorePtr->updateOwnerRegistrationControlEntry(ownerRce);
    localDomainAccessStorePtr->updateMasterRegistrationControlEntry(masterRce);

    EXPECT_EQ(Permission::ASK,
              _accessController->getProviderPermission(
                      AccessControllerProtectedScopeTest::TEST_USER,
                      AccessControllerProtectedScopeTest::TEST_DOMAIN1,
                      AccessControllerProtectedScopeTest::TEST_INTERFACE1,
                      TrustLevel::HIGH));
    EXPECT_EQ(Permission::NO,
              _accessController->getProviderPermission(
                      AccessControllerProtectedScopeTest::TEST_USER,
                      AccessControllerProtectedScopeTest::TEST_DOMAIN1,
                      AccessControllerProtectedScopeTest::TEST_INTERFACE1,
                      TrustLevel::LOW));
}

TEST_P(AccessControllerProtectedScopeTest, DISABLED_providerPermissionAmbigious)
{
    // Setup the master with a wildcard operation
    masterAce.setOperation(access_control::WILDCARD);
    localDomainAccessStorePtr->updateOwnerAccessControlEntry(ownerAce);
    localDomainAccessStorePtr->updateMasterAccessControlEntry(masterAce);

    // Get the provider permission (async)
    auto getProviderPermissionCallback = std::make_shared<PermissionCallback>();

    _accessController->getProviderPermission(AccessControllerProtectedScopeTest::TEST_USER,
                                             AccessControllerProtectedScopeTest::TEST_DOMAIN1,
                                             AccessControllerProtectedScopeTest::TEST_INTERFACE1,
                                             TrustLevel::HIGH,
                                             getProviderPermissionCallback);

    EXPECT_TRUE(getProviderPermissionCallback->expectCallback(1000));

    // The operation is ambigious and interface level permission is not available
    EXPECT_FALSE(getProviderPermissionCallback->isPermissionAvailable());

    // Operation level permission should work
    EXPECT_EQ(Permission::YES,
              _accessController->getProviderPermission(
                      AccessControllerProtectedScopeTest::TEST_USER,
                      AccessControllerProtectedScopeTest::TEST_DOMAIN1,
                      AccessControllerProtectedScopeTest::TEST_INTERFACE1,
                      TrustLevel::HIGH));
}

TEST(AccessControllerPersistedTest, persistedRcesAreUsed)
{
    // Load persisted ACEs
    joynr::test::util::copyTestResourceToCurrentDirectory("AccessStoreTest.persist");
    auto localDomainAccessStore =
            std::make_unique<LocalDomainAccessStore>("AccessStoreTest.persist");
    auto accessController = std::make_unique<AccessControllerProtectedScopeWrapper>(
            nullptr, std::move(localDomainAccessStore));

    EXPECT_EQ(Permission::NO,
              accessController->getProviderPermission(
                      AccessControllerProtectedScopeTest::TEST_USER,
                      AccessControllerProtectedScopeTest::TEST_DOMAIN1,
                      AccessControllerProtectedScopeTest::TEST_INTERFACE1,
                      TrustLevel::HIGH));
}
