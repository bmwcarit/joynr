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

private:
    DISALLOW_COPY_AND_ASSIGN(MessageQueueTest);
};

TEST_F(MessageQueueTest, initialQueueIsEmpty) {
    EXPECT_EQ(messageQueue.getQueueLength(), 0);
}

TEST_F(MessageQueueTest, addMultipleMessages) {
    MutableMessage mutableMsg1;
    mutableMsg1.setExpiryDate(expiryDate);
    auto immutableMsg1 = mutableMsg1.getImmutableMessage();
    auto recipient1 = immutableMsg1->getRecipient();
    EXPECT_EQ(messageQueue.queueMessage(recipient1, std::move(immutableMsg1)), 1);

    MutableMessage mutableMsg2;
    mutableMsg2.setExpiryDate(expiryDate);
    auto immutableMsg2 = mutableMsg2.getImmutableMessage();
    auto recipient2 = immutableMsg2->getRecipient();
    EXPECT_EQ(messageQueue.queueMessage(recipient2, std::move(immutableMsg2)), 2);

    MutableMessage mutableMsg3;
    mutableMsg3.setExpiryDate(expiryDate);
    auto immutableMsg3 = mutableMsg3.getImmutableMessage();
    auto recipient3 = immutableMsg3->getRecipient();
    EXPECT_EQ(messageQueue.queueMessage(recipient3, std::move(immutableMsg3)), 3);

    MutableMessage mutableMsg4;
    mutableMsg4.setExpiryDate(expiryDate);
    auto immutableMsg4 = mutableMsg4.getImmutableMessage();
    auto recipient4 = immutableMsg4->getRecipient();
    EXPECT_EQ(messageQueue.queueMessage(recipient4, std::move(immutableMsg4)), 4);
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
    compareMutableImmutableMessage(mutableMsg1, item->getContent());
    EXPECT_EQ(messageQueue.getQueueLength(), 1);

    item = messageQueue.getNextMessageFor(recipient2);
    compareMutableImmutableMessage(mutableMsg2, item->getContent());
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
    compareMutableImmutableMessage(mutableMessage, item->getContent());
    EXPECT_EQ(messageQueue.getQueueLength(), 1);

    item = messageQueue.getNextMessageFor(participantId);
    compareMutableImmutableMessage(mutableMessage, item->getContent());
    EXPECT_EQ(messageQueue.getQueueLength(), 0);
}

TEST_F(MessageQueueTest, dequeueInvalidParticipantId) {
    EXPECT_EQ(messageQueue.getNextMessageFor("TEST"), nullptr);
}
