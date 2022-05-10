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

#include "tests/utils/Gtest.h"
#include "tests/utils/Gmock.h"

#include "libjoynrclustercontroller/access-control/LocalDomainAccessController.h"
#include "libjoynrclustercontroller/access-control/LocalDomainAccessStore.h"

#include "joynr/Semaphore.h"
#include "joynr/Settings.h"
#include "joynr/PrivateCopyAssign.h"

#include "tests/JoynrTest.h"

using namespace ::testing;
using namespace joynr;
using namespace joynr::infrastructure;
using namespace joynr::infrastructure::DacTypes;

// Consumer permissions are obtained asynchronously
class PermissionCallback : public LocalDomainAccessController::IGetPermissionCallback
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

// Test class
class LocalDomainAccessControllerTest : public testing::TestWithParam<bool>
{
public:
    LocalDomainAccessControllerTest()
            : localDomainAccessStorePtr(nullptr)
    {
    }

    ~LocalDomainAccessControllerTest() override
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
        localDomainAccessController = std::make_unique<LocalDomainAccessController>(
                std::move(localDomainAccessStore));

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
    std::unique_ptr<LocalDomainAccessController> localDomainAccessController;

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
        LocalDomainAccessControllerTest::TEST_DOMAIN1};
const std::vector<Permission::Enum> LocalDomainAccessControllerTest::PERMISSIONS = {
        Permission::NO,
        Permission::ASK,
        Permission::YES};
const std::vector<TrustLevel::Enum> LocalDomainAccessControllerTest::TRUST_LEVELS = {
        TrustLevel::LOW,
        TrustLevel::MID,
        TrustLevel::HIGH};

//----- Tests ------------------------------------------------------------------

TEST_P(LocalDomainAccessControllerTest, testHasRole)
{
    localDomainAccessStorePtr->updateDomainRole(userDre);
    EXPECT_TRUE(localDomainAccessController->hasRole(LocalDomainAccessControllerTest::TEST_USER,
                                                     LocalDomainAccessControllerTest::TEST_DOMAIN1,
                                                     Role::OWNER));
}

TEST_P(LocalDomainAccessControllerTest, testHasRoleWithWildcard)
{
    const std::string wildcard = "*";
    userDre.setDomains({wildcard});
    localDomainAccessStorePtr->updateDomainRole(userDre);
    EXPECT_TRUE(localDomainAccessController->hasRole(LocalDomainAccessControllerTest::TEST_USER,
                                                     LocalDomainAccessControllerTest::TEST_DOMAIN1,
                                                     Role::OWNER));
}

TEST_P(LocalDomainAccessControllerTest, consumerPermission)
{
    localDomainAccessStorePtr->updateOwnerAccessControlEntry(ownerAce);
    EXPECT_EQ(Permission::YES,
              localDomainAccessController->getConsumerPermission(
                      LocalDomainAccessControllerTest::TEST_USER,
                      LocalDomainAccessControllerTest::TEST_DOMAIN1,
                      LocalDomainAccessControllerTest::TEST_INTERFACE1,
                      LocalDomainAccessControllerTest::TEST_OPERATION1,
                      TrustLevel::HIGH));
}

TEST_P(LocalDomainAccessControllerTest, consumerPermissionInvalidOwnerAce)
{
    localDomainAccessStorePtr->updateOwnerAccessControlEntry(ownerAce);

    // Update the MasterACE so that it does not permit Permission::YES
    std::vector<Permission::Enum> possiblePermissions = {Permission::NO, Permission::ASK};
    masterAce.setDefaultConsumerPermission(Permission::ASK);
    masterAce.setPossibleConsumerPermissions(possiblePermissions);
    localDomainAccessStorePtr->updateMasterAccessControlEntry(masterAce);

    EXPECT_EQ(Permission::NO,
              localDomainAccessController->getConsumerPermission(
                      LocalDomainAccessControllerTest::TEST_USER,
                      LocalDomainAccessControllerTest::TEST_DOMAIN1,
                      LocalDomainAccessControllerTest::TEST_INTERFACE1,
                      LocalDomainAccessControllerTest::TEST_OPERATION1,
                      TrustLevel::HIGH));
}

