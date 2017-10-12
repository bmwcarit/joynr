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

#include <tuple>
#include <string>

#include <gtest/gtest.h>

#include "joynr/ClusterControllerSettings.h"
#include "joynr/MutableMessageFactory.h"
#include "joynr/MutableMessage.h"
#include "joynr/ImmutableMessage.h"
#include "joynr/MulticastSubscriptionRequest.h"
#include "joynr/Request.h"
#include "joynr/PrivateCopyAssign.h"
#include "tests/utils/MockObjects.h"
#include "libjoynrclustercontroller/access-control/AccessController.h"
#include "libjoynrclustercontroller/access-control/LocalDomainAccessStore.h"
#include "joynr/types/DiscoveryEntry.h"
#include "joynr/types/Version.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/serializer/Serializer.h"

using namespace ::testing;
using namespace joynr;
using namespace joynr::types;
using namespace joynr::infrastructure;
using namespace joynr::infrastructure::DacTypes;


template <typename... Ts>
joynr::Request initOutgoingRequest(std::string methodName, std::vector<std::string> paramDataTypes, Ts... paramValues)
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
    explicit ConsumerPermissionCallbackMaker(Permission::Enum permission) :
        permission(permission)
    {}

    void consumerPermission(
            const std::string& userId,
            const std::string& domain,
            const std::string& interfaceName,
            TrustLevel::Enum trustLevel,
            std::shared_ptr<LocalDomainAccessController::IGetPermissionCallback> callback
    ) {
        std::ignore = userId;
        std::ignore = domain;
        std::ignore = interfaceName;
        std::ignore = trustLevel;
        callback->permission(permission);
    }

    void operationNeeded(
            const std::string& userId,
            const std::string& domain,
            const std::string& interfaceName,
            TrustLevel::Enum trustLevel,
            std::shared_ptr<LocalDomainAccessController::IGetPermissionCallback> callback
    ) {
        std::ignore = userId;
        std::ignore = domain;
        std::ignore = interfaceName;
        std::ignore = trustLevel;
        callback->operationNeeded();
    }

private:
    Permission::Enum permission;
};


class AccessControllerTest : public ::testing::Test {
public:
    AccessControllerTest() :
        emptySettings(),
        clusterControllerSettings(emptySettings),
        singleThreadedIOService(),
        localDomainAccessControllerMock(std::make_shared<MockLocalDomainAccessController>(std::make_unique<LocalDomainAccessStore>(), false)),
        accessControllerCallback(std::make_shared<MockConsumerPermissionCallback>()),
        messageRouter(std::make_shared<MockMessageRouter>(singleThreadedIOService.getIOService())),
        localCapabilitiesDirectoryMock(std::make_shared<MockLocalCapabilitiesDirectory>(clusterControllerSettings, messageRouter, singleThreadedIOService.getIOService())),
        accessController(
                localCapabilitiesDirectoryMock,
                localDomainAccessControllerMock
        ),
        messagingQos(MessagingQos(5000))
    {
        singleThreadedIOService.start();
    }

    ~AccessControllerTest()
    {
        localCapabilitiesDirectoryMock->shutdown();
        singleThreadedIOService.stop();
    }

    void invokeOnSuccessCallbackFct (std::string participantId,
                            std::function<void(const joynr::types::DiscoveryEntryWithMetaInfo&)> onSuccess,
                            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError) {
        std::ignore = participantId;
        onSuccess(discoveryEntry);
    }

    std::shared_ptr<ImmutableMessage> getImmutableMessage()
    {
        std::shared_ptr<ImmutableMessage> immutableMessage = mutableMessage.getImmutableMessage();
        immutableMessage->setCreator(DUMMY_USERID);
        return immutableMessage;
    }

    void SetUp(){
        mutableMessage = messageFactory.createRequest(fromParticipantId,
                                     toParticipantId,
                                     messagingQos,
                                     initOutgoingRequest(TEST_OPERATION, {}),
                                     isLocalMessage);

        std::int64_t lastSeenDateMs = 0;
        std::int64_t expiryDateMs = 0;
        joynr::types::Version providerVersion(47, 11);
        discoveryEntry = DiscoveryEntryWithMetaInfo(
                providerVersion,
                TEST_DOMAIN,
                TEST_INTERFACE,
                toParticipantId,
                types::ProviderQos(),
                lastSeenDateMs,
                expiryDateMs,
                TEST_PUBLICKEYID,
                false
        );
    }

    void prepareConsumerTest() {
        EXPECT_CALL(
                *localCapabilitiesDirectoryMock,
                lookup(toParticipantId,
                       A<std::function<void(const joynr::types::DiscoveryEntryWithMetaInfo&)>>(),
                       A<std::function<void(const joynr::exceptions::ProviderRuntimeException&)>>())
        )
                .Times(1)
                .WillOnce(Invoke(this, &AccessControllerTest::invokeOnSuccessCallbackFct));
    }

