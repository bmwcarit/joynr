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
using namespace joynr::infrastructure::DacTypes;

// Consumer permissions are obtained asynchronously
class ConsumerPermissionCallback : public LocalDomainAccessController::IGetConsumerPermissionCallback
{
public:
    ConsumerPermissionCallback() :
        isValid(false),
        permission(StdPermission::YES),
        sem(0)
    {}

    ~ConsumerPermissionCallback() {}

    void consumerPermission(StdPermission::Enum permission) {
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

    StdPermission::Enum getPermission() const {
        return permission;
    }

    // Returns true if the callback was made
    bool expectCallback(int millisecs) {
        return sem.tryAcquire(1, millisecs);
    }

private:
    bool isValid;
    StdPermission::Enum permission;
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

        userDre = StdDomainRoleEntry(TEST_USER, DOMAINS, StdRole::OWNER);
        masterAce = StdMasterAccessControlEntry(
                TEST_USER,       // uid
                TEST_DOMAIN1,    // domain
                TEST_INTERFACE1, // interface name
                StdTrustLevel::LOW, // default required trust level
                TRUST_LEVELS,    // possible required trust levels
                StdTrustLevel::LOW, // default required control entry change trust level
                TRUST_LEVELS,    // possible required control entry change trust levels
                TEST_OPERATION1, // operation
                StdPermission::NO,  // default comsumer permission
                PERMISSIONS      // possible comsumer permissions
        );
        ownerAce = StdOwnerAccessControlEntry(
                TEST_USER,       // uid
                TEST_DOMAIN1,    // domain
                TEST_INTERFACE1, // interface name
                StdTrustLevel::LOW, // required trust level
                StdTrustLevel::LOW, // required ACE change trust level
                TEST_OPERATION1, // operation
                StdPermission::YES  // consumer permission
        );
    }

    void TearDown(){
        delete localDomainAccessController;
    }

protected:
    LocalDomainAccessStore* localDomainAccessStore;
    LocalDomainAccessController* localDomainAccessController;
    MockGlobalDomainAccessControllerProxy* mockGdacProxy;
    StdOwnerAccessControlEntry ownerAce;
    StdMasterAccessControlEntry masterAce;
    StdDomainRoleEntry userDre;
    static const std::string TEST_USER;
    static const std::string TEST_DOMAIN1;
    static const std::string TEST_INTERFACE1;
    static const std::string TEST_OPERATION1;
    static const std::vector<std::string> DOMAINS;
    static const std::vector<StdPermission::Enum> PERMISSIONS;
    static const std::vector<StdTrustLevel::Enum> TRUST_LEVELS;
    static const std::string joynrDomain;
private:
    DISALLOW_COPY_AND_ASSIGN(LocalDomainAccessControllerTest);
};

//----- Constants --------------------------------------------------------------
const std::string LocalDomainAccessControllerTest::TEST_USER("testUser");
const std::string LocalDomainAccessControllerTest::TEST_DOMAIN1("domain1");
const std::string LocalDomainAccessControllerTest::TEST_INTERFACE1("interface1");
const std::string LocalDomainAccessControllerTest::TEST_OPERATION1("operation1");
const std::vector<std::string> LocalDomainAccessControllerTest::DOMAINS = {
        LocalDomainAccessControllerTest::TEST_DOMAIN1
};
const std::vector<StdPermission::Enum> LocalDomainAccessControllerTest::PERMISSIONS = {
        StdPermission::NO, StdPermission::ASK, StdPermission::YES
};
const std::vector<StdTrustLevel::Enum> LocalDomainAccessControllerTest::TRUST_LEVELS = {
        StdTrustLevel::LOW, StdTrustLevel::MID, StdTrustLevel::HIGH
};
const std::string LocalDomainAccessControllerTest::joynrDomain("LocalDomainAccessControllerTest.Domain.A");

//----- Tests ------------------------------------------------------------------

TEST_F(LocalDomainAccessControllerTest, testHasRole) {
    localDomainAccessStore->updateDomainRole(DomainRoleEntry::createQt(userDre));

    QString defaultString;
    DefaultValue<QString>::Set(defaultString);

    EXPECT_TRUE(localDomainAccessController->hasRole(QString::fromStdString(LocalDomainAccessControllerTest::TEST_USER),
                                                     QString::fromStdString(LocalDomainAccessControllerTest::TEST_DOMAIN1),
                                                     Role::OWNER));
}

