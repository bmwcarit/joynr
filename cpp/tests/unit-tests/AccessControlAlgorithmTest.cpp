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

#include "libjoynrclustercontroller/access-control/AccessControlAlgorithm.h"
#include "joynr/PrivateCopyAssign.h"
#include "tests/utils/Gtest.h"

#include "joynr/infrastructure/DacTypes/MasterAccessControlEntry.h"
#include "joynr/infrastructure/DacTypes/OwnerAccessControlEntry.h"
#include "joynr/infrastructure/DacTypes/Permission.h"
#include "joynr/infrastructure/DacTypes/TrustLevel.h"

using namespace ::testing;
using namespace joynr;
using namespace joynr::infrastructure::DacTypes;

class AccessControlAlgorithmTest : public ::testing::Test
{
public:
    AccessControlAlgorithmTest() : accessControlAlgorithm()
    {
    }

    ~AccessControlAlgorithmTest() = default;

    void SetUp()
    {
        masterAce = MasterAccessControlEntry(TEST_USER,
                                             TEST_DOMAIN1,
                                             TEST_INTERFACE1,
                                             TrustLevel::LOW,
                                             ALL_TRUST_LEVELS,
                                             TrustLevel::LOW,
                                             ALL_TRUST_LEVELS,
                                             std::string(),
                                             Permission::NO,
                                             ALL_PERMISSIONS);

        mediatorAce = MasterAccessControlEntry(TEST_USER,
                                               TEST_DOMAIN1,
                                               TEST_INTERFACE1,
                                               TrustLevel::LOW,
                                               ALL_TRUST_LEVELS,
                                               TrustLevel::LOW,
                                               ALL_TRUST_LEVELS,
                                               std::string(),
                                               Permission::NO,
                                               ALL_PERMISSIONS);

        ownerAce = OwnerAccessControlEntry(TEST_USER,
                                           TEST_DOMAIN1,
                                           TEST_INTERFACE1,
                                           TrustLevel::LOW,
                                           TrustLevel::LOW,
                                           std::string(),
                                           Permission::NO);
    }

    static const std::vector<TrustLevel::Enum> ALL_TRUST_LEVELS;
    static const std::vector<Permission::Enum> ALL_PERMISSIONS;

protected:
    AccessControlAlgorithm accessControlAlgorithm;
    MasterAccessControlEntry masterAce;
    MasterAccessControlEntry mediatorAce;
    OwnerAccessControlEntry ownerAce;
    static const std::string TEST_USER;
    static const std::string TEST_DOMAIN1;
    static const std::string TEST_INTERFACE1;

private:
    DISALLOW_COPY_AND_ASSIGN(AccessControlAlgorithmTest);
};

const std::string AccessControlAlgorithmTest::TEST_USER("testUser");
const std::string AccessControlAlgorithmTest::TEST_DOMAIN1("domain1");
const std::string AccessControlAlgorithmTest::TEST_INTERFACE1("interface1");
const std::vector<TrustLevel::Enum> AccessControlAlgorithmTest::ALL_TRUST_LEVELS = {
        TrustLevel::LOW, TrustLevel::MID, TrustLevel::HIGH};
const std::vector<Permission::Enum> AccessControlAlgorithmTest::ALL_PERMISSIONS = {
        Permission::NO, Permission::ASK, Permission::YES};

TEST_F(AccessControlAlgorithmTest, permissionWithMasterAceOnly)
{
    masterAce.setDefaultConsumerPermission(Permission::YES);
    masterAce.setDefaultRequiredTrustLevel(TrustLevel::HIGH);
    Permission::Enum consumerPermission = accessControlAlgorithm.getConsumerPermission(
            masterAce,
            boost::optional<MasterAccessControlEntry>(),
            boost::optional<OwnerAccessControlEntry>(),
            TrustLevel::HIGH);

    EXPECT_EQ(Permission::YES, consumerPermission);
}

TEST_F(AccessControlAlgorithmTest, permissionMessageTrustLevelDoesntMatchAce)
{
    masterAce.setDefaultConsumerPermission(Permission::YES);
    masterAce.setDefaultRequiredTrustLevel(TrustLevel::MID);
    Permission::Enum consumerPermission = accessControlAlgorithm.getConsumerPermission(
            masterAce,
            boost::optional<MasterAccessControlEntry>(),
            boost::optional<OwnerAccessControlEntry>(),
            TrustLevel::LOW);

    EXPECT_EQ(Permission::NO, consumerPermission);
}

TEST_F(AccessControlAlgorithmTest, permissionWithAllAceNull)
{

    Permission::Enum consumerPermission = accessControlAlgorithm.getConsumerPermission(
            boost::optional<MasterAccessControlEntry>(),
            boost::optional<MasterAccessControlEntry>(),
            boost::optional<OwnerAccessControlEntry>(),
            TrustLevel::HIGH);

    EXPECT_EQ(Permission::NO, consumerPermission);
}

TEST_F(AccessControlAlgorithmTest, permissionWithMasterAndMediatorAce)
{
    masterAce.setDefaultConsumerPermission(Permission::YES);
    masterAce.setDefaultRequiredTrustLevel(TrustLevel::HIGH);
    masterAce.setPossibleConsumerPermissions(ALL_PERMISSIONS);
    masterAce.setPossibleRequiredTrustLevels(ALL_TRUST_LEVELS);

    mediatorAce.setDefaultConsumerPermission(Permission::ASK);
    mediatorAce.setDefaultRequiredTrustLevel(TrustLevel::LOW);

    Permission::Enum consumerPermission = accessControlAlgorithm.getConsumerPermission(
            masterAce, mediatorAce, boost::optional<OwnerAccessControlEntry>(), TrustLevel::LOW);

    EXPECT_EQ(Permission::ASK, consumerPermission);
}

