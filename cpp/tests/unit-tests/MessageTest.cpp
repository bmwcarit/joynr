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
#include "joynr/PrivateCopyAssign.h"
#include "joynr/TimePoint.h"

#include "tests/mock/MockKeychain.h"

using namespace ::testing;
using namespace joynr;

class ImmutableMessageTest : public ::testing::Test
{
public:
    ImmutableMessageTest()
    {
        _mutableMessage.setSender("sender");
        _mutableMessage.setRecipient("recipient");
        _mutableMessage.setExpiryDate(TimePoint::fromRelativeMs(1234567));
        _mutableMessage.setReplyTo("replyTo");
        _mutableMessage.setEffort("effort");
        _mutableMessage.setPayload("payload");
    }

protected:
    MutableMessage _mutableMessage;
    std::unique_ptr<ImmutableMessage> createImmutableMessage(
            std::unordered_map<std::string, std::string> prefixedCustomHeaders)
    {
        MutableMessage message;

        message.setPrefixedCustomHeaders(prefixedCustomHeaders);

        return message.getImmutableMessage();
    }

private:
    DISALLOW_COPY_AND_ASSIGN(ImmutableMessageTest);
};

TEST_F(ImmutableMessageTest, retrieveCustomHeaders_tooShortHeader)
{
    std::unique_ptr<joynr::ImmutableMessage> message = createImmutableMessage({{"c", "value"}});

    auto prefixedCustomHeaders = message->getPrefixedCustomHeaders();
    auto customHeaders = message->getCustomHeaders();

    EXPECT_EQ(prefixedCustomHeaders.size(), 0);
    EXPECT_EQ(customHeaders.size(), 0);
}

TEST_F(ImmutableMessageTest, retrieveCustomHeaders)
{
    const std::string headerKey = "my-header-key";
    const std::string prefixedHeaderKey = joynr::Message::CUSTOM_HEADER_PREFIX() + headerKey;

    std::unique_ptr<joynr::ImmutableMessage> message =
            createImmutableMessage({{prefixedHeaderKey, "value"}});

    auto prefixedCustomHeaders = message->getPrefixedCustomHeaders();
    auto customHeaders = message->getCustomHeaders();

    EXPECT_EQ(prefixedCustomHeaders.size(), 1);
    EXPECT_EQ(customHeaders.size(), 1);

    EXPECT_EQ(customHeaders.cbegin()->first, headerKey);
    EXPECT_EQ(prefixedCustomHeaders.cbegin()->first, prefixedHeaderKey);
}

TEST_F(ImmutableMessageTest, isReceivedFromGlobal)
{
    MutableMessage localMutableMessage;
    auto immutableMessage = localMutableMessage.getImmutableMessage();
    const bool expectedValue = true;
    immutableMessage->setReceivedFromGlobal(expectedValue);
    EXPECT_EQ(immutableMessage->isReceivedFromGlobal(), expectedValue);
}

TEST_F(ImmutableMessageTest, TestIsLocalInMutableMessage)
{
    const bool expectedValue = true;
    _mutableMessage.setLocalMessage(expectedValue);
    EXPECT_EQ(_mutableMessage.isLocalMessage(), expectedValue);
}

TEST_F(ImmutableMessageTest, TestMessageRoundtripInMutableMessage)
{
    auto immutableMessage = _mutableMessage.getImmutableMessage();

    EXPECT_EQ(_mutableMessage.getSender(), immutableMessage->getSender());
    EXPECT_EQ(_mutableMessage.getRecipient(), immutableMessage->getRecipient());
    EXPECT_EQ(_mutableMessage.getExpiryDate(), immutableMessage->getExpiryDate());
    EXPECT_EQ(_mutableMessage.getId(), immutableMessage->getId());
    EXPECT_EQ(_mutableMessage.getType(), immutableMessage->getType());
    EXPECT_EQ(_mutableMessage.getEffort(), *(immutableMessage->getEffort()));

    smrf::ByteArrayView byteArrayView = immutableMessage->getUnencryptedBody();
    std::string payload(byteArrayView.data(), byteArrayView.data() + byteArrayView.size());
    EXPECT_EQ(_mutableMessage.getPayload(), payload);
}

TEST_F(ImmutableMessageTest, TestOwnerSigningCallbackInMutableMessage)
{
    auto mockKeyChain = std::make_shared<MockKeychain>();
    _mutableMessage.setKeychain(mockKeyChain);
    std::string signatureWithOwnerIdStr = std::string("testSignatureSignedWithOwnerId");

    ON_CALL(*mockKeyChain, getOwnerId()).WillByDefault(Return(signatureWithOwnerIdStr));
    auto immutableMessage = _mutableMessage.getImmutableMessage();
    auto signatureByteArrayView = immutableMessage->getSignature();
    std::string signatureStr(signatureByteArrayView.data(),
                             signatureByteArrayView.data() + signatureByteArrayView.size());
    EXPECT_EQ(signatureWithOwnerIdStr, signatureStr);
}

TEST_F(ImmutableMessageTest, isNotCompressedByDefault)
{
    auto immutableMessage = _mutableMessage.getImmutableMessage();
    EXPECT_FALSE(immutableMessage->isCompressed());
}

TEST_F(ImmutableMessageTest, isCompressed)
{
    const bool expectedValue = true;
    _mutableMessage.setCompress(expectedValue);
    auto immutableMessage = _mutableMessage.getImmutableMessage();
    EXPECT_EQ(immutableMessage->isCompressed(), expectedValue);
}
