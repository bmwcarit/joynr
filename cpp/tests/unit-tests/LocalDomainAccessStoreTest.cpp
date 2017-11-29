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

#include <gtest/gtest.h>

#include "JoynrTest.h"

#include "joynr/Settings.h"
#include "joynr/ClusterControllerSettings.h"
#include "joynr/PrivateCopyAssign.h"

#include "libjoynrclustercontroller/access-control/LocalDomainAccessStore.h"

using namespace ::testing;
using namespace joynr;
using namespace joynr::infrastructure::DacTypes;

struct AccessStoreTestData
{
    std::string userId;
    std::string domain;
    std::string interfaceName;
};

class LocalDomainAccessStoreTest : public ::testing::Test {
public:
    LocalDomainAccessStoreTest()
        : localDomainAccessStore()
    {
        expectedDomainRoleEntry = DomainRoleEntry(TEST_USER1, DOMAINS, Role::OWNER);
        expectedMasterAccessControlEntry = MasterAccessControlEntry(TEST_USER1,
                                                                    TEST_DOMAIN1,
                                                                    TEST_INTERFACE1,
                                                                    TrustLevel::LOW,
                                                                    TRUST_LEVELS,
                                                                    TrustLevel::LOW,
                                                                    TRUST_LEVELS,
                                                                    TEST_OPERATION1,
                                                                    Permission::NO,
                                                                    PERMISSIONS);
        expectedOwnerAccessControlEntry = OwnerAccessControlEntry(TEST_USER1,
                                                                  TEST_DOMAIN1,
                                                                  TEST_INTERFACE1,
                                                                  TrustLevel::LOW,
                                                                  TrustLevel::LOW,
                                                                  TEST_OPERATION1,
                                                                  Permission::NO);
    }

    ~LocalDomainAccessStoreTest() override
    {
        // Delete test specific files
        joynr::test::util::removeFileInCurrentDirectory(".*\\.settings");
        joynr::test::util::removeFileInCurrentDirectory(".*\\.persist");
        joynr::test::util::removeFileInCurrentDirectory(".*\\.entries");
    }

protected:
    LocalDomainAccessStore localDomainAccessStore;
    DomainRoleEntry expectedDomainRoleEntry;
    MasterAccessControlEntry expectedMasterAccessControlEntry;
    OwnerAccessControlEntry expectedOwnerAccessControlEntry;

    static const std::string TEST_USER;
    static const std::string TEST_USER1;
    static const std::string TEST_USER2;
    static const std::string TEST_DOMAIN1;
    static const std::string TEST_INTERFACE1;
    static const std::string TEST_INTERFACE2;
    static const std::string TEST_OPERATION1;
    static const std::string TEST_OPERATION2;
    static const std::vector<std::string> DOMAINS;
    static const std::vector<Permission::Enum> PERMISSIONS;
    static const std::vector<TrustLevel::Enum> TRUST_LEVELS;

    void queryAccessStoreAndVerifyOutput(const std::string& uid,
                                         const std::string& domain,
                                         const std::string& interfaceName,
                                         const AccessStoreTestData& expectedResult){
        const MasterAccessControlEntry expectedEntry (expectedResult.userId,
                                                      expectedResult.domain,
                                                      expectedResult.interfaceName,
                                                      TrustLevel::LOW,
                                                      TRUST_LEVELS,
                                                      TrustLevel::LOW,
                                                      TRUST_LEVELS,
                                                      TEST_OPERATION1,
                                                      Permission::NO,
                                                      PERMISSIONS);

        // The last parameter is the operation which we do not currently support
        auto result = localDomainAccessStore.getMasterAccessControlEntry(uid,
                                                           domain,
                                                           interfaceName,
                                                           joynr::access_control::WILDCARD);
        ASSERT_TRUE(result);
        EXPECT_EQ(expectedEntry, *result);
    }

private:
    DISALLOW_COPY_AND_ASSIGN(LocalDomainAccessStoreTest);
};

const std::string LocalDomainAccessStoreTest::TEST_USER("testUser");
const std::string LocalDomainAccessStoreTest::TEST_USER1("testUser1");
const std::string LocalDomainAccessStoreTest::TEST_USER2("testUser2");
const std::string LocalDomainAccessStoreTest::TEST_DOMAIN1("domain1");
const std::string LocalDomainAccessStoreTest::TEST_INTERFACE1("interface1");
const std::string LocalDomainAccessStoreTest::TEST_INTERFACE2("interface2");
const std::string LocalDomainAccessStoreTest::TEST_OPERATION1("READ");
const std::string LocalDomainAccessStoreTest::TEST_OPERATION2("WRITE");
const std::vector<std::string> LocalDomainAccessStoreTest::DOMAINS = {TEST_DOMAIN1};
const std::vector<Permission::Enum> LocalDomainAccessStoreTest::PERMISSIONS = {Permission::NO, Permission::ASK};
const std::vector<TrustLevel::Enum> LocalDomainAccessStoreTest::TRUST_LEVELS = {TrustLevel::LOW, TrustLevel::MID};

const std::vector<Permission::Enum> PERMISSIONS_EMPTY;
const std::vector<Permission::Enum> PERMISSIONS_ALL = {Permission::YES, Permission::NO, Permission::ASK};
const std::vector<Permission::Enum> PERMISSIONS_WITHOUT_NO = {Permission::YES, Permission::ASK};
const std::vector<Permission::Enum> PERMISSIONS_WITHOUT_YES = {Permission::NO, Permission::ASK};
const std::vector<TrustLevel::Enum> TRUST_LEVELS_EMPTY;
const std::vector<TrustLevel::Enum> TRUST_LEVELS_ALL = {TrustLevel::NONE, TrustLevel::LOW, TrustLevel::MID, TrustLevel::HIGH};
const std::vector<TrustLevel::Enum> TRUST_LEVELS_WITHOUT_LOW = {TrustLevel::NONE, TrustLevel::MID, TrustLevel::HIGH};
const std::vector<TrustLevel::Enum> TRUST_LEVELS_WITHOUT_NONE = {TrustLevel::LOW, TrustLevel::MID, TrustLevel::HIGH};

//----- Tests ------------------------------------------------------------------

