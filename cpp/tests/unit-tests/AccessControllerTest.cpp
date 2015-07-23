/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

#include "joynr/PrivateCopyAssign.h"
#include "tests/utils/MockObjects.h"
#include "gtest/gtest.h"
#include "cluster-controller/access-control/AccessController.h"
#include "cluster-controller/access-control/LocalDomainAccessStore.h"
#include "joynr/types/DiscoveryEntry.h"
#include <string>

using namespace ::testing;
using namespace joynr;
using namespace joynr::types;
using namespace joynr::infrastructure;
using namespace joynr::infrastructure::DacTypes;

// Mock objects cannot make callbacks themselves but can make calls to methods
// with the same arguments as the mocked method call.
class ConsumerPermissionCallbackMaker
{
public:
    ConsumerPermissionCallbackMaker(StdPermission::Enum permission) :
        permission(permission)
    {}

    void consumerPermission(
            const std::string& userId,
            const std::string& domain,
            const std::string& interfaceName,
            StdTrustLevel::Enum trustLevel,
            QSharedPointer<LocalDomainAccessController::IGetConsumerPermissionCallback> callback
    ) {
        Q_UNUSED(userId)
        Q_UNUSED(domain)
        Q_UNUSED(interfaceName)
        Q_UNUSED(trustLevel)
        callback->consumerPermission(permission);
    }

    void operationNeeded(
            const std::string& userId,
            const std::string& domain,
            const std::string& interfaceName,
            StdTrustLevel::Enum trustLevel,
            QSharedPointer<LocalDomainAccessController::IGetConsumerPermissionCallback> callback
    ) {
        Q_UNUSED(userId)
        Q_UNUSED(domain)
        Q_UNUSED(interfaceName)
        Q_UNUSED(trustLevel)
        callback->operationNeeded();
    }

private:
    StdPermission::Enum permission;
};


class AccessControllerTest : public ::testing::Test {
public:
    AccessControllerTest() :
        localDomainAccessControllerMock(new LocalDomainAccessStore(
                        true // start with clean database
        )),
        accessControllerCallback(new MockConsumerPermissionCallback()),
        settings(),
        messagingSettingsMock(settings),
        localCapabilitiesDirectoryMock(messagingSettingsMock),
        accessController(
                localCapabilitiesDirectoryMock,
                localDomainAccessControllerMock
        )
    {

    }

    ~AccessControllerTest() {
    }

    void invokeCallbackFct (std::string participantId,
                            std::function<void(const joynr::types::StdDiscoveryEntry&)> callbackFct) {
        Q_UNUSED(participantId);
        callbackFct(discoveryEntry);
    }

    void SetUp(){
        request.setMethodName(QString::fromStdString(TEST_OPERATION));
        messagingQos = MessagingQos(5000);
        message = messageFactory.createRequest(QString::fromStdString(fromParticipantId),
                                     QString::fromStdString(toParticipantId),
                                     messagingQos,
                                     request);
        message.setHeaderCreatorUserId(QString::fromStdString(DUMMY_USERID));

        ON_CALL(
                messagingSettingsMock,
                getDiscoveryDirectoriesDomain()
        )
                .WillByDefault(Return("fooDomain"));
        ON_CALL(
                messagingSettingsMock,
                getCapabilitiesDirectoryParticipantId()
        )
                .WillByDefault(Return("fooParticipantId"));

        discoveryEntry = StdDiscoveryEntry(
                TEST_DOMAIN,
                TEST_INTERFACE,
                toParticipantId,
                types::StdProviderQos(),
                connections
        );
        EXPECT_CALL(
                localCapabilitiesDirectoryMock,
                lookup(toParticipantId, A<std::function<void(
                           const joynr::types::StdDiscoveryEntry&)>>())
        )
                .Times(1)
                .WillOnce(Invoke(this, &AccessControllerTest::invokeCallbackFct));
    }

