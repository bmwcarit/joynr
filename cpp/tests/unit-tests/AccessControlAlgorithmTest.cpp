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
#include <gtest/gtest.h>
#include "cluster-controller/access-control/AccessControlAlgorithm.h"

#include "joynr/infrastructure/DacTypes/MasterAccessControlEntry.h"
#include "joynr/infrastructure/DacTypes/OwnerAccessControlEntry.h"
#include "joynr/infrastructure/DacTypes/Permission.h"
#include "joynr/infrastructure/DacTypes/TrustLevel.h"

#include "joynr/TypeUtil.h"

using namespace ::testing;
using namespace joynr;
using namespace joynr::infrastructure::DacTypes;

class AccessControlAlgorithmTest : public ::testing::Test {
public:
    AccessControlAlgorithmTest() : accessControlAlgorithm()
    {
    }

    ~AccessControlAlgorithmTest()
    {
    }

    void SetUp()
    {
        masterAce = MasterAccessControlEntry(TEST_USER.toStdString(),
                                             TEST_DOMAIN1.toStdString(),
                                             TEST_INTERFACE1.toStdString(),
                                             TrustLevel::LOW,
                                             TypeUtil::toStd(ALL_TRUST_LEVELS),
                                             TrustLevel::LOW,
                                             TypeUtil::toStd(ALL_TRUST_LEVELS),
                                             std::string(),
                                             Permission::NO,
                                             TypeUtil::toStd(ALL_PERMISSIONS));

        mediatorAce = MasterAccessControlEntry(TEST_USER.toStdString(),
                                               TEST_DOMAIN1.toStdString(),
                                               TEST_INTERFACE1.toStdString(),
                                               TrustLevel::LOW,
                                               TypeUtil::toStd(ALL_TRUST_LEVELS),
                                               TrustLevel::LOW,
                                               TypeUtil::toStd(ALL_TRUST_LEVELS),
                                               std::string(),
                                               Permission::NO,
                                               TypeUtil::toStd(ALL_PERMISSIONS));

        ownerAce = OwnerAccessControlEntry(TEST_USER.toStdString(),
                                           TEST_DOMAIN1.toStdString(),
                                           TEST_INTERFACE1.toStdString(),
                                           TrustLevel::LOW,
                                           TrustLevel::LOW,
                                           std::string(),
                                           Permission::NO);
    }

    void TearDown()
    {
    }

    static const QList<TrustLevel::Enum> ALL_TRUST_LEVELS;
    static const QList<Permission::Enum> ALL_PERMISSIONS;

protected:
    AccessControlAlgorithm accessControlAlgorithm;
    MasterAccessControlEntry masterAce;
    MasterAccessControlEntry mediatorAce;
    OwnerAccessControlEntry ownerAce;
    static const QString TEST_USER;
    static const QString TEST_DOMAIN1;
    static const QString TEST_INTERFACE1;

private:
    DISALLOW_COPY_AND_ASSIGN(AccessControlAlgorithmTest);
};

const QString AccessControlAlgorithmTest::TEST_USER("testUser");
const QString AccessControlAlgorithmTest::TEST_DOMAIN1("domain1");
const QString AccessControlAlgorithmTest::TEST_INTERFACE1("interface1");
const QList<TrustLevel::Enum> AccessControlAlgorithmTest::ALL_TRUST_LEVELS =
        QList<TrustLevel::Enum>()
        << TrustLevel::LOW << TrustLevel::MID << TrustLevel::HIGH;
const QList<Permission::Enum> AccessControlAlgorithmTest::ALL_PERMISSIONS =
        QList<Permission::Enum>()
        << Permission::NO << Permission::ASK << Permission::YES;


TEST_F(AccessControlAlgorithmTest, permissionWithMasterAceOnly)
{
    masterAce.setDefaultConsumerPermission(Permission::YES);
    masterAce.setDefaultRequiredTrustLevel(TrustLevel::HIGH);
    Permission::Enum consumerPermission = accessControlAlgorithm.getConsumerPermission(
                masterAce,
                Optional<MasterAccessControlEntry>::createNull(),
                Optional<OwnerAccessControlEntry>::createNull(),
                TrustLevel::HIGH);

    EXPECT_EQ(Permission::YES, consumerPermission);
}

TEST_F(AccessControlAlgorithmTest, permissionMessageTrustLevelDoesntMatchAce) {
    masterAce.setDefaultConsumerPermission(Permission::YES);
    masterAce.setDefaultRequiredTrustLevel(TrustLevel::MID);
    Permission::Enum consumerPermission = accessControlAlgorithm.getConsumerPermission(
                masterAce,
                Optional<MasterAccessControlEntry>::createNull(),
                Optional<OwnerAccessControlEntry>::createNull(),
                TrustLevel::LOW);

    EXPECT_EQ(Permission::NO, consumerPermission);
}

TEST_F(AccessControlAlgorithmTest, permissionWithAllAceNull) {

    Permission::Enum consumerPermission = accessControlAlgorithm.getConsumerPermission(
                Optional<MasterAccessControlEntry>::createNull(),
                Optional<MasterAccessControlEntry>::createNull(),
                Optional<OwnerAccessControlEntry>::createNull(),
                TrustLevel::HIGH);

    EXPECT_EQ(Permission::NO, consumerPermission);
}