TEST_P(LocalDomainAccessControllerTest, consumerPermissionOwnerAceOverrulesMaster)
{
    ownerAce.setRequiredTrustLevel(TrustLevel::MID);
    ownerAce.setConsumerPermission(Permission::ASK);
    localDomainAccessStorePtr->updateOwnerAccessControlEntry(ownerAce);
    localDomainAccessStorePtr->updateMasterAccessControlEntry(masterAce);

    EXPECT_EQ(Permission::ASK,
              localDomainAccessController->getConsumerPermission(
                      LocalDomainAccessControllerTest::TEST_USER,
                      LocalDomainAccessControllerTest::TEST_DOMAIN1,
                      LocalDomainAccessControllerTest::TEST_INTERFACE1,
                      LocalDomainAccessControllerTest::TEST_OPERATION1,
                      TrustLevel::HIGH));
    EXPECT_EQ(Permission::NO,
              localDomainAccessController->getConsumerPermission(
                      LocalDomainAccessControllerTest::TEST_USER,
                      LocalDomainAccessControllerTest::TEST_DOMAIN1,
                      LocalDomainAccessControllerTest::TEST_INTERFACE1,
                      LocalDomainAccessControllerTest::TEST_OPERATION1,
                      TrustLevel::LOW));
}

TEST_P(LocalDomainAccessControllerTest, consumerPermissionOperationWildcard)
{
    ownerAce.setOperation(access_control::WILDCARD);
    localDomainAccessStorePtr->updateOwnerAccessControlEntry(ownerAce);

    EXPECT_EQ(Permission::YES,
              localDomainAccessController->getConsumerPermission(
                      LocalDomainAccessControllerTest::TEST_USER,
                      LocalDomainAccessControllerTest::TEST_DOMAIN1,
                      LocalDomainAccessControllerTest::TEST_INTERFACE1,
                      LocalDomainAccessControllerTest::TEST_OPERATION1,
                      TrustLevel::HIGH));
}

TEST_P(LocalDomainAccessControllerTest, consumerPermissionAmbigious)
{
    // Setup the master with a wildcard operation
    masterAce.setOperation(access_control::WILDCARD);
    localDomainAccessStorePtr->updateOwnerAccessControlEntry(ownerAce);
    localDomainAccessStorePtr->updateMasterAccessControlEntry(masterAce);

    // Get the consumer permission (async)
    auto getConsumerPermissionCallback = std::make_shared<PermissionCallback>();

    localDomainAccessController->getConsumerPermission(
            LocalDomainAccessControllerTest::TEST_USER,
            LocalDomainAccessControllerTest::TEST_DOMAIN1,
            LocalDomainAccessControllerTest::TEST_INTERFACE1,
            TrustLevel::HIGH,
            getConsumerPermissionCallback);

    EXPECT_TRUE(getConsumerPermissionCallback->expectCallback(1000));

    // The operation is ambigious and interface level permission is not available
    EXPECT_FALSE(getConsumerPermissionCallback->isPermissionAvailable());

    // Operation level permission should work
    EXPECT_EQ(Permission::YES,
              localDomainAccessController->getConsumerPermission(
                      LocalDomainAccessControllerTest::TEST_USER,
                      LocalDomainAccessControllerTest::TEST_DOMAIN1,
                      LocalDomainAccessControllerTest::TEST_INTERFACE1,
                      LocalDomainAccessControllerTest::TEST_OPERATION1,
                      TrustLevel::HIGH));
}

// true: with persist file
// false: without
INSTANTIATE_TEST_SUITE_P(WithOrWithoutPersistFile, LocalDomainAccessControllerTest, Bool());

TEST(LocalDomainAccessControllerPersistedTest, persistedAcesAreUsed)
{
    // Load persisted ACEs
    joynr::test::util::copyTestResourceToCurrentDirectory("AccessStoreTest.persist");
    auto localDomainAccessStore =
            std::make_unique<LocalDomainAccessStore>("AccessStoreTest.persist");
    auto localDomainAccessController = std::make_unique<LocalDomainAccessController>(
            std::move(localDomainAccessStore));

    EXPECT_EQ(Permission::NO,
              localDomainAccessController->getConsumerPermission(
                      LocalDomainAccessControllerTest::TEST_USER,
                      LocalDomainAccessControllerTest::TEST_DOMAIN1,
                      LocalDomainAccessControllerTest::TEST_INTERFACE1,
                      LocalDomainAccessControllerTest::TEST_OPERATION1,
                      TrustLevel::HIGH));
}

// Registration control entries

TEST_P(LocalDomainAccessControllerTest, providerPermission)
{
    localDomainAccessStorePtr->updateOwnerRegistrationControlEntry(ownerRce);
    EXPECT_EQ(Permission::YES,
              localDomainAccessController->getProviderPermission(
                      LocalDomainAccessControllerTest::TEST_USER,
                      LocalDomainAccessControllerTest::TEST_DOMAIN1,
                      LocalDomainAccessControllerTest::TEST_INTERFACE1,
                      TrustLevel::HIGH));
}