TEST_F(LocalDomainAccessControllerTest, consumerPermission) {
    localDomainAccessStore->updateOwnerAccessControlEntry(OwnerAccessControlEntry::createQt(ownerAce));

    QString defaultString;
    DefaultValue<QString>::Set(defaultString);
    EXPECT_EQ(
            StdPermission::YES,
            localDomainAccessController->getConsumerPermission(
                    LocalDomainAccessControllerTest::TEST_USER,
                    LocalDomainAccessControllerTest::TEST_DOMAIN1,
                    LocalDomainAccessControllerTest::TEST_INTERFACE1,
                    LocalDomainAccessControllerTest::TEST_OPERATION1,
                    StdTrustLevel::HIGH
            )
    );
}

TEST_F(LocalDomainAccessControllerTest, consumerPermissionInvalidOwnerAce) {
    localDomainAccessStore->updateOwnerAccessControlEntry(OwnerAccessControlEntry::createQt(ownerAce));

    // Update the MasterACE so that it does not permit StdPermission::YES
    std::vector<StdPermission::Enum> possiblePermissions = {
            StdPermission::NO, StdPermission::ASK
    };
    masterAce.setDefaultConsumerPermission(StdPermission::ASK);
    masterAce.setPossibleConsumerPermissions(possiblePermissions);
    localDomainAccessStore->updateMasterAccessControlEntry(MasterAccessControlEntry::createQt(masterAce));

    QString defaultString;
    DefaultValue<QString>::Set(defaultString);
    EXPECT_EQ(
            StdPermission::NO,
            localDomainAccessController->getConsumerPermission(
                    LocalDomainAccessControllerTest::TEST_USER,
                    LocalDomainAccessControllerTest::TEST_DOMAIN1,
                    LocalDomainAccessControllerTest::TEST_INTERFACE1,
                    LocalDomainAccessControllerTest::TEST_OPERATION1,
                    StdTrustLevel::HIGH
            )
    );
}

TEST_F(LocalDomainAccessControllerTest, consumerPermissionOwnerAceOverrulesMaster) {
    ownerAce.setRequiredTrustLevel(StdTrustLevel::MID);
    ownerAce.setConsumerPermission(StdPermission::ASK);
    localDomainAccessStore->updateOwnerAccessControlEntry(OwnerAccessControlEntry::createQt(ownerAce));
    localDomainAccessStore->updateMasterAccessControlEntry(MasterAccessControlEntry::createQt(masterAce));

    QString defaultString;
    DefaultValue<QString>::Set(defaultString);
    EXPECT_EQ(
            StdPermission::ASK,
            localDomainAccessController->getConsumerPermission(
                    LocalDomainAccessControllerTest::TEST_USER,
                    LocalDomainAccessControllerTest::TEST_DOMAIN1,
                    LocalDomainAccessControllerTest::TEST_INTERFACE1,
                    LocalDomainAccessControllerTest::TEST_OPERATION1,
                    StdTrustLevel::HIGH
            )
    );
    EXPECT_EQ(
            StdPermission::NO,
            localDomainAccessController->getConsumerPermission(
                    LocalDomainAccessControllerTest::TEST_USER,
                    LocalDomainAccessControllerTest::TEST_DOMAIN1,
                    LocalDomainAccessControllerTest::TEST_INTERFACE1,
                    LocalDomainAccessControllerTest::TEST_OPERATION1,
                    StdTrustLevel::LOW
            )
    );
}

TEST_F(LocalDomainAccessControllerTest, consumerPermissionOperationWildcard) {
    ownerAce.setOperation(LocalDomainAccessStore::WILDCARD);
    localDomainAccessStore->updateOwnerAccessControlEntry(OwnerAccessControlEntry::createQt(ownerAce));

    QString defaultString;
    DefaultValue<QString>::Set(defaultString);
    EXPECT_EQ(
            StdPermission::YES,
            localDomainAccessController->getConsumerPermission(
                    LocalDomainAccessControllerTest::TEST_USER,
                    LocalDomainAccessControllerTest::TEST_DOMAIN1,
                    LocalDomainAccessControllerTest::TEST_INTERFACE1,
                    LocalDomainAccessControllerTest::TEST_OPERATION1,
                    StdTrustLevel::HIGH
            )
    );
}

