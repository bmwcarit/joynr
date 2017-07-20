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

#include <chrono>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "libjoynrclustercontroller/access-control/LocalDomainAccessController.h"
#include "libjoynrclustercontroller/access-control/LocalDomainAccessStore.h"

#include "joynr/Semaphore.h"
#include "joynr/PrivateCopyAssign.h"

#include "tests/JoynrTest.h"
#include "tests/utils/MockObjects.h"

using namespace ::testing;
using namespace joynr;
using namespace joynr::infrastructure;
using namespace joynr::infrastructure::DacTypes;

// Consumer permissions are obtained asynchronously
class PermissionCallback : public LocalDomainAccessController::IGetPermissionCallback
{
public:
    PermissionCallback() :
        isValid(false),
        storedPermission(Permission::YES),
        sem(0)
    {}

    ~PermissionCallback() = default;

    void permission(Permission::Enum permission) {
        this->storedPermission = permission;
        isValid = true;
        sem.notify();
    }

    void operationNeeded() {
        sem.notify(); // isValid stays false
    }

    bool isPermissionAvailable() const {
        return isValid;
    }

    Permission::Enum getPermission() const {
        return storedPermission;
    }

    // Returns true if the callback was made
    bool expectCallback(int millisecs) {
        return sem.waitFor(std::chrono::milliseconds(millisecs));
    }

private:
    bool isValid;
    Permission::Enum storedPermission;
    Semaphore sem;
};

// Test class
class LocalDomainAccessControllerTest : public testing::TestWithParam<bool> {
public:
    LocalDomainAccessControllerTest() : localDomainAccessStorePtr(nullptr),
                                        localDomainAccessController(),
                                        mockGdacProxyMock(nullptr),
                                        mockGdrcProxy()
    {
    }

    ~LocalDomainAccessControllerTest() override {
        // Delete test specific files
        joynr::test::util::removeFileInCurrentDirectory(".*\\.settings");
        joynr::test::util::removeFileInCurrentDirectory(".*\\.persist");
    }