TEST_F(LocalDomainAccessStoreTest, mergeMultipleLocalDomainAccessStores) {
    Settings testSettings("test-resources/AclRclJoynrClusterControllerRuntimeTest.settings");
    ASSERT_TRUE(testSettings.isLoaded());
    ClusterControllerSettings clusterControllerSettings(testSettings);

    LocalDomainAccessStore masterAccess(clusterControllerSettings.getAclEntriesDirectory() + "/MasterAccessTable.json");
    LocalDomainAccessStore masterRegistration(clusterControllerSettings.getAclEntriesDirectory() + "/MasterRegistrationTable.json");
    LocalDomainAccessStore ownerRegistration(clusterControllerSettings.getAclEntriesDirectory() + "/OwnerRegistrationTable.json");
    LocalDomainAccessStore ownerAccess(clusterControllerSettings.getAclEntriesDirectory() + "/OwnerAccessTable.json");
    LocalDomainAccessStore mergedLocalDomainAccessStore;

    EXPECT_EQ(mergedLocalDomainAccessStore.getMasterAccessControlEntries(TEST_USER).size(), 0);
    EXPECT_EQ(mergedLocalDomainAccessStore.getMasterRegistrationControlEntries(TEST_USER).size(), 0);
    EXPECT_EQ(mergedLocalDomainAccessStore.getOwnerAccessControlEntries(TEST_USER).size(), 0);
    EXPECT_EQ(mergedLocalDomainAccessStore.getOwnerRegistrationControlEntries(TEST_USER).size(), 0);

    EXPECT_TRUE(mergedLocalDomainAccessStore.mergeDomainAccessStore(masterAccess));
    EXPECT_TRUE(mergedLocalDomainAccessStore.mergeDomainAccessStore(masterRegistration));
    EXPECT_TRUE(mergedLocalDomainAccessStore.mergeDomainAccessStore(ownerRegistration));
    EXPECT_TRUE(mergedLocalDomainAccessStore.mergeDomainAccessStore(ownerAccess));

    EXPECT_EQ(mergedLocalDomainAccessStore.getMasterAccessControlEntries(TEST_USER).size(), 1);
    EXPECT_EQ(mergedLocalDomainAccessStore.getMasterRegistrationControlEntries(TEST_USER).size(), 1);
    EXPECT_EQ(mergedLocalDomainAccessStore.getOwnerAccessControlEntries(TEST_USER).size(), 1);
    EXPECT_EQ(mergedLocalDomainAccessStore.getOwnerRegistrationControlEntries(TEST_USER).size(), 1);
}

TEST_F(LocalDomainAccessStoreTest, mergeLocalDomainAccessStoreSingleEntryOnEmptyStore) {
    localDomainAccessStore.updateDomainRole(expectedDomainRoleEntry);
    LocalDomainAccessStore otherStore;

    EXPECT_TRUE(otherStore.mergeDomainAccessStore(localDomainAccessStore));

    boost::optional<DomainRoleEntry> domainRole = localDomainAccessStore.getDomainRole(expectedDomainRoleEntry.getUid(),
                                                                                 expectedDomainRoleEntry.getRole());
    boost::optional<DomainRoleEntry> domainRoleOther = otherStore.getDomainRole(expectedDomainRoleEntry.getUid(),
                                                                                 expectedDomainRoleEntry.getRole());
    EXPECT_EQ(*domainRole, *domainRoleOther);
}

TEST_F(LocalDomainAccessStoreTest, mergeEmptyLocalDomainAccessStores) {
    LocalDomainAccessStore store;
    LocalDomainAccessStore otherStore;
    EXPECT_TRUE(otherStore.mergeDomainAccessStore(store));
}

TEST_F(LocalDomainAccessStoreTest, getDomainRoles) {
    localDomainAccessStore.updateDomainRole(expectedDomainRoleEntry);

    std::vector<DomainRoleEntry> domainRoles = localDomainAccessStore.getDomainRoles(expectedDomainRoleEntry.getUid());
    EXPECT_EQ(expectedDomainRoleEntry, *domainRoles.begin());

    boost::optional<DomainRoleEntry> domainRole = localDomainAccessStore.getDomainRole(expectedDomainRoleEntry.getUid(),
                                                                                 expectedDomainRoleEntry.getRole());
    EXPECT_TRUE(bool(domainRole));
    EXPECT_EQ(expectedDomainRoleEntry, *domainRole);
}

TEST_F(LocalDomainAccessStoreTest, updateDomainRole) {
    EXPECT_TRUE(localDomainAccessStore.updateDomainRole(expectedDomainRoleEntry));

    // Check that an entry was added
    std::vector<DomainRoleEntry> dres = localDomainAccessStore.getDomainRoles(expectedDomainRoleEntry.getUid());
    EXPECT_FALSE(dres.empty());
    boost::optional<DomainRoleEntry> dreFromDb = localDomainAccessStore.getDomainRole(expectedDomainRoleEntry.getUid(),
                                                                                expectedDomainRoleEntry.getRole());

    EXPECT_EQ(expectedDomainRoleEntry, *dreFromDb);
}

TEST_F(LocalDomainAccessStoreTest, removeDomainRole) {
    localDomainAccessStore.updateDomainRole(expectedDomainRoleEntry);

    EXPECT_TRUE(localDomainAccessStore.removeDomainRole(expectedDomainRoleEntry.getUid(), expectedDomainRoleEntry.getRole()));
    boost::optional<DomainRoleEntry> dreFromDb = localDomainAccessStore.getDomainRole(expectedDomainRoleEntry.getUid(),
                                                                                expectedDomainRoleEntry.getRole());
    EXPECT_FALSE(bool(dreFromDb));
}

TEST_F(LocalDomainAccessStoreTest, getMasterAces) {
    localDomainAccessStore.updateMasterAccessControlEntry(expectedMasterAccessControlEntry);
    EXPECT_EQ(expectedMasterAccessControlEntry, *localDomainAccessStore.getMasterAccessControlEntries(expectedMasterAccessControlEntry.getUid()).begin());
    EXPECT_EQ(expectedMasterAccessControlEntry, *localDomainAccessStore.getMasterAccessControlEntries(expectedMasterAccessControlEntry.getDomain(),
                                                                                                      expectedMasterAccessControlEntry.getInterfaceName()).begin());
    EXPECT_EQ(expectedMasterAccessControlEntry, *localDomainAccessStore.getMasterAccessControlEntries(expectedMasterAccessControlEntry.getUid(),
                                                                                                     expectedMasterAccessControlEntry.getDomain(),
                                                                                                     expectedMasterAccessControlEntry.getInterfaceName()).begin());
    EXPECT_EQ(expectedMasterAccessControlEntry, localDomainAccessStore.getMasterAccessControlEntry(expectedMasterAccessControlEntry.getUid(),
                                                                                                    expectedMasterAccessControlEntry.getDomain(),
                                                                                                    expectedMasterAccessControlEntry.getInterfaceName(),
                                                                                                    expectedMasterAccessControlEntry.getOperation()).get());
    MasterAccessControlEntry masterAceWildcardUser(expectedMasterAccessControlEntry);
    masterAceWildcardUser.setUid(access_control::WILDCARD);
    localDomainAccessStore.updateMasterAccessControlEntry(masterAceWildcardUser);

    std::vector<MasterAccessControlEntry> masterAces = localDomainAccessStore.getMasterAccessControlEntries(TEST_DOMAIN1, TEST_INTERFACE1);
    int expectedNumberOfMasterAces = 2;
    EXPECT_EQ(expectedNumberOfMasterAces, masterAces.size());
    EXPECT_TRUE(util::vectorContains(masterAces, expectedMasterAccessControlEntry));
    EXPECT_TRUE(util::vectorContains(masterAces, masterAceWildcardUser));
    EXPECT_EQ(masterAceWildcardUser, localDomainAccessStore.getMasterAccessControlEntry(TEST_USER2,
                                                                                         masterAceWildcardUser.getDomain(),
                                                                                         masterAceWildcardUser.getInterfaceName(),
                                                                                         masterAceWildcardUser.getOperation()).get());
    EXPECT_EQ(masterAceWildcardUser, *(localDomainAccessStore.getMasterAccessControlEntries(TEST_USER2)).begin());
    EXPECT_EQ(masterAceWildcardUser, *(localDomainAccessStore.getMasterAccessControlEntries(TEST_USER2,
                                                                                            masterAceWildcardUser.getDomain(),
                                                                                            masterAceWildcardUser.getInterfaceName())).begin());
}

