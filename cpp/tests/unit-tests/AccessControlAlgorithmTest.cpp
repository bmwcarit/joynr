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
#include "gtest/gtest.h"
#include "cluster-controller/access-control/AccessControlAlgorithm.h"

#include "joynr/infrastructure/DacTypes/QtMasterAccessControlEntry.h"
#include "joynr/infrastructure/DacTypes/QtOwnerAccessControlEntry.h"
#include "joynr/infrastructure/DacTypes/QtPermission.h"
#include "joynr/infrastructure/DacTypes/QtTrustLevel.h"

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
        masterAce = QtMasterAccessControlEntry(TEST_USER,TEST_DOMAIN1,TEST_INTERFACE1,QtTrustLevel::LOW,ALL_TRUST_LEVELS,QtTrustLevel::LOW,ALL_TRUST_LEVELS,QString(),QtPermission::NO,ALL_PERMISSIONS);

        mediatorAce = QtMasterAccessControlEntry(TEST_USER,
                                               TEST_DOMAIN1,
                                               TEST_INTERFACE1,
                                               QtTrustLevel::LOW,
                                               ALL_TRUST_LEVELS,
                                               QtTrustLevel::LOW,
                                               ALL_TRUST_LEVELS,
                                               QString(),
                                               QtPermission::NO,
                                               ALL_PERMISSIONS);

        ownerAce = QtOwnerAccessControlEntry(TEST_USER,TEST_DOMAIN1,TEST_INTERFACE1,QtTrustLevel::LOW,QtTrustLevel::LOW,QString(),QtPermission::NO);
    }

    void TearDown()
    {
    }

    static const QList<QtTrustLevel::Enum> ALL_TRUST_LEVELS;
    static const QList<QtPermission::Enum> ALL_PERMISSIONS;

protected:
    AccessControlAlgorithm accessControlAlgorithm;
    QtMasterAccessControlEntry masterAce;
    QtMasterAccessControlEntry mediatorAce;
    QtOwnerAccessControlEntry ownerAce;
    static const QString TEST_USER;
    static const QString TEST_DOMAIN1;
    static const QString TEST_INTERFACE1;

private:
    DISALLOW_COPY_AND_ASSIGN(AccessControlAlgorithmTest);
};

const QString AccessControlAlgorithmTest::TEST_USER("testUser");
const QString AccessControlAlgorithmTest::TEST_DOMAIN1("domain1");
const QString AccessControlAlgorithmTest::TEST_INTERFACE1("interface1");
const QList<QtTrustLevel::Enum> AccessControlAlgorithmTest::ALL_TRUST_LEVELS =
        QList<QtTrustLevel::Enum>()
        << QtTrustLevel::LOW << QtTrustLevel::MID << QtTrustLevel::HIGH;
const QList<QtPermission::Enum> AccessControlAlgorithmTest::ALL_PERMISSIONS =
        QList<QtPermission::Enum>()
        << QtPermission::NO << QtPermission::ASK << QtPermission::YES;


TEST_F(AccessControlAlgorithmTest, permissionWithMasterAceOnly)
{
    masterAce.setDefaultConsumerPermission(QtPermission::YES);
    masterAce.setDefaultRequiredTrustLevel(QtTrustLevel::HIGH);
    QtPermission::Enum consumerPermission = accessControlAlgorithm.getConsumerPermission(
                masterAce,
                Optional<QtMasterAccessControlEntry>::createNull(),
                Optional<QtOwnerAccessControlEntry>::createNull(),
                QtTrustLevel::HIGH);

    EXPECT_EQ(QtPermission::YES, consumerPermission);
}

TEST_F(AccessControlAlgorithmTest, permissionMessageTrustLevelDoesntMatchAce) {
    masterAce.setDefaultConsumerPermission(QtPermission::YES);
    masterAce.setDefaultRequiredTrustLevel(QtTrustLevel::MID);
    QtPermission::Enum consumerPermission = accessControlAlgorithm.getConsumerPermission(
                masterAce,
                Optional<QtMasterAccessControlEntry>::createNull(),
                Optional<QtOwnerAccessControlEntry>::createNull(),
                QtTrustLevel::LOW);

    EXPECT_EQ(QtPermission::NO, consumerPermission);
}

TEST_F(AccessControlAlgorithmTest, permissionWithAllAceNull) {

    QtPermission::Enum consumerPermission = accessControlAlgorithm.getConsumerPermission(
                Optional<QtMasterAccessControlEntry>::createNull(),
                Optional<QtMasterAccessControlEntry>::createNull(),
                Optional<QtOwnerAccessControlEntry>::createNull(),
                QtTrustLevel::HIGH);

    EXPECT_EQ(QtPermission::NO, consumerPermission);
}

TEST_F(AccessControlAlgorithmTest, permissionWithMasterAndMediatorAce) {
    masterAce.setDefaultConsumerPermission(QtPermission::YES);
    masterAce.setDefaultRequiredTrustLevel(QtTrustLevel::HIGH);
    masterAce.setPossibleConsumerPermissions(ALL_PERMISSIONS);
    masterAce.setPossibleRequiredTrustLevels(ALL_TRUST_LEVELS);

    mediatorAce.setDefaultConsumerPermission(QtPermission::ASK);
    mediatorAce.setDefaultRequiredTrustLevel(QtTrustLevel::LOW);

    QtPermission::Enum consumerPermission = accessControlAlgorithm.getConsumerPermission(
                masterAce,
                mediatorAce,
                Optional<QtOwnerAccessControlEntry>::createNull(),
                QtTrustLevel::LOW);

    EXPECT_EQ(QtPermission::ASK, consumerPermission);
}

