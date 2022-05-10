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
#include <string>

#include "tests/utils/Gtest.h"
#include "tests/utils/Gmock.h"

#include "joynr/AbstractJoynrProvider.h"
#include "joynr/SubscriptionAttributeListener.h"
#include "joynr/SingleThreadedIOService.h"

#include "joynr/types/ProviderQos.h"

#include "joynr/tests/DefaulttestProvider.h"
#include "joynr/tests/TestWithoutVersionProvider.h"

#include "tests/utils/MyTestProvider.h"

using namespace joynr;

class DummyProvider : public AbstractJoynrProvider
{
public:
    types::ProviderQos getProviderQos() const
    {
        types::ProviderQos ret;
        return ret;
    }

    const std::string& getInterfaceName() const override
    {
        static const std::string interfaceName = "DummyProviderInterface";
        return interfaceName;
    }

    template <typename T>
    void onAttributeValueChanged(const std::string& attributeName, const T& value)
    {
        AbstractJoynrProvider::onAttributeValueChanged(attributeName, value);
    }

    template <typename... Ts>
    void fireBroadcast(const std::string& broadcastName, const Ts&... values)
    {
        AbstractJoynrProvider::fireBroadcast(broadcastName, std::forward<Ts>(values)...);
    }
};

TEST(ProviderTest, versionIsSetCorrectly)
{
    const std::uint32_t expectedMajorVersion = 47;
    const std::uint32_t expectedMinorVersion = 11;
    EXPECT_EQ(expectedMajorVersion, tests::testProvider::MAJOR_VERSION);
    EXPECT_EQ(expectedMinorVersion, tests::testProvider::MINOR_VERSION);
}

TEST(ProviderTest, defaultVersionIsSetCorrectly)
{
    const std::uint32_t expectedDefaultMajorVersion = 0;
    const std::uint32_t expectedDefaultMinorVersion = 0;
    EXPECT_EQ(expectedDefaultMajorVersion, tests::TestWithoutVersionProvider::MAJOR_VERSION);
    EXPECT_EQ(expectedDefaultMinorVersion, tests::TestWithoutVersionProvider::MINOR_VERSION);
}