TEST_F(LocalDomainAccessStoreTest, getMasterAceWithWildcardOperation) {
    expectedMasterAccessControlEntry.setOperation(access_control::WILDCARD);
    localDomainAccessStore.updateMasterAccessControlEntry(expectedMasterAccessControlEntry);

    EXPECT_EQ(expectedMasterAccessControlEntry, localDomainAccessStore.getMasterAccessControlEntry(expectedMasterAccessControlEntry.getUid(),
                                                                                                    expectedMasterAccessControlEntry.getDomain(),
                                                                                                    expectedMasterAccessControlEntry.getInterfaceName(),
                                                                                                    TEST_OPERATION1).get());
}

TEST_F(LocalDomainAccessStoreTest, editableMasterAces) {
    expectedDomainRoleEntry.setRole(Role::MASTER);
    localDomainAccessStore.updateDomainRole(expectedDomainRoleEntry);
    localDomainAccessStore.updateMasterAccessControlEntry(expectedMasterAccessControlEntry);

    std::vector<MasterAccessControlEntry> editableMasterAces = localDomainAccessStore.getEditableMasterAccessControlEntries(TEST_USER1);
    int expectedNumberOfMasterAces = 1;
    EXPECT_EQ(expectedNumberOfMasterAces, editableMasterAces.size());
    EXPECT_EQ(expectedMasterAccessControlEntry, *editableMasterAces.begin());
}

TEST_F(LocalDomainAccessStoreTest, editableMasterAccessControlEntryNoMatchingDre) {
    expectedMasterAccessControlEntry.setUid(TEST_USER2);
    localDomainAccessStore.updateMasterAccessControlEntry(expectedMasterAccessControlEntry);

    std::vector<MasterAccessControlEntry> editableMasterAces = localDomainAccessStore.getEditableMasterAccessControlEntries(TEST_USER1);
    EXPECT_TRUE(editableMasterAces.empty());
}

TEST_F(LocalDomainAccessStoreTest, updateMasterAce) {
    EXPECT_TRUE(localDomainAccessStore.updateMasterAccessControlEntry(expectedMasterAccessControlEntry));

    MasterAccessControlEntry masterAceFromDb =
            localDomainAccessStore.getMasterAccessControlEntry(expectedMasterAccessControlEntry.getUid(),
                                                 expectedMasterAccessControlEntry.getDomain(),
                                                 expectedMasterAccessControlEntry.getInterfaceName(),
                                                 expectedMasterAccessControlEntry.getOperation()).get();
    EXPECT_EQ(expectedMasterAccessControlEntry, masterAceFromDb);
}

TEST_F(LocalDomainAccessStoreTest, removeMasterAce) {
    localDomainAccessStore.updateMasterAccessControlEntry(expectedMasterAccessControlEntry);

    EXPECT_TRUE(localDomainAccessStore.removeMasterAccessControlEntry(expectedMasterAccessControlEntry.getUid(),
                                                         expectedMasterAccessControlEntry.getDomain(),
                                                         expectedMasterAccessControlEntry.getInterfaceName(),
                                                         expectedMasterAccessControlEntry.getOperation()));

    // Check the ACE does not exist
    std::vector<MasterAccessControlEntry> masterAces =
            localDomainAccessStore.getMasterAccessControlEntries(expectedMasterAccessControlEntry.getUid(),
                                                  expectedMasterAccessControlEntry.getDomain(),
                                                  expectedMasterAccessControlEntry.getInterfaceName());

    EXPECT_TRUE(masterAces.empty());
}

TEST_F(LocalDomainAccessStoreTest, getOwnerAccessControlEntry) {
    localDomainAccessStore.updateOwnerAccessControlEntry(expectedOwnerAccessControlEntry);

    EXPECT_EQ(expectedOwnerAccessControlEntry, *localDomainAccessStore.getOwnerAccessControlEntries(expectedOwnerAccessControlEntry.getUid()).begin());
    EXPECT_EQ(expectedOwnerAccessControlEntry, *localDomainAccessStore.getOwnerAccessControlEntries(expectedOwnerAccessControlEntry.getDomain(),
                                                                                                    expectedOwnerAccessControlEntry.getInterfaceName()).begin());
    EXPECT_EQ(expectedOwnerAccessControlEntry, *localDomainAccessStore.getOwnerAccessControlEntries(expectedOwnerAccessControlEntry.getUid(),
                                                                                                    expectedOwnerAccessControlEntry.getDomain(),
                                                                                                    expectedOwnerAccessControlEntry.getInterfaceName()).begin());
    EXPECT_EQ(expectedOwnerAccessControlEntry, localDomainAccessStore.getOwnerAccessControlEntry(expectedOwnerAccessControlEntry.getUid(),
                                                                                                    expectedOwnerAccessControlEntry.getDomain(),
                                                                                                    expectedOwnerAccessControlEntry.getInterfaceName(),
                                                                                                    expectedOwnerAccessControlEntry.getOperation()).get());
    OwnerAccessControlEntry ownerAceWildcardUser(expectedOwnerAccessControlEntry);
    ownerAceWildcardUser.setUid(access_control::WILDCARD);
    EXPECT_TRUE(localDomainAccessStore.updateOwnerAccessControlEntry(ownerAceWildcardUser));

    std::vector<OwnerAccessControlEntry> ownerAces = localDomainAccessStore.getOwnerAccessControlEntries(TEST_DOMAIN1, TEST_INTERFACE1);
    int expectedNumberOfOwnerAces = 2;
    EXPECT_EQ(expectedNumberOfOwnerAces, ownerAces.size());
    EXPECT_TRUE(util::vectorContains(ownerAces, expectedOwnerAccessControlEntry));
    EXPECT_TRUE(util::vectorContains(ownerAces, ownerAceWildcardUser));
    EXPECT_EQ(ownerAceWildcardUser, localDomainAccessStore.getOwnerAccessControlEntry(TEST_USER2,
                                                                                         ownerAceWildcardUser.getDomain(),
                                                                                         ownerAceWildcardUser.getInterfaceName(),
                                                                                         ownerAceWildcardUser.getOperation()).get());
    EXPECT_EQ(ownerAceWildcardUser, *(localDomainAccessStore.getOwnerAccessControlEntries(TEST_USER2)).begin());
    EXPECT_EQ(ownerAceWildcardUser, *(localDomainAccessStore.getOwnerAccessControlEntries(TEST_USER2,
                                                                                            ownerAceWildcardUser.getDomain(),
                                                                                            ownerAceWildcardUser.getInterfaceName())).begin());
}