TEST_F(AccessControlAlgorithmTest, permissionWithMasterAndMediatorAce) {
    masterAce.setDefaultConsumerPermission(Permission::YES);
    masterAce.setDefaultRequiredTrustLevel(TrustLevel::HIGH);
    masterAce.setPossibleConsumerPermissions(TypeUtil::toStd(ALL_PERMISSIONS));
    masterAce.setPossibleRequiredTrustLevels(TypeUtil::toStd(ALL_TRUST_LEVELS));

    mediatorAce.setDefaultConsumerPermission(Permission::ASK);
    mediatorAce.setDefaultRequiredTrustLevel(TrustLevel::LOW);

    Permission::Enum consumerPermission = accessControlAlgorithm.getConsumerPermission(
                masterAce,
                mediatorAce,
                Optional<OwnerAccessControlEntry>::createNull(),
                TrustLevel::LOW);

    EXPECT_EQ(Permission::ASK, consumerPermission);
}

TEST_F(AccessControlAlgorithmTest, permissionWithMediatorOnly) {
    mediatorAce.setDefaultConsumerPermission(Permission::YES);
    mediatorAce.setDefaultRequiredTrustLevel(TrustLevel::MID);
    Permission::Enum consumerPermission = accessControlAlgorithm.getConsumerPermission(
                Optional<MasterAccessControlEntry>::createNull(),
                mediatorAce,
                Optional<OwnerAccessControlEntry>::createNull(),
                TrustLevel::HIGH);

    EXPECT_EQ(Permission::YES, consumerPermission);
}

TEST_F(AccessControlAlgorithmTest, permissionWithMasterAndInvalidMediatorAce) {
    QList<Permission::Enum> possibleMasterConsumerPermissions;
    possibleMasterConsumerPermissions.append(Permission::NO);
    masterAce.setPossibleConsumerPermissions(TypeUtil::toStd(possibleMasterConsumerPermissions));

    QList<Permission::Enum> possibleMediatorConsumerPermissions;
    possibleMediatorConsumerPermissions.append(Permission::ASK);
    possibleMediatorConsumerPermissions.append(Permission::YES);
    mediatorAce.setPossibleConsumerPermissions(TypeUtil::toStd(possibleMediatorConsumerPermissions));
    mediatorAce.setDefaultConsumerPermission(Permission::YES);
    mediatorAce.setDefaultRequiredTrustLevel(TrustLevel::MID);

    Permission::Enum consumerPermission = accessControlAlgorithm.getConsumerPermission(
                masterAce,
                mediatorAce,
                Optional<OwnerAccessControlEntry>::createNull(),
                TrustLevel::HIGH);

    EXPECT_EQ(Permission::NO, consumerPermission);
}

TEST_F(AccessControlAlgorithmTest, permissionWithMasterMediatorAndOwnerAce) {
    masterAce.setDefaultConsumerPermission(Permission::YES);
    masterAce.setDefaultRequiredTrustLevel(TrustLevel::LOW);

    mediatorAce.setDefaultConsumerPermission(Permission::ASK);
    mediatorAce.setDefaultRequiredTrustLevel(TrustLevel::HIGH);

    ownerAce.setConsumerPermission(Permission::YES);
    ownerAce.setRequiredTrustLevel(TrustLevel::MID);

    Permission::Enum consumerPermission = accessControlAlgorithm.getConsumerPermission(
                masterAce,
                mediatorAce,
                ownerAce,
                TrustLevel::MID);

    EXPECT_EQ(Permission::YES, consumerPermission);
}


TEST_F(AccessControlAlgorithmTest, permissionWithMasterAndOwnerAce) {
    masterAce.setDefaultConsumerPermission(Permission::ASK);
    masterAce.setDefaultRequiredTrustLevel(TrustLevel::LOW);

    ownerAce.setConsumerPermission(Permission::YES);
    ownerAce.setRequiredTrustLevel(TrustLevel::HIGH);

    Permission::Enum consumerPermission = accessControlAlgorithm.getConsumerPermission(
                masterAce,
                mediatorAce,
                ownerAce,
                TrustLevel::HIGH);

    EXPECT_EQ(Permission::YES, consumerPermission);
}


TEST_F(AccessControlAlgorithmTest, permissionWithOwnerAceOnly) {
    ownerAce.setConsumerPermission(Permission::YES);
    ownerAce.setRequiredTrustLevel(TrustLevel::HIGH);

    Permission::Enum consumerPermission = accessControlAlgorithm.getConsumerPermission(
                Optional<MasterAccessControlEntry>::createNull(),
                Optional<MasterAccessControlEntry>::createNull(),
                ownerAce,
                TrustLevel::HIGH);

    EXPECT_EQ(Permission::YES, consumerPermission);
}

TEST_F(AccessControlAlgorithmTest, permissionWithMediatorAndInvalidOwnerAce) {
    QList<Permission::Enum> possibleMediatorConsumerPermissions;
    possibleMediatorConsumerPermissions.append(Permission::NO);
    mediatorAce.setPossibleConsumerPermissions(TypeUtil::toStd(possibleMediatorConsumerPermissions));

    ownerAce.setConsumerPermission(Permission::ASK);

    Permission::Enum consumerPermission = accessControlAlgorithm.getConsumerPermission(
                Optional<MasterAccessControlEntry>::createNull(),
                mediatorAce,
                ownerAce,
                TrustLevel::HIGH);

    EXPECT_EQ(Permission::NO, consumerPermission);
}

TEST_F(AccessControlAlgorithmTest, permissionWithMasterAndInvalidOwnerAce) {
    QList<Permission::Enum> possibleMasterConsumerPermissions;
    possibleMasterConsumerPermissions.append(Permission::NO);
    masterAce.setPossibleConsumerPermissions(TypeUtil::toStd(possibleMasterConsumerPermissions));

    ownerAce.setConsumerPermission(Permission::ASK);

    Permission::Enum consumerPermission = accessControlAlgorithm.getConsumerPermission(
                masterAce,
                Optional<MasterAccessControlEntry>::createNull(),
                ownerAce,
                TrustLevel::HIGH);

    EXPECT_EQ(Permission::NO, consumerPermission);
}