    void SetUp() override {
        std::unique_ptr<LocalDomainAccessStore> localDomainAccessStore;
        if(GetParam()) {
            // copy access entry file to bin folder for the test so that runtimes will find and load the file
            joynr::test::util::copyTestResourceToCurrentDirectory("AccessStoreTest.persist");

            localDomainAccessStore = std::make_unique<LocalDomainAccessStore>("AccessStoreTest.persist");
        } else {
            localDomainAccessStore = std::make_unique<LocalDomainAccessStore>();
        }
        localDomainAccessStorePtr = localDomainAccessStore.get();
        const bool useLocalDomainAccessStoreOnly = false;
        localDomainAccessController = std::make_unique<LocalDomainAccessController>(std::move(localDomainAccessStore), useLocalDomainAccessStoreOnly);

        auto mockGdacProxy = std::make_unique<MockGlobalDomainAccessControllerProxy>();
        mockGdacProxyMock = mockGdacProxy.get();
        localDomainAccessController->setGlobalDomainAccessControllerProxy(std::move(mockGdacProxy));

        mockGdrcProxy = std::make_shared<MockGlobalDomainRoleControllerProxy>();
        localDomainAccessController->setGlobalDomainRoleControllerProxy(mockGdrcProxy);

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
        ownerRce = OwnerRegistrationControlEntry(
                TEST_USER,       // uid
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
    static const std::string joynrDomain;

protected:
    LocalDomainAccessStore* localDomainAccessStorePtr;
    std::unique_ptr<LocalDomainAccessController> localDomainAccessController;

    MockGlobalDomainAccessControllerProxy* mockGdacProxyMock;
    std::shared_ptr<MockGlobalDomainRoleControllerProxy> mockGdrcProxy;
    OwnerAccessControlEntry ownerAce;
    MasterAccessControlEntry masterAce;
    OwnerRegistrationControlEntry ownerRce;
    MasterRegistrationControlEntry masterRce;
    DomainRoleEntry userDre;
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
const std::vector<Permission::Enum> LocalDomainAccessControllerTest::PERMISSIONS = {
        Permission::NO, Permission::ASK, Permission::YES
};
const std::vector<TrustLevel::Enum> LocalDomainAccessControllerTest::TRUST_LEVELS = {
        TrustLevel::LOW, TrustLevel::MID, TrustLevel::HIGH
};
const std::string LocalDomainAccessControllerTest::joynrDomain("LocalDomainAccessControllerTest.Domain.A");

//----- Tests ------------------------------------------------------------------

TEST_P(LocalDomainAccessControllerTest, testHasRole) {
    localDomainAccessStorePtr->updateDomainRole(userDre);

    std::string defaultString;
    DefaultValue<std::string>::Set(defaultString);

    EXPECT_TRUE(localDomainAccessController->hasRole(LocalDomainAccessControllerTest::TEST_USER,
                                                     LocalDomainAccessControllerTest::TEST_DOMAIN1,
                                                     Role::OWNER));
}

TEST_P(LocalDomainAccessControllerTest, consumerPermission) {
    localDomainAccessStorePtr->updateOwnerAccessControlEntry(ownerAce);

    std::string defaultString;
    DefaultValue<std::string>::Set(defaultString);
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

TEST_P(LocalDomainAccessControllerTest, consumerPermissionInvalidOwnerAce) {
    localDomainAccessStorePtr->updateOwnerAccessControlEntry(ownerAce);

    // Update the MasterACE so that it does not permit Permission::YES
    std::vector<Permission::Enum> possiblePermissions = {
            Permission::NO, Permission::ASK
    };
    masterAce.setDefaultConsumerPermission(Permission::ASK);
    masterAce.setPossibleConsumerPermissions(possiblePermissions);
    localDomainAccessStorePtr->updateMasterAccessControlEntry(masterAce);

    std::string defaultString;
    DefaultValue<std::string>::Set(defaultString);
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

TEST_P(LocalDomainAccessControllerTest, consumerPermissionOwnerAceOverrulesMaster) {
    ownerAce.setRequiredTrustLevel(TrustLevel::MID);
    ownerAce.setConsumerPermission(Permission::ASK);
    localDomainAccessStorePtr->updateOwnerAccessControlEntry(ownerAce);
    localDomainAccessStorePtr->updateMasterAccessControlEntry(masterAce);

    std::string defaultString;
    DefaultValue<std::string>::Set(defaultString);
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

TEST_P(LocalDomainAccessControllerTest, consumerPermissionOperationWildcard) {
    ownerAce.setOperation(access_control::WILDCARD);
    localDomainAccessStorePtr->updateOwnerAccessControlEntry(ownerAce);

    std::string defaultString;
    DefaultValue<std::string>::Set(defaultString);
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

TEST_P(LocalDomainAccessControllerTest, consumerPermissionAmbigious) {
    // Setup the master with a wildcard operation
    masterAce.setOperation(access_control::WILDCARD);
    std::vector<MasterAccessControlEntry> masterAcesFromGlobalDac;
    masterAcesFromGlobalDac.push_back(masterAce);
    std::vector<OwnerAccessControlEntry> ownerAcesFromGlobalDac;
    ownerAcesFromGlobalDac.push_back(ownerAce);

    // Setup the mock GDAC proxy
    EXPECT_CALL(*mockGdrcProxy, getDomainRolesAsync(_,_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<1>(std::vector<DomainRoleEntry>()),
                    Return(std::shared_ptr<Future<std::vector<DomainRoleEntry>>>()) // nullptr pointer
            ));
    EXPECT_CALL(*mockGdacProxyMock, getMasterAccessControlEntriesAsync(_,_,_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<2>(masterAcesFromGlobalDac),
                    Return(std::shared_ptr<Future<std::vector<MasterAccessControlEntry>>>()) // nullptr pointer
            ));
    EXPECT_CALL(*mockGdacProxyMock, getMediatorAccessControlEntriesAsync(_,_,_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<2>(std::vector<MasterAccessControlEntry>()),
                    Return(std::shared_ptr<Future<std::vector<MasterAccessControlEntry>>>()) // nullptr pointer
            ));
    EXPECT_CALL(*mockGdacProxyMock, getOwnerAccessControlEntriesAsync(_,_,_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<2>(ownerAcesFromGlobalDac),
                    Return(std::shared_ptr<Future<std::vector<OwnerAccessControlEntry>>>()) // nullptr pointer
            ));

    // Set default return value for Google mock
    std::string defaultString;
    DefaultValue<std::string>::Set(defaultString);

    // Get the consumer permission (async)
    auto getConsumerPermissionCallback = std::make_shared<PermissionCallback>();

    localDomainAccessController->getConsumerPermission(
            LocalDomainAccessControllerTest::TEST_USER,
            LocalDomainAccessControllerTest::TEST_DOMAIN1,
            LocalDomainAccessControllerTest::TEST_INTERFACE1,
            TrustLevel::HIGH,
            getConsumerPermissionCallback
    );

    EXPECT_TRUE(getConsumerPermissionCallback->expectCallback(1000));

    // The operation is ambigious and interface level permission is not available
    EXPECT_FALSE(getConsumerPermissionCallback->isPermissionAvailable());

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

TEST_P(LocalDomainAccessControllerTest, consumerPermissionCommunicationFailure) {
    // Setup the master with a wildcard operation
    masterAce.setOperation(access_control::WILDCARD);
    std::vector<MasterAccessControlEntry> masterAcesFromGlobalDac;
    masterAcesFromGlobalDac.push_back(masterAce);
    std::vector<OwnerAccessControlEntry> ownerAcesFromGlobalDac;
    ownerAcesFromGlobalDac.push_back(ownerAce);

    // Setup the mock GDAC proxy
    EXPECT_CALL(*mockGdrcProxy, getDomainRolesAsync(_,_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<1>(std::vector<DomainRoleEntry>()),
                    Return(std::shared_ptr<Future<std::vector<DomainRoleEntry>>>()) // nullptr pointer
            ));
    EXPECT_CALL(*mockGdacProxyMock, getMasterAccessControlEntriesAsync(_,_,_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<2>(masterAcesFromGlobalDac),
                    Return(std::shared_ptr<Future<std::vector<MasterAccessControlEntry>>>()) // nullptr pointer
            ));
    EXPECT_CALL(*mockGdacProxyMock, getMediatorAccessControlEntriesAsync(_,_,_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<3>(exceptions::JoynrRuntimeException("simulated communication failure")),
                    Return(std::shared_ptr<Future<std::vector<MasterAccessControlEntry>>>()) // nullptr pointer
            ));
    EXPECT_CALL(*mockGdacProxyMock, getOwnerAccessControlEntriesAsync(_,_,_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<2>(ownerAcesFromGlobalDac),
                    Return(std::shared_ptr<Future<std::vector<OwnerAccessControlEntry>>>()) // nullptr pointer
            ));

    // Set default return value for Google mock
    std::string defaultString;
    DefaultValue<std::string>::Set(defaultString);

    // Get the consumer permission (async)
    auto getConsumerPermissionCallback = std::make_shared<PermissionCallback>();

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

TEST_P(LocalDomainAccessControllerTest, consumerPermissionQueuedRequests) {
    // Setup the master with a wildcard operation
    masterAce.setOperation(access_control::WILDCARD);
    std::vector<MasterAccessControlEntry> masterAcesFromGlobalDac;
    masterAcesFromGlobalDac.push_back(masterAce);
    ownerAce.setOperation(access_control::WILDCARD);
    std::vector<OwnerAccessControlEntry> ownerAcesFromGlobalDac;
    ownerAcesFromGlobalDac.push_back(ownerAce);

    std::function<void(const std::vector<MasterAccessControlEntry>& masterAces)> getMasterAcesOnSuccessFct = [](auto){};

    // Setup the mock GDAC proxy
    EXPECT_CALL(*mockGdrcProxy, getDomainRolesAsync(_,_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<1>(std::vector<DomainRoleEntry>()),
                    Return(std::shared_ptr<Future<std::vector<DomainRoleEntry>>>()) // nullptr pointer
            ));
    EXPECT_CALL(*mockGdacProxyMock, getMasterAccessControlEntriesAsync(_,_,_,_))
            .Times(1)
            .WillOnce(DoAll(
                    SaveArg<2>(&getMasterAcesOnSuccessFct),
                    Return(std::shared_ptr<Future<std::vector<MasterAccessControlEntry>>>()) // nullptr pointer
            ));
    EXPECT_CALL(*mockGdacProxyMock, getMediatorAccessControlEntriesAsync(_,_,_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<2>(std::vector<MasterAccessControlEntry>()),
                    Return(std::shared_ptr<Future<std::vector<MasterAccessControlEntry>>>()) // nullptr pointer
            ));
    EXPECT_CALL(*mockGdacProxyMock, getOwnerAccessControlEntriesAsync(_,_,_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<2>(ownerAcesFromGlobalDac),
                    Return(std::shared_ptr<Future<std::vector<OwnerAccessControlEntry>>>()) // nullptr pointer
            ));

    // Expect a call to subscribe for the not present Entry
    EXPECT_CALL(*mockGdacProxyMock, subscribeToMasterAccessControlEntryChangedBroadcast(_,_,_)).    Times(1);
    EXPECT_CALL(*mockGdacProxyMock, subscribeToMediatorAccessControlEntryChangedBroadcast(_,_,_)).  Times(1);
    EXPECT_CALL(*mockGdacProxyMock, subscribeToOwnerAccessControlEntryChangedBroadcast(_,_,_)).     Times(1);

    // Set default return value for Google mock
    std::string defaultString;
    DefaultValue<std::string>::Set(defaultString);

    // Get the consumer permission (async)
    auto getConsumerPermissionCallback1 = std::make_shared<PermissionCallback>();

    localDomainAccessController->getConsumerPermission(
            LocalDomainAccessControllerTest::TEST_USER,
            LocalDomainAccessControllerTest::TEST_DOMAIN1,
            LocalDomainAccessControllerTest::TEST_INTERFACE1,
            TrustLevel::HIGH,
            getConsumerPermissionCallback1
    );

    // Make another request for consumer permission
    auto getConsumerPermissionCallback2 = std::make_shared<PermissionCallback>();

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
    getMasterAcesOnSuccessFct(masterAcesFromGlobalDac);

    EXPECT_TRUE(getConsumerPermissionCallback1->isPermissionAvailable());
    EXPECT_TRUE(getConsumerPermissionCallback2->isPermissionAvailable());
    EXPECT_EQ(Permission::YES, getConsumerPermissionCallback1->getPermission());
    EXPECT_EQ(Permission::YES, getConsumerPermissionCallback2->getPermission());

}

// true: with persist file
// false: without
INSTANTIATE_TEST_CASE_P(WithOrWithoutPersistFile,
    LocalDomainAccessControllerTest,
    Bool()
);

TEST(LocalDomainAccessControllerPersistedTest, persistedAcesAreUsed) {
    auto mockGdacProxyPtr = std::make_unique<MockGlobalDomainAccessControllerProxy>();
    auto mockGdacProxy = mockGdacProxyPtr.get();

    // Do not contact GDAC (do not perform any get* operation) for persisted ACEs
    EXPECT_CALL(*mockGdacProxy, getMasterAccessControlEntriesAsync(_,_,_,_))    .Times(0);
    EXPECT_CALL(*mockGdacProxy, getMediatorAccessControlEntriesAsync(_,_,_,_))  .Times(0);
    EXPECT_CALL(*mockGdacProxy, getOwnerAccessControlEntriesAsync(_,_,_,_))     .Times(0);

    // Expect only calls to subscribeTo methods
    EXPECT_CALL(*mockGdacProxy, subscribeToMasterAccessControlEntryChangedBroadcast(_,_,_)).    Times(1);
    EXPECT_CALL(*mockGdacProxy, subscribeToMediatorAccessControlEntryChangedBroadcast(_,_,_)).  Times(1);
    EXPECT_CALL(*mockGdacProxy, subscribeToOwnerAccessControlEntryChangedBroadcast(_,_,_)).     Times(1);

    // Copy access entry file to bin folder for the test so that runtimes will find and load the file
    joynr::test::util::copyTestResourceToCurrentDirectory("AccessStoreTest.persist");

    // Load persisted ACEs
    const bool useLocalDomainAccessStoreOnly = false;
    auto localDomainAccessStore = std::make_unique<LocalDomainAccessStore>("AccessStoreTest.persist");
    auto localDomainAccessController =
            std::make_unique<LocalDomainAccessController>(std::move(localDomainAccessStore), useLocalDomainAccessStoreOnly);

    localDomainAccessController->setGlobalDomainAccessControllerProxy(std::move(mockGdacProxyPtr));

    // Set default return value for Google mock
    std::string defaultString;
    DefaultValue<std::string>::Set(defaultString);

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

// Registration control entries

TEST_P(LocalDomainAccessControllerTest, providerPermission) {
    localDomainAccessStorePtr->updateOwnerRegistrationControlEntry(ownerRce);

    std::string defaultString;
    DefaultValue<std::string>::Set(defaultString);
    EXPECT_EQ(
            Permission::YES,
            localDomainAccessController->getProviderPermission(
                    LocalDomainAccessControllerTest::TEST_USER,
                    LocalDomainAccessControllerTest::TEST_DOMAIN1,
                    LocalDomainAccessControllerTest::TEST_INTERFACE1,
                    TrustLevel::HIGH
            )
    );
}

TEST_P(LocalDomainAccessControllerTest, providerPermissionInvalidOwnerRce) {
    localDomainAccessStorePtr->updateOwnerRegistrationControlEntry(ownerRce);

    // Update the MasterACE so that it does not permit Permission::YES
    std::vector<Permission::Enum> possiblePermissions = {
            Permission::NO, Permission::ASK
    };
    masterRce.setDefaultProviderPermission(Permission::ASK);
    masterRce.setPossibleProviderPermissions(possiblePermissions);
    localDomainAccessStorePtr->updateMasterRegistrationControlEntry(masterRce);

    std::string defaultString;
    DefaultValue<std::string>::Set(defaultString);
    EXPECT_EQ(
            Permission::NO,
            localDomainAccessController->getProviderPermission(
                    LocalDomainAccessControllerTest::TEST_USER,
                    LocalDomainAccessControllerTest::TEST_DOMAIN1,
                    LocalDomainAccessControllerTest::TEST_INTERFACE1,
                    TrustLevel::HIGH
            )
    );
}

TEST_P(LocalDomainAccessControllerTest, providerPermissionOwnerRceOverrulesMaster) {
    ownerRce.setRequiredTrustLevel(TrustLevel::MID);
    ownerRce.setProviderPermission(Permission::ASK);
    localDomainAccessStorePtr->updateOwnerRegistrationControlEntry(ownerRce);
    localDomainAccessStorePtr->updateMasterRegistrationControlEntry(masterRce);

    std::string defaultString;
    DefaultValue<std::string>::Set(defaultString);
    EXPECT_EQ(
            Permission::ASK,
            localDomainAccessController->getProviderPermission(
                    LocalDomainAccessControllerTest::TEST_USER,
                    LocalDomainAccessControllerTest::TEST_DOMAIN1,
                    LocalDomainAccessControllerTest::TEST_INTERFACE1,
                    TrustLevel::HIGH
            )
    );
    EXPECT_EQ(
            Permission::NO,
            localDomainAccessController->getProviderPermission(
                    LocalDomainAccessControllerTest::TEST_USER,
                    LocalDomainAccessControllerTest::TEST_DOMAIN1,
                    LocalDomainAccessControllerTest::TEST_INTERFACE1,
                    TrustLevel::LOW
            )
    );
}

TEST_P(LocalDomainAccessControllerTest, DISABLED_providerPermissionAmbigious) {
    // Setup the master with a wildcard operation
    masterAce.setOperation(access_control::WILDCARD);
    std::vector<MasterRegistrationControlEntry> masterRcesFromGlobalDac;
    masterRcesFromGlobalDac.push_back(masterRce);
    std::vector<OwnerRegistrationControlEntry> ownerRcesFromGlobalDac;
    ownerRcesFromGlobalDac.push_back(ownerRce);

    // Setup the mock GDAC proxy
    EXPECT_CALL(*mockGdrcProxy, getDomainRolesAsync(_,_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<1>(std::vector<DomainRoleEntry>()),
                    Return(std::shared_ptr<Future<std::vector<DomainRoleEntry>>>()) // nullptr pointer
            ));
    EXPECT_CALL(*mockGdacProxyMock, getMasterRegistrationControlEntriesAsync(_,_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<1>(masterRcesFromGlobalDac),
                    Return(std::shared_ptr<Future<std::vector<MasterRegistrationControlEntry>>>()) // nullptr pointer
            ));
    EXPECT_CALL(*mockGdacProxyMock, getMediatorRegistrationControlEntriesAsync(_,_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<1>(std::vector<MasterRegistrationControlEntry>()),
                    Return(std::shared_ptr<Future<std::vector<MasterRegistrationControlEntry>>>()) // nullptr pointer
            ));
    EXPECT_CALL(*mockGdacProxyMock, getOwnerRegistrationControlEntriesAsync(_,_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<1>(ownerRcesFromGlobalDac),
                    Return(std::shared_ptr<Future<std::vector<OwnerRegistrationControlEntry>>>()) // nullptr pointer
            ));

    // Set default return value for Google mock
    std::string defaultString;
    DefaultValue<std::string>::Set(defaultString);

    // Get the provider permission (async)
    auto getProviderPermissionCallback = std::make_shared<PermissionCallback>();

    localDomainAccessController->getProviderPermission(
            LocalDomainAccessControllerTest::TEST_USER,
            LocalDomainAccessControllerTest::TEST_DOMAIN1,
            LocalDomainAccessControllerTest::TEST_INTERFACE1,
            TrustLevel::HIGH,
            getProviderPermissionCallback
    );

    EXPECT_TRUE(getProviderPermissionCallback->expectCallback(1000));

    // The operation is ambigious and interface level permission is not available
    EXPECT_FALSE(getProviderPermissionCallback->isPermissionAvailable());

    // Operation level permission should work
    EXPECT_EQ(
            Permission::YES,
            localDomainAccessController->getProviderPermission(
                    LocalDomainAccessControllerTest::TEST_USER,
                    LocalDomainAccessControllerTest::TEST_DOMAIN1,
                    LocalDomainAccessControllerTest::TEST_INTERFACE1,
                    TrustLevel::HIGH
            )
    );
}

TEST_P(LocalDomainAccessControllerTest, providerPermissionCommunicationFailure) {
    // Setup the master with a wildcard operation
    masterAce.setOperation(access_control::WILDCARD);
    std::vector<MasterRegistrationControlEntry> masterRcesFromGlobalDac;
    masterRcesFromGlobalDac.push_back(masterRce);
    std::vector<OwnerRegistrationControlEntry> ownerRcesFromGlobalDac;
    ownerRcesFromGlobalDac.push_back(ownerRce);

    // Setup the mock GDAC proxy
    EXPECT_CALL(*mockGdrcProxy, getDomainRolesAsync(_,_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<1>(std::vector<DomainRoleEntry>()),
                    Return(std::shared_ptr<Future<std::vector<DomainRoleEntry>>>()) // nullptr pointer
            ));
    EXPECT_CALL(*mockGdacProxyMock, getMasterRegistrationControlEntriesAsync(_,_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<1>(masterRcesFromGlobalDac),
                    Return(std::shared_ptr<Future<std::vector<MasterRegistrationControlEntry>>>()) // nullptr pointer
            ));
    EXPECT_CALL(*mockGdacProxyMock, getMediatorRegistrationControlEntriesAsync(_,_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<2>(exceptions::JoynrRuntimeException("simulated communication failure")),
                    Return(std::shared_ptr<Future<std::vector<MasterRegistrationControlEntry>>>()) // nullptr pointer
            ));
    EXPECT_CALL(*mockGdacProxyMock, getOwnerRegistrationControlEntriesAsync(_,_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<1>(ownerRcesFromGlobalDac),
                    Return(std::shared_ptr<Future<std::vector<OwnerRegistrationControlEntry>>>()) // nullptr pointer
            ));

    // Set default return value for Google mock
    std::string defaultString;
    DefaultValue<std::string>::Set(defaultString);

    // Get the provider permission (async)
    auto getProviderPermissionCallback = std::make_shared<PermissionCallback>();

    localDomainAccessController->getProviderPermission(
            LocalDomainAccessControllerTest::TEST_USER,
            LocalDomainAccessControllerTest::TEST_DOMAIN1,
            LocalDomainAccessControllerTest::TEST_INTERFACE1,
            TrustLevel::HIGH,
            getProviderPermissionCallback
    );

    EXPECT_TRUE(getProviderPermissionCallback->expectCallback(1000));
    EXPECT_TRUE(getProviderPermissionCallback->isPermissionAvailable());
    EXPECT_EQ(Permission::NO, getProviderPermissionCallback->getPermission());
}

TEST(LocalDomainAccessControllerTest, onlyLdasUsed) {
    auto mockGdacProxy = std::make_unique<MockGlobalDomainAccessControllerProxy>();

    // Expect zero interactions with the backend because only the LDAS shall be used
    EXPECT_CALL(*mockGdacProxy, getMasterAccessControlEntriesAsync(_, _, _, _)).Times(0);
    EXPECT_CALL(*mockGdacProxy, getMediatorAccessControlEntriesAsync(_, _, _, _)).Times(0);
    EXPECT_CALL(*mockGdacProxy, getOwnerAccessControlEntriesAsync(_, _, _, _)).Times(0);

    EXPECT_CALL(*mockGdacProxy, subscribeToMasterAccessControlEntryChangedBroadcast(_, _, _)).Times(0);
    EXPECT_CALL(*mockGdacProxy, subscribeToMediatorAccessControlEntryChangedBroadcast(_, _, _)).Times(0);
    EXPECT_CALL(*mockGdacProxy, subscribeToOwnerAccessControlEntryChangedBroadcast(_, _, _)).Times(0);

    EXPECT_CALL(*mockGdacProxy, getMasterRegistrationControlEntriesAsync(_, _, _)).Times(0);
    EXPECT_CALL(*mockGdacProxy, getMediatorRegistrationControlEntriesAsync(_, _, _)).Times(0);
    EXPECT_CALL(*mockGdacProxy, getOwnerRegistrationControlEntriesAsync(_, _, _)).Times(0);

    EXPECT_CALL(*mockGdacProxy, subscribeToMasterRegistrationControlEntryChangedBroadcast(_, _, _)).Times(0);
    EXPECT_CALL(*mockGdacProxy, subscribeToMediatorRegistrationControlEntryChangedBroadcast(_, _, _)).Times(0);
    EXPECT_CALL(*mockGdacProxy, subscribeToOwnerRegistrationControlEntryChangedBroadcast(_, _, _)).Times(0);

    const std::string user("user");
    const std::string domain("domain");
    const std::string interface("interface");

    auto masterAce = MasterAccessControlEntry(
            user, domain, interface,
            TrustLevel::LOW, { TrustLevel::LOW },
            TrustLevel::LOW, { TrustLevel::LOW },
            std::string(access_control::WILDCARD),
            Permission::YES,
            { Permission::YES, Permission::NO }
    );

    auto masterRce = MasterRegistrationControlEntry(
            user, domain, interface,
            TrustLevel::LOW, { TrustLevel::LOW },
            TrustLevel::LOW, { TrustLevel::LOW },
            Permission::YES,
            { Permission::YES, Permission::NO }
    );

    auto localDomainAccessStore = std::make_unique<LocalDomainAccessStore>();
    localDomainAccessStore->updateMasterAccessControlEntry(masterAce);
    localDomainAccessStore->updateMasterRegistrationControlEntry(masterRce);

    constexpr bool useLDASOnly = true;
    auto localDomainAccessController =
            std::make_unique<LocalDomainAccessController>(std::move(localDomainAccessStore), useLDASOnly);
    localDomainAccessController->setGlobalDomainAccessControllerProxy(std::move(mockGdacProxy));

    auto getConsumerPermissionCallback = std::make_shared<PermissionCallback>();

    localDomainAccessController->getConsumerPermission(user,
                                                       domain,
                                                       interface,
                                                       TrustLevel::LOW,
                                                       getConsumerPermissionCallback);

    EXPECT_TRUE(getConsumerPermissionCallback->isPermissionAvailable());
    EXPECT_EQ(Permission::YES, getConsumerPermissionCallback->getPermission());

    auto getProviderPermissionCallback = std::make_shared<PermissionCallback>();

    localDomainAccessController->getProviderPermission(user,
                                                       domain,
                                                       interface,
                                                       TrustLevel::LOW,
                                                       getProviderPermissionCallback);

    EXPECT_TRUE(getProviderPermissionCallback->isPermissionAvailable());
    EXPECT_EQ(Permission::YES, getProviderPermissionCallback->getPermission());
}

TEST_P(LocalDomainAccessControllerTest, providerPermissionQueuedRequests) {
    // Setup the master with a wildcard operation
    masterAce.setOperation(access_control::WILDCARD);
    std::vector<MasterRegistrationControlEntry> masterRcesFromGlobalDac;
    masterRcesFromGlobalDac.push_back(masterRce);
    std::vector<OwnerRegistrationControlEntry> ownerRcesFromGlobalDac;
    ownerRcesFromGlobalDac.push_back(ownerRce);

    std::function<void(const std::vector<MasterRegistrationControlEntry>& masterRces)> getMasterRcesOnSuccessFct = [](auto){};

    // Setup the mock GDAC proxy
    EXPECT_CALL(*mockGdrcProxy, getDomainRolesAsync(_,_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<1>(std::vector<DomainRoleEntry>()),
                    Return(std::shared_ptr<Future<std::vector<DomainRoleEntry>>>()) // nullptr pointer
            ));
    EXPECT_CALL(*mockGdacProxyMock, getMasterRegistrationControlEntriesAsync(_,_,_))
            .Times(1)
            .WillOnce(DoAll(
                    SaveArg<1>(&getMasterRcesOnSuccessFct),
                    Return(std::shared_ptr<Future<std::vector<MasterRegistrationControlEntry>>>()) // nullptr pointer
            ));
    EXPECT_CALL(*mockGdacProxyMock, getMediatorRegistrationControlEntriesAsync(_,_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<1>(std::vector<MasterRegistrationControlEntry>()),
                    Return(std::shared_ptr<Future<std::vector<MasterRegistrationControlEntry>>>()) // nullptr pointer
            ));
    EXPECT_CALL(*mockGdacProxyMock, getOwnerRegistrationControlEntriesAsync(_,_,_))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeArgument<1>(ownerRcesFromGlobalDac),
                    Return(std::shared_ptr<Future<std::vector<OwnerRegistrationControlEntry>>>()) // nullptr pointer
            ));

    // Expect a call to subscribe for the not present Entry
    EXPECT_CALL(*mockGdacProxyMock, subscribeToMasterRegistrationControlEntryChangedBroadcast(_,_,_)).    Times(1);
    EXPECT_CALL(*mockGdacProxyMock, subscribeToMediatorRegistrationControlEntryChangedBroadcast(_,_,_)).  Times(1);
    EXPECT_CALL(*mockGdacProxyMock, subscribeToOwnerRegistrationControlEntryChangedBroadcast(_,_,_)).     Times(1);

    // Set default return value for Google mock
    std::string defaultString;
    DefaultValue<std::string>::Set(defaultString);

    // Get the provider permission (async)
    auto getProviderPermissionCallback1 = std::make_shared<PermissionCallback>();

    localDomainAccessController->getProviderPermission(
            LocalDomainAccessControllerTest::TEST_USER,
            LocalDomainAccessControllerTest::TEST_DOMAIN1,
            LocalDomainAccessControllerTest::TEST_INTERFACE1,
            TrustLevel::HIGH,
            getProviderPermissionCallback1
    );

    // Make another request for provider permission
    auto getProviderPermissionCallback2 = std::make_shared<PermissionCallback>();

    localDomainAccessController->getProviderPermission(
                LocalDomainAccessControllerTest::TEST_USER,
                LocalDomainAccessControllerTest::TEST_DOMAIN1,
                LocalDomainAccessControllerTest::TEST_INTERFACE1,
                TrustLevel::HIGH,
                getProviderPermissionCallback2
    );

    EXPECT_FALSE(getProviderPermissionCallback1->expectCallback(0));
    EXPECT_FALSE(getProviderPermissionCallback2->expectCallback(0));

    // Provide the missing response to the LocalDomainAccessController
    getMasterRcesOnSuccessFct(masterRcesFromGlobalDac);

    EXPECT_TRUE(getProviderPermissionCallback1->isPermissionAvailable());
    EXPECT_TRUE(getProviderPermissionCallback2->isPermissionAvailable());
    EXPECT_EQ(Permission::YES, getProviderPermissionCallback1->getPermission());
    EXPECT_EQ(Permission::YES, getProviderPermissionCallback2->getPermission());

}

TEST(LocalDomainAccessControllerPersistedTest, persistedRcesAreUsed) {
    auto mockGdacProxyPtr = std::make_unique<MockGlobalDomainAccessControllerProxy>();
    auto mockGdacProxy = mockGdacProxyPtr.get();

    // Do not contact GDAC (do not perform any get* operation) for persisted ACEs
    EXPECT_CALL(*mockGdacProxy, getMasterRegistrationControlEntriesAsync(_,_,_))    .Times(0);
    EXPECT_CALL(*mockGdacProxy, getMediatorRegistrationControlEntriesAsync(_,_,_))  .Times(0);
    EXPECT_CALL(*mockGdacProxy, getOwnerRegistrationControlEntriesAsync(_,_,_))     .Times(0);

    // Expect only calls to subscribeTo methods
    EXPECT_CALL(*mockGdacProxy, subscribeToMasterRegistrationControlEntryChangedBroadcast(_,_,_)).    Times(1);
    EXPECT_CALL(*mockGdacProxy, subscribeToMediatorRegistrationControlEntryChangedBroadcast(_,_,_)).  Times(1);
    EXPECT_CALL(*mockGdacProxy, subscribeToOwnerRegistrationControlEntryChangedBroadcast(_,_,_)).     Times(1);

    // Copy access entry file to bin folder for the test so that runtimes will find and load the file
    joynr::test::util::copyTestResourceToCurrentDirectory("AccessStoreTest.persist");

    // Load persisted ACEs
    const bool useLocalDomainAccessStoreOnly = false;
    auto localDomainAccessStore = std::make_unique<LocalDomainAccessStore>("AccessStoreTest.persist");
    auto localDomainAccessController =
            std::make_unique<LocalDomainAccessController>(std::move(localDomainAccessStore),
                                                          useLocalDomainAccessStoreOnly);

    localDomainAccessController->setGlobalDomainAccessControllerProxy(std::move(mockGdacProxyPtr));

    // Set default return value for Google mock
    std::string defaultString;
    DefaultValue<std::string>::Set(defaultString);

    EXPECT_EQ(
            Permission::NO,
            localDomainAccessController->getProviderPermission(
                    LocalDomainAccessControllerTest::TEST_USER,
                    LocalDomainAccessControllerTest::TEST_DOMAIN1,
                    LocalDomainAccessControllerTest::TEST_INTERFACE1,
                    TrustLevel::HIGH
            )
    );
}
