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

#include "joynr/ClusterControllerSettings.h"
#include "joynr/PrivateCopyAssign.h"
#include "libjoynrclustercontroller/access-control/LocalDomainAccessStore.h"

using namespace ::testing;
using namespace joynr;
using namespace joynr::infrastructure::DacTypes;

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
        std::remove(ClusterControllerSettings::DEFAULT_LOCAL_DOMAIN_ACCESS_STORE_PERSISTENCE_FILENAME().c_str());
    }

protected:
    LocalDomainAccessStore localDomainAccessStore;
    DomainRoleEntry expectedDomainRoleEntry;
    MasterAccessControlEntry expectedMasterAccessControlEntry;
    OwnerAccessControlEntry expectedOwnerAccessControlEntry;

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
private:
    DISALLOW_COPY_AND_ASSIGN(LocalDomainAccessStoreTest);
};

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

//----- Tests ------------------------------------------------------------------

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

TEST_F(LocalDomainAccessStoreTest, updateOwnerAccessControlEntry) {
    EXPECT_TRUE(localDomainAccessStore.updateOwnerAccessControlEntry(expectedOwnerAccessControlEntry));
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

TEST_F(LocalDomainAccessStoreTest, restoreFromPersistenceFile) {
    {
        LocalDomainAccessStore localDomainAccessStore(ClusterControllerSettings::DEFAULT_LOCAL_DOMAIN_ACCESS_STORE_PERSISTENCE_FILENAME());
        localDomainAccessStore.updateOwnerAccessControlEntry(expectedOwnerAccessControlEntry);
    }

    {
        LocalDomainAccessStore localDomainAccessStore(ClusterControllerSettings::DEFAULT_LOCAL_DOMAIN_ACCESS_STORE_PERSISTENCE_FILENAME());
        EXPECT_EQ(expectedOwnerAccessControlEntry, localDomainAccessStore.getOwnerAccessControlEntry(expectedOwnerAccessControlEntry.getUid(),
                                                                                                        expectedOwnerAccessControlEntry.getDomain(),
                                                                                                        expectedOwnerAccessControlEntry.getInterfaceName(),
                                                                                                        expectedOwnerAccessControlEntry.getOperation()).get());
    }
}
