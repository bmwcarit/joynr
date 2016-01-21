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
#include <gmock/gmock.h>
#include <string>
#include "tests/utils/MockObjects.h"
#include "joynr/Request.h"
#include "joynr/Dispatcher.h"
#include "joynr/tests/DefaulttestProvider.h"

using ::testing::A;
using ::testing::_;
using ::testing::Return;
using ::testing::Eq;
using ::testing::AllOf;
using ::testing::Property;
using ::testing::Invoke;
using ::testing::Unused;

using namespace ::testing;

using namespace joynr;



class ProviderIntegrationTest : public ::testing::Test {
public:

    ProviderIntegrationTest() :
        provider(new tests::DefaulttestProvider())
    {
    }

    // Sets up the test fixture.
    void SetUp(){
    }

    // Tears down the test fixture.
    void TearDown(){

    }

    ~ProviderIntegrationTest() = default;

protected:

    std::shared_ptr<joynr::tests::DefaulttestProvider> provider;

private:
    DISALLOW_COPY_AND_ASSIGN(ProviderIntegrationTest);
};

TEST_F(ProviderIntegrationTest, deserializeStructHavingMemberStruct)
{
    /* Ensure, that all datatypes are registered with Variant::registerType and
     * SeralizerRegistry::registerType to allow JsonSerializer to deserialize the incoming request correctly.
     * It is the task of the type itself and its corresponding serializer to register themselves.
     * Prior to this test, the NeverUsedAsAttributeTypeOrMethodParameterStruct
     * has not been registered, thus the deserialization of the joynr.Request failed.
     */
    std::string serializedContent(
                R"({"_typeName":"joynr.Request",)"
                R"("methodName": "setAttributeArrayOfNestedStructs",)"
                R"("paramDatatypes": ["List"],)"
                R"("params": [[{"_typeName":"joynr.tests.testTypes.HavingComplexArrayMemberStruct",)"
                R"("arrayMember": [{"_typeName":"joynr.tests.testTypes.NeverUsedAsAttributeTypeOrMethodParameterStruct",)"
                R"("name": "neverUsed"}]}]],)"
                R"("requestReplyId": "570b7626-4140-4714-922a-a7c49c52c54c"})");
    JsonSerializer::deserialize<Request>(serializedContent);
}
