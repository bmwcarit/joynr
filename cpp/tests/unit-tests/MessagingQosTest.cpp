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
#include "joynr/MessagingQos.h"
#include "joynr/Logger.h"
#include "tests/utils/Gtest.h"
#include <chrono>
#include <cstdint>
#include <exception>
#include <iostream>

using namespace joynr;

class MessagingQosTest : public ::testing::Test
{
public:
    MessagingQosTest()
    {
        qos = MessagingQos();
    }

protected:
    ADD_LOGGER(MessagingQosTest)
    MessagingQos qos;
};

TEST_F(MessagingQosTest, addCustomHeader_validData)
{
    qos.putCustomMessageHeader("test-header", "test aA0-;:,+&?.*/\\_");
    const auto& headers = qos.getCustomMessageHeaders();
    EXPECT_EQ(1, headers.size());
    EXPECT_NE(headers.cend(), headers.find("test-header"));
    EXPECT_EQ(headers.cend(), headers.find("unknown"));
    EXPECT_STREQ("test aA0-;:,+&?.*/\\_", headers.at("test-header").c_str());
}

TEST_F(MessagingQosTest, addCustomHeader_invalidKeyData)
{
    EXPECT_THROW(qos.putCustomMessageHeader("$  ! _ 123 test-header", "test-header-value"),
                 std::invalid_argument);
}

TEST_F(MessagingQosTest, addCustomHeader_invalidValueData)
{
    EXPECT_THROW(qos.putCustomMessageHeader("test-header", "😳"), std::invalid_argument);
}

TEST_F(MessagingQosTest, addManyCustomHeaders_validData)
{
    std::unordered_map<std::string, std::string> values;
    values["one"] = "the first entry";
    values["two"] = "the second entry";
    values["three"] = "the third entry";
    qos.putAllCustomMessageHeaders(values);
    const auto& headers = qos.getCustomMessageHeaders();
    EXPECT_EQ(3, headers.size());
    EXPECT_NE(headers.cend(), headers.find("one"));
    EXPECT_NE(headers.cend(), headers.find("two"));
    EXPECT_NE(headers.cend(), headers.find("three"));
    EXPECT_EQ(headers.cend(), headers.find("unknown"));
    EXPECT_STREQ("the first entry", headers.at("one").c_str());
    EXPECT_STREQ("the second entry", headers.at("two").c_str());
    EXPECT_STREQ("the third entry", headers.at("three").c_str());
}

TEST_F(MessagingQosTest, addManyCustomHeaders_invalidData)
{
    std::unordered_map<std::string, std::string> values;
    values["one"] = "the first entry";
    values["two"] = "the second entry";
    values["three"] = "the third entry";
    values["broken"] = "😳";
    EXPECT_THROW(qos.putAllCustomMessageHeaders(values), std::invalid_argument);
}

TEST_F(MessagingQosTest, defaultEffort)
{
    EXPECT_EQ(MessagingQosEffort::Enum::NORMAL, qos.getEffort());
}

TEST_F(MessagingQosTest, setCustomEffort)
{
    EXPECT_EQ(MessagingQosEffort::Enum::NORMAL, qos.getEffort());
    qos.setEffort(MessagingQosEffort::Enum::BEST_EFFORT);
    EXPECT_EQ(MessagingQosEffort::Enum::BEST_EFFORT, qos.getEffort());
}

TEST_F(MessagingQosTest, defaultEncrypt)
{
    EXPECT_EQ(false, qos.getEncrypt());
    EXPECT_EQ(false, qos.getCompress());
}

TEST_F(MessagingQosTest, setCustomEncrypt)
{
    EXPECT_EQ(false, qos.getEncrypt());
    qos.setEncrypt(true);
    EXPECT_EQ(true, qos.getEncrypt());
}

TEST_F(MessagingQosTest, setCustomCompress)
{
    EXPECT_EQ(false, qos.getCompress());
    qos.setCompress(true);
    EXPECT_EQ(true, qos.getCompress());
}

TEST_F(MessagingQosTest, constructorWithCustomEffort)
{
    MessagingQos customEffortInstance = MessagingQos(0L, MessagingQosEffort::Enum::BEST_EFFORT);
    EXPECT_EQ(MessagingQosEffort::Enum::BEST_EFFORT, customEffortInstance.getEffort());
}

TEST_F(MessagingQosTest, constructorWithCustomEncrypt)
{
    bool encrypt = true;
    MessagingQos customEncryptInstance =
            MessagingQos(0L, MessagingQosEffort::Enum::BEST_EFFORT, encrypt);
    EXPECT_EQ(encrypt, customEncryptInstance.getEncrypt());

    customEncryptInstance = MessagingQos(MessagingQosEffort::Enum::BEST_EFFORT, encrypt);
    EXPECT_EQ(encrypt, customEncryptInstance.getEncrypt());

    customEncryptInstance = MessagingQos(0L, encrypt);
    EXPECT_EQ(encrypt, customEncryptInstance.getEncrypt());
}