TEST_F(AccessControlAlgorithmTest, permissionWithMediatorOnly)
{
    mediatorAce.setDefaultConsumerPermission(Permission::YES);
    mediatorAce.setDefaultRequiredTrustLevel(TrustLevel::MID);
    Permission::Enum consumerPermission = accessControlAlgorithm.getConsumerPermission(
            boost::optional<MasterAccessControlEntry>(),
            mediatorAce,
            boost::optional<OwnerAccessControlEntry>(),
            TrustLevel::HIGH);

    EXPECT_EQ(Permission::YES, consumerPermission);
}

TEST_F(AccessControlAlgorithmTest, permissionWithMasterAndInvalidMediatorAce)
{
    std::vector<Permission::Enum> possibleMasterConsumerPermissions;
    possibleMasterConsumerPermissions.push_back(Permission::NO);
    masterAce.setPossibleConsumerPermissions(possibleMasterConsumerPermissions);

    std::vector<Permission::Enum> possibleMediatorConsumerPermissions;
    possibleMediatorConsumerPermissions.push_back(Permission::ASK);
    possibleMediatorConsumerPermissions.push_back(Permission::YES);
    mediatorAce.setPossibleConsumerPermissions(possibleMediatorConsumerPermissions);
    mediatorAce.setDefaultConsumerPermission(Permission::YES);
    mediatorAce.setDefaultRequiredTrustLevel(TrustLevel::MID);

    Permission::Enum consumerPermission = accessControlAlgorithm.getConsumerPermission(
            masterAce, mediatorAce, boost::optional<OwnerAccessControlEntry>(), TrustLevel::HIGH);

    EXPECT_EQ(Permission::NO, consumerPermission);
}

TEST_F(AccessControlAlgorithmTest, permissionWithMasterMediatorAndOwnerAce)
{
    masterAce.setDefaultConsumerPermission(Permission::YES);
    masterAce.setDefaultRequiredTrustLevel(TrustLevel::LOW);

    mediatorAce.setDefaultConsumerPermission(Permission::ASK);
    mediatorAce.setDefaultRequiredTrustLevel(TrustLevel::HIGH);

    ownerAce.setConsumerPermission(Permission::YES);
    ownerAce.setRequiredTrustLevel(TrustLevel::MID);

    Permission::Enum consumerPermission = accessControlAlgorithm.getConsumerPermission(
            masterAce, mediatorAce, ownerAce, TrustLevel::MID);

    EXPECT_EQ(Permission::YES, consumerPermission);
}

TEST_F(AccessControlAlgorithmTest, permissionWithMasterAndOwnerAce)
{
    masterAce.setDefaultConsumerPermission(Permission::ASK);
    masterAce.setDefaultRequiredTrustLevel(TrustLevel::LOW);

    ownerAce.setConsumerPermission(Permission::YES);
    ownerAce.setRequiredTrustLevel(TrustLevel::HIGH);

    Permission::Enum consumerPermission = accessControlAlgorithm.getConsumerPermission(
            masterAce, mediatorAce, ownerAce, TrustLevel::HIGH);

    EXPECT_EQ(Permission::YES, consumerPermission);
}

TEST_F(AccessControlAlgorithmTest, permissionWithOwnerAceOnly)
{
    ownerAce.setConsumerPermission(Permission::YES);
    ownerAce.setRequiredTrustLevel(TrustLevel::HIGH);

    Permission::Enum consumerPermission = accessControlAlgorithm.getConsumerPermission(
            boost::optional<MasterAccessControlEntry>(),
            boost::optional<MasterAccessControlEntry>(),
            ownerAce,
            TrustLevel::HIGH);

    EXPECT_EQ(Permission::YES, consumerPermission);
}

TEST_F(AccessControlAlgorithmTest, permissionWithMediatorAndInvalidOwnerAce)
{
    std::vector<Permission::Enum> possibleMediatorConsumerPermissions;
    possibleMediatorConsumerPermissions.push_back(Permission::NO);
    mediatorAce.setPossibleConsumerPermissions(possibleMediatorConsumerPermissions);

    ownerAce.setConsumerPermission(Permission::ASK);

    Permission::Enum consumerPermission = accessControlAlgorithm.getConsumerPermission(
            boost::optional<MasterAccessControlEntry>(), mediatorAce, ownerAce, TrustLevel::HIGH);

    EXPECT_EQ(Permission::NO, consumerPermission);
}

TEST_F(AccessControlAlgorithmTest, permissionWithMasterAndInvalidOwnerAce)
{
    std::vector<Permission::Enum> possibleMasterConsumerPermissions;
    possibleMasterConsumerPermissions.push_back(Permission::NO);
    masterAce.setPossibleConsumerPermissions(possibleMasterConsumerPermissions);

    ownerAce.setConsumerPermission(Permission::ASK);

    Permission::Enum consumerPermission = accessControlAlgorithm.getConsumerPermission(
            masterAce, boost::optional<MasterAccessControlEntry>(), ownerAce, TrustLevel::HIGH);

    EXPECT_EQ(Permission::NO, consumerPermission);
}