TEST_F(LocalDomainAccessStoreTest, getEditableOwnerAces) {
    localDomainAccessStore.updateDomainRole(expectedDomainRoleEntry);
    localDomainAccessStore.updateOwnerAccessControlEntry(expectedOwnerAccessControlEntry);

    // Check that the correct editable domain is returned
    std::vector<OwnerAccessControlEntry> editableOwnerAces =
            localDomainAccessStore.getEditableOwnerAccessControlEntries(TEST_USER1);

    int expectedEditableOwnerAces = 1;
    EXPECT_EQ(expectedEditableOwnerAces, editableOwnerAces.size());
    EXPECT_EQ(expectedOwnerAccessControlEntry, *editableOwnerAces.begin());
}

TEST_F(LocalDomainAccessStoreTest, editableOwnerAccessControlEntryNoMatchingDre) {
    localDomainAccessStore.updateDomainRole(expectedDomainRoleEntry);
    expectedOwnerAccessControlEntry.setUid(TEST_USER2);
    localDomainAccessStore.updateOwnerAccessControlEntry(expectedOwnerAccessControlEntry);

    // Check that the correct editable domain is returned
    std::vector<OwnerAccessControlEntry> editableOwnerAces =
            localDomainAccessStore.getEditableOwnerAccessControlEntries(TEST_USER2);

    EXPECT_TRUE(editableOwnerAces.empty());
}

TEST_F(LocalDomainAccessStoreTest, updateOwnerAce) {
    EXPECT_TRUE(localDomainAccessStore.updateOwnerAccessControlEntry(expectedOwnerAccessControlEntry));

    // Check that the entry was added
    OwnerAccessControlEntry ownerAceFromDb =
            localDomainAccessStore.getOwnerAccessControlEntry(expectedOwnerAccessControlEntry.getUid(),
                                                expectedOwnerAccessControlEntry.getDomain(),
                                                expectedOwnerAccessControlEntry.getInterfaceName(),
                                                expectedOwnerAccessControlEntry.getOperation()).get();
    EXPECT_EQ(expectedOwnerAccessControlEntry, ownerAceFromDb);
}

TEST_F(LocalDomainAccessStoreTest, removeOwnerAce) {
    localDomainAccessStore.updateOwnerAccessControlEntry(expectedOwnerAccessControlEntry);
    // Remove the ACE
    EXPECT_TRUE( localDomainAccessStore.removeOwnerAccessControlEntry(expectedOwnerAccessControlEntry.getUid(),
                                                        expectedOwnerAccessControlEntry.getDomain(),
                                                        expectedOwnerAccessControlEntry.getInterfaceName(),
                                                        expectedOwnerAccessControlEntry.getOperation()));

    // Check the ACE does not exist
    std::vector<OwnerAccessControlEntry> ownerAces =
            localDomainAccessStore.getOwnerAccessControlEntries(expectedOwnerAccessControlEntry.getUid(),
                                                  expectedOwnerAccessControlEntry.getDomain(),
                                                  expectedOwnerAccessControlEntry.getInterfaceName());
    EXPECT_TRUE(ownerAces.empty());
}

TEST_F(LocalDomainAccessStoreTest, updateOwnerRce_notAllowedByMasterRce) {
    // test no Owner RCE allowed
    MasterRegistrationControlEntry testMasterRce = MasterRegistrationControlEntry(TEST_USER1,
                                                                                  TEST_DOMAIN1,
                                                                                  TEST_INTERFACE1,
                                                                                  TrustLevel::LOW,
                                                                                  TRUST_LEVELS_EMPTY,
                                                                                  TrustLevel::LOW,
                                                                                  TRUST_LEVELS_EMPTY,
                                                                                  Permission::NO,
                                                                                  PERMISSIONS_EMPTY);
    OwnerRegistrationControlEntry testOwnerRce = OwnerRegistrationControlEntry(TEST_USER1,
                                                                               TEST_DOMAIN1,
                                                                               TEST_INTERFACE1,
                                                                               TrustLevel::LOW,
                                                                               TrustLevel::LOW,
                                                                               Permission::NO);
    OwnerRegistrationControlEntry allowedOwnerRce = OwnerRegistrationControlEntry(TEST_USER1,
                                                                                  TEST_DOMAIN1,
                                                                                  TEST_INTERFACE1,
                                                                                  TrustLevel::MID,
                                                                                  TrustLevel::HIGH,
                                                                                  Permission::ASK);

    EXPECT_TRUE(localDomainAccessStore.updateMasterRegistrationControlEntry(testMasterRce));
    EXPECT_FALSE(localDomainAccessStore.updateOwnerRegistrationControlEntry(testOwnerRce));
    EXPECT_FALSE(localDomainAccessStore.updateOwnerRegistrationControlEntry(allowedOwnerRce));

    // test Owner RCE with required trust level LOW not allowed
    testMasterRce.setPossibleRequiredTrustLevels(TRUST_LEVELS_WITHOUT_LOW);
    testMasterRce.setPossibleRequiredControlEntryChangeTrustLevels(TRUST_LEVELS_ALL);
    testMasterRce.setPossibleProviderPermissions(PERMISSIONS_ALL);

    EXPECT_TRUE(localDomainAccessStore.updateMasterRegistrationControlEntry(testMasterRce));
    EXPECT_FALSE(localDomainAccessStore.updateOwnerRegistrationControlEntry(testOwnerRce));
    EXPECT_TRUE(localDomainAccessStore.updateOwnerRegistrationControlEntry(allowedOwnerRce));

    // test Owner RCE with provider permission NO not allowed
    testMasterRce.setPossibleRequiredTrustLevels(TRUST_LEVELS_ALL);
    testMasterRce.setPossibleRequiredControlEntryChangeTrustLevels(TRUST_LEVELS_ALL);
    testMasterRce.setPossibleProviderPermissions(PERMISSIONS_WITHOUT_NO);

    EXPECT_TRUE(localDomainAccessStore.updateMasterRegistrationControlEntry(testMasterRce));
    EXPECT_FALSE(localDomainAccessStore.updateOwnerRegistrationControlEntry(testOwnerRce));
    EXPECT_TRUE(localDomainAccessStore.updateOwnerRegistrationControlEntry(allowedOwnerRce));
}