TEST_P(LocalDomainAccessControllerTest, providerPermissionInvalidOwnerRce)
{
    localDomainAccessStorePtr->updateOwnerRegistrationControlEntry(ownerRce);

    // Update the MasterACE so that it does not permit Permission::YES
    std::vector<Permission::Enum> possiblePermissions = {Permission::NO, Permission::ASK};
    masterRce.setDefaultProviderPermission(Permission::ASK);
    masterRce.setPossibleProviderPermissions(possiblePermissions);
    localDomainAccessStorePtr->updateMasterRegistrationControlEntry(masterRce);

    EXPECT_EQ(Permission::NO,
              localDomainAccessController->getProviderPermission(
                      LocalDomainAccessControllerTest::TEST_USER,
                      LocalDomainAccessControllerTest::TEST_DOMAIN1,
                      LocalDomainAccessControllerTest::TEST_INTERFACE1,
                      TrustLevel::HIGH));
}

TEST_P(LocalDomainAccessControllerTest, providerPermissionOwnerRceOverrulesMaster)
{
    ownerRce.setRequiredTrustLevel(TrustLevel::MID);
    ownerRce.setProviderPermission(Permission::ASK);
    localDomainAccessStorePtr->updateOwnerRegistrationControlEntry(ownerRce);
    localDomainAccessStorePtr->updateMasterRegistrationControlEntry(masterRce);

    EXPECT_EQ(Permission::ASK,
              localDomainAccessController->getProviderPermission(
                      LocalDomainAccessControllerTest::TEST_USER,
                      LocalDomainAccessControllerTest::TEST_DOMAIN1,
                      LocalDomainAccessControllerTest::TEST_INTERFACE1,
                      TrustLevel::HIGH));
    EXPECT_EQ(Permission::NO,
              localDomainAccessController->getProviderPermission(
                      LocalDomainAccessControllerTest::TEST_USER,
                      LocalDomainAccessControllerTest::TEST_DOMAIN1,
                      LocalDomainAccessControllerTest::TEST_INTERFACE1,
                      TrustLevel::LOW));
}

TEST_P(LocalDomainAccessControllerTest, DISABLED_providerPermissionAmbigious)
{
    // Setup the master with a wildcard operation
    masterAce.setOperation(access_control::WILDCARD);
    localDomainAccessStorePtr->updateOwnerAccessControlEntry(ownerAce);
    localDomainAccessStorePtr->updateMasterAccessControlEntry(masterAce);

    // Get the provider permission (async)
    auto getProviderPermissionCallback = std::make_shared<PermissionCallback>();

    localDomainAccessController->getProviderPermission(
            LocalDomainAccessControllerTest::TEST_USER,
            LocalDomainAccessControllerTest::TEST_DOMAIN1,
            LocalDomainAccessControllerTest::TEST_INTERFACE1,
            TrustLevel::HIGH,
            getProviderPermissionCallback);

    EXPECT_TRUE(getProviderPermissionCallback->expectCallback(1000));

    // The operation is ambigious and interface level permission is not available
    EXPECT_FALSE(getProviderPermissionCallback->isPermissionAvailable());

    // Operation level permission should work
    EXPECT_EQ(Permission::YES,
              localDomainAccessController->getProviderPermission(
                      LocalDomainAccessControllerTest::TEST_USER,
                      LocalDomainAccessControllerTest::TEST_DOMAIN1,
                      LocalDomainAccessControllerTest::TEST_INTERFACE1,
                      TrustLevel::HIGH));
}

TEST(LocalDomainAccessControllerPersistedTest, persistedRcesAreUsed)
{
    // Load persisted ACEs
    joynr::test::util::copyTestResourceToCurrentDirectory("AccessStoreTest.persist");
    auto localDomainAccessStore =
            std::make_unique<LocalDomainAccessStore>("AccessStoreTest.persist");
    auto localDomainAccessController = std::make_unique<LocalDomainAccessController>(
            std::move(localDomainAccessStore));

    EXPECT_EQ(Permission::NO,
              localDomainAccessController->getProviderPermission(
                      LocalDomainAccessControllerTest::TEST_USER,
                      LocalDomainAccessControllerTest::TEST_DOMAIN1,
                      LocalDomainAccessControllerTest::TEST_INTERFACE1,
                      TrustLevel::HIGH));
}
