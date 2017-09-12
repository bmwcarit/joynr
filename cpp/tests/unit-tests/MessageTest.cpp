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

#include "joynr/ImmutableMessage.h"
#include "joynr/Message.h"
#include "joynr/MutableMessage.h"
#include "joynr/DispatcherUtils.h"

#include "tests/utils/MockObjects.h"

using namespace ::testing;
using namespace joynr;

class ImmutableMessageTest : public ::testing::Test {
public:
    ImmutableMessageTest() {
        mutableMessage.setSender("sender");
        mutableMessage.setRecipient("recipient");
        mutableMessage.setExpiryDate(DispatcherUtils::convertTtlToAbsoluteTime(1234567));
        mutableMessage.setReplyTo("replyTo");
        mutableMessage.setEffort("effort");
        mutableMessage.setPayload("payload");
    }

protected:
    MutableMessage mutableMessage;
    std::unique_ptr<ImmutableMessage> createImmutableMessage(std::unordered_map<std::string, std::string> prefixedCustomHeaders) {
        MutableMessage message;

        message.setPrefixedCustomHeaders(prefixedCustomHeaders);

        return message.getImmutableMessage();
    }

private:
    DISALLOW_COPY_AND_ASSIGN(ImmutableMessageTest);
};

TEST_F(ImmutableMessageTest, retrieveCustomHeaders_tooShortHeader) {
    std::unique_ptr<joynr::ImmutableMessage> message = createImmutableMessage({{"c","value"}});

    auto prefixedCustomHeaders = message->getPrefixedCustomHeaders();
    auto customHeaders = message->getCustomHeaders();

    EXPECT_EQ(prefixedCustomHeaders.size(), 0);
    EXPECT_EQ(customHeaders.size(), 0);
}

TEST_F(ImmutableMessageTest, retrieveCustomHeaders) {
    const std::string headerKey = "my-header-key";
    const std::string prefixedHeaderKey = joynr::Message::CUSTOM_HEADER_PREFIX() + headerKey;

    std::unique_ptr<joynr::ImmutableMessage> message = createImmutableMessage({{prefixedHeaderKey,"value"}});

    auto prefixedCustomHeaders = message->getPrefixedCustomHeaders();
    auto customHeaders = message->getCustomHeaders();

    EXPECT_EQ(prefixedCustomHeaders.size(), 1);
    EXPECT_EQ(customHeaders.size(), 1);

    EXPECT_EQ(customHeaders.cbegin()->first, headerKey);
    EXPECT_EQ(prefixedCustomHeaders.cbegin()->first, prefixedHeaderKey);
}

TEST_F(ImmutableMessageTest, isReceivedFromGlobal)
{
    MutableMessage mutableMessage;
    auto immutableMessage = mutableMessage.getImmutableMessage();
    const bool expectedValue = true;
    immutableMessage->setReceivedFromGlobal(expectedValue);
    EXPECT_EQ(immutableMessage->isReceivedFromGlobal(), expectedValue);
}

TEST_F(ImmutableMessageTest, TestIsLocalInMutableMessage)
{
    const bool expectedValue = true;
    mutableMessage.setLocalMessage(expectedValue);
    EXPECT_EQ(mutableMessage.isLocalMessage(), expectedValue);
}

TEST_F(ImmutableMessageTest, TestMessageRoundtripInMutableMessage)
{
    auto immutableMessage = mutableMessage.getImmutableMessage();

    EXPECT_EQ(mutableMessage.getSender(), immutableMessage->getSender());
    EXPECT_EQ(mutableMessage.getRecipient(), immutableMessage->getRecipient());
    EXPECT_EQ(mutableMessage.getExpiryDate(), immutableMessage->getExpiryDate());
    EXPECT_EQ(mutableMessage.getId(), immutableMessage->getId());
    EXPECT_EQ(mutableMessage.getType(), immutableMessage->getType());
    EXPECT_EQ(mutableMessage.getEffort(), *(immutableMessage->getEffort()));

    smrf::ByteArrayView byteArrayView = immutableMessage->getUnencryptedBody();
    std::string payload(byteArrayView.data(), byteArrayView.data() + byteArrayView.size());
    EXPECT_EQ(mutableMessage.getPayload(), payload);
}

TEST_F(ImmutableMessageTest, TestOwnerSigningCallbackInMutableMessage)
{
    auto mockKeyChain = std::make_shared<MockKeychain>();
    mutableMessage.setKeychain(mockKeyChain);
    std::string signatureWithOwnerIdStr = std::string("testSignatureSignedWithOwnerId");

    ON_CALL(*mockKeyChain, getOwnerId()).WillByDefault(Return(signatureWithOwnerIdStr));
    auto immutableMessage = mutableMessage.getImmutableMessage();
    auto signatureByteArrayView = immutableMessage->getSignature();
    std::string signatureStr(signatureByteArrayView.data(), signatureByteArrayView.data() + signatureByteArrayView.size());
    EXPECT_EQ(signatureWithOwnerIdStr, signatureStr);
}