    void testPermission(Permission::Enum testPermission, bool expectedPermission)
    {
        prepareConsumerTest();
        std::shared_ptr<ImmutableMessage> immutableMessage = mutableMessage.getImmutableMessage();
        immutableMessage->setCreator(DUMMY_USERID);

        ConsumerPermissionCallbackMaker makeCallback(testPermission);
        EXPECT_CALL(
                *localDomainAccessControllerMock,
                getConsumerPermission(DUMMY_USERID, TEST_DOMAIN, TEST_INTERFACE, TrustLevel::HIGH, _)
        )
                .WillOnce(Invoke(&makeCallback, &ConsumerPermissionCallbackMaker::consumerPermission));
        EXPECT_CALL(*accessControllerCallback, hasConsumerPermission(expectedPermission))
                .Times(1);

        // pass the immutable message to hasConsumerPermission
        accessController.hasConsumerPermission(
                immutableMessage,
                std::static_pointer_cast<IAccessController::IHasConsumerPermissionCallback>(accessControllerCallback)
        );
    }

protected:
    Settings emptySettings;
    ClusterControllerSettings clusterControllerSettings;
    SingleThreadedIOService singleThreadedIOService;
    std::shared_ptr<MockLocalDomainAccessController> localDomainAccessControllerMock;
    std::shared_ptr<MockConsumerPermissionCallback> accessControllerCallback;
    std::shared_ptr<MockMessageRouter> messageRouter;
    std::shared_ptr<MockLocalCapabilitiesDirectory> localCapabilitiesDirectoryMock;
    AccessController accessController;
    MutableMessageFactory messageFactory;
    MutableMessage mutableMessage;
    MessagingQos messagingQos;
    const bool isLocalMessage = true;
    DiscoveryEntryWithMetaInfo discoveryEntry;
    static const std::string fromParticipantId;
    static const std::string toParticipantId;
    static const std::string subscriptionId;
    static const std::string replyToChannelId;
    static const std::string DUMMY_USERID;
    static const std::string TEST_DOMAIN;
    static const std::string TEST_INTERFACE;
    static const std::string TEST_OPERATION;
    static const std::string TEST_PUBLICKEYID;
private:
    DISALLOW_COPY_AND_ASSIGN(AccessControllerTest);
};

//----- Constants --------------------------------------------------------------
const std::string AccessControllerTest::fromParticipantId("sender");
const std::string AccessControllerTest::toParticipantId("receiver");
const std::string AccessControllerTest::subscriptionId("testSubscriptionId");
const std::string AccessControllerTest::replyToChannelId("replyToId");
const std::string AccessControllerTest::DUMMY_USERID("testUserId");
const std::string AccessControllerTest::TEST_DOMAIN("testDomain");
const std::string AccessControllerTest::TEST_INTERFACE("testInterface");
const std::string AccessControllerTest::TEST_OPERATION("testOperation");
const std::string AccessControllerTest::TEST_PUBLICKEYID("publicKeyId");

//----- Tests ------------------------------------------------------------------

TEST_F(AccessControllerTest, accessWithInterfaceLevelAccessControl) {
    prepareConsumerTest();
    ConsumerPermissionCallbackMaker makeCallback(Permission::YES);
    EXPECT_CALL(
            *localDomainAccessControllerMock,
            getConsumerPermission(DUMMY_USERID, TEST_DOMAIN, TEST_INTERFACE, TrustLevel::HIGH, _)
    )
            .Times(1)
            .WillOnce(Invoke(&makeCallback, &ConsumerPermissionCallbackMaker::consumerPermission));

    EXPECT_CALL(*accessControllerCallback, hasConsumerPermission(true))
            .Times(1);

    accessController.hasConsumerPermission(
            getImmutableMessage(),
            std::dynamic_pointer_cast<IAccessController::IHasConsumerPermissionCallback>(accessControllerCallback)
    );
}

TEST_F(AccessControllerTest, accessWithOperationLevelAccessControl) {
    prepareConsumerTest();
    ConsumerPermissionCallbackMaker makeCallback(Permission::YES);
    EXPECT_CALL(
            *localDomainAccessControllerMock,
            getConsumerPermission(DUMMY_USERID, TEST_DOMAIN, TEST_INTERFACE, TrustLevel::HIGH, _)
    )
            .Times(1)
            .WillOnce(Invoke(&makeCallback, &ConsumerPermissionCallbackMaker::operationNeeded));

    Permission::Enum permissionYes = Permission::YES;
    DefaultValue<Permission::Enum>::Set(permissionYes);
    EXPECT_CALL(
            *localDomainAccessControllerMock,
            getConsumerPermission(
                    DUMMY_USERID,
                    TEST_DOMAIN,
                    TEST_INTERFACE,
                    TEST_OPERATION,
                    TrustLevel::HIGH
            )
    )
            .WillOnce(Return(permissionYes));

    EXPECT_CALL(*accessControllerCallback, hasConsumerPermission(true))
            .Times(1);

    accessController.hasConsumerPermission(
            getImmutableMessage(),
            std::dynamic_pointer_cast<IAccessController::IHasConsumerPermissionCallback>(accessControllerCallback)
    );
}

