/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
#include "cluster-controller/access-control/AceValidator.h"

using namespace ::testing;
using namespace joynr;
using namespace joynr::infrastructure::dactypes;

class AceValidatorTest : public ::testing::Test {
public:
    QtMasterAccessControlEntry masterAce;
    QtMasterAccessControlEntry mediatorAce;
    QtOwnerAccessControlEntry ownerAce;
public:
    AceValidatorTest()
    {
    }

    ~AceValidatorTest()
    {
    }

    void SetUp(){
        QList<QtPermission::Enum> possiblePermissions;
        possiblePermissions.append(QtPermission::NO);
        possiblePermissions.append(QtPermission::ASK);
        QList<QtTrustLevel::Enum> possibleRequiredTrustLevels;
        possibleRequiredTrustLevels.append(QtTrustLevel::LOW);
        possibleRequiredTrustLevels.append(QtTrustLevel::MID);
        QList<QtTrustLevel::Enum> possibleRequiredAceChangeTrustLevels;
        possibleRequiredAceChangeTrustLevels.append(QtTrustLevel::MID);
        possibleRequiredAceChangeTrustLevels.append(QtTrustLevel::HIGH);
        masterAce = QtMasterAccessControlEntry(TEST_USER,
                                                                    QString(),
                                                                    QString(),
                                                                    QtTrustLevel::LOW,
                                                                    possibleRequiredTrustLevels,
                                                                    QtTrustLevel::LOW,
                                                                    possibleRequiredAceChangeTrustLevels,
                                                                    QString(),
                                                                    QtPermission::NO,
                                                                    possiblePermissions);

        mediatorAce = QtMasterAccessControlEntry(TEST_USER,
                                               QString(),
                                               QString(),
                                               QtTrustLevel::LOW,
                                               possibleRequiredTrustLevels,
                                               QtTrustLevel::LOW,
                                               possibleRequiredAceChangeTrustLevels,
                                               QString(),
                                               QtPermission::NO,
                                               possiblePermissions);

        ownerAce = QtOwnerAccessControlEntry(TEST_USER,
                                                                  QString(),
                                                                  QString(),
                                                                  QtTrustLevel::MID,
                                                                  QtTrustLevel::HIGH,
                                                                  QString(),
                                                                  QtPermission::ASK);
    }

    void TearDown()
    {
    }
protected:
    static const QString TEST_USER;
private:
    DISALLOW_COPY_AND_ASSIGN(AceValidatorTest);
};

const QString AceValidatorTest::TEST_USER("testUser");

//----- Tests ------------------------------------------------------------------

TEST_F(AceValidatorTest, TestMediatorInvalidPossiblePermissions)
{
    QList<QtPermission::Enum> possiblePermissions;
    possiblePermissions.append(QtPermission::ASK);
    possiblePermissions.append(QtPermission::YES);
    mediatorAce.setPossibleConsumerPermissions(possiblePermissions);
    AceValidator validator(masterAce, mediatorAce, ownerAce);

    EXPECT_FALSE(validator.isMediatorValid());
}

TEST_F(AceValidatorTest, TestMediatorInvalidPossibleTrustLevels)
{
    QList<QtTrustLevel::Enum> possibleRequiredTrustLevels;
    possibleRequiredTrustLevels.append(QtTrustLevel::HIGH);
    possibleRequiredTrustLevels.append(QtTrustLevel::MID);
    mediatorAce.setPossibleRequiredTrustLevels(possibleRequiredTrustLevels);
    AceValidator validator(masterAce, mediatorAce, ownerAce);

    EXPECT_FALSE(validator.isMediatorValid());
}

TEST_F(AceValidatorTest, TestMediatorValid)
{
    AceValidator validator(masterAce, mediatorAce, ownerAce);

    EXPECT_TRUE(validator.isMediatorValid());
}

TEST_F(AceValidatorTest, TestOwnerValid)
{
    AceValidator validator(masterAce, mediatorAce, ownerAce);

    EXPECT_TRUE(validator.isOwnerValid());
}

TEST_F(AceValidatorTest, TestOwnerInvalid)
{
    ownerAce.setConsumerPermission(QtPermission::YES);
    AceValidator validator(masterAce, mediatorAce, ownerAce);
    EXPECT_FALSE(validator.isOwnerValid());
}
