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
#include "cluster-controller/access-control/LocalDomainAccessController.h"
#include "cluster-controller/access-control/LocalDomainAccessStore.h"
#include "cluster-controller/access-control/AccessControlAlgorithm.h"

#include <QSemaphore>

using namespace ::testing;
using namespace joynr;
using namespace joynr::infrastructure;

// Consumer permissions are obtained asynchronously
class ConsumerPermissionCallback : public IConsumerPermissionCallback
{
public:
    ConsumerPermissionCallback() :
        isValid(false),
        permission(Permission::YES),
        sem(0)
    {}

    ~ConsumerPermissionCallback() {}

    void consumerPermission(Permission::Enum permission) {
        this->permission = permission;
        isValid = true;
        sem.release(1);
    }

    void operationNeeded() {
        sem.release(1); // isValid stays false
    }

    bool isPermissionAvailable() const {
        return isValid;
    }

    Permission::Enum getPermission() const {
        return permission;
    }

    // Returns true if the callback was made
    bool expectCallback(int millisecs) {
        return sem.tryAcquire(1, millisecs);
    }

private:
    bool isValid;
    Permission::Enum permission;
    QSemaphore sem;
};

// Test class
class LocalDomainAccessControllerTest : public ::testing::Test {
public:
    LocalDomainAccessControllerTest()
    {
    }

    ~LocalDomainAccessControllerTest() {
    }

    void SetUp(){
        bool startWithCleanDatabase = true;
        localDomainAccessStore = new LocalDomainAccessStore(startWithCleanDatabase);
        localDomainAccessController =
                new LocalDomainAccessController(localDomainAccessStore);
        mockGdacProxy = new MockGlobalDomainAccessControllerProxy();
        QSharedPointer<GlobalDomainAccessControllerProxy> mockGDACPtr(mockGdacProxy);
        localDomainAccessController->init(mockGDACPtr);

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
        ownerAce = OwnerAccessControlEntry(
                TEST_USER,       // uid
                TEST_DOMAIN1,    // domain
                TEST_INTERFACE1, // interface name
                TrustLevel::LOW, // required trust level
                TrustLevel::LOW, // required ACE change trust level
                TEST_OPERATION1, // operation
                Permission::YES  // consumer permission
        );
    }

    void TearDown(){
        delete localDomainAccessController;
    }

protected:
    LocalDomainAccessStore* localDomainAccessStore;
    LocalDomainAccessController* localDomainAccessController;
    MockGlobalDomainAccessControllerProxy* mockGdacProxy;
    OwnerAccessControlEntry ownerAce;
    MasterAccessControlEntry masterAce;
    DomainRoleEntry userDre;
    static const QString TEST_USER;
    static const QString TEST_DOMAIN1;
    static const QString TEST_INTERFACE1;
    static const QString TEST_OPERATION1;
    static const QList<QString> DOMAINS;
    static const QList<Permission::Enum> PERMISSIONS;
    static const QList<TrustLevel::Enum> TRUST_LEVELS;
    static const QString joynrDomain;
private:
    DISALLOW_COPY_AND_ASSIGN(LocalDomainAccessControllerTest);
};

//----- Constants --------------------------------------------------------------
const QString LocalDomainAccessControllerTest::TEST_USER("testUser");
const QString LocalDomainAccessControllerTest::TEST_DOMAIN1("domain1");
const QString LocalDomainAccessControllerTest::TEST_INTERFACE1("interface1");
const QString LocalDomainAccessControllerTest::TEST_OPERATION1("operation1");
const QList<QString> LocalDomainAccessControllerTest::DOMAINS =
        QList<QString>()
        << LocalDomainAccessControllerTest::TEST_DOMAIN1;
const QList<Permission::Enum> LocalDomainAccessControllerTest::PERMISSIONS =
        QList<Permission::Enum>()
        << Permission::NO << Permission::ASK << Permission::YES;
const QList<TrustLevel::Enum> LocalDomainAccessControllerTest::TRUST_LEVELS =
        QList<TrustLevel::Enum>()
        << TrustLevel::LOW << TrustLevel::MID << TrustLevel::HIGH;
const QString LocalDomainAccessControllerTest::joynrDomain("LocalDomainAccessControllerTest.Domain.A");

//----- Tests ------------------------------------------------------------------

TEST_F(LocalDomainAccessControllerTest, testHasRole) {
    localDomainAccessStore->updateDomainRole(userDre);

    QString defaultString;
    DefaultValue<QString>::Set(defaultString);

    EXPECT_TRUE(localDomainAccessController->hasRole(LocalDomainAccessControllerTest::TEST_USER,
                                                     LocalDomainAccessControllerTest::TEST_DOMAIN1,
                                                     Role::OWNER));
}