TEST_F(AccessControllerTest, accessWithOperationLevelAccessControlAndFaultyMessage) {
    prepareConsumerTest();
    ConsumerPermissionCallbackMaker makeCallback(Permission::YES);
    EXPECT_CALL(
            *localDomainAccessControllerMock,
            getConsumerPermission(DUMMY_USERID, TEST_DOMAIN, TEST_INTERFACE, TrustLevel::HIGH, _)
    )
            .Times(1)
            .WillOnce(Invoke(&makeCallback, &ConsumerPermissionCallbackMaker::operationNeeded));

    EXPECT_CALL(*accessControllerCallback, hasConsumerPermission(false))
            .Times(1);

    std::string payload("invalid serialization of Request object");
    mutableMessage.setPayload(payload);

    accessController.hasConsumerPermission(
            getImmutableMessage(),
            std::dynamic_pointer_cast<IAccessController::IHasConsumerPermissionCallback>(accessControllerCallback)
    );
}

TEST_F(AccessControllerTest, hasProviderPermission) {
    Permission::Enum permissionYes = Permission::YES;
    DefaultValue<Permission::Enum>::Set(permissionYes);
    EXPECT_CALL(
            *localDomainAccessControllerMock,
            getProviderPermission(_, _, _, _)
    )
            .Times(1)
            .WillOnce(Return(permissionYes));
    bool retval = accessController.hasProviderPermission(DUMMY_USERID, TrustLevel::HIGH, TEST_DOMAIN, TEST_INTERFACE);
    EXPECT_TRUE(retval);
}

TEST_F(AccessControllerTest, hasNoProviderPermission) {
    Permission::Enum permissionNo = Permission::NO;
    DefaultValue<Permission::Enum>::Set(permissionNo);
    EXPECT_CALL(
            *localDomainAccessControllerMock,
            getProviderPermission(_, _, _, _)
    )
            .Times(1)
            .WillOnce(Return(permissionNo));
    bool retval = accessController.hasProviderPermission(DUMMY_USERID, TrustLevel::HIGH, TEST_DOMAIN, TEST_INTERFACE);
    EXPECT_FALSE(retval);
}

//----- Test Types --------------------------------------------------------------
typedef ::testing::Types<
        SubscriptionRequest,
        MulticastSubscriptionRequest,
        BroadcastSubscriptionRequest
> SubscriptionTypes;

template <typename T>
class AccessControllerSubscriptionTest : public AccessControllerTest {
public:

    template<typename U = T>
    typename std::enable_if_t<std::is_same<U, SubscriptionRequest>::value>
    createMutableMessage()
    {
        SubscriptionRequest subscriptionRequest;
        subscriptionRequest.setSubscriptionId(subscriptionId);
        mutableMessage = messageFactory.createSubscriptionRequest(fromParticipantId,
                                                                  toParticipantId,
                                                                  messagingQos,
                                                                  subscriptionRequest,
                                                                  isLocalMessage);
    }

    template<typename U = T>
    typename std::enable_if_t<std::is_same<U, MulticastSubscriptionRequest>::value>
    createMutableMessage()
    {
        MulticastSubscriptionRequest subscriptionRequest;
        mutableMessage = messageFactory.createMulticastSubscriptionRequest(fromParticipantId,
                                                                  toParticipantId,
                                                                  messagingQos,
                                                                  subscriptionRequest,
                                                                  isLocalMessage);
    }

    template<typename U = T>
    typename std::enable_if_t<std::is_same<U, BroadcastSubscriptionRequest>::value>
    createMutableMessage()
    {
        BroadcastSubscriptionRequest subscriptionRequest;
        mutableMessage = messageFactory.createBroadcastSubscriptionRequest(fromParticipantId,
                                                                  toParticipantId,
                                                                  messagingQos,
                                                                  subscriptionRequest,
                                                                  isLocalMessage);
    }
};

TYPED_TEST_CASE(AccessControllerSubscriptionTest, SubscriptionTypes);

TYPED_TEST(AccessControllerSubscriptionTest, hasNoConsumerPermission) {
    const Permission::Enum permissionNo = Permission::NO;
    const bool expectedPermissionFalse = false;

    this->createMutableMessage();

    std::shared_ptr<ImmutableMessage> immutableMessage = this->mutableMessage.getImmutableMessage();

    this->testPermission(permissionNo, expectedPermissionFalse);
}

TYPED_TEST(AccessControllerSubscriptionTest, hasConsumerPermission) {
    const Permission::Enum permissionNo = Permission::YES;
    const bool expectedPermissionFalse = true;

    this->createMutableMessage();

    std::shared_ptr<ImmutableMessage> immutableMessage = this->mutableMessage.getImmutableMessage();

    this->testPermission(permissionNo, expectedPermissionFalse);
}
