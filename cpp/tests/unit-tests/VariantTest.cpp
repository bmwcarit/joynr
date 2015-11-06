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
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "joynr/Variant.h"
#include "joynr/joynrlogging.h"
#include "joynr/TypeUtil.h"
//#include "tests/utils/MockObjects.h"

using ::testing::Property;
using ::testing::Eq;
using ::testing::ByRef;
using ::testing::NotNull;
using ::testing::_;
using namespace ::testing;
using namespace joynr;

/*
 * This tests the Variant class.
 */

class VariantTest : public ::testing::Test {
public:

    VariantTest():logger(joynr_logging::Logging::getInstance()->getLogger("TST", "VariantTest")){}

    void SetUp() {}

protected:
    joynr_logging::Logger* logger;
};

/*
 * just for test Variant with custom types
 */
struct ExampleCustomType
{
    ExampleCustomType(int content):expectedInt(content){}
    int expectedInt;
};

static bool isExampleCustomTypeRegistered = Variant::registerType<ExampleCustomType>("ExampleCustomType");

TEST_F(VariantTest, checkTypeId) {
    Variant variant = Variant::make<std::string>("Hallo");
    ASSERT_TRUE(variant.is<std::string>());
}

TEST_F(VariantTest, checkVariantStdContent) {
    std::string expectedString{"Hallo"};
    Variant variant = Variant::make<std::string>(expectedString);
    ASSERT_EQ(variant.get<std::string>(), expectedString);
}

TEST_F(VariantTest, checkVariantCustomContent) {
    ExampleCustomType expectedCustomType{42};
    Variant variant = Variant::make<ExampleCustomType>(expectedCustomType);
    ASSERT_TRUE(variant.is<ExampleCustomType>());
    // to save on implementing operator== for ExampleCustomType, just check if expectedInt is the same
    ASSERT_EQ(variant.get<ExampleCustomType>().expectedInt, expectedCustomType.expectedInt);
}

TEST_F(VariantTest, checkCollectionOfVariants) {
    std::vector<Variant> variants;
    std::string expectedString{"Hallo"};
    variants.push_back(Variant::make<std::string>(expectedString));
    ExampleCustomType expectedCustomType{42};
    variants.push_back(Variant::make<ExampleCustomType>(expectedCustomType));

    variants.push_back(Variant::make<std::vector<Variant>>
                       (std::initializer_list<Variant>{
                            Variant::make<ExampleCustomType>(ExampleCustomType(1)),
                            Variant::make<ExampleCustomType>(ExampleCustomType(2)),
                            Variant::make<ExampleCustomType>(ExampleCustomType(3)),
                            Variant::make<ExampleCustomType>(ExampleCustomType(4)),
                            Variant::make<ExampleCustomType>(ExampleCustomType(5))}));

    ASSERT_TRUE(variants.at(0).is<std::string>());
    ASSERT_TRUE(variants.at(1).is<ExampleCustomType>());
    ASSERT_TRUE(variants.at(2).is<std::vector<Variant>>());

    // Use the collection of Variants
    for (auto& v : variants) {
        if (v.is<std::string>()) {
            LOG_DEBUG(logger, QString("Variant is std::string, value: %1").arg(TypeUtil::toQt(v.get<std::string>())));
        } else if (v.is<ExampleCustomType>()) {
            LOG_DEBUG(logger, QString("Variant is ExampleCustomType, value: %1").arg(TypeUtil::toQt(v.get<ExampleCustomType>().expectedInt)));
        } else if (v.is<std::vector<Variant>>()) {
            LOG_DEBUG(logger, QString("Variant is a collection of variants"));
            auto& vec = v.get<std::vector<Variant>>();
            for (auto& i : vec) {
                LOG_DEBUG(logger, QString("expectedInt: %1").arg(i.get<ExampleCustomType>().expectedInt));
            }
        }
    }
}
