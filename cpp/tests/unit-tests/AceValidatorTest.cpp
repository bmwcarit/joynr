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

#include "joynr/PrivateCopyAssign.h"
#include <gtest/gtest.h>
#include "cluster-controller/access-control/AceValidator.h"
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

    ~AceValidatorTest() = default;

    void SetUp(){
        std::vector<Permission::Enum> possiblePermissions;
        possiblePermissions.push_back(Permission::NO);
        possiblePermissions.push_back(Permission::ASK);
        std::vector<TrustLevel::Enum> possibleRequiredTrustLevels;
        possibleRequiredTrustLevels.push_back(TrustLevel::LOW);
        possibleRequiredTrustLevels.push_back(TrustLevel::MID);
        std::vector<TrustLevel::Enum> possibleRequiredAceChangeTrustLevels;
        possibleRequiredAceChangeTrustLevels.push_back(TrustLevel::MID);
        possibleRequiredAceChangeTrustLevels.push_back(TrustLevel::HIGH);
        masterAce = MasterAccessControlEntry(TEST_USER,
                                                                    std::string(),
                                                                    std::string(),
                                                                    TrustLevel::LOW,
                                                                    possibleRequiredTrustLevels,
                                                                    TrustLevel::LOW,
                                                                    possibleRequiredAceChangeTrustLevels,
                                                                    std::string(),
                                                                    Permission::NO,
                                                                    possiblePermissions);

        mediatorAce = MasterAccessControlEntry(TEST_USER,
                                               std::string(),
                                               std::string(),
                                               TrustLevel::LOW,
                                               possibleRequiredTrustLevels,
                                               TrustLevel::LOW,
                                               possibleRequiredAceChangeTrustLevels,
                                               std::string(),
                                               Permission::NO,
                                               possiblePermissions);

        ownerAce = OwnerAccessControlEntry(TEST_USER,
                                                                  std::string(),
                                                                  std::string(),
                                                                  TrustLevel::MID,
                                                                  TrustLevel::HIGH,
                                                                  std::string(),
                                                                  Permission::ASK);
    }

protected:
    static const std::string TEST_USER;
private:
    DISALLOW_COPY_AND_ASSIGN(AceValidatorTest);
};

const std::string AceValidatorTest::TEST_USER("testUser");

//----- Tests ------------------------------------------------------------------

TEST_F(AceValidatorTest, TestMediatorInvalidPossiblePermissions)
{
    std::vector<Permission::Enum> possiblePermissions;
    possiblePermissions.push_back(Permission::ASK);
    possiblePermissions.push_back(Permission::YES);
    mediatorAce.setPossibleConsumerPermissions(possiblePermissions);
    AceValidator validator(masterAce, mediatorAce, ownerAce);

    EXPECT_FALSE(validator.isMediatorValid());
}

TEST_F(AceValidatorTest, TestMediatorInvalidPossibleTrustLevels)
{
    std::vector<TrustLevel::Enum> possibleRequiredTrustLevels;
    possibleRequiredTrustLevels.push_back(TrustLevel::HIGH);
    possibleRequiredTrustLevels.push_back(TrustLevel::MID);
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
    ownerAce.setConsumerPermission(Permission::YES);
    AceValidator validator(masterAce, mediatorAce, ownerAce);
    EXPECT_FALSE(validator.isOwnerValid());
}