TEST_F(AccessControlAlgorithmTest, permissionWithMediatorOnly) {
    mediatorAce.setDefaultConsumerPermission(QtPermission::YES);
    mediatorAce.setDefaultRequiredTrustLevel(QtTrustLevel::MID);
    QtPermission::Enum consumerPermission = accessControlAlgorithm.getConsumerPermission(
                Optional<QtMasterAccessControlEntry>::createNull(),
                mediatorAce,
                Optional<QtOwnerAccessControlEntry>::createNull(),
                QtTrustLevel::HIGH);

    EXPECT_EQ(QtPermission::YES, consumerPermission);
}

TEST_F(AccessControlAlgorithmTest, permissionWithMasterAndInvalidMediatorAce) {
    QList<QtPermission::Enum> possibleMasterConsumerPermissions;
    possibleMasterConsumerPermissions.append(QtPermission::NO);
    masterAce.setPossibleConsumerPermissions(possibleMasterConsumerPermissions);

    QList<QtPermission::Enum> possibleMediatorConsumerPermissions;
    possibleMediatorConsumerPermissions.append(QtPermission::ASK);
    possibleMediatorConsumerPermissions.append(QtPermission::YES);
    mediatorAce.setPossibleConsumerPermissions(possibleMediatorConsumerPermissions);
    mediatorAce.setDefaultConsumerPermission(QtPermission::YES);
    mediatorAce.setDefaultRequiredTrustLevel(QtTrustLevel::MID);

    QtPermission::Enum consumerPermission = accessControlAlgorithm.getConsumerPermission(
                masterAce,
                mediatorAce,
                Optional<QtOwnerAccessControlEntry>::createNull(),
                QtTrustLevel::HIGH);

    EXPECT_EQ(QtPermission::NO, consumerPermission);
}

TEST_F(AccessControlAlgorithmTest, permissionWithMasterMediatorAndOwnerAce) {
    masterAce.setDefaultConsumerPermission(QtPermission::YES);
    masterAce.setDefaultRequiredTrustLevel(QtTrustLevel::LOW);

    mediatorAce.setDefaultConsumerPermission(QtPermission::ASK);
    mediatorAce.setDefaultRequiredTrustLevel(QtTrustLevel::HIGH);

    ownerAce.setConsumerPermission(QtPermission::YES);
    ownerAce.setRequiredTrustLevel(QtTrustLevel::MID);

    QtPermission::Enum consumerPermission = accessControlAlgorithm.getConsumerPermission(
                masterAce,
                mediatorAce,
                ownerAce,
                QtTrustLevel::MID);

    EXPECT_EQ(QtPermission::YES, consumerPermission);
}


TEST_F(AccessControlAlgorithmTest, permissionWithMasterAndOwnerAce) {
    masterAce.setDefaultConsumerPermission(QtPermission::ASK);
    masterAce.setDefaultRequiredTrustLevel(QtTrustLevel::LOW);

    ownerAce.setConsumerPermission(QtPermission::YES);
    ownerAce.setRequiredTrustLevel(QtTrustLevel::HIGH);

    QtPermission::Enum consumerPermission = accessControlAlgorithm.getConsumerPermission(
                masterAce,
                mediatorAce,
                ownerAce,
                QtTrustLevel::HIGH);

    EXPECT_EQ(QtPermission::YES, consumerPermission);
}


TEST_F(AccessControlAlgorithmTest, permissionWithOwnerAceOnly) {
    ownerAce.setConsumerPermission(QtPermission::YES);
    ownerAce.setRequiredTrustLevel(QtTrustLevel::HIGH);

    QtPermission::Enum consumerPermission = accessControlAlgorithm.getConsumerPermission(
                Optional<QtMasterAccessControlEntry>::createNull(),
                Optional<QtMasterAccessControlEntry>::createNull(),
                ownerAce,
                QtTrustLevel::HIGH);

    EXPECT_EQ(QtPermission::YES, consumerPermission);
}

TEST_F(AccessControlAlgorithmTest, permissionWithMediatorAndInvalidOwnerAce) {
    QList<QtPermission::Enum> possibleMediatorConsumerPermissions;
    possibleMediatorConsumerPermissions.append(QtPermission::NO);
    mediatorAce.setPossibleConsumerPermissions(possibleMediatorConsumerPermissions);

    ownerAce.setConsumerPermission(QtPermission::ASK);

    QtPermission::Enum consumerPermission = accessControlAlgorithm.getConsumerPermission(
                Optional<QtMasterAccessControlEntry>::createNull(),
                mediatorAce,
                ownerAce,
                QtTrustLevel::HIGH);

    EXPECT_EQ(QtPermission::NO, consumerPermission);
}

TEST_F(AccessControlAlgorithmTest, permissionWithMasterAndInvalidOwnerAce) {
    QList<QtPermission::Enum> possibleMasterConsumerPermissions;
    possibleMasterConsumerPermissions.append(QtPermission::NO);
    masterAce.setPossibleConsumerPermissions(possibleMasterConsumerPermissions);

    ownerAce.setConsumerPermission(QtPermission::ASK);

    QtPermission::Enum consumerPermission = accessControlAlgorithm.getConsumerPermission(
                masterAce,
                Optional<QtMasterAccessControlEntry>::createNull(),
                ownerAce,
                QtTrustLevel::HIGH);

    EXPECT_EQ(QtPermission::NO, consumerPermission);
}