TEST_F(LocalDomainAccessControllerTest, consumerPermission) {
    localDomainAccessStore->updateOwnerAccessControlEntry(ownerAce);

    QString defaultString;
    DefaultValue<QString>::Set(defaultString);
    EXPECT_EQ(
            Permission::YES,
            localDomainAccessController->getConsumerPermission(
                    LocalDomainAccessControllerTest::TEST_USER,
                    LocalDomainAccessControllerTest::TEST_DOMAIN1,
                    LocalDomainAccessControllerTest::TEST_INTERFACE1,
                    LocalDomainAccessControllerTest::TEST_OPERATION1,
                    TrustLevel::HIGH
            )
    );
}

TEST_F(LocalDomainAccessControllerTest, consumerPermissionInvalidOwnerAce) {
    localDomainAccessStore->updateOwnerAccessControlEntry(ownerAce);

    // Update the MasterACE so that it does not permit Permission::YES
    QList<Permission::Enum> possiblePermissions;
    possiblePermissions << Permission::NO << Permission::ASK;
    masterAce.setDefaultConsumerPermission(Permission::ASK);
    masterAce.setPossibleConsumerPermissions(possiblePermissions);
    localDomainAccessStore->updateMasterAccessControlEntry(masterAce);

    QString defaultString;
    DefaultValue<QString>::Set(defaultString);
    EXPECT_EQ(
            Permission::NO,
            localDomainAccessController->getConsumerPermission(
                    LocalDomainAccessControllerTest::TEST_USER,
                    LocalDomainAccessControllerTest::TEST_DOMAIN1,
                    LocalDomainAccessControllerTest::TEST_INTERFACE1,
                    LocalDomainAccessControllerTest::TEST_OPERATION1,
                    TrustLevel::HIGH
            )
    );
}

TEST_F(LocalDomainAccessControllerTest, consumerPermissionOwnerAceOverrulesMaster) {
    ownerAce.setRequiredTrustLevel(TrustLevel::MID);
    ownerAce.setConsumerPermission(Permission::ASK);
    localDomainAccessStore->updateOwnerAccessControlEntry(ownerAce);
    localDomainAccessStore->updateMasterAccessControlEntry(masterAce);

    QString defaultString;
    DefaultValue<QString>::Set(defaultString);
    EXPECT_EQ(
            Permission::ASK,
            localDomainAccessController->getConsumerPermission(
                    LocalDomainAccessControllerTest::TEST_USER,
                    LocalDomainAccessControllerTest::TEST_DOMAIN1,
                    LocalDomainAccessControllerTest::TEST_INTERFACE1,
                    LocalDomainAccessControllerTest::TEST_OPERATION1,
                    TrustLevel::HIGH
            )
    );
    EXPECT_EQ(
            Permission::NO,
            localDomainAccessController->getConsumerPermission(
                    LocalDomainAccessControllerTest::TEST_USER,
                    LocalDomainAccessControllerTest::TEST_DOMAIN1,
                    LocalDomainAccessControllerTest::TEST_INTERFACE1,
                    LocalDomainAccessControllerTest::TEST_OPERATION1,
                    TrustLevel::LOW
            )
    );
}

TEST_F(LocalDomainAccessControllerTest, consumerPermissionOperationWildcard) {
    ownerAce.setOperation(LocalDomainAccessStore::WILDCARD);
    localDomainAccessStore->updateOwnerAccessControlEntry(ownerAce);

    QString defaultString;
    DefaultValue<QString>::Set(defaultString);
    EXPECT_EQ(
            Permission::YES,
            localDomainAccessController->getConsumerPermission(
                    LocalDomainAccessControllerTest::TEST_USER,
                    LocalDomainAccessControllerTest::TEST_DOMAIN1,
                    LocalDomainAccessControllerTest::TEST_INTERFACE1,
                    LocalDomainAccessControllerTest::TEST_OPERATION1,
                    TrustLevel::HIGH
            )
    );
}