    void TearDown(){
    }

protected:
    MockLocalDomainAccessController localDomainAccessControllerMock;
    QSharedPointer<MockConsumerPermissionCallback> accessControllerCallback;
    QSettings settings;
    MockMessagingSettings messagingSettingsMock;
    MockLocalCapabilitiesDirectory localCapabilitiesDirectoryMock;
    AccessController accessController;
    JoynrMessageFactory messageFactory;
    JoynrMessage message;
    Request request;
    MessagingQos messagingQos;
    StdDiscoveryEntry discoveryEntry;
    static const std::string fromParticipantId;
    static const std::string toParticipantId;
    static const std::string replyToChannelId;
    static const std::string DUMMY_USERID;
    static const std::string TEST_DOMAIN;
    static const std::string TEST_INTERFACE;
    static const std::string TEST_OPERATION;
    static const std::vector<StdCommunicationMiddleware::Enum> connections;
private:
    DISALLOW_COPY_AND_ASSIGN(AccessControllerTest);
};

//----- Constants --------------------------------------------------------------
const std::string AccessControllerTest::fromParticipantId("sender");
const std::string AccessControllerTest::toParticipantId("receiver");
const std::string AccessControllerTest::replyToChannelId("replyToId");
const std::string AccessControllerTest::DUMMY_USERID("testUserId");
const std::string AccessControllerTest::TEST_DOMAIN("testDomain");
const std::string AccessControllerTest::TEST_INTERFACE("testInterface");
const std::string AccessControllerTest::TEST_OPERATION("testOperation");
const std::vector<StdCommunicationMiddleware::Enum> AccessControllerTest::connections = {
        StdCommunicationMiddleware::SOME_IP
};

//----- Tests ------------------------------------------------------------------

TEST_F(AccessControllerTest, accessWithInterfaceLevelAccessControl) {
    ConsumerPermissionCallbackMaker makeCallback(StdPermission::YES);
    EXPECT_CALL(
            localDomainAccessControllerMock,
            getConsumerPermission(DUMMY_USERID, TEST_DOMAIN, TEST_INTERFACE, StdTrustLevel::HIGH, _)
    )
            .Times(1)
            .WillOnce(Invoke(&makeCallback, &ConsumerPermissionCallbackMaker::consumerPermission));

    EXPECT_CALL(*accessControllerCallback, hasConsumerPermission(true))
            .Times(1);

    accessController.hasConsumerPermission(
            message,
            accessControllerCallback.dynamicCast<IAccessController::IHasConsumerPermissionCallback>()
    );
}

TEST_F(AccessControllerTest, accessWithOperationLevelAccessControl) {
    ConsumerPermissionCallbackMaker makeCallback(StdPermission::YES);
    EXPECT_CALL(
            localDomainAccessControllerMock,
            getConsumerPermission(DUMMY_USERID, TEST_DOMAIN, TEST_INTERFACE, StdTrustLevel::HIGH, _)
    )
            .Times(1)
            .WillOnce(Invoke(&makeCallback, &ConsumerPermissionCallbackMaker::operationNeeded));

    StdPermission::Enum permissionYes = StdPermission::YES;
    DefaultValue<StdPermission::Enum>::Set(permissionYes);
    EXPECT_CALL(
            localDomainAccessControllerMock,
            getConsumerPermission(
                    DUMMY_USERID,
                    TEST_DOMAIN,
                    TEST_INTERFACE,
                    TEST_OPERATION,
                    StdTrustLevel::HIGH
            )
    )
            .WillOnce(Return(Permission::createQt(permissionYes)));

    EXPECT_CALL(*accessControllerCallback, hasConsumerPermission(true))
            .Times(1);

    accessController.hasConsumerPermission(
            message,
            accessControllerCallback.dynamicCast<IAccessController::IHasConsumerPermissionCallback>()
    );
}

TEST_F(AccessControllerTest, accessWithOperationLevelAccessControlAndFaultyMessage) {
    ConsumerPermissionCallbackMaker makeCallback(StdPermission::YES);
    EXPECT_CALL(
            localDomainAccessControllerMock,
            getConsumerPermission(DUMMY_USERID, TEST_DOMAIN, TEST_INTERFACE, StdTrustLevel::HIGH, _)
    )
            .Times(1)
            .WillOnce(Invoke(&makeCallback, &ConsumerPermissionCallbackMaker::operationNeeded));

    EXPECT_CALL(*accessControllerCallback, hasConsumerPermission(false))
            .Times(1);

    QString payload("invalid serialization of Request object");
    QByteArray buff;
    buff.append(payload);
    message.setPayload(buff);

    accessController.hasConsumerPermission(
            message,
            accessControllerCallback.dynamicCast<IAccessController::IHasConsumerPermissionCallback>()
    );
}

