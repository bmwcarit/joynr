/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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

#include "joynr/infrastructure/DacTypes/Permission.h"
#include "joynr/infrastructure/DacTypes/TrustLevel.h"
#include "libjoynrclustercontroller/access-control/Validator.h"

using namespace ::testing;
using namespace joynr;

using joynr::infrastructure::DacTypes::Permission;
using joynr::infrastructure::DacTypes::TrustLevel;

template <typename Tag>
class AceRceValidatorTest : public ::testing::Test
{
public:
    using ValidatorType = Validator<Tag>;
    using MasterEntry = typename ValidatorType::MasterEntry;
    using MediatorEntry = typename ValidatorType::MediatorEntry;
    using OwnerEntry = typename ValidatorType::OwnerEntry;

    MasterEntry _masterEntry;
    MediatorEntry _mediatorEntry;
    OwnerEntry _ownerEntry;

public:
    AceRceValidatorTest()
    {
        _possiblePermissions.push_back(Permission::NO);
        _possiblePermissions.push_back(Permission::ASK);
        _possibleRequiredTrustLevels.push_back(TrustLevel::LOW);
        _possibleRequiredTrustLevels.push_back(TrustLevel::MID);
        _possibleRequiredChangeTrustLevels.push_back(TrustLevel::MID);
        _possibleRequiredChangeTrustLevels.push_back(TrustLevel::HIGH);

        init(Tag{});
    }

    template <typename Entry>
    void setPossiblePermissions(Entry& entry,
                                const std::vector<Permission::Enum>& possiblePermissions,
                                const tags::Access&)
    {
        entry.setPossibleConsumerPermissions(possiblePermissions);
    }

    template <typename Entry>
    void setPossiblePermissions(Entry& entry,
                                const std::vector<Permission::Enum>& possiblePermissions,
                                const tags::Registration&)
    {
        entry.setPossibleProviderPermissions(possiblePermissions);
    }

    template <typename Entry>
    void setPermission(Entry& entry, Permission::Enum permission, const tags::Access&)
    {
        entry.setConsumerPermission(permission);
    }

    template <typename Entry>
    void setPermission(Entry& entry, Permission::Enum permission, const tags::Registration&)
    {
        entry.setProviderPermission(permission);
    }
    void init(const tags::Access&)
    {
        _masterEntry = MasterEntry(_TEST_USER,
                                  std::string(),
                                  std::string(),
                                  TrustLevel::LOW,
                                  _possibleRequiredTrustLevels,
                                  TrustLevel::LOW,
                                  _possibleRequiredChangeTrustLevels,
                                  std::string(),
                                  Permission::NO,
                                  _possiblePermissions);

        _mediatorEntry = MediatorEntry(_TEST_USER,
                                      std::string(),
                                      std::string(),
                                      TrustLevel::LOW,
                                      _possibleRequiredTrustLevels,
                                      TrustLevel::LOW,
                                      _possibleRequiredChangeTrustLevels,
                                      std::string(),
                                      Permission::NO,
                                      _possiblePermissions);

        _ownerEntry = OwnerEntry(_TEST_USER,
                                std::string(),
                                std::string(),
                                TrustLevel::MID,
                                TrustLevel::HIGH,
                                std::string(),
                                Permission::ASK);
    }

    void init(const tags::Registration&)
    {
        _masterEntry = MasterEntry(_TEST_USER,
                                  std::string(),
                                  std::string(),
                                  TrustLevel::LOW,
                                  _possibleRequiredTrustLevels,
                                  TrustLevel::LOW,
                                  _possibleRequiredChangeTrustLevels,
                                  Permission::NO,
                                  _possiblePermissions);

        _mediatorEntry = MediatorEntry(_TEST_USER,
                                      std::string(),
                                      std::string(),
                                      TrustLevel::LOW,
                                      _possibleRequiredTrustLevels,
                                      TrustLevel::LOW,
                                      _possibleRequiredChangeTrustLevels,
                                      Permission::NO,
                                      _possiblePermissions);

        _ownerEntry = OwnerEntry(_TEST_USER,
                                std::string(),
                                std::string(),
                                TrustLevel::MID,
                                TrustLevel::HIGH,
                                Permission::ASK);
    }

protected:
    static const std::string _TEST_USER;
    std::vector<Permission::Enum> _possiblePermissions;
    std::vector<TrustLevel::Enum> _possibleRequiredChangeTrustLevels;
    std::vector<TrustLevel::Enum> _possibleRequiredTrustLevels;
};

template <typename Tag>
const std::string AceRceValidatorTest<Tag>::_TEST_USER("testUser");

using TagTypes = ::testing::Types<tags::Access, tags::Registration>;
TYPED_TEST_CASE(AceRceValidatorTest, TagTypes);

TYPED_TEST(AceRceValidatorTest, TestMediatorInvalidPossiblePermissions)
{
    std::vector<Permission::Enum> possiblePermissions;
    possiblePermissions.push_back(Permission::ASK);
    possiblePermissions.push_back(Permission::YES);
    this->setPossiblePermissions(this->_mediatorEntry, possiblePermissions, TypeParam{});
    Validator<TypeParam> validator(this->_masterEntry, this->_mediatorEntry, this->_ownerEntry);

    EXPECT_FALSE(validator.isMediatorValid());
}

TYPED_TEST(AceRceValidatorTest, TestMediatorInvalidPossibleTrustLevels)
{
    std::vector<TrustLevel::Enum> possibleRequiredTrustLevels;
    possibleRequiredTrustLevels.push_back(TrustLevel::HIGH);
    possibleRequiredTrustLevels.push_back(TrustLevel::MID);
    this->_mediatorEntry.setPossibleRequiredTrustLevels(possibleRequiredTrustLevels);
    Validator<TypeParam> validator(this->_masterEntry, this->_mediatorEntry, this->_ownerEntry);

    EXPECT_FALSE(validator.isMediatorValid());
}

TYPED_TEST(AceRceValidatorTest, TestMediatorValid)
{
    Validator<TypeParam> validator(this->_masterEntry, this->_mediatorEntry, this->_ownerEntry);

    EXPECT_TRUE(validator.isMediatorValid());
}

TYPED_TEST(AceRceValidatorTest, TestOwnerValid)
{
    Validator<TypeParam> validator(this->_masterEntry, this->_mediatorEntry, this->_ownerEntry);

    EXPECT_TRUE(validator.isOwnerValid());
}

TYPED_TEST(AceRceValidatorTest, TestOwnerInvalid)
{
    this->setPermission(this->_ownerEntry, Permission::YES, TypeParam{});
    Validator<TypeParam> validator(this->_masterEntry, this->_mediatorEntry, this->_ownerEntry);
    EXPECT_FALSE(validator.isOwnerValid());
}