TEST_F(LocalDomainAccessControllerTest, consumerPermissionAmbigious) {
    // Setup the master with a wildcard operation
    masterAce.setOperation(LocalDomainAccessStore::WILDCARD);
    QList<MasterAccessControlEntry> masterAcesFromGlobalDac;
    masterAcesFromGlobalDac << masterAce;

    QList<OwnerAccessControlEntry> ownerAcesFromGlobalDac;
    ownerAcesFromGlobalDac << ownerAce;

    // Setup the mock GDAC proxy
    EXPECT_CALL(*mockGdacProxy, getDomainRoles(_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<1>(RequestStatus(RequestStatusCode::OK), QList<DomainRoleEntry>()),
                    Return(QSharedPointer<Future<QList<DomainRoleEntry>>>()) // null pointer
            ));
    EXPECT_CALL(*mockGdacProxy, getMasterAccessControlEntries(_,_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<2>(RequestStatus(RequestStatusCode::OK), masterAcesFromGlobalDac),
                    Return(QSharedPointer<Future<QList<MasterAccessControlEntry>>>()) // null pointer
            ));
    EXPECT_CALL(*mockGdacProxy, getMediatorAccessControlEntries(_,_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<2>(RequestStatus(RequestStatusCode::OK), QList<MasterAccessControlEntry>()),
                    Return(QSharedPointer<Future<QList<MasterAccessControlEntry>>>()) // null pointer
            ));
    EXPECT_CALL(*mockGdacProxy, getOwnerAccessControlEntries(_,_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<2>(RequestStatus(RequestStatusCode::OK), ownerAcesFromGlobalDac),
                    Return(QSharedPointer<Future<QList<OwnerAccessControlEntry>>>()) // null pointer
            ));

    // Set default return value for Google mock
    QString defaultString;
    DefaultValue<QString>::Set(defaultString);

    // Get the consumer permission (async)
    QSharedPointer<ConsumerPermissionCallback> getConsumerPersmissionCallback(
            new ConsumerPermissionCallback()
    );

    localDomainAccessController->getConsumerPermission(
            LocalDomainAccessControllerTest::TEST_USER,
            LocalDomainAccessControllerTest::TEST_DOMAIN1,
            LocalDomainAccessControllerTest::TEST_INTERFACE1,
            TrustLevel::HIGH,
            getConsumerPersmissionCallback
    );

    EXPECT_TRUE(getConsumerPersmissionCallback->expectCallback(1000));

    // The operation is ambigious and interface level permission is not available
    EXPECT_FALSE(getConsumerPersmissionCallback->isPermissionAvailable());

    // Operation level permission should work
    EXPECT_EQ(
            Permission::YES,
            localDomainAccessController->getConsumerPermission(
                    LocalDomainAccessControllerTest::TEST_USER,
                    LocalDomainAccessControllerTest::TEST_DOMAIN1,
                    LocalDomainAccessControllerTest::TEST_INTERFACE1,
                    LocalDomainAccessControllerTest::TEST_OPERATION1,
                    TrustLevel::HIGH
            )
    );
}


TEST_F(LocalDomainAccessControllerTest, consumerPermissionCommunicationFailure) {
    // Setup the master with a wildcard operation
    masterAce.setOperation(LocalDomainAccessStore::WILDCARD);
    QList<MasterAccessControlEntry> masterAcesFromGlobalDac;
    masterAcesFromGlobalDac << masterAce;

    QList<OwnerAccessControlEntry> ownerAcesFromGlobalDac;
    ownerAcesFromGlobalDac << ownerAce;

    RequestStatus getMediatorAceCommunicationError(RequestStatusCode::ERROR);
    getMediatorAceCommunicationError.addDescription("Simulated communication failure");

    // Setup the mock GDAC proxy
    EXPECT_CALL(*mockGdacProxy, getDomainRoles(_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<1>(RequestStatus(RequestStatusCode::OK), QList<DomainRoleEntry>()),
                    Return(QSharedPointer<Future<QList<DomainRoleEntry>>>()) // null pointer
            ));
    EXPECT_CALL(*mockGdacProxy, getMasterAccessControlEntries(_,_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<2>(RequestStatus(RequestStatusCode::OK), masterAcesFromGlobalDac),
                    Return(QSharedPointer<Future<QList<MasterAccessControlEntry>>>()) // null pointer
            ));
    EXPECT_CALL(*mockGdacProxy, getMediatorAccessControlEntries(_,_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<2>(getMediatorAceCommunicationError, QList<MasterAccessControlEntry>()),
                    Return(QSharedPointer<Future<QList<MasterAccessControlEntry>>>()) // null pointer
            ));
    EXPECT_CALL(*mockGdacProxy, getOwnerAccessControlEntries(_,_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<2>(RequestStatus(RequestStatusCode::OK), ownerAcesFromGlobalDac),
                    Return(QSharedPointer<Future<QList<OwnerAccessControlEntry>>>()) // null pointer
            ));

    // Set default return value for Google mock
    QString defaultString;
    DefaultValue<QString>::Set(defaultString);

    // Get the consumer permission (async)
    QSharedPointer<ConsumerPermissionCallback> getConsumerPermissionCallback(
            new ConsumerPermissionCallback()
    );

    localDomainAccessController->getConsumerPermission(
            LocalDomainAccessControllerTest::TEST_USER,
            LocalDomainAccessControllerTest::TEST_DOMAIN1,
            LocalDomainAccessControllerTest::TEST_INTERFACE1,
            TrustLevel::HIGH,
            getConsumerPermissionCallback
    );

    EXPECT_TRUE(getConsumerPermissionCallback->expectCallback(1000));
    EXPECT_TRUE(getConsumerPermissionCallback->isPermissionAvailable());
    EXPECT_EQ(Permission::NO, getConsumerPermissionCallback->getPermission());
}