TEST_F(LocalDomainAccessStoreTest, updateOwnerRce_notAllowedByMediatorRce) {
    // test no Owner RCE allowed
    MasterRegistrationControlEntry testMediatorRce = MasterRegistrationControlEntry(TEST_USER1,
                                                                                  TEST_DOMAIN1,
                                                                                  TEST_INTERFACE1,
                                                                                  TrustLevel::LOW,
                                                                                  TRUST_LEVELS_EMPTY,
                                                                                  TrustLevel::LOW,
                                                                                  TRUST_LEVELS_EMPTY,
                                                                                  Permission::NO,
                                                                                  PERMISSIONS_EMPTY);
    OwnerRegistrationControlEntry testOwnerRce = OwnerRegistrationControlEntry(TEST_USER1,
                                                                               TEST_DOMAIN1,
                                                                               TEST_INTERFACE1,
                                                                               TrustLevel::LOW,
                                                                               TrustLevel::LOW,
                                                                               Permission::NO);
    OwnerRegistrationControlEntry allowedOwnerRce = OwnerRegistrationControlEntry(TEST_USER1,
                                                                                  TEST_DOMAIN1,
                                                                                  TEST_INTERFACE1,
                                                                                  TrustLevel::MID,
                                                                                  TrustLevel::HIGH,
                                                                                  Permission::ASK);

    EXPECT_TRUE(localDomainAccessStore.updateMediatorRegistrationControlEntry(testMediatorRce));
    EXPECT_FALSE(localDomainAccessStore.updateOwnerRegistrationControlEntry(testOwnerRce));
    EXPECT_FALSE(localDomainAccessStore.updateOwnerRegistrationControlEntry(allowedOwnerRce));

    // test Owner RCE with required trust level LOW not allowed
    testMediatorRce.setPossibleRequiredTrustLevels(TRUST_LEVELS_WITHOUT_LOW);
    testMediatorRce.setPossibleRequiredControlEntryChangeTrustLevels(TRUST_LEVELS_ALL);
    testMediatorRce.setPossibleProviderPermissions(PERMISSIONS_ALL);

    EXPECT_TRUE(localDomainAccessStore.updateMediatorRegistrationControlEntry(testMediatorRce));
    EXPECT_FALSE(localDomainAccessStore.updateOwnerRegistrationControlEntry(testOwnerRce));
    EXPECT_TRUE(localDomainAccessStore.updateOwnerRegistrationControlEntry(allowedOwnerRce));

    // test Owner RCE with provider permission NO not allowed
    testMediatorRce.setPossibleRequiredTrustLevels(TRUST_LEVELS_ALL);
    testMediatorRce.setPossibleRequiredControlEntryChangeTrustLevels(TRUST_LEVELS_ALL);
    testMediatorRce.setPossibleProviderPermissions(PERMISSIONS_WITHOUT_NO);

    EXPECT_TRUE(localDomainAccessStore.updateMediatorRegistrationControlEntry(testMediatorRce));
    EXPECT_FALSE(localDomainAccessStore.updateOwnerRegistrationControlEntry(testOwnerRce));
    EXPECT_TRUE(localDomainAccessStore.updateOwnerRegistrationControlEntry(allowedOwnerRce));
}
TEST_F(LocalDomainAccessStoreTest, updateMediatorRce_notAllowedByMasterRce) {
    // test no Mediator RCE allowed
    MasterRegistrationControlEntry testMasterRce = MasterRegistrationControlEntry(TEST_USER1,
                                                                                  TEST_DOMAIN1,
                                                                                  TEST_INTERFACE1,
                                                                                  TrustLevel::LOW,
                                                                                  TRUST_LEVELS_EMPTY,
                                                                                  TrustLevel::LOW,
                                                                                  TRUST_LEVELS_EMPTY,
                                                                                  Permission::NO,
                                                                                  PERMISSIONS_EMPTY);
    MasterRegistrationControlEntry testMediatorRce = MasterRegistrationControlEntry(TEST_USER1,
                                                                                    TEST_DOMAIN1,
                                                                                    TEST_INTERFACE1,
                                                                                    TrustLevel::LOW,
                                                                                    TRUST_LEVELS_EMPTY,
                                                                                    TrustLevel::LOW,
                                                                                    TRUST_LEVELS_EMPTY,
                                                                                    Permission::NO,
                                                                                    PERMISSIONS_EMPTY);
    MasterRegistrationControlEntry allowedMediatorRce = MasterRegistrationControlEntry(TEST_USER1,
                                                                                       TEST_DOMAIN1,
                                                                                       TEST_INTERFACE1,
                                                                                       TrustLevel::MID,
                                                                                       TRUST_LEVELS_WITHOUT_LOW,
                                                                                       TrustLevel::HIGH,
                                                                                       TRUST_LEVELS_WITHOUT_LOW,
                                                                                       Permission::ASK,
                                                                                       PERMISSIONS_WITHOUT_NO);

    EXPECT_TRUE(localDomainAccessStore.updateMasterRegistrationControlEntry(testMasterRce));
    EXPECT_FALSE(localDomainAccessStore.updateMediatorRegistrationControlEntry(testMediatorRce));
    EXPECT_FALSE(localDomainAccessStore.updateMediatorRegistrationControlEntry(allowedMediatorRce));

    // test Mediator RCE with required trust level LOW
    // as default required trust level not allowed
    testMasterRce.setPossibleRequiredTrustLevels(TRUST_LEVELS_WITHOUT_LOW);
    testMasterRce.setPossibleRequiredControlEntryChangeTrustLevels(TRUST_LEVELS_ALL);
    testMasterRce.setPossibleProviderPermissions(PERMISSIONS_ALL);

    EXPECT_TRUE(localDomainAccessStore.updateMasterRegistrationControlEntry(testMasterRce));
    EXPECT_FALSE(localDomainAccessStore.updateMediatorRegistrationControlEntry(testMediatorRce));
    EXPECT_TRUE(
                localDomainAccessStore.updateMediatorRegistrationControlEntry(allowedMediatorRce)
                );

    // test Mediator RCE with provider permission NO
    // in default provider permission not allowed
    testMasterRce.setPossibleRequiredTrustLevels(TRUST_LEVELS_ALL);
    testMasterRce.setPossibleRequiredControlEntryChangeTrustLevels(TRUST_LEVELS_ALL);
    testMasterRce.setPossibleProviderPermissions(PERMISSIONS_WITHOUT_NO);

    EXPECT_TRUE(localDomainAccessStore.updateMasterRegistrationControlEntry(testMasterRce));
    EXPECT_FALSE(localDomainAccessStore.updateMediatorRegistrationControlEntry(testMediatorRce));
    EXPECT_TRUE(localDomainAccessStore.updateMediatorRegistrationControlEntry(allowedMediatorRce));

    // test Mediator RCE with required trust level NONE
    // in possible required trust levels not allowed
    testMediatorRce.setPossibleRequiredTrustLevels(TRUST_LEVELS_WITHOUT_LOW);
    allowedMediatorRce.setPossibleRequiredTrustLevels(TRUST_LEVELS_WITHOUT_NONE);

    testMasterRce.setPossibleRequiredTrustLevels(TRUST_LEVELS_WITHOUT_NONE);
    testMasterRce.setPossibleRequiredControlEntryChangeTrustLevels(TRUST_LEVELS_ALL);
    testMasterRce.setPossibleProviderPermissions(PERMISSIONS_ALL);

    EXPECT_TRUE(localDomainAccessStore.updateMasterRegistrationControlEntry(testMasterRce));
    EXPECT_FALSE(localDomainAccessStore.updateMediatorRegistrationControlEntry(testMediatorRce));
    EXPECT_TRUE(
                localDomainAccessStore.updateMediatorRegistrationControlEntry(allowedMediatorRce)
                );
    testMediatorRce.setPossibleRequiredTrustLevels(TRUST_LEVELS_EMPTY);
    allowedMediatorRce.setPossibleRequiredTrustLevels(TRUST_LEVELS_WITHOUT_LOW);

    // test Mediator RCE with provider permission NO
    // in possible provider permissions not allowed
    testMediatorRce.setDefaultProviderPermission(Permission::ASK);
    testMediatorRce.setPossibleProviderPermissions(PERMISSIONS_WITHOUT_YES);

    testMasterRce.setPossibleRequiredTrustLevels(TRUST_LEVELS_ALL);
    testMasterRce.setPossibleRequiredControlEntryChangeTrustLevels(TRUST_LEVELS_ALL);
    testMasterRce.setPossibleProviderPermissions(PERMISSIONS_WITHOUT_NO);

    EXPECT_TRUE(localDomainAccessStore.updateMasterRegistrationControlEntry(testMasterRce));
    EXPECT_FALSE(localDomainAccessStore.updateMediatorRegistrationControlEntry(testMediatorRce));
    EXPECT_TRUE(localDomainAccessStore.updateMediatorRegistrationControlEntry(allowedMediatorRce));
}