TEST_F(LocalDomainAccessControllerTest, consumerPermissionAmbigious) {
    // Setup the master with a wildcard operation
    masterAce.setOperation(LocalDomainAccessStore::WILDCARD);
    std::vector<StdMasterAccessControlEntry> masterAcesFromGlobalDac;
    masterAcesFromGlobalDac.push_back(masterAce);

    std::vector<StdOwnerAccessControlEntry> ownerAcesFromGlobalDac;
    ownerAcesFromGlobalDac.push_back(ownerAce);

    // Setup the mock GDAC proxy
    EXPECT_CALL(*mockGdacProxy, getDomainRoles(_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<1>(RequestStatus(RequestStatusCode::OK), std::vector<StdDomainRoleEntry>()),
                    Return(std::shared_ptr<Future<std::vector<StdDomainRoleEntry>>>()) // null pointer
            ));
    EXPECT_CALL(*mockGdacProxy, getMasterAccessControlEntries(_,_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<2>(RequestStatus(RequestStatusCode::OK), masterAcesFromGlobalDac),
                    Return(std::shared_ptr<Future<std::vector<StdMasterAccessControlEntry>>>()) // null pointer
            ));
    EXPECT_CALL(*mockGdacProxy, getMediatorAccessControlEntries(_,_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<2>(RequestStatus(RequestStatusCode::OK), std::vector<StdMasterAccessControlEntry>()),
                    Return(std::shared_ptr<Future<std::vector<StdMasterAccessControlEntry>>>()) // null pointer
            ));
    EXPECT_CALL(*mockGdacProxy, getOwnerAccessControlEntries(_,_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<2>(RequestStatus(RequestStatusCode::OK), ownerAcesFromGlobalDac),
                    Return(std::shared_ptr<Future<std::vector<StdOwnerAccessControlEntry>>>()) // null pointer
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
            StdTrustLevel::HIGH,
            getConsumerPersmissionCallback
    );

    EXPECT_TRUE(getConsumerPersmissionCallback->expectCallback(1000));

    // The operation is ambigious and interface level permission is not available
    EXPECT_FALSE(getConsumerPersmissionCallback->isPermissionAvailable());

    // Operation level permission should work
    EXPECT_EQ(
            StdPermission::YES,
            localDomainAccessController->getConsumerPermission(
                    LocalDomainAccessControllerTest::TEST_USER,
                    LocalDomainAccessControllerTest::TEST_DOMAIN1,
                    LocalDomainAccessControllerTest::TEST_INTERFACE1,
                    LocalDomainAccessControllerTest::TEST_OPERATION1,
                    StdTrustLevel::HIGH
            )
    );
}


TEST_F(LocalDomainAccessControllerTest, consumerPermissionCommunicationFailure) {
    // Setup the master with a wildcard operation
    masterAce.setOperation(LocalDomainAccessStore::WILDCARD);
    std::vector<StdMasterAccessControlEntry> masterAcesFromGlobalDac;
    masterAcesFromGlobalDac.push_back(masterAce);

    std::vector<StdOwnerAccessControlEntry> ownerAcesFromGlobalDac;
    ownerAcesFromGlobalDac.push_back(ownerAce);

    RequestStatus getMediatorAceCommunicationError(RequestStatusCode::ERROR);
    getMediatorAceCommunicationError.addDescription("Simulated communication failure");

    // Setup the mock GDAC proxy
    EXPECT_CALL(*mockGdacProxy, getDomainRoles(_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<1>(RequestStatus(RequestStatusCode::OK), std::vector<StdDomainRoleEntry>()),
                    Return(std::shared_ptr<Future<std::vector<StdDomainRoleEntry>>>()) // null pointer
            ));
    EXPECT_CALL(*mockGdacProxy, getMasterAccessControlEntries(_,_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<2>(RequestStatus(RequestStatusCode::OK), masterAcesFromGlobalDac),
                    Return(std::shared_ptr<Future<std::vector<StdMasterAccessControlEntry>>>()) // null pointer
            ));
    EXPECT_CALL(*mockGdacProxy, getMediatorAccessControlEntries(_,_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<2>(getMediatorAceCommunicationError, std::vector<StdMasterAccessControlEntry>()),
                    Return(std::shared_ptr<Future<std::vector<StdMasterAccessControlEntry>>>()) // null pointer
            ));
    EXPECT_CALL(*mockGdacProxy, getOwnerAccessControlEntries(_,_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<2>(RequestStatus(RequestStatusCode::OK), ownerAcesFromGlobalDac),
                    Return(std::shared_ptr<Future<std::vector<StdOwnerAccessControlEntry>>>()) // null pointer
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
            StdTrustLevel::HIGH,
            getConsumerPermissionCallback
    );

    EXPECT_TRUE(getConsumerPermissionCallback->expectCallback(1000));
    EXPECT_TRUE(getConsumerPermissionCallback->isPermissionAvailable());
    EXPECT_EQ(StdPermission::NO, getConsumerPermissionCallback->getPermission());
}

TEST_F(LocalDomainAccessControllerTest, consumerPermissionQueuedRequests) {
    // Setup the master with a wildcard operation
    masterAce.setOperation(LocalDomainAccessStore::WILDCARD);
    std::vector<StdMasterAccessControlEntry> masterAcesFromGlobalDac;
    masterAcesFromGlobalDac.push_back(masterAce);

    ownerAce.setOperation(LocalDomainAccessStore::WILDCARD);
    std::vector<StdOwnerAccessControlEntry> ownerAcesFromGlobalDac;
    ownerAcesFromGlobalDac.push_back(ownerAce);

    std::function<void(const joynr::RequestStatus& status,
                       const std::vector<StdMasterAccessControlEntry>&
                               masterAces)> getMasterAcesCallbackFct;

    // Setup the mock GDAC proxy
    EXPECT_CALL(*mockGdacProxy, getDomainRoles(_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<1>(RequestStatus(RequestStatusCode::OK), std::vector<StdDomainRoleEntry>()),
                    Return(std::shared_ptr<Future<std::vector<StdDomainRoleEntry>>>()) // null pointer
            ));
    EXPECT_CALL(*mockGdacProxy, getMasterAccessControlEntries(_,_,_))
            .Times(1)
            .WillOnce(DoAll(
                    SaveArg<2>(&getMasterAcesCallbackFct),
                    Return(std::shared_ptr<Future<std::vector<StdMasterAccessControlEntry>>>()) // null pointer
            ));
    EXPECT_CALL(*mockGdacProxy, getMediatorAccessControlEntries(_,_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<2>(RequestStatus(RequestStatusCode::OK), std::vector<StdMasterAccessControlEntry>()),
                    Return(std::shared_ptr<Future<std::vector<StdMasterAccessControlEntry>>>()) // null pointer
            ));
    EXPECT_CALL(*mockGdacProxy, getOwnerAccessControlEntries(_,_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<2>(RequestStatus(RequestStatusCode::OK), ownerAcesFromGlobalDac),
                    Return(std::shared_ptr<Future<std::vector<StdOwnerAccessControlEntry>>>()) // null pointer
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
            StdTrustLevel::HIGH,
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
                StdTrustLevel::HIGH,
                getConsumerPermissionCallback2
    );

    EXPECT_FALSE(getConsumerPermissionCallback1->expectCallback(0));
    EXPECT_FALSE(getConsumerPermissionCallback2->expectCallback(0));

    // Provide the missing response to the LocalDomainAccessController
    getMasterAcesCallbackFct(RequestStatus(RequestStatusCode::OK), masterAcesFromGlobalDac);

    EXPECT_TRUE(getConsumerPermissionCallback1->isPermissionAvailable());
    EXPECT_TRUE(getConsumerPermissionCallback2->isPermissionAvailable());
    EXPECT_EQ(StdPermission::YES, getConsumerPermissionCallback1->getPermission());
    EXPECT_EQ(StdPermission::YES, getConsumerPermissionCallback2->getPermission());

}