TEST_F(LocalDomainAccessControllerTest, consumerPermissionQueuedRequests) {
    // Setup the master with a wildcard operation
    masterAce.setOperation(LocalDomainAccessStore::WILDCARD);
    QList<MasterAccessControlEntry> masterAcesFromGlobalDac;
    masterAcesFromGlobalDac << masterAce;

    ownerAce.setOperation(LocalDomainAccessStore::WILDCARD);
    QList<OwnerAccessControlEntry> ownerAcesFromGlobalDac;
    ownerAcesFromGlobalDac << ownerAce;

    std::function<void(const joynr::RequestStatus& status,
                       const QList<joynr::infrastructure::MasterAccessControlEntry>&
                               masterAces)> getMasterAcesCallbackFct;

    // Setup the mock GDAC proxy
    EXPECT_CALL(*mockGdacProxy, getDomainRoles(_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<1>(RequestStatus(RequestStatusCode::OK), QList<DomainRoleEntry>()),
                    Return(QSharedPointer<Future<QList<DomainRoleEntry>>>()) // null pointer
            ));
    EXPECT_CALL(*mockGdacProxy, getMasterAccessControlEntries(_,_,_))
            .Times(1)
            .WillOnce(DoAll(
                    SaveArg<2>(&getMasterAcesCallbackFct),
                    Return(QSharedPointer<Future<QList<MasterAccessControlEntry>>>()) // null pointer
            ));
    EXPECT_CALL(*mockGdacProxy, getMediatorAccessControlEntries(_,_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<2>(RequestStatus(RequestStatusCode::OK), QList<MasterAccessControlEntry>()),
                    Return(QSharedPointer<Future<QList<MasterAccessControlEntry>>>()) // null pointer
            ));
    EXPECT_CALL(*mockGdacProxy, getOwnerAccessControlEntries(_,_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<2>(RequestStatus(RequestStatusCode::OK), ownerAcesFromGlobalDac),
                    Return(QSharedPointer<Future<QList<OwnerAccessControlEntry>>>()) // null pointer
            ));

    // Set default return value for Google mock
    QString defaultString;
    DefaultValue<QString>::Set(defaultString);

    // Get the consumer permission (async)
    QSharedPointer<ConsumerPermissionCallback> getConsumerPermissionCallback1(
            new ConsumerPermissionCallback()
    );

    localDomainAccessController->getConsumerPermission(
            LocalDomainAccessControllerTest::TEST_USER,
            LocalDomainAccessControllerTest::TEST_DOMAIN1,
            LocalDomainAccessControllerTest::TEST_INTERFACE1,
            TrustLevel::HIGH,
            getConsumerPermissionCallback1
    );

    // Make another request for consumer permission
    QSharedPointer<ConsumerPermissionCallback> getConsumerPermissionCallback2(
                new ConsumerPermissionCallback()
    );

    localDomainAccessController->getConsumerPermission(
                LocalDomainAccessControllerTest::TEST_USER,
                LocalDomainAccessControllerTest::TEST_DOMAIN1,
                LocalDomainAccessControllerTest::TEST_INTERFACE1,
                TrustLevel::HIGH,
                getConsumerPermissionCallback2
    );

    EXPECT_FALSE(getConsumerPermissionCallback1->expectCallback(0));
    EXPECT_FALSE(getConsumerPermissionCallback2->expectCallback(0));

    // Provide the missing response to the LocalDomainAccessController
    getMasterAcesCallbackFct(RequestStatus(RequestStatusCode::OK), masterAcesFromGlobalDac);

    EXPECT_TRUE(getConsumerPermissionCallback1->isPermissionAvailable());
    EXPECT_TRUE(getConsumerPermissionCallback2->isPermissionAvailable());
    EXPECT_EQ(Permission::YES, getConsumerPermissionCallback1->getPermission());
    EXPECT_EQ(Permission::YES, getConsumerPermissionCallback2->getPermission());

}
