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

#include "tests/utils/Gtest.h"

#include "tests/JoynrTest.h"

#include "joynr/ClusterControllerSettings.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/Settings.h"

#include "libjoynrclustercontroller/access-control/LocalDomainAccessStore.h"

using namespace ::testing;
using namespace joynr;
using namespace joynr::infrastructure::DacTypes;

struct AccessStoreTestData {
    std::string userId;
    std::string domain;
    std::string interfaceName;
};

class LocalDomainAccessStoreTest : public ::testing::Test
{
public:
    LocalDomainAccessStoreTest() : _localDomainAccessStore()
    {
        _expectedDomainRoleEntry = DomainRoleEntry(_TEST_USER1, _DOMAINS, Role::OWNER);
        _expectedMasterAccessControlEntry = MasterAccessControlEntry(_TEST_USER1,
                                                                     _TEST_DOMAIN1,
                                                                     _TEST_INTERFACE1,
                                                                     TrustLevel::LOW,
                                                                     _TRUST_LEVELS,
                                                                     TrustLevel::LOW,
                                                                     _TRUST_LEVELS,
                                                                     _TEST_OPERATION1,
                                                                     Permission::NO,
                                                                     _PERMISSIONS);
        _expectedOwnerAccessControlEntry = OwnerAccessControlEntry(_TEST_USER1,
                                                                   _TEST_DOMAIN1,
                                                                   _TEST_INTERFACE1,
                                                                   TrustLevel::LOW,
                                                                   TrustLevel::LOW,
                                                                   _TEST_OPERATION1,
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
    LocalDomainAccessStore _localDomainAccessStore;
    DomainRoleEntry _expectedDomainRoleEntry;
    MasterAccessControlEntry _expectedMasterAccessControlEntry;
    OwnerAccessControlEntry _expectedOwnerAccessControlEntry;

    static const std::string _TEST_USER;
    static const std::string _TEST_USER1;
    static const std::string _TEST_USER2;
    static const std::string _TEST_DOMAIN1;
    static const std::string _TEST_INTERFACE1;
    static const std::string _TEST_INTERFACE2;
    static const std::string _TEST_OPERATION1;
    static const std::string _TEST_OPERATION2;
    static const std::vector<std::string> _DOMAINS;
    static const std::vector<Permission::Enum> _PERMISSIONS;
    static const std::vector<TrustLevel::Enum> _TRUST_LEVELS;

    void queryAccessStoreAndVerifyOutput(const std::string& uid,
                                         const std::string& domain,
                                         const std::string& interfaceName,
                                         const AccessStoreTestData& expectedResult)
    {
        const MasterAccessControlEntry expectedEntry(expectedResult.userId,
                                                     expectedResult.domain,
                                                     expectedResult.interfaceName,
                                                     TrustLevel::LOW,
                                                     _TRUST_LEVELS,
                                                     TrustLevel::LOW,
                                                     _TRUST_LEVELS,
                                                     _TEST_OPERATION1,
                                                     Permission::NO,
                                                     _PERMISSIONS);

        // The last parameter is the operation which we do not currently support
        auto result = _localDomainAccessStore.getMasterAccessControlEntry(
                uid, domain, interfaceName, joynr::access_control::WILDCARD);
        ASSERT_TRUE(result);
        EXPECT_EQ(expectedEntry, *result);
    }

    void queryAccessStoreAndVerifyOutputForTwoIdenticalTemplatesWithDifferentUserIdTests(
            std::string localDomainAccessFile)
    {
        _localDomainAccessStore.mergeDomainAccessStore(
                LocalDomainAccessStore(localDomainAccessFile));
        _localDomainAccessStore.logContent();

        auto result = _localDomainAccessStore.getMasterAccessControlEntry(
                "application_1",
                "same.prefix.domain.domain1",
                "same/prefix/with/different/interface1",
                joynr::access_control::WILDCARD);
        ASSERT_TRUE(result);
        EXPECT_EQ(result->getUid(), "application_1");
        EXPECT_EQ(result->getDomain(), "same.prefix.domain.*");
        EXPECT_EQ(result->getInterfaceName(), "same/prefix/with/different/interface1");
        EXPECT_EQ(result->getDefaultConsumerPermission(),
                  joynr::infrastructure::DacTypes::Permission::YES);

        auto result2 = _localDomainAccessStore.getMasterAccessControlEntry(
                "application_1",
                "same.prefix.domain.domain1",
                "same/prefix/with/different/interface2",
                joynr::access_control::WILDCARD);
        ASSERT_TRUE(result2);
        EXPECT_EQ(result2->getUid(), "application_1");
        EXPECT_EQ(result2->getDomain(), "same.prefix.domain.*");
        EXPECT_EQ(result2->getInterfaceName(), "same/prefix/with/different/interface2");
        EXPECT_EQ(result2->getDefaultConsumerPermission(),
                  joynr::infrastructure::DacTypes::Permission::YES);
    }

private:
    DISALLOW_COPY_AND_ASSIGN(LocalDomainAccessStoreTest);
};

const std::string LocalDomainAccessStoreTest::_TEST_USER("testUser");
const std::string LocalDomainAccessStoreTest::_TEST_USER1("testUser1");
const std::string LocalDomainAccessStoreTest::_TEST_USER2("testUser2");
const std::string LocalDomainAccessStoreTest::_TEST_DOMAIN1("domain1");
const std::string LocalDomainAccessStoreTest::_TEST_INTERFACE1("interface1");
const std::string LocalDomainAccessStoreTest::_TEST_INTERFACE2("interface2");
const std::string LocalDomainAccessStoreTest::_TEST_OPERATION1("READ");
const std::string LocalDomainAccessStoreTest::_TEST_OPERATION2("WRITE");
const std::vector<std::string> LocalDomainAccessStoreTest::_DOMAINS = {_TEST_DOMAIN1};
const std::vector<Permission::Enum> LocalDomainAccessStoreTest::_PERMISSIONS = {
        Permission::NO, Permission::ASK};
const std::vector<TrustLevel::Enum> LocalDomainAccessStoreTest::_TRUST_LEVELS = {
        TrustLevel::LOW, TrustLevel::MID};

const std::vector<Permission::Enum> PERMISSIONS_EMPTY;
const std::vector<Permission::Enum> PERMISSIONS_ALL = {
        Permission::YES, Permission::NO, Permission::ASK};
const std::vector<Permission::Enum> PERMISSIONS_WITHOUT_NO = {Permission::YES, Permission::ASK};
const std::vector<Permission::Enum> PERMISSIONS_WITHOUT_YES = {Permission::NO, Permission::ASK};
const std::vector<TrustLevel::Enum> TRUST_LEVELS_EMPTY;
const std::vector<TrustLevel::Enum> TRUST_LEVELS_ALL = {
        TrustLevel::NONE, TrustLevel::LOW, TrustLevel::MID, TrustLevel::HIGH};
const std::vector<TrustLevel::Enum> TRUST_LEVELS_WITHOUT_LOW = {
        TrustLevel::NONE, TrustLevel::MID, TrustLevel::HIGH};
const std::vector<TrustLevel::Enum> TRUST_LEVELS_WITHOUT_NONE = {
        TrustLevel::LOW, TrustLevel::MID, TrustLevel::HIGH};

//----- Tests ------------------------------------------------------------------

TEST_F(LocalDomainAccessStoreTest, mergeMultipleLocalDomainAccessStores)
{
    Settings testSettings("test-resources/AclRclJoynrClusterControllerRuntimeTest.settings");
    ASSERT_TRUE(testSettings.isLoaded());
    ClusterControllerSettings clusterControllerSettings(testSettings);

    LocalDomainAccessStore masterAccess(clusterControllerSettings.getAclEntriesDirectory() +
                                        "/MasterAccessTable.json");
    LocalDomainAccessStore masterRegistration(clusterControllerSettings.getAclEntriesDirectory() +
                                              "/MasterRegistrationTable.json");
    LocalDomainAccessStore ownerRegistration(clusterControllerSettings.getAclEntriesDirectory() +
                                             "/OwnerRegistrationTable.json");
    LocalDomainAccessStore ownerAccess(clusterControllerSettings.getAclEntriesDirectory() +
                                       "/OwnerAccessTable.json");
    LocalDomainAccessStore mergedLocalDomainAccessStore;

    EXPECT_EQ(mergedLocalDomainAccessStore.getMasterAccessControlEntries(_TEST_USER).size(), 0);
    EXPECT_EQ(
            mergedLocalDomainAccessStore.getMasterRegistrationControlEntries(_TEST_USER).size(), 0);
    EXPECT_EQ(mergedLocalDomainAccessStore.getOwnerAccessControlEntries(_TEST_USER).size(), 0);
    EXPECT_EQ(
            mergedLocalDomainAccessStore.getOwnerRegistrationControlEntries(_TEST_USER).size(), 0);

    EXPECT_TRUE(mergedLocalDomainAccessStore.mergeDomainAccessStore(masterAccess));
    EXPECT_TRUE(mergedLocalDomainAccessStore.mergeDomainAccessStore(masterRegistration));
    EXPECT_TRUE(mergedLocalDomainAccessStore.mergeDomainAccessStore(ownerRegistration));
    EXPECT_TRUE(mergedLocalDomainAccessStore.mergeDomainAccessStore(ownerAccess));

    EXPECT_EQ(mergedLocalDomainAccessStore.getMasterAccessControlEntries(_TEST_USER).size(), 1);
    EXPECT_EQ(
            mergedLocalDomainAccessStore.getMasterRegistrationControlEntries(_TEST_USER).size(), 1);
    EXPECT_EQ(mergedLocalDomainAccessStore.getOwnerAccessControlEntries(_TEST_USER).size(), 1);
    EXPECT_EQ(
            mergedLocalDomainAccessStore.getOwnerRegistrationControlEntries(_TEST_USER).size(), 1);
}

TEST_F(LocalDomainAccessStoreTest, mergeLocalDomainAccessStoreSingleEntryOnEmptyStore)
{
    _localDomainAccessStore.updateDomainRole(_expectedDomainRoleEntry);
    LocalDomainAccessStore otherStore;

    EXPECT_TRUE(otherStore.mergeDomainAccessStore(_localDomainAccessStore));

    boost::optional<DomainRoleEntry> domainRole = _localDomainAccessStore.getDomainRole(
            _expectedDomainRoleEntry.getUid(), _expectedDomainRoleEntry.getRole());
    boost::optional<DomainRoleEntry> domainRoleOther = otherStore.getDomainRole(
            _expectedDomainRoleEntry.getUid(), _expectedDomainRoleEntry.getRole());
    EXPECT_EQ(*domainRole, *domainRoleOther);
}

TEST_F(LocalDomainAccessStoreTest, mergeEmptyLocalDomainAccessStores)
{
    LocalDomainAccessStore store;
    LocalDomainAccessStore otherStore;
    EXPECT_TRUE(otherStore.mergeDomainAccessStore(store));
}

TEST_F(LocalDomainAccessStoreTest, getDomainRoles)
{
    _localDomainAccessStore.updateDomainRole(_expectedDomainRoleEntry);

    std::vector<DomainRoleEntry> domainRoles =
            _localDomainAccessStore.getDomainRoles(_expectedDomainRoleEntry.getUid());
    EXPECT_EQ(_expectedDomainRoleEntry, *domainRoles.begin());

    boost::optional<DomainRoleEntry> domainRole = _localDomainAccessStore.getDomainRole(
            _expectedDomainRoleEntry.getUid(), _expectedDomainRoleEntry.getRole());
    EXPECT_TRUE(bool(domainRole));
    EXPECT_EQ(_expectedDomainRoleEntry, *domainRole);
}

TEST_F(LocalDomainAccessStoreTest, updateDomainRole)
{
    EXPECT_TRUE(_localDomainAccessStore.updateDomainRole(_expectedDomainRoleEntry));

    // Check that an entry was added
    std::vector<DomainRoleEntry> dres =
            _localDomainAccessStore.getDomainRoles(_expectedDomainRoleEntry.getUid());
    EXPECT_FALSE(dres.empty());
    boost::optional<DomainRoleEntry> dreFromDb = _localDomainAccessStore.getDomainRole(
            _expectedDomainRoleEntry.getUid(), _expectedDomainRoleEntry.getRole());

    EXPECT_EQ(_expectedDomainRoleEntry, *dreFromDb);
}

TEST_F(LocalDomainAccessStoreTest, removeDomainRole)
{
    _localDomainAccessStore.updateDomainRole(_expectedDomainRoleEntry);

    EXPECT_TRUE(_localDomainAccessStore.removeDomainRole(
            _expectedDomainRoleEntry.getUid(), _expectedDomainRoleEntry.getRole()));
    boost::optional<DomainRoleEntry> dreFromDb = _localDomainAccessStore.getDomainRole(
            _expectedDomainRoleEntry.getUid(), _expectedDomainRoleEntry.getRole());
    EXPECT_FALSE(bool(dreFromDb));
}

TEST_F(LocalDomainAccessStoreTest, getMasterAces)
{
    _localDomainAccessStore.updateMasterAccessControlEntry(_expectedMasterAccessControlEntry);
    EXPECT_EQ(_expectedMasterAccessControlEntry,
              *_localDomainAccessStore
                       .getMasterAccessControlEntries(_expectedMasterAccessControlEntry.getUid())
                       .begin());
    EXPECT_EQ(_expectedMasterAccessControlEntry,
              *_localDomainAccessStore
                       .getMasterAccessControlEntries(
                               _expectedMasterAccessControlEntry.getDomain(),
                               _expectedMasterAccessControlEntry.getInterfaceName())
                       .begin());
    EXPECT_EQ(_expectedMasterAccessControlEntry,
              *_localDomainAccessStore
                       .getMasterAccessControlEntries(
                               _expectedMasterAccessControlEntry.getUid(),
                               _expectedMasterAccessControlEntry.getDomain(),
                               _expectedMasterAccessControlEntry.getInterfaceName())
                       .begin());
    EXPECT_EQ(_expectedMasterAccessControlEntry,
              _localDomainAccessStore
                      .getMasterAccessControlEntry(
                              _expectedMasterAccessControlEntry.getUid(),
                              _expectedMasterAccessControlEntry.getDomain(),
                              _expectedMasterAccessControlEntry.getInterfaceName(),
                              _expectedMasterAccessControlEntry.getOperation())
                      .get());
    MasterAccessControlEntry masterAceWildcardUser(_expectedMasterAccessControlEntry);
    masterAceWildcardUser.setUid(access_control::WILDCARD);
    _localDomainAccessStore.updateMasterAccessControlEntry(masterAceWildcardUser);

    std::vector<MasterAccessControlEntry> masterAces =
            _localDomainAccessStore.getMasterAccessControlEntries(_TEST_DOMAIN1, _TEST_INTERFACE1);
    int expectedNumberOfMasterAces = 2;
    EXPECT_EQ(expectedNumberOfMasterAces, masterAces.size());
    EXPECT_TRUE(util::vectorContains(masterAces, _expectedMasterAccessControlEntry));
    EXPECT_TRUE(util::vectorContains(masterAces, masterAceWildcardUser));
    EXPECT_EQ(masterAceWildcardUser,
              _localDomainAccessStore
                      .getMasterAccessControlEntry(_TEST_USER2,
                                                   masterAceWildcardUser.getDomain(),
                                                   masterAceWildcardUser.getInterfaceName(),
                                                   masterAceWildcardUser.getOperation())
                      .get());
    EXPECT_EQ(masterAceWildcardUser,
              *(_localDomainAccessStore.getMasterAccessControlEntries(_TEST_USER2)).begin());
    EXPECT_EQ(masterAceWildcardUser,
              *(_localDomainAccessStore.getMasterAccessControlEntries(
                        _TEST_USER2,
                        masterAceWildcardUser.getDomain(),
                        masterAceWildcardUser.getInterfaceName()))
                       .begin());
}

TEST_F(LocalDomainAccessStoreTest, getMasterAceWithWildcardOperation)
{
    _expectedMasterAccessControlEntry.setOperation(access_control::WILDCARD);
    _localDomainAccessStore.updateMasterAccessControlEntry(_expectedMasterAccessControlEntry);

    EXPECT_EQ(_expectedMasterAccessControlEntry,
              _localDomainAccessStore
                      .getMasterAccessControlEntry(
                              _expectedMasterAccessControlEntry.getUid(),
                              _expectedMasterAccessControlEntry.getDomain(),
                              _expectedMasterAccessControlEntry.getInterfaceName(),
                              _TEST_OPERATION1)
                      .get());
}

TEST_F(LocalDomainAccessStoreTest, editableMasterAces)
{
    _expectedDomainRoleEntry.setRole(Role::MASTER);
    _localDomainAccessStore.updateDomainRole(_expectedDomainRoleEntry);
    _localDomainAccessStore.updateMasterAccessControlEntry(_expectedMasterAccessControlEntry);

    std::vector<MasterAccessControlEntry> editableMasterAces =
            _localDomainAccessStore.getEditableMasterAccessControlEntries(_TEST_USER1);
    int expectedNumberOfMasterAces = 1;
    EXPECT_EQ(expectedNumberOfMasterAces, editableMasterAces.size());
    EXPECT_EQ(_expectedMasterAccessControlEntry, *editableMasterAces.begin());
}

TEST_F(LocalDomainAccessStoreTest, editableMasterAccessControlEntryNoMatchingDre)
{
    _expectedMasterAccessControlEntry.setUid(_TEST_USER2);
    _localDomainAccessStore.updateMasterAccessControlEntry(_expectedMasterAccessControlEntry);

    std::vector<MasterAccessControlEntry> editableMasterAces =
            _localDomainAccessStore.getEditableMasterAccessControlEntries(_TEST_USER1);
    EXPECT_TRUE(editableMasterAces.empty());
}

TEST_F(LocalDomainAccessStoreTest, updateMasterAce)
{
    EXPECT_TRUE(_localDomainAccessStore.updateMasterAccessControlEntry(
            _expectedMasterAccessControlEntry));

    MasterAccessControlEntry masterAceFromDb =
            _localDomainAccessStore
                    .getMasterAccessControlEntry(
                            _expectedMasterAccessControlEntry.getUid(),
                            _expectedMasterAccessControlEntry.getDomain(),
                            _expectedMasterAccessControlEntry.getInterfaceName(),
                            _expectedMasterAccessControlEntry.getOperation())
                    .get();
    EXPECT_EQ(_expectedMasterAccessControlEntry, masterAceFromDb);
}

TEST_F(LocalDomainAccessStoreTest, removeMasterAce)
{
    _localDomainAccessStore.updateMasterAccessControlEntry(_expectedMasterAccessControlEntry);

    EXPECT_TRUE(_localDomainAccessStore.removeMasterAccessControlEntry(
            _expectedMasterAccessControlEntry.getUid(),
            _expectedMasterAccessControlEntry.getDomain(),
            _expectedMasterAccessControlEntry.getInterfaceName(),
            _expectedMasterAccessControlEntry.getOperation()));

    // Check the ACE does not exist
    std::vector<MasterAccessControlEntry> masterAces =
            _localDomainAccessStore.getMasterAccessControlEntries(
                    _expectedMasterAccessControlEntry.getUid(),
                    _expectedMasterAccessControlEntry.getDomain(),
                    _expectedMasterAccessControlEntry.getInterfaceName());

    EXPECT_TRUE(masterAces.empty());
}

TEST_F(LocalDomainAccessStoreTest, getOwnerAccessControlEntry)
{
    _localDomainAccessStore.updateOwnerAccessControlEntry(_expectedOwnerAccessControlEntry);

    EXPECT_EQ(_expectedOwnerAccessControlEntry,
              *_localDomainAccessStore
                       .getOwnerAccessControlEntries(_expectedOwnerAccessControlEntry.getUid())
                       .begin());
    EXPECT_EQ(_expectedOwnerAccessControlEntry,
              *_localDomainAccessStore
                       .getOwnerAccessControlEntries(
                               _expectedOwnerAccessControlEntry.getDomain(),
                               _expectedOwnerAccessControlEntry.getInterfaceName())
                       .begin());
    EXPECT_EQ(_expectedOwnerAccessControlEntry,
              *_localDomainAccessStore
                       .getOwnerAccessControlEntries(
                               _expectedOwnerAccessControlEntry.getUid(),
                               _expectedOwnerAccessControlEntry.getDomain(),
                               _expectedOwnerAccessControlEntry.getInterfaceName())
                       .begin());
    EXPECT_EQ(
            _expectedOwnerAccessControlEntry,
            _localDomainAccessStore
                    .getOwnerAccessControlEntry(_expectedOwnerAccessControlEntry.getUid(),
                                                _expectedOwnerAccessControlEntry.getDomain(),
                                                _expectedOwnerAccessControlEntry.getInterfaceName(),
                                                _expectedOwnerAccessControlEntry.getOperation())
                    .get());
    OwnerAccessControlEntry ownerAceWildcardUser(_expectedOwnerAccessControlEntry);
    ownerAceWildcardUser.setUid(access_control::WILDCARD);
    EXPECT_TRUE(_localDomainAccessStore.updateOwnerAccessControlEntry(ownerAceWildcardUser));

    std::vector<OwnerAccessControlEntry> ownerAces =
            _localDomainAccessStore.getOwnerAccessControlEntries(_TEST_DOMAIN1, _TEST_INTERFACE1);
    int expectedNumberOfOwnerAces = 2;
    EXPECT_EQ(expectedNumberOfOwnerAces, ownerAces.size());
    EXPECT_TRUE(util::vectorContains(ownerAces, _expectedOwnerAccessControlEntry));
    EXPECT_TRUE(util::vectorContains(ownerAces, ownerAceWildcardUser));
    EXPECT_EQ(ownerAceWildcardUser,
              _localDomainAccessStore
                      .getOwnerAccessControlEntry(_TEST_USER2,
                                                  ownerAceWildcardUser.getDomain(),
                                                  ownerAceWildcardUser.getInterfaceName(),
                                                  ownerAceWildcardUser.getOperation())
                      .get());
    EXPECT_EQ(ownerAceWildcardUser,
              *(_localDomainAccessStore.getOwnerAccessControlEntries(_TEST_USER2)).begin());
    EXPECT_EQ(ownerAceWildcardUser,
              *(_localDomainAccessStore.getOwnerAccessControlEntries(
                        _TEST_USER2,
                        ownerAceWildcardUser.getDomain(),
                        ownerAceWildcardUser.getInterfaceName()))
                       .begin());
}

TEST_F(LocalDomainAccessStoreTest, getEditableOwnerAces)
{
    _localDomainAccessStore.updateDomainRole(_expectedDomainRoleEntry);
    _localDomainAccessStore.updateOwnerAccessControlEntry(_expectedOwnerAccessControlEntry);

    // Check that the correct editable domain is returned
    std::vector<OwnerAccessControlEntry> editableOwnerAces =
            _localDomainAccessStore.getEditableOwnerAccessControlEntries(_TEST_USER1);

    int expectedEditableOwnerAces = 1;
    EXPECT_EQ(expectedEditableOwnerAces, editableOwnerAces.size());
    EXPECT_EQ(_expectedOwnerAccessControlEntry, *editableOwnerAces.begin());
}

TEST_F(LocalDomainAccessStoreTest, editableOwnerAccessControlEntryNoMatchingDre)
{
    _localDomainAccessStore.updateDomainRole(_expectedDomainRoleEntry);
    _expectedOwnerAccessControlEntry.setUid(_TEST_USER2);
    _localDomainAccessStore.updateOwnerAccessControlEntry(_expectedOwnerAccessControlEntry);

    // Check that the correct editable domain is returned
    std::vector<OwnerAccessControlEntry> editableOwnerAces =
            _localDomainAccessStore.getEditableOwnerAccessControlEntries(_TEST_USER2);

    EXPECT_TRUE(editableOwnerAces.empty());
}

TEST_F(LocalDomainAccessStoreTest, updateOwnerAce)
{
    EXPECT_TRUE(_localDomainAccessStore.updateOwnerAccessControlEntry(
            _expectedOwnerAccessControlEntry));

    // Check that the entry was added
    OwnerAccessControlEntry ownerAceFromDb =
            _localDomainAccessStore
                    .getOwnerAccessControlEntry(_expectedOwnerAccessControlEntry.getUid(),
                                                _expectedOwnerAccessControlEntry.getDomain(),
                                                _expectedOwnerAccessControlEntry.getInterfaceName(),
                                                _expectedOwnerAccessControlEntry.getOperation())
                    .get();
    EXPECT_EQ(_expectedOwnerAccessControlEntry, ownerAceFromDb);
}

TEST_F(LocalDomainAccessStoreTest, removeOwnerAce)
{
    _localDomainAccessStore.updateOwnerAccessControlEntry(_expectedOwnerAccessControlEntry);
    // Remove the ACE
    EXPECT_TRUE(_localDomainAccessStore.removeOwnerAccessControlEntry(
            _expectedOwnerAccessControlEntry.getUid(),
            _expectedOwnerAccessControlEntry.getDomain(),
            _expectedOwnerAccessControlEntry.getInterfaceName(),
            _expectedOwnerAccessControlEntry.getOperation()));

    // Check the ACE does not exist
    std::vector<OwnerAccessControlEntry> ownerAces =
            _localDomainAccessStore.getOwnerAccessControlEntries(
                    _expectedOwnerAccessControlEntry.getUid(),
                    _expectedOwnerAccessControlEntry.getDomain(),
                    _expectedOwnerAccessControlEntry.getInterfaceName());
    EXPECT_TRUE(ownerAces.empty());
}

TEST_F(LocalDomainAccessStoreTest, updateOwnerRce_notAllowedByMasterRce)
{
    // test no Owner RCE allowed
    MasterRegistrationControlEntry testMasterRce =
            MasterRegistrationControlEntry(_TEST_USER1,
                                           _TEST_DOMAIN1,
                                           _TEST_INTERFACE1,
                                           TrustLevel::LOW,
                                           TRUST_LEVELS_EMPTY,
                                           TrustLevel::LOW,
                                           TRUST_LEVELS_EMPTY,
                                           Permission::NO,
                                           PERMISSIONS_EMPTY);
    OwnerRegistrationControlEntry testOwnerRce = OwnerRegistrationControlEntry(_TEST_USER1,
                                                                               _TEST_DOMAIN1,
                                                                               _TEST_INTERFACE1,
                                                                               TrustLevel::LOW,
                                                                               TrustLevel::LOW,
                                                                               Permission::NO);
    OwnerRegistrationControlEntry allowedOwnerRce = OwnerRegistrationControlEntry(_TEST_USER1,
                                                                                  _TEST_DOMAIN1,
                                                                                  _TEST_INTERFACE1,
                                                                                  TrustLevel::MID,
                                                                                  TrustLevel::HIGH,
                                                                                  Permission::ASK);

    EXPECT_TRUE(_localDomainAccessStore.updateMasterRegistrationControlEntry(testMasterRce));
    EXPECT_FALSE(_localDomainAccessStore.updateOwnerRegistrationControlEntry(testOwnerRce));
    EXPECT_FALSE(_localDomainAccessStore.updateOwnerRegistrationControlEntry(allowedOwnerRce));

    // test Owner RCE with required trust level LOW not allowed
    testMasterRce.setPossibleRequiredTrustLevels(TRUST_LEVELS_WITHOUT_LOW);
    testMasterRce.setPossibleRequiredControlEntryChangeTrustLevels(TRUST_LEVELS_ALL);
    testMasterRce.setPossibleProviderPermissions(PERMISSIONS_ALL);

    EXPECT_TRUE(_localDomainAccessStore.updateMasterRegistrationControlEntry(testMasterRce));
    EXPECT_FALSE(_localDomainAccessStore.updateOwnerRegistrationControlEntry(testOwnerRce));
    EXPECT_TRUE(_localDomainAccessStore.updateOwnerRegistrationControlEntry(allowedOwnerRce));

    // test Owner RCE with provider permission NO not allowed
    testMasterRce.setPossibleRequiredTrustLevels(TRUST_LEVELS_ALL);
    testMasterRce.setPossibleRequiredControlEntryChangeTrustLevels(TRUST_LEVELS_ALL);
    testMasterRce.setPossibleProviderPermissions(PERMISSIONS_WITHOUT_NO);

    EXPECT_TRUE(_localDomainAccessStore.updateMasterRegistrationControlEntry(testMasterRce));
    EXPECT_FALSE(_localDomainAccessStore.updateOwnerRegistrationControlEntry(testOwnerRce));
    EXPECT_TRUE(_localDomainAccessStore.updateOwnerRegistrationControlEntry(allowedOwnerRce));
}

TEST_F(LocalDomainAccessStoreTest, updateOwnerRce_notAllowedByMediatorRce)
{
    // test no Owner RCE allowed
    MasterRegistrationControlEntry testMediatorRce =
            MasterRegistrationControlEntry(_TEST_USER1,
                                           _TEST_DOMAIN1,
                                           _TEST_INTERFACE1,
                                           TrustLevel::LOW,
                                           TRUST_LEVELS_EMPTY,
                                           TrustLevel::LOW,
                                           TRUST_LEVELS_EMPTY,
                                           Permission::NO,
                                           PERMISSIONS_EMPTY);
    OwnerRegistrationControlEntry testOwnerRce = OwnerRegistrationControlEntry(_TEST_USER1,
                                                                               _TEST_DOMAIN1,
                                                                               _TEST_INTERFACE1,
                                                                               TrustLevel::LOW,
                                                                               TrustLevel::LOW,
                                                                               Permission::NO);
    OwnerRegistrationControlEntry allowedOwnerRce = OwnerRegistrationControlEntry(_TEST_USER1,
                                                                                  _TEST_DOMAIN1,
                                                                                  _TEST_INTERFACE1,
                                                                                  TrustLevel::MID,
                                                                                  TrustLevel::HIGH,
                                                                                  Permission::ASK);

    EXPECT_TRUE(_localDomainAccessStore.updateMediatorRegistrationControlEntry(testMediatorRce));
    EXPECT_FALSE(_localDomainAccessStore.updateOwnerRegistrationControlEntry(testOwnerRce));
    EXPECT_FALSE(_localDomainAccessStore.updateOwnerRegistrationControlEntry(allowedOwnerRce));

    // test Owner RCE with required trust level LOW not allowed
    testMediatorRce.setPossibleRequiredTrustLevels(TRUST_LEVELS_WITHOUT_LOW);
    testMediatorRce.setPossibleRequiredControlEntryChangeTrustLevels(TRUST_LEVELS_ALL);
    testMediatorRce.setPossibleProviderPermissions(PERMISSIONS_ALL);

    EXPECT_TRUE(_localDomainAccessStore.updateMediatorRegistrationControlEntry(testMediatorRce));
    EXPECT_FALSE(_localDomainAccessStore.updateOwnerRegistrationControlEntry(testOwnerRce));
    EXPECT_TRUE(_localDomainAccessStore.updateOwnerRegistrationControlEntry(allowedOwnerRce));

    // test Owner RCE with provider permission NO not allowed
    testMediatorRce.setPossibleRequiredTrustLevels(TRUST_LEVELS_ALL);
    testMediatorRce.setPossibleRequiredControlEntryChangeTrustLevels(TRUST_LEVELS_ALL);
    testMediatorRce.setPossibleProviderPermissions(PERMISSIONS_WITHOUT_NO);

    EXPECT_TRUE(_localDomainAccessStore.updateMediatorRegistrationControlEntry(testMediatorRce));
    EXPECT_FALSE(_localDomainAccessStore.updateOwnerRegistrationControlEntry(testOwnerRce));
    EXPECT_TRUE(_localDomainAccessStore.updateOwnerRegistrationControlEntry(allowedOwnerRce));
}
TEST_F(LocalDomainAccessStoreTest, updateMediatorRce_notAllowedByMasterRce)
{
    // test no Mediator RCE allowed
    MasterRegistrationControlEntry testMasterRce =
            MasterRegistrationControlEntry(_TEST_USER1,
                                           _TEST_DOMAIN1,
                                           _TEST_INTERFACE1,
                                           TrustLevel::LOW,
                                           TRUST_LEVELS_EMPTY,
                                           TrustLevel::LOW,
                                           TRUST_LEVELS_EMPTY,
                                           Permission::NO,
                                           PERMISSIONS_EMPTY);
    MasterRegistrationControlEntry testMediatorRce =
            MasterRegistrationControlEntry(_TEST_USER1,
                                           _TEST_DOMAIN1,
                                           _TEST_INTERFACE1,
                                           TrustLevel::LOW,
                                           TRUST_LEVELS_EMPTY,
                                           TrustLevel::LOW,
                                           TRUST_LEVELS_EMPTY,
                                           Permission::NO,
                                           PERMISSIONS_EMPTY);
    MasterRegistrationControlEntry allowedMediatorRce =
            MasterRegistrationControlEntry(_TEST_USER1,
                                           _TEST_DOMAIN1,
                                           _TEST_INTERFACE1,
                                           TrustLevel::MID,
                                           TRUST_LEVELS_WITHOUT_LOW,
                                           TrustLevel::HIGH,
                                           TRUST_LEVELS_WITHOUT_LOW,
                                           Permission::ASK,
                                           PERMISSIONS_WITHOUT_NO);

    EXPECT_TRUE(_localDomainAccessStore.updateMasterRegistrationControlEntry(testMasterRce));
    EXPECT_FALSE(_localDomainAccessStore.updateMediatorRegistrationControlEntry(testMediatorRce));
    EXPECT_FALSE(
            _localDomainAccessStore.updateMediatorRegistrationControlEntry(allowedMediatorRce));

    // test Mediator RCE with required trust level LOW
    // as default required trust level not allowed
    testMasterRce.setPossibleRequiredTrustLevels(TRUST_LEVELS_WITHOUT_LOW);
    testMasterRce.setPossibleRequiredControlEntryChangeTrustLevels(TRUST_LEVELS_ALL);
    testMasterRce.setPossibleProviderPermissions(PERMISSIONS_ALL);

    EXPECT_TRUE(_localDomainAccessStore.updateMasterRegistrationControlEntry(testMasterRce));
    EXPECT_FALSE(_localDomainAccessStore.updateMediatorRegistrationControlEntry(testMediatorRce));
    EXPECT_TRUE(_localDomainAccessStore.updateMediatorRegistrationControlEntry(allowedMediatorRce));

    // test Mediator RCE with provider permission NO
    // in default provider permission not allowed
    testMasterRce.setPossibleRequiredTrustLevels(TRUST_LEVELS_ALL);
    testMasterRce.setPossibleRequiredControlEntryChangeTrustLevels(TRUST_LEVELS_ALL);
    testMasterRce.setPossibleProviderPermissions(PERMISSIONS_WITHOUT_NO);

    EXPECT_TRUE(_localDomainAccessStore.updateMasterRegistrationControlEntry(testMasterRce));
    EXPECT_FALSE(_localDomainAccessStore.updateMediatorRegistrationControlEntry(testMediatorRce));
    EXPECT_TRUE(_localDomainAccessStore.updateMediatorRegistrationControlEntry(allowedMediatorRce));

    // test Mediator RCE with required trust level NONE
    // in possible required trust levels not allowed
    testMediatorRce.setPossibleRequiredTrustLevels(TRUST_LEVELS_WITHOUT_LOW);
    allowedMediatorRce.setPossibleRequiredTrustLevels(TRUST_LEVELS_WITHOUT_NONE);

    testMasterRce.setPossibleRequiredTrustLevels(TRUST_LEVELS_WITHOUT_NONE);
    testMasterRce.setPossibleRequiredControlEntryChangeTrustLevels(TRUST_LEVELS_ALL);
    testMasterRce.setPossibleProviderPermissions(PERMISSIONS_ALL);

    EXPECT_TRUE(_localDomainAccessStore.updateMasterRegistrationControlEntry(testMasterRce));
    EXPECT_FALSE(_localDomainAccessStore.updateMediatorRegistrationControlEntry(testMediatorRce));
    EXPECT_TRUE(_localDomainAccessStore.updateMediatorRegistrationControlEntry(allowedMediatorRce));
    testMediatorRce.setPossibleRequiredTrustLevels(TRUST_LEVELS_EMPTY);
    allowedMediatorRce.setPossibleRequiredTrustLevels(TRUST_LEVELS_WITHOUT_LOW);

    // test Mediator RCE with provider permission NO
    // in possible provider permissions not allowed
    testMediatorRce.setDefaultProviderPermission(Permission::ASK);
    testMediatorRce.setPossibleProviderPermissions(PERMISSIONS_WITHOUT_YES);

    testMasterRce.setPossibleRequiredTrustLevels(TRUST_LEVELS_ALL);
    testMasterRce.setPossibleRequiredControlEntryChangeTrustLevels(TRUST_LEVELS_ALL);
    testMasterRce.setPossibleProviderPermissions(PERMISSIONS_WITHOUT_NO);

    EXPECT_TRUE(_localDomainAccessStore.updateMasterRegistrationControlEntry(testMasterRce));
    EXPECT_FALSE(_localDomainAccessStore.updateMediatorRegistrationControlEntry(testMediatorRce));
    EXPECT_TRUE(_localDomainAccessStore.updateMediatorRegistrationControlEntry(allowedMediatorRce));
}

TEST_F(LocalDomainAccessStoreTest, updateOwnerAce_notAllowedByMasterAce)
{
    // test no Owner ACE allowed
    MasterAccessControlEntry testMasterAce = MasterAccessControlEntry(_TEST_USER1,
                                                                      _TEST_DOMAIN1,
                                                                      _TEST_INTERFACE1,
                                                                      TrustLevel::LOW,
                                                                      TRUST_LEVELS_EMPTY,
                                                                      TrustLevel::LOW,
                                                                      TRUST_LEVELS_EMPTY,
                                                                      _TEST_OPERATION1,
                                                                      Permission::NO,
                                                                      PERMISSIONS_EMPTY);
    OwnerAccessControlEntry testOwnerAce = OwnerAccessControlEntry(_TEST_USER1,
                                                                   _TEST_DOMAIN1,
                                                                   _TEST_INTERFACE1,
                                                                   TrustLevel::LOW,
                                                                   TrustLevel::LOW,
                                                                   _TEST_OPERATION1,
                                                                   Permission::NO);
    OwnerAccessControlEntry allowedOwnerAce = OwnerAccessControlEntry(_TEST_USER1,
                                                                      _TEST_DOMAIN1,
                                                                      _TEST_INTERFACE1,
                                                                      TrustLevel::MID,
                                                                      TrustLevel::HIGH,
                                                                      _TEST_OPERATION1,
                                                                      Permission::ASK);

    EXPECT_TRUE(_localDomainAccessStore.updateMasterAccessControlEntry(testMasterAce));
    EXPECT_FALSE(_localDomainAccessStore.updateOwnerAccessControlEntry(testOwnerAce));
    EXPECT_FALSE(_localDomainAccessStore.updateOwnerAccessControlEntry(allowedOwnerAce));

    // test Owner ACE with required trust level LOW not allowed
    testMasterAce.setPossibleRequiredTrustLevels(TRUST_LEVELS_WITHOUT_LOW);
    testMasterAce.setPossibleRequiredControlEntryChangeTrustLevels(TRUST_LEVELS_ALL);
    testMasterAce.setPossibleConsumerPermissions(PERMISSIONS_ALL);

    EXPECT_TRUE(_localDomainAccessStore.updateMasterAccessControlEntry(testMasterAce));
    EXPECT_FALSE(_localDomainAccessStore.updateOwnerAccessControlEntry(testOwnerAce));
    EXPECT_TRUE(_localDomainAccessStore.updateOwnerAccessControlEntry(allowedOwnerAce));

    // test Owner ACE with Consumer permission NO not allowed
    testMasterAce.setPossibleRequiredTrustLevels(TRUST_LEVELS_ALL);
    testMasterAce.setPossibleRequiredControlEntryChangeTrustLevels(TRUST_LEVELS_ALL);
    testMasterAce.setPossibleConsumerPermissions(PERMISSIONS_WITHOUT_NO);

    EXPECT_TRUE(_localDomainAccessStore.updateMasterAccessControlEntry(testMasterAce));
    EXPECT_FALSE(_localDomainAccessStore.updateOwnerAccessControlEntry(testOwnerAce));
    EXPECT_TRUE(_localDomainAccessStore.updateOwnerAccessControlEntry(allowedOwnerAce));
}

TEST_F(LocalDomainAccessStoreTest, updateOwnerAce_notAllowedByMediatorAce)
{
    // test no Owner ACE allowed
    MasterAccessControlEntry testMediatorAce = MasterAccessControlEntry(_TEST_USER1,
                                                                        _TEST_DOMAIN1,
                                                                        _TEST_INTERFACE1,
                                                                        TrustLevel::LOW,
                                                                        TRUST_LEVELS_EMPTY,
                                                                        TrustLevel::LOW,
                                                                        TRUST_LEVELS_EMPTY,
                                                                        _TEST_OPERATION1,
                                                                        Permission::NO,
                                                                        PERMISSIONS_EMPTY);
    OwnerAccessControlEntry testOwnerAce = OwnerAccessControlEntry(_TEST_USER1,
                                                                   _TEST_DOMAIN1,
                                                                   _TEST_INTERFACE1,
                                                                   TrustLevel::LOW,
                                                                   TrustLevel::LOW,
                                                                   _TEST_OPERATION1,
                                                                   Permission::NO);
    OwnerAccessControlEntry allowedOwnerAce = OwnerAccessControlEntry(_TEST_USER1,
                                                                      _TEST_DOMAIN1,
                                                                      _TEST_INTERFACE1,
                                                                      TrustLevel::MID,
                                                                      TrustLevel::HIGH,
                                                                      _TEST_OPERATION1,
                                                                      Permission::ASK);

    EXPECT_TRUE(_localDomainAccessStore.updateMediatorAccessControlEntry(testMediatorAce));
    EXPECT_FALSE(_localDomainAccessStore.updateOwnerAccessControlEntry(testOwnerAce));
    EXPECT_FALSE(_localDomainAccessStore.updateOwnerAccessControlEntry(allowedOwnerAce));

    // test Owner ACE with required trust level LOW not allowed
    testMediatorAce.setPossibleRequiredTrustLevels(TRUST_LEVELS_WITHOUT_LOW);
    testMediatorAce.setPossibleRequiredControlEntryChangeTrustLevels(TRUST_LEVELS_ALL);
    testMediatorAce.setPossibleConsumerPermissions(PERMISSIONS_ALL);

    EXPECT_TRUE(_localDomainAccessStore.updateMediatorAccessControlEntry(testMediatorAce));
    EXPECT_FALSE(_localDomainAccessStore.updateOwnerAccessControlEntry(testOwnerAce));
    EXPECT_TRUE(_localDomainAccessStore.updateOwnerAccessControlEntry(allowedOwnerAce));

    // test Owner ACE with Consumer permission NO not allowed
    testMediatorAce.setPossibleRequiredTrustLevels(TRUST_LEVELS_ALL);
    testMediatorAce.setPossibleRequiredControlEntryChangeTrustLevels(TRUST_LEVELS_ALL);
    testMediatorAce.setPossibleConsumerPermissions(PERMISSIONS_WITHOUT_NO);

    EXPECT_TRUE(_localDomainAccessStore.updateMediatorAccessControlEntry(testMediatorAce));
    EXPECT_FALSE(_localDomainAccessStore.updateOwnerAccessControlEntry(testOwnerAce));
    EXPECT_TRUE(_localDomainAccessStore.updateOwnerAccessControlEntry(allowedOwnerAce));
}
TEST_F(LocalDomainAccessStoreTest, updateMediatorAce_notAllowedByMasterAce)
{
    // test no Mediator ACE allowed
    MasterAccessControlEntry testMasterAce = MasterAccessControlEntry(_TEST_USER1,
                                                                      _TEST_DOMAIN1,
                                                                      _TEST_INTERFACE1,
                                                                      TrustLevel::LOW,
                                                                      TRUST_LEVELS_EMPTY,
                                                                      TrustLevel::LOW,
                                                                      TRUST_LEVELS_EMPTY,
                                                                      _TEST_OPERATION1,
                                                                      Permission::NO,
                                                                      PERMISSIONS_EMPTY);
    MasterAccessControlEntry testMediatorAce = MasterAccessControlEntry(_TEST_USER1,
                                                                        _TEST_DOMAIN1,
                                                                        _TEST_INTERFACE1,
                                                                        TrustLevel::LOW,
                                                                        TRUST_LEVELS_EMPTY,
                                                                        TrustLevel::LOW,
                                                                        TRUST_LEVELS_EMPTY,
                                                                        _TEST_OPERATION1,
                                                                        Permission::NO,
                                                                        PERMISSIONS_EMPTY);
    MasterAccessControlEntry allowedMediatorAce = MasterAccessControlEntry(_TEST_USER1,
                                                                           _TEST_DOMAIN1,
                                                                           _TEST_INTERFACE1,
                                                                           TrustLevel::MID,
                                                                           TRUST_LEVELS_WITHOUT_LOW,
                                                                           TrustLevel::HIGH,
                                                                           TRUST_LEVELS_WITHOUT_LOW,
                                                                           _TEST_OPERATION1,
                                                                           Permission::ASK,
                                                                           PERMISSIONS_WITHOUT_NO);

    EXPECT_TRUE(_localDomainAccessStore.updateMasterAccessControlEntry(testMasterAce));
    EXPECT_FALSE(_localDomainAccessStore.updateMediatorAccessControlEntry(testMediatorAce));
    EXPECT_FALSE(_localDomainAccessStore.updateMediatorAccessControlEntry(allowedMediatorAce));

    // test Mediator ACE with required trust level LOW
    // as default required trust level not allowed
    testMasterAce.setPossibleRequiredTrustLevels(TRUST_LEVELS_WITHOUT_LOW);
    testMasterAce.setPossibleRequiredControlEntryChangeTrustLevels(TRUST_LEVELS_ALL);
    testMasterAce.setPossibleConsumerPermissions(PERMISSIONS_ALL);

    EXPECT_TRUE(_localDomainAccessStore.updateMasterAccessControlEntry(testMasterAce));
    EXPECT_FALSE(_localDomainAccessStore.updateMediatorAccessControlEntry(testMediatorAce));
    EXPECT_TRUE(_localDomainAccessStore.updateMediatorAccessControlEntry(allowedMediatorAce));

    // test Mediator ACE with consumer permission NO
    // as default consumer permission not allowed
    testMasterAce.setPossibleRequiredTrustLevels(TRUST_LEVELS_ALL);
    testMasterAce.setPossibleRequiredControlEntryChangeTrustLevels(TRUST_LEVELS_ALL);
    testMasterAce.setPossibleConsumerPermissions(PERMISSIONS_WITHOUT_NO);

    EXPECT_TRUE(_localDomainAccessStore.updateMasterAccessControlEntry(testMasterAce));
    EXPECT_FALSE(_localDomainAccessStore.updateMediatorAccessControlEntry(testMediatorAce));
    EXPECT_TRUE(_localDomainAccessStore.updateMediatorAccessControlEntry(allowedMediatorAce));

    // test Mediator ACE with required trust level NONE
    // in possible required trust levels not allowed
    testMediatorAce.setPossibleRequiredTrustLevels(TRUST_LEVELS_WITHOUT_LOW);
    allowedMediatorAce.setPossibleRequiredTrustLevels(TRUST_LEVELS_WITHOUT_NONE);

    testMasterAce.setPossibleRequiredTrustLevels(TRUST_LEVELS_WITHOUT_NONE);
    testMasterAce.setPossibleRequiredControlEntryChangeTrustLevels(TRUST_LEVELS_ALL);
    testMasterAce.setPossibleConsumerPermissions(PERMISSIONS_ALL);

    EXPECT_TRUE(_localDomainAccessStore.updateMasterAccessControlEntry(testMasterAce));
    EXPECT_FALSE(_localDomainAccessStore.updateMediatorAccessControlEntry(testMediatorAce));
    EXPECT_TRUE(_localDomainAccessStore.updateMediatorAccessControlEntry(allowedMediatorAce));
    testMediatorAce.setPossibleRequiredTrustLevels(TRUST_LEVELS_EMPTY);
    allowedMediatorAce.setPossibleRequiredTrustLevels(TRUST_LEVELS_WITHOUT_LOW);

    // test Mediator ACE with consumer permission NO
    // in possible consumer permissions not allowed
    testMediatorAce.setDefaultConsumerPermission(Permission::ASK);
    testMediatorAce.setPossibleConsumerPermissions(PERMISSIONS_WITHOUT_YES);

    testMasterAce.setPossibleRequiredTrustLevels(TRUST_LEVELS_ALL);
    testMasterAce.setPossibleRequiredControlEntryChangeTrustLevels(TRUST_LEVELS_ALL);
    testMasterAce.setPossibleConsumerPermissions(PERMISSIONS_WITHOUT_NO);

    EXPECT_TRUE(_localDomainAccessStore.updateMasterAccessControlEntry(testMasterAce));
    EXPECT_FALSE(_localDomainAccessStore.updateMediatorAccessControlEntry(testMediatorAce));
    EXPECT_TRUE(_localDomainAccessStore.updateMediatorAccessControlEntry(allowedMediatorAce));
}

TEST_F(LocalDomainAccessStoreTest, restoreFromPersistenceFile)
{

    const std::string masterACEinterfaceName = "this/is/a/test/interface";
    _expectedMasterAccessControlEntry.setInterfaceName(masterACEinterfaceName);

    {
        LocalDomainAccessStore localDomainAccessStore(
                ClusterControllerSettings::
                        DEFAULT_LOCAL_DOMAIN_ACCESS_STORE_PERSISTENCE_FILENAME());
        localDomainAccessStore.updateOwnerAccessControlEntry(_expectedOwnerAccessControlEntry);
        localDomainAccessStore.updateMasterAccessControlEntry(_expectedMasterAccessControlEntry);
    }

    {
        LocalDomainAccessStore localDomainAccessStore(
                ClusterControllerSettings::
                        DEFAULT_LOCAL_DOMAIN_ACCESS_STORE_PERSISTENCE_FILENAME());
        EXPECT_EQ(_expectedOwnerAccessControlEntry,
                  localDomainAccessStore
                          .getOwnerAccessControlEntry(
                                  _expectedOwnerAccessControlEntry.getUid(),
                                  _expectedOwnerAccessControlEntry.getDomain(),
                                  _expectedOwnerAccessControlEntry.getInterfaceName(),
                                  _expectedOwnerAccessControlEntry.getOperation())
                          .get());

        EXPECT_EQ(_expectedMasterAccessControlEntry,
                  localDomainAccessStore
                          .getMasterAccessControlEntry(
                                  _expectedMasterAccessControlEntry.getUid(),
                                  _expectedMasterAccessControlEntry.getDomain(),
                                  _expectedMasterAccessControlEntry.getInterfaceName(),
                                  _expectedMasterAccessControlEntry.getOperation())
                          .get());
    }
}

TEST_F(LocalDomainAccessStoreTest, doesNotContainOnlyWildcardOperations)
{

    // add a wildcard and a non-wildcard operation
    _expectedOwnerAccessControlEntry.setOperation(access_control::WILDCARD);
    _localDomainAccessStore.updateOwnerAccessControlEntry(_expectedOwnerAccessControlEntry);
    _expectedOwnerAccessControlEntry.setOperation(_TEST_OPERATION1);
    _localDomainAccessStore.updateOwnerAccessControlEntry(_expectedOwnerAccessControlEntry);

    // still return false
    EXPECT_FALSE(_localDomainAccessStore.onlyWildcardOperations(
            _TEST_USER1, _TEST_DOMAIN1, _TEST_INTERFACE1));
}

TEST_F(LocalDomainAccessStoreTest, containsOnlyWildcardOperations)
{
    // Test on empty access store
    EXPECT_TRUE(_localDomainAccessStore.onlyWildcardOperations(
            _TEST_USER1, _TEST_DOMAIN1, _TEST_INTERFACE1));

    // add entries with wildcard operation
    _expectedOwnerAccessControlEntry.setOperation(access_control::WILDCARD);
    _expectedMasterAccessControlEntry.setOperation(access_control::WILDCARD);
    _localDomainAccessStore.updateOwnerAccessControlEntry(_expectedOwnerAccessControlEntry);
    _localDomainAccessStore.updateMasterAccessControlEntry(_expectedMasterAccessControlEntry);

    // still return true
    EXPECT_TRUE(_localDomainAccessStore.onlyWildcardOperations(
            _TEST_USER1, _TEST_DOMAIN1, _TEST_INTERFACE1));
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
TEST_F(LocalDomainAccessStoreTest, getDomainAndInterfaceWithWildcard)
{
    // ***
    // Prepare LocalDomainAccessStore
    // ***
    const std::vector<AccessStoreTestData> accessStoreData = {
            //
            //           UID                   DOMAIN     INTERFACE
            //
            {_TEST_USER1, "domain", "interfaceName"},
            {_TEST_USER1, "domain", "interface*"},
            {_TEST_USER1, "dom*", "interfaceName"},
            {_TEST_USER1, "dom*", "interface*"},
            {joynr::access_control::WILDCARD, "domain", "interfaceName"},
            {joynr::access_control::WILDCARD, "domain", "interface*"},
            {joynr::access_control::WILDCARD, "dom*", "interfaceName"},
            {joynr::access_control::WILDCARD, "dom*", "interface*"},
            {joynr::access_control::WILDCARD, "*", "*"}};

    // add data to LocalDomainAccessStore
    for (const auto& entry : accessStoreData) {
        MasterAccessControlEntry masterACE(entry.userId,
                                           entry.domain,
                                           entry.interfaceName,
                                           TrustLevel::LOW,
                                           _TRUST_LEVELS,
                                           TrustLevel::LOW,
                                           _TRUST_LEVELS,
                                           _TEST_OPERATION1,
                                           Permission::NO,
                                           _PERMISSIONS);
        _localDomainAccessStore.updateMasterAccessControlEntry(masterACE);
    };

    // ***
    // Query storage and verify result:
    // the match should always return the most specific entry from the access store.
    // ***

    // EXACT MATCH
    // In the access store there is an entry as from the query
    queryAccessStoreAndVerifyOutput(_TEST_USER1, "domain", "interfaceName", accessStoreData[0]);

    // MATCH INTERFACE WILDCARD
    queryAccessStoreAndVerifyOutput(_TEST_USER1, "domain", "interface1", accessStoreData[1]);

    // MATCH DOMAIN WILDCARD
    queryAccessStoreAndVerifyOutput(_TEST_USER1, "dom1", "interfaceName", accessStoreData[2]);

    // MATCH DOMAIN AND INTERFACE WITH WILDCARD
    queryAccessStoreAndVerifyOutput(_TEST_USER1, "dom1", "interface1", accessStoreData[3]);

    // MATCH UID WILDCARD
    queryAccessStoreAndVerifyOutput(_TEST_USER2, "domain", "interfaceName", accessStoreData[4]);

    // MATCH UID AND INTERFACE WITH WILDCARD
    queryAccessStoreAndVerifyOutput(_TEST_USER2, "domain", "interface1", accessStoreData[5]);

    // MATCH UID AND DOMAIN WITH WILDCARD
    queryAccessStoreAndVerifyOutput(_TEST_USER2, "dom1", "interfaceName", accessStoreData[6]);

    // MATCH UID, DOMAIN AND INTERFACE WITH WILDCARD
    queryAccessStoreAndVerifyOutput(_TEST_USER2, "dom1", "interface1", accessStoreData[7]);
}

TEST_F(LocalDomainAccessStoreTest, allowEverything)
{
    // add data to LocalDomainAccessStore
    const MasterAccessControlEntry masterACE(joynr::access_control::WILDCARD,
                                             joynr::access_control::WILDCARD,
                                             joynr::access_control::WILDCARD,
                                             TrustLevel::LOW,
                                             _TRUST_LEVELS,
                                             TrustLevel::LOW,
                                             _TRUST_LEVELS,
                                             _TEST_OPERATION1,
                                             Permission::NO,
                                             _PERMISSIONS);
    _localDomainAccessStore.updateMasterAccessControlEntry(masterACE);

    const AccessStoreTestData expectedData = {joynr::access_control::WILDCARD,
                                              joynr::access_control::WILDCARD,
                                              joynr::access_control::WILDCARD};

    // MATCH ANYTHING
    queryAccessStoreAndVerifyOutput(
            std::string("joynr"), std::string("acl"), std::string("wildcards"), expectedData);
}

TEST_F(LocalDomainAccessStoreTest, denyEverything)
{
    // do not add anything to the store
    // try to perform a query
    auto result = _localDomainAccessStore.getMasterAccessControlEntry(
            "user", "domain", "interfaceName", joynr::access_control::WILDCARD);
    ASSERT_FALSE(result);
}

TEST_F(LocalDomainAccessStoreTest, loadTwoIdenticalTemplatesWithDifferentUserId)
{
    // load both templates
    _localDomainAccessStore.mergeDomainAccessStore(
            LocalDomainAccessStore("test-resources/application1_ACL_RCL_Permissions.json"));
    _localDomainAccessStore.mergeDomainAccessStore(
            LocalDomainAccessStore("test-resources/application2_ACL_RCL_Permissions.json"));

    // verify both userId have rights as defined in the permission file
    auto result = _localDomainAccessStore.getMasterAccessControlEntry(
            "application_1", "domain", "interfaceName", joynr::access_control::WILDCARD);
    EXPECT_TRUE(result);
    EXPECT_EQ(result->getUid(), "application_1");
    EXPECT_EQ(result->getDomain(), "*");
    EXPECT_EQ(result->getInterfaceName(), "*");
    EXPECT_EQ(result->getDefaultConsumerPermission(),
              joynr::infrastructure::DacTypes::Permission::YES);

    result = _localDomainAccessStore.getMasterAccessControlEntry(
            "application_2", "domain", "interfaceName", joynr::access_control::WILDCARD);
    EXPECT_TRUE(result);
    EXPECT_EQ(result->getUid(), "application_2");
    EXPECT_EQ(result->getDomain(), "*");
    EXPECT_EQ(result->getInterfaceName(), "*");
    EXPECT_EQ(result->getDefaultConsumerPermission(),
              joynr::infrastructure::DacTypes::Permission::YES);
}

TEST_F(LocalDomainAccessStoreTest,
       loadTwoIdenticalTemplatesWithDifferentInterfacesWithDomainAndInterfaceWildcardEntries)
{
    queryAccessStoreAndVerifyOutputForTwoIdenticalTemplatesWithDifferentUserIdTests(
            "test-resources/application3_ACL_RCL_Permissions.json");
}

TEST_F(LocalDomainAccessStoreTest,
       loadTwoIdenticalTemplatesWithDifferentInterfacesWithDomainWildcardEntries)
{
    queryAccessStoreAndVerifyOutputForTwoIdenticalTemplatesWithDifferentUserIdTests(
            "test-resources/application4_ACL_RCL_Permissions.json");
}

TEST_F(LocalDomainAccessStoreTest,
       loadTwoIdenticalTemplatesWithDifferentDomainsWithoutDomainWildcardEntries)
{
    _localDomainAccessStore.mergeDomainAccessStore(
            LocalDomainAccessStore("test-resources/application5_ACL_RCL_Permissions.json"));
    _localDomainAccessStore.logContent();

    auto result =
            _localDomainAccessStore.getMasterAccessControlEntry("application_1",
                                                                "common.prefix.domain1",
                                                                "same/interface/prefix/value",
                                                                joynr::access_control::WILDCARD);
    ASSERT_TRUE(result);
    EXPECT_EQ(result->getUid(), "application_1");
    EXPECT_EQ(result->getDomain(), "common.prefix.domain1");
    EXPECT_EQ(result->getInterfaceName(), "same/interface/prefix/*");
    EXPECT_EQ(result->getDefaultConsumerPermission(),
              joynr::infrastructure::DacTypes::Permission::YES);

    auto result2 =
            _localDomainAccessStore.getMasterAccessControlEntry("application_1",
                                                                "common.prefix.domain2",
                                                                "same/interface/prefix/value",
                                                                joynr::access_control::WILDCARD);
    ASSERT_TRUE(result2);
    EXPECT_EQ(result2->getUid(), "application_1");
    EXPECT_EQ(result2->getDomain(), "common.prefix.domain2");
    EXPECT_EQ(result2->getInterfaceName(), "same/interface/prefix/*");
    EXPECT_EQ(result2->getDefaultConsumerPermission(),
              joynr::infrastructure::DacTypes::Permission::YES);
}
