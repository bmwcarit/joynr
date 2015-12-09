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

#include "joynr/types/TestTypes_QtTStructExtended.h"
#include "joynr/types/TestTypes/TStructExtended.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include <unordered_map>

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

TEST_F(ComplexDataTypeTest, hashCodeFunction)
{
    using namespace joynr::system::RoutingTypes;

    WebSocketAddress address1(WebSocketProtocol::WS,
                              "host",
                              4242,
                              "path");

    WebSocketAddress address2(WebSocketProtocol::WS,
                              "host",
                              4242,
                              "path");

    WebSocketAddress address3(WebSocketProtocol::WS,
                              "host",
                              4242,
                              "otherPath");
    EXPECT_EQ(address1.hashCode(), address2.hashCode());
    EXPECT_NE(address1.hashCode(), address3.hashCode());
}

TEST_F(ComplexDataTypeTest, UnorderedMapUsingComplexTypesAsKey)
{
    using namespace joynr::system::RoutingTypes;
    std::unordered_map<WebSocketAddress, std::string> unorderedMapExample;

    std::string address1Host = "host1";
    std::string address2Host = "host2";
    std::string address3Host = "host3";
    WebSocketAddress address1(WebSocketProtocol::WS,
                              address1Host,
                              4242,
                              "path");

    WebSocketAddress address2(WebSocketProtocol::WS,
                              address1Host,
                              4242,
                              "path");

    WebSocketAddress address3(WebSocketProtocol::WS,
                              address3Host,
                              4242,
                              "otherPath");
    EXPECT_TRUE(unorderedMapExample.insert({address1, address1Host}).second);
    auto search = unorderedMapExample.find(address1);

    EXPECT_TRUE(search != unorderedMapExample.end());
    EXPECT_EQ(address1Host, search->second);

    /*
     * unsorted_map does not allow to insert a new pair having a key with
     * the same hashcode as an already existing one
     */
    EXPECT_FALSE(unorderedMapExample.insert({address2, address2Host}).second);

    /*
     * looking for address2 now, brings as the pair inserted via
     * map.insert({address1, address1Host}), as the hashCode of
     * address1 and address2 are the same
     */
    search = unorderedMapExample.find(address2);
    EXPECT_TRUE(search != unorderedMapExample.end());
    EXPECT_EQ(address1Host, search->second);

    EXPECT_TRUE(unorderedMapExample.insert({address3, address3Host}).second);
    search = unorderedMapExample.find(address3);
    EXPECT_TRUE(search != unorderedMapExample.end());
    EXPECT_EQ(address3Host, search->second);
}
