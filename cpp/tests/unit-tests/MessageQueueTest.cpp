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
#include <chrono>
#include <cstdint>
#include <memory>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/ImmutableMessage.h"
#include "joynr/MessageQueue.h"
#include "joynr/MutableMessage.h"
#include "joynr/PrivateCopyAssign.h"

#include "tests/JoynrTest.h"

using namespace joynr;

class MessageQueueTest : public ::testing::Test {
public:
    MessageQueueTest()
        : messageQueue(),
          expiryDate(std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()) + std::chrono::milliseconds(100))
    {
    }

    ~MessageQueueTest() = default;

protected:
    MessageQueue<std::string> messageQueue;
    JoynrTimePoint expiryDate;

    void createAndQueueMessage(const JoynrTimePoint& expiryDate) {
        MutableMessage mutableMsg;
        mutableMsg.setExpiryDate(expiryDate);
        auto immutableMessage = mutableMsg.getImmutableMessage();
        auto recipient = immutableMessage->getRecipient();
        messageQueue.queueMessage(recipient, std::move(immutableMessage));
    }

private:
    DISALLOW_COPY_AND_ASSIGN(MessageQueueTest);
};

TEST_F(MessageQueueTest, initialQueueIsEmpty) {
    EXPECT_EQ(messageQueue.getQueueLength(), 0);
}

TEST_F(MessageQueueTest, addMultipleMessages) {
    createAndQueueMessage(expiryDate);
    EXPECT_EQ(1, messageQueue.getQueueLength());

    createAndQueueMessage(expiryDate);
    EXPECT_EQ(2, messageQueue.getQueueLength());

    createAndQueueMessage(expiryDate);
    EXPECT_EQ(3, messageQueue.getQueueLength());

    createAndQueueMessage(expiryDate);
    EXPECT_EQ(4, messageQueue.getQueueLength());
}

TEST_F(MessageQueueTest, queueDequeueMessages) {
    // add messages to the queue
    MutableMessage mutableMsg1;
    const std::string recipient1("TEST1");
    mutableMsg1.setRecipient(recipient1);
    mutableMsg1.setExpiryDate(expiryDate);
    auto immutableMsg1 = mutableMsg1.getImmutableMessage();
    messageQueue.queueMessage(recipient1, std::move(immutableMsg1));

    MutableMessage mutableMsg2;
    const std::string recipient2("TEST2");
    mutableMsg2.setRecipient(recipient2);
    mutableMsg2.setExpiryDate(expiryDate);
    auto immutableMsg2 = mutableMsg2.getImmutableMessage();
    messageQueue.queueMessage(recipient2, std::move(immutableMsg2));
    EXPECT_EQ(messageQueue.getQueueLength(), 2);

    // get messages from queue
    auto item = messageQueue.getNextMessageFor(recipient1);
    compareMutableImmutableMessage(mutableMsg1, item);
    EXPECT_EQ(messageQueue.getQueueLength(), 1);

    item = messageQueue.getNextMessageFor(recipient2);
    compareMutableImmutableMessage(mutableMsg2, item);
    EXPECT_EQ(messageQueue.getQueueLength(), 0);
}

TEST_F(MessageQueueTest, queueDequeueMultipleMessagesForOneParticipant) {
    // add messages to the queue
    MutableMessage mutableMessage;
    const std::string participantId("TEST");
    mutableMessage.setRecipient(participantId);
    mutableMessage.setExpiryDate(expiryDate);
    messageQueue.queueMessage(participantId, mutableMessage.getImmutableMessage());
    messageQueue.queueMessage(participantId, mutableMessage.getImmutableMessage());
    EXPECT_EQ(messageQueue.getQueueLength(), 2);

    // get messages from queue
    auto item = messageQueue.getNextMessageFor(participantId);
    compareMutableImmutableMessage(mutableMessage, item);
    EXPECT_EQ(messageQueue.getQueueLength(), 1);

    item = messageQueue.getNextMessageFor(participantId);
    compareMutableImmutableMessage(mutableMessage, item);
    EXPECT_EQ(messageQueue.getQueueLength(), 0);
}

TEST_F(MessageQueueTest, dequeueInvalidParticipantId) {
    EXPECT_EQ(messageQueue.getNextMessageFor("TEST"), nullptr);
}

class MessageQueueWithLimitTest : public ::testing::Test
{
public:
    MessageQueueWithLimitTest(): messageQueue(messageQueueLimit) {}
    ~MessageQueueWithLimitTest() = default;

protected:
    MessageQueue<std::string> messageQueue;
    static constexpr std::uint64_t messageQueueLimit = 4;

private:
    DISALLOW_COPY_AND_ASSIGN(MessageQueueWithLimitTest);
};

// Why this definition needs to be here:
// https://github.com/google/googletest/blob/master/googletest/docs/FAQ.md#the-compiler-complains-about-undefined-references-to-some-static-const-member-variables-but-i-did-define-them-in-the-class-body-whats-wrong
// http://en.cppreference.com/w/cpp/language/definition#ODR-use
constexpr std::uint64_t MessageQueueWithLimitTest::messageQueueLimit;

JoynrTimePoint getExpiryDateFromNow(long long offset)
{
    return (std::chrono::time_point_cast<std::chrono::milliseconds>( std::chrono::system_clock::now()) + std::chrono::milliseconds(offset));
}

TEST_F(MessageQueueWithLimitTest, testAddingMessages)
{
    const int messageCount = 5;
    // Keep in mind that message 1 expires later than message 3. This is done in order to check
    // if removal deletes the message with lowest ttl and not the first inserted message.
    const JoynrTimePoint expiryDate[messageCount] = {
        getExpiryDateFromNow(300),
        getExpiryDateFromNow(200),
        getExpiryDateFromNow(100),
        getExpiryDateFromNow(500),
        getExpiryDateFromNow(600)
    };

    const std::string recipient[messageCount] = {
        "TEST1",
        "TEST2",
        "TEST3",
        "TEST4",
        "TEST5"};

    for (int i=0; i < messageCount; i++) {
        MutableMessage mutableMsg;
        mutableMsg.setRecipient(recipient[i]);
        mutableMsg.setExpiryDate(expiryDate[i]);
        auto immutableMsg = mutableMsg.getImmutableMessage();
        messageQueue.queueMessage(recipient[i], std::move(immutableMsg));
    }

    EXPECT_EQ(messageQueue.getQueueLength(), MessageQueueWithLimitTest::messageQueueLimit);

    // Check if the message with the lowest TTL (message3) was removed.
    EXPECT_EQ(messageQueue.getNextMessageFor(recipient[0])->getRecipient(), recipient[0]);
    EXPECT_EQ(messageQueue.getNextMessageFor(recipient[1])->getRecipient(), recipient[1]);
    EXPECT_EQ(messageQueue.getNextMessageFor(recipient[2]), nullptr);
    EXPECT_EQ(messageQueue.getNextMessageFor(recipient[3])->getRecipient(), recipient[3]);
    EXPECT_EQ(messageQueue.getNextMessageFor(recipient[4])->getRecipient(), recipient[4]);
}
