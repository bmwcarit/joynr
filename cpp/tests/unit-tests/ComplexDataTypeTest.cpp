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
#include <gtest/gtest.h>

#include "PrettyPrint.h"
#include "joynr/joynrlogging.h"
#include <string>

#include "joynr/types/TestTypes/QtTStructExtended.h"
#include "joynr/types/TestTypes/TStructExtended.h"

using namespace joynr::types;

class ComplexDataTypeTest : public testing::Test
{
public:
    ComplexDataTypeTest()
    {
    }

    virtual ~ComplexDataTypeTest()
    {
    }

protected:
    static joynr::joynr_logging::Logger* logger;
};

joynr::joynr_logging::Logger* ComplexDataTypeTest::logger(
        joynr::joynr_logging::Logging::getInstance()->getLogger("TST", "ComplexDataTypeTest"));

TEST_F(ComplexDataTypeTest, createCStdomplexDataType)
{
    joynr::types::TestTypes::QtTStructExtended fixture;
    joynr::types::TestTypes::TStructExtended result = joynr::types::TestTypes::QtTStructExtended::createStd(fixture);
    EXPECT_EQ(fixture.getTDouble(), result.getTDouble());
}