TEST_F(LocalDomainAccessStoreTest, updateOwnerAce_notAllowedByMasterAce) {
    // test no Owner ACE allowed
    MasterAccessControlEntry testMasterAce = MasterAccessControlEntry(TEST_USER1,
                                                                      TEST_DOMAIN1,
                                                                      TEST_INTERFACE1,
                                                                      TrustLevel::LOW,
                                                                      TRUST_LEVELS_EMPTY,
                                                                      TrustLevel::LOW,
                                                                      TRUST_LEVELS_EMPTY,
                                                                      TEST_OPERATION1,
                                                                      Permission::NO,
                                                                      PERMISSIONS_EMPTY);
    OwnerAccessControlEntry testOwnerAce = OwnerAccessControlEntry(TEST_USER1,
                                                                   TEST_DOMAIN1,
                                                                   TEST_INTERFACE1,
                                                                   TrustLevel::LOW,
                                                                   TrustLevel::LOW,
                                                                   TEST_OPERATION1,
                                                                   Permission::NO);
    OwnerAccessControlEntry allowedOwnerAce = OwnerAccessControlEntry(TEST_USER1,
                                                                      TEST_DOMAIN1,
                                                                      TEST_INTERFACE1,
                                                                      TrustLevel::MID,
                                                                      TrustLevel::HIGH,
                                                                      TEST_OPERATION1,
                                                                      Permission::ASK);

    EXPECT_TRUE(localDomainAccessStore.updateMasterAccessControlEntry(testMasterAce));
    EXPECT_FALSE(localDomainAccessStore.updateOwnerAccessControlEntry(testOwnerAce));
    EXPECT_FALSE(localDomainAccessStore.updateOwnerAccessControlEntry(allowedOwnerAce));

    // test Owner ACE with required trust level LOW not allowed
    testMasterAce.setPossibleRequiredTrustLevels(TRUST_LEVELS_WITHOUT_LOW);
    testMasterAce.setPossibleRequiredControlEntryChangeTrustLevels(TRUST_LEVELS_ALL);
    testMasterAce.setPossibleConsumerPermissions(PERMISSIONS_ALL);

    EXPECT_TRUE(localDomainAccessStore.updateMasterAccessControlEntry(testMasterAce));
    EXPECT_FALSE(localDomainAccessStore.updateOwnerAccessControlEntry(testOwnerAce));
    EXPECT_TRUE(localDomainAccessStore.updateOwnerAccessControlEntry(allowedOwnerAce));

    // test Owner ACE with Consumer permission NO not allowed
    testMasterAce.setPossibleRequiredTrustLevels(TRUST_LEVELS_ALL);
    testMasterAce.setPossibleRequiredControlEntryChangeTrustLevels(TRUST_LEVELS_ALL);
    testMasterAce.setPossibleConsumerPermissions(PERMISSIONS_WITHOUT_NO);

    EXPECT_TRUE(localDomainAccessStore.updateMasterAccessControlEntry(testMasterAce));
    EXPECT_FALSE(localDomainAccessStore.updateOwnerAccessControlEntry(testOwnerAce));
    EXPECT_TRUE(localDomainAccessStore.updateOwnerAccessControlEntry(allowedOwnerAce));
}

