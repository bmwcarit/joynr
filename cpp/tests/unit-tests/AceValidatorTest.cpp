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
#include <gtest/gtest.h>
#include "cluster-controller/access-control/AceValidator.h"
#include "joynr/TypeUtil.h"
using namespace ::testing;
using namespace joynr;
using namespace joynr::infrastructure::DacTypes;

class AceValidatorTest : public ::testing::Test {
public:
    MasterAccessControlEntry masterAce;
    MasterAccessControlEntry mediatorAce;
    OwnerAccessControlEntry ownerAce;
public:
    AceValidatorTest()
    {
    }

    ~AceValidatorTest()
    {
    }

    void SetUp(){
        QList<Permission::Enum> possiblePermissions;
        possiblePermissions.append(Permission::NO);
        possiblePermissions.append(Permission::ASK);
        QList<TrustLevel::Enum> possibleRequiredTrustLevels;
        possibleRequiredTrustLevels.append(TrustLevel::LOW);
        possibleRequiredTrustLevels.append(TrustLevel::MID);
        QList<TrustLevel::Enum> possibleRequiredAceChangeTrustLevels;
        possibleRequiredAceChangeTrustLevels.append(TrustLevel::MID);
        possibleRequiredAceChangeTrustLevels.append(TrustLevel::HIGH);
        masterAce = MasterAccessControlEntry(TEST_USER.toStdString(),
                                                                    std::string(),
                                                                    std::string(),
                                                                    TrustLevel::LOW,
                                                                    TypeUtil::toStd(possibleRequiredTrustLevels),
                                                                    TrustLevel::LOW,
                                                                    TypeUtil::toStd(possibleRequiredAceChangeTrustLevels),
                                                                    std::string(),
                                                                    Permission::NO,
                                                                    TypeUtil::toStd(possiblePermissions));

        mediatorAce = MasterAccessControlEntry(TEST_USER.toStdString(),
                                               std::string(),
                                               std::string(),
                                               TrustLevel::LOW,
                                               TypeUtil::toStd(possibleRequiredTrustLevels),
                                               TrustLevel::LOW,
                                               TypeUtil::toStd(possibleRequiredAceChangeTrustLevels),
                                               std::string(),
                                               Permission::NO,
                                               TypeUtil::toStd(possiblePermissions));

        ownerAce = OwnerAccessControlEntry(TEST_USER.toStdString(),
                                                                  std::string(),
                                                                  std::string(),
                                                                  TrustLevel::MID,
                                                                  TrustLevel::HIGH,
                                                                  std::string(),
                                                                  Permission::ASK);
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
    QList<Permission::Enum> possiblePermissions;
    possiblePermissions.append(Permission::ASK);
    possiblePermissions.append(Permission::YES);
    mediatorAce.setPossibleConsumerPermissions(TypeUtil::toStd(possiblePermissions));
    AceValidator validator(masterAce, mediatorAce, ownerAce);

    EXPECT_FALSE(validator.isMediatorValid());
}

TEST_F(AceValidatorTest, TestMediatorInvalidPossibleTrustLevels)
{
    QList<TrustLevel::Enum> possibleRequiredTrustLevels;
    possibleRequiredTrustLevels.append(TrustLevel::HIGH);
    possibleRequiredTrustLevels.append(TrustLevel::MID);
    mediatorAce.setPossibleRequiredTrustLevels(TypeUtil::toStd(possibleRequiredTrustLevels));
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
    ownerAce.setConsumerPermission(Permission::YES);
    AceValidator validator(masterAce, mediatorAce, ownerAce);
    EXPECT_FALSE(validator.isOwnerValid());
}
