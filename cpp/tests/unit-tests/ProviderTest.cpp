/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
#include <gmock/gmock.h>
#include <string>

#include "joynr/AbstractJoynrProvider.h"
#include "libjoynr/subscription/SubscriptionAttributeListener.h"
#include "tests/utils/MockObjects.h"
#include "joynr/tests/testProvider.h"
#include "joynr/tests/TestWithoutVersionProvider.h"

using namespace joynr;

class DummyProvider : public AbstractJoynrProvider {
public:
    types::ProviderQos getProviderQos() const {
        types::ProviderQos ret;
        return ret;
    }
    std::string getInterfaceName() const {
        return "DummyProviderInterface";
    }

    void onAttributeValueChanged(const std::string& attributeName, const Variant& value) {
        AbstractJoynrProvider::onAttributeValueChanged(attributeName, value);
    }

    void fireBroadcast(const std::string& broadcastName, const std::vector<Variant>& values) {
        AbstractJoynrProvider::fireBroadcast(broadcastName, values);
    }
};

TEST(ProviderTest, register_attributeListener) {
    MockPublicationManager publicationManager;
    std::string attributeName("testAttribute");
    std::string subscriptionId("test-subscription-id");
    Variant attributeValue(Variant::make<int>(42));

    // Expect the publicationManager to be called when the attribute value changes
    EXPECT_CALL(publicationManager,
                attributeValueChanged(Eq(subscriptionId),Eq(attributeValue)))
            .Times(1);

    DummyProvider provider;
    provider.registerAttributeListener(attributeName,
                                       new SubscriptionAttributeListener(subscriptionId, publicationManager));

    provider.onAttributeValueChanged(attributeName, attributeValue);
}

TEST(ProviderTest, unregister_attributeListener) {
    MockPublicationManager publicationManager;
    std::string attributeName("testAttribute");
    std::string subscriptionId("test-subscription-id");
    Variant attributeValue(Variant::make<int>(42));

    // Expect the publicationManager not to be called when the attribute value changes
    EXPECT_CALL(publicationManager,
                attributeValueChanged(Eq(subscriptionId),Eq(attributeValue)))
            .Times(0);

    DummyProvider provider;

    // This should not contact the publicationManager
    provider.onAttributeValueChanged(attributeName, attributeValue);

    // Do a register then unregister
    SubscriptionAttributeListener* attributeListener =
            new SubscriptionAttributeListener(subscriptionId, publicationManager);
    provider.registerAttributeListener(attributeName, attributeListener);
    provider.unregisterAttributeListener(attributeName, attributeListener);

    // This should not contact the publicationManager
    provider.onAttributeValueChanged(attributeName, attributeValue);
}

TEST(ProviderTest, versionIsSetCorrectly) {
    std::uint32_t expectedMajorVersion = 47;
    std::uint32_t expectedMinorVersion = 11;
    EXPECT_EQ(expectedMajorVersion, tests::testProvider::MAJOR_VERSION);
    EXPECT_EQ(expectedMinorVersion, tests::testProvider::MINOR_VERSION);
}

TEST(ProviderTest, defaultVersionIsSetCorrectly) {
    std::uint32_t expectedDefaultMajorVersion = 0;
    std::uint32_t expectedDefaultMinorVersion = 0;
    EXPECT_EQ(expectedDefaultMajorVersion, tests::TestWithoutVersionProvider::MAJOR_VERSION);
    EXPECT_EQ(expectedDefaultMinorVersion, tests::TestWithoutVersionProvider::MINOR_VERSION);
}