TEST_F(LocalDomainAccessStoreTest, updateOwnerAce_notAllowedByMediatorAce) {
    // test no Owner ACE allowed
    MasterAccessControlEntry testMediatorAce = MasterAccessControlEntry(TEST_USER1,
                                                                        TEST_DOMAIN1,
                                                                        TEST_INTERFACE1,
                                                                        TrustLevel::LOW,
                                                                        TRUST_LEVELS_EMPTY,
                                                                        TrustLevel::LOW,
                                                                        TRUST_LEVELS_EMPTY,
                                                                        TEST_OPERATION1,
                                                                        Permission::NO,
                                                                        PERMISSIONS_EMPTY);
    OwnerAccessControlEntry testOwnerAce = OwnerAccessControlEntry(TEST_USER1,
                                                                   TEST_DOMAIN1,
                                                                   TEST_INTERFACE1,
                                                                   TrustLevel::LOW,
                                                                   TrustLevel::LOW,
                                                                   TEST_OPERATION1,
                                                                   Permission::NO);
    OwnerAccessControlEntry allowedOwnerAce = OwnerAccessControlEntry(TEST_USER1,
                                                                      TEST_DOMAIN1,
                                                                      TEST_INTERFACE1,
                                                                      TrustLevel::MID,
                                                                      TrustLevel::HIGH,
                                                                      TEST_OPERATION1,
                                                                      Permission::ASK);

    EXPECT_TRUE(localDomainAccessStore.updateMediatorAccessControlEntry(testMediatorAce));
    EXPECT_FALSE(localDomainAccessStore.updateOwnerAccessControlEntry(testOwnerAce));
    EXPECT_FALSE(localDomainAccessStore.updateOwnerAccessControlEntry(allowedOwnerAce));

    // test Owner ACE with required trust level LOW not allowed
    testMediatorAce.setPossibleRequiredTrustLevels(TRUST_LEVELS_WITHOUT_LOW);
    testMediatorAce.setPossibleRequiredControlEntryChangeTrustLevels(TRUST_LEVELS_ALL);
    testMediatorAce.setPossibleConsumerPermissions(PERMISSIONS_ALL);

    EXPECT_TRUE(localDomainAccessStore.updateMediatorAccessControlEntry(testMediatorAce));
    EXPECT_FALSE(localDomainAccessStore.updateOwnerAccessControlEntry(testOwnerAce));
    EXPECT_TRUE(localDomainAccessStore.updateOwnerAccessControlEntry(allowedOwnerAce));

    // test Owner ACE with Consumer permission NO not allowed
    testMediatorAce.setPossibleRequiredTrustLevels(TRUST_LEVELS_ALL);
    testMediatorAce.setPossibleRequiredControlEntryChangeTrustLevels(TRUST_LEVELS_ALL);
    testMediatorAce.setPossibleConsumerPermissions(PERMISSIONS_WITHOUT_NO);

    EXPECT_TRUE(localDomainAccessStore.updateMediatorAccessControlEntry(testMediatorAce));
    EXPECT_FALSE(localDomainAccessStore.updateOwnerAccessControlEntry(testOwnerAce));
    EXPECT_TRUE(localDomainAccessStore.updateOwnerAccessControlEntry(allowedOwnerAce));
}
TEST_F(LocalDomainAccessStoreTest, updateMediatorAce_notAllowedByMasterAce) {
    // test no Mediator ACE allowed
    MasterAccessControlEntry testMasterAce = MasterAccessControlEntry(TEST_USER1,
                                                                      TEST_DOMAIN1,
                                                                      TEST_INTERFACE1,
                                                                      TrustLevel::LOW,
                                                                      TRUST_LEVELS_EMPTY,
                                                                      TrustLevel::LOW,
                                                                      TRUST_LEVELS_EMPTY,
                                                                      TEST_OPERATION1,
                                                                      Permission::NO,
                                                                      PERMISSIONS_EMPTY);
    MasterAccessControlEntry testMediatorAce = MasterAccessControlEntry(TEST_USER1,
                                                                        TEST_DOMAIN1,
                                                                        TEST_INTERFACE1,
                                                                        TrustLevel::LOW,
                                                                        TRUST_LEVELS_EMPTY,
                                                                        TrustLevel::LOW,
                                                                        TRUST_LEVELS_EMPTY,
                                                                        TEST_OPERATION1,
                                                                        Permission::NO,
                                                                        PERMISSIONS_EMPTY);
    MasterAccessControlEntry allowedMediatorAce = MasterAccessControlEntry(TEST_USER1,
                                                                           TEST_DOMAIN1,
                                                                           TEST_INTERFACE1,
                                                                           TrustLevel::MID,
                                                                           TRUST_LEVELS_WITHOUT_LOW,
                                                                           TrustLevel::HIGH,
                                                                           TRUST_LEVELS_WITHOUT_LOW,
                                                                           TEST_OPERATION1,
                                                                           Permission::ASK,
                                                                           PERMISSIONS_WITHOUT_NO);

    EXPECT_TRUE(localDomainAccessStore.updateMasterAccessControlEntry(testMasterAce));
    EXPECT_FALSE(localDomainAccessStore.updateMediatorAccessControlEntry(testMediatorAce));
    EXPECT_FALSE(localDomainAccessStore.updateMediatorAccessControlEntry(allowedMediatorAce));

    // test Mediator ACE with required trust level LOW
    // as default required trust level not allowed
    testMasterAce.setPossibleRequiredTrustLevels(TRUST_LEVELS_WITHOUT_LOW);
    testMasterAce.setPossibleRequiredControlEntryChangeTrustLevels(TRUST_LEVELS_ALL);
    testMasterAce.setPossibleConsumerPermissions(PERMISSIONS_ALL);

    EXPECT_TRUE(localDomainAccessStore.updateMasterAccessControlEntry(testMasterAce));
    EXPECT_FALSE(localDomainAccessStore.updateMediatorAccessControlEntry(testMediatorAce));
    EXPECT_TRUE(
                localDomainAccessStore.updateMediatorAccessControlEntry(allowedMediatorAce)
                );

    // test Mediator ACE with consumer permission NO
    // as default consumer permission not allowed
    testMasterAce.setPossibleRequiredTrustLevels(TRUST_LEVELS_ALL);
    testMasterAce.setPossibleRequiredControlEntryChangeTrustLevels(TRUST_LEVELS_ALL);
    testMasterAce.setPossibleConsumerPermissions(PERMISSIONS_WITHOUT_NO);

    EXPECT_TRUE(localDomainAccessStore.updateMasterAccessControlEntry(testMasterAce));
    EXPECT_FALSE(localDomainAccessStore.updateMediatorAccessControlEntry(testMediatorAce));
    EXPECT_TRUE(localDomainAccessStore.updateMediatorAccessControlEntry(allowedMediatorAce));

    // test Mediator ACE with required trust level NONE
    // in possible required trust levels not allowed
    testMediatorAce.setPossibleRequiredTrustLevels(TRUST_LEVELS_WITHOUT_LOW);
    allowedMediatorAce.setPossibleRequiredTrustLevels(TRUST_LEVELS_WITHOUT_NONE);

    testMasterAce.setPossibleRequiredTrustLevels(TRUST_LEVELS_WITHOUT_NONE);
    testMasterAce.setPossibleRequiredControlEntryChangeTrustLevels(TRUST_LEVELS_ALL);
    testMasterAce.setPossibleConsumerPermissions(PERMISSIONS_ALL);

    EXPECT_TRUE(localDomainAccessStore.updateMasterAccessControlEntry(testMasterAce));
    EXPECT_FALSE(localDomainAccessStore.updateMediatorAccessControlEntry(testMediatorAce));
    EXPECT_TRUE(
                localDomainAccessStore.updateMediatorAccessControlEntry(allowedMediatorAce)
                );
    testMediatorAce.setPossibleRequiredTrustLevels(TRUST_LEVELS_EMPTY);
    allowedMediatorAce.setPossibleRequiredTrustLevels(TRUST_LEVELS_WITHOUT_LOW);

    // test Mediator ACE with consumer permission NO
    // in possible consumer permissions not allowed
    testMediatorAce.setDefaultConsumerPermission(Permission::ASK);
    testMediatorAce.setPossibleConsumerPermissions(PERMISSIONS_WITHOUT_YES);

    testMasterAce.setPossibleRequiredTrustLevels(TRUST_LEVELS_ALL);
    testMasterAce.setPossibleRequiredControlEntryChangeTrustLevels(TRUST_LEVELS_ALL);
    testMasterAce.setPossibleConsumerPermissions(PERMISSIONS_WITHOUT_NO);

    EXPECT_TRUE(localDomainAccessStore.updateMasterAccessControlEntry(testMasterAce));
    EXPECT_FALSE(localDomainAccessStore.updateMediatorAccessControlEntry(testMediatorAce));
    EXPECT_TRUE(localDomainAccessStore.updateMediatorAccessControlEntry(allowedMediatorAce));
}

TEST_F(LocalDomainAccessStoreTest, restoreFromPersistenceFile) {

    const std::string masterACEinterfaceName = "this/is/a/test/interface";
    expectedMasterAccessControlEntry.setInterfaceName(masterACEinterfaceName);

    {
        LocalDomainAccessStore localDomainAccessStore(ClusterControllerSettings::DEFAULT_LOCAL_DOMAIN_ACCESS_STORE_PERSISTENCE_FILENAME());
        localDomainAccessStore.updateOwnerAccessControlEntry(expectedOwnerAccessControlEntry);
        localDomainAccessStore.updateMasterAccessControlEntry(expectedMasterAccessControlEntry);
    }

    {
        LocalDomainAccessStore localDomainAccessStore(ClusterControllerSettings::DEFAULT_LOCAL_DOMAIN_ACCESS_STORE_PERSISTENCE_FILENAME());
        EXPECT_EQ(expectedOwnerAccessControlEntry, localDomainAccessStore.getOwnerAccessControlEntry(expectedOwnerAccessControlEntry.getUid(),
                                                                                                     expectedOwnerAccessControlEntry.getDomain(),
                                                                                                     expectedOwnerAccessControlEntry.getInterfaceName(),
                                                                                                     expectedOwnerAccessControlEntry.getOperation()).get());

        EXPECT_EQ(expectedMasterAccessControlEntry, localDomainAccessStore.getMasterAccessControlEntry(expectedMasterAccessControlEntry.getUid(),
                                                                                                      expectedMasterAccessControlEntry.getDomain(),
                                                                                                      expectedMasterAccessControlEntry.getInterfaceName(),
                                                                                                      expectedMasterAccessControlEntry.getOperation()).get());
    }
}

