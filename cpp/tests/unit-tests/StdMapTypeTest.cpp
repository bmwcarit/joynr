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

#include "joynr/tests/MapInsideInterface.h"
#include "joynr/tests/MapInsideInterfaceWithoutVersion.h"
#include "joynr/types/TestTypes/TStringKeyMap.h"
#include "joynr/types/TestTypesWithoutVersion/MapInsideTypeCollectionWithoutVersion.h"

#include "tests/PrettyPrint.h"

class StdMapTypeTest : public testing::Test
{
public:
    StdMapTypeTest() = default;

    virtual ~StdMapTypeTest() = default;

protected:
};

TEST_F(StdMapTypeTest, versionIsSetInMapInsideInterface)
{
    std::uint32_t expectedMajorVersion = 47;
    std::uint32_t expectedMinorVersion = 11;

    EXPECT_EQ(expectedMajorVersion, joynr::tests::MapInsideInterface::MAJOR_VERSION);
    EXPECT_EQ(expectedMinorVersion, joynr::tests::MapInsideInterface::MINOR_VERSION);
}

TEST_F(StdMapTypeTest, defaultVersionIsSetInMapInsideInterfaceWithoutVersion)
{
    std::uint32_t expectedMajorVersion = 0;
    std::uint32_t expectedMinorVersion = 0;

    EXPECT_EQ(expectedMajorVersion, joynr::tests::MapInsideInterfaceWithoutVersion::MAJOR_VERSION);
    EXPECT_EQ(expectedMinorVersion, joynr::tests::MapInsideInterfaceWithoutVersion::MINOR_VERSION);
}

TEST_F(StdMapTypeTest, versionIsSetInMapInsideTypeCollection)
{
    std::uint32_t expectedMajorVersion = 49;
    std::uint32_t expectedMinorVersion = 13;

    EXPECT_EQ(expectedMajorVersion, joynr::types::TestTypes::TStringKeyMap::MAJOR_VERSION);
    EXPECT_EQ(expectedMinorVersion, joynr::types::TestTypes::TStringKeyMap::MINOR_VERSION);
}

TEST_F(StdMapTypeTest, defaultVersionIsSetInMapInsideTypeCollectionWithoutVersion)
{
    std::uint32_t expectedMajorVersion = 0;
    std::uint32_t expectedMinorVersion = 0;

    EXPECT_EQ(expectedMajorVersion,
              joynr::types::TestTypesWithoutVersion::MapInsideTypeCollectionWithoutVersion::
                      MAJOR_VERSION);
    EXPECT_EQ(expectedMinorVersion,
              joynr::types::TestTypesWithoutVersion::MapInsideTypeCollectionWithoutVersion::
                      MINOR_VERSION);
}