TEST_F(LocalDomainAccessStoreTest, doesNotContainOnlyWildcardOperations) {

    // add a wildcard and a non-wildcard operation
    expectedOwnerAccessControlEntry.setOperation(access_control::WILDCARD);
    localDomainAccessStore.updateOwnerAccessControlEntry(expectedOwnerAccessControlEntry);
    expectedOwnerAccessControlEntry.setOperation(TEST_OPERATION1);
    localDomainAccessStore.updateOwnerAccessControlEntry(expectedOwnerAccessControlEntry);

    // still return false
    EXPECT_FALSE(localDomainAccessStore.onlyWildcardOperations(TEST_USER1,
                                                               TEST_DOMAIN1,
                                                               TEST_INTERFACE1));
}

TEST_F(LocalDomainAccessStoreTest, containsOnlyWildcardOperations) {
    // Test on empty access store
    EXPECT_TRUE(localDomainAccessStore.onlyWildcardOperations(TEST_USER1,
                                                TEST_DOMAIN1,
                                                TEST_INTERFACE1));

    // add entries with wildcard operation
    expectedOwnerAccessControlEntry.setOperation(access_control::WILDCARD);
    expectedMasterAccessControlEntry.setOperation(access_control::WILDCARD);
    localDomainAccessStore.updateOwnerAccessControlEntry(expectedOwnerAccessControlEntry);
    localDomainAccessStore.updateMasterAccessControlEntry(expectedMasterAccessControlEntry);

    // still return true
    EXPECT_TRUE(localDomainAccessStore.onlyWildcardOperations(TEST_USER1,
                                                              TEST_DOMAIN1,
                                                              TEST_INTERFACE1));
}

/*
 * The test only works with MasterAccessControlEntry.
 * It could be extended to include all other types but it mainly focuses on the correctness
 * of the retrived result instead of how all ACE/RCE work together.
 *
 * The test is divided in two main parts:
 *  1. in the first part the access store is filled with entries (here wildcards can appear)
 *  2. in the second part the access store is queried (query cannot contain wildcards)
 */
TEST_F(LocalDomainAccessStoreTest, getDomainAndInterfaceWithWildcard){
    // ***
    // Prepare LocalDomainAccessStore
    // ***
    const std::vector<AccessStoreTestData> accessStoreData = {
       //
       //           UID                   DOMAIN     INTERFACE
       //
       {TEST_USER1,                      "domain", "interfaceName"},
       {TEST_USER1,                      "domain", "interface*"},
       {TEST_USER1,                      "dom*",   "interfaceName"},
       {TEST_USER1,                      "dom*",   "interface*"},
       {joynr::access_control::WILDCARD, "domain", "interfaceName"},
       {joynr::access_control::WILDCARD, "domain", "interface*"},
       {joynr::access_control::WILDCARD, "dom*",   "interfaceName"},
       {joynr::access_control::WILDCARD, "dom*",   "interface*"},
       {joynr::access_control::WILDCARD, "*",      "*"}
    };

    // add data to LocalDomainAccessStore
    for(const auto& entry : accessStoreData) {
        MasterAccessControlEntry masterACE (entry.userId,
                                            entry.domain,
                                            entry.interfaceName,
                                            TrustLevel::LOW,
                                            TRUST_LEVELS,
                                            TrustLevel::LOW,
                                            TRUST_LEVELS,
                                            TEST_OPERATION1,
                                            Permission::NO,
                                            PERMISSIONS);
        localDomainAccessStore.updateMasterAccessControlEntry(masterACE);
    };

    // ***
    // Query storage and verify result:
    // the match should always return the most specific entry from the access store.
    // ***

    // EXACT MATCH
    // In the access store there is an entry as from the query
    queryAccessStoreAndVerifyOutput(TEST_USER1,                      "domain", "interfaceName", accessStoreData[0]);

    // MATCH INTERFACE WILDCARD
    queryAccessStoreAndVerifyOutput(TEST_USER1,                      "domain", "interface1",    accessStoreData[1]);

    // MATCH DOMAIN WILDCARD
    queryAccessStoreAndVerifyOutput(TEST_USER1,                      "dom1",   "interfaceName", accessStoreData[2]);

    // MATCH DOMAIN AND INTERFACE WITH WILDCARD
    queryAccessStoreAndVerifyOutput(TEST_USER1,                      "dom1",   "interface1",    accessStoreData[3]);

    // MATCH UID WILDCARD
    queryAccessStoreAndVerifyOutput(TEST_USER2,                      "domain", "interfaceName", accessStoreData[4]);

    // MATCH UID AND INTERFACE WITH WILDCARD
    queryAccessStoreAndVerifyOutput(TEST_USER2,                      "domain", "interface1",    accessStoreData[5]);

    // MATCH UID AND DOMAIN WITH WILDCARD
    queryAccessStoreAndVerifyOutput(TEST_USER2,                      "dom1",   "interfaceName", accessStoreData[6]);

    // MATCH UID, DOMAIN AND INTERFACE WITH WILDCARD
    queryAccessStoreAndVerifyOutput(TEST_USER2,                      "dom1",   "interface1",    accessStoreData[7]);
}

TEST_F(LocalDomainAccessStoreTest, allowEverything) {
    // add data to LocalDomainAccessStore
    const MasterAccessControlEntry masterACE (joynr::access_control::WILDCARD,
                                        joynr::access_control::WILDCARD,
                                        joynr::access_control::WILDCARD,
                                        TrustLevel::LOW,
                                        TRUST_LEVELS,
                                        TrustLevel::LOW,
                                        TRUST_LEVELS,
                                        TEST_OPERATION1,
                                        Permission::NO,
                                        PERMISSIONS);
    localDomainAccessStore.updateMasterAccessControlEntry(masterACE);

    const AccessStoreTestData expectedData = {
        joynr::access_control::WILDCARD,
        joynr::access_control::WILDCARD,
        joynr::access_control::WILDCARD
    };

    // MATCH ANYTHING
    queryAccessStoreAndVerifyOutput(std::string("joynr"),
                                    std::string("acl"),
                                    std::string("wildcards"),
                                    expectedData);
}

TEST_F(LocalDomainAccessStoreTest, denyEverything) {
    // do not add anything to the store
    // try to perform a query
    auto result = localDomainAccessStore.getMasterAccessControlEntry("user",
                                                       "domain",
                                                       "interfaceName",
                                                       joynr::access_control::WILDCARD);
    ASSERT_FALSE(result);
}
