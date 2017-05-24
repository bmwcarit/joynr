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
    MessageQueue messageQueue;
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
    EXPECT_EQ(messageQueue.queueMessage(mutableMsg1.getImmutableMessage()), 1);
    MutableMessage mutableMsg2;
    mutableMsg2.setExpiryDate(expiryDate);
    EXPECT_EQ(messageQueue.queueMessage(mutableMsg2.getImmutableMessage()), 2);
    MutableMessage mutableMsg3;
    mutableMsg3.setExpiryDate(expiryDate);
    EXPECT_EQ(messageQueue.queueMessage(mutableMsg3.getImmutableMessage()), 3);
    MutableMessage mutableMsg4;
    mutableMsg4.setExpiryDate(expiryDate);
    EXPECT_EQ(messageQueue.queueMessage(mutableMsg4.getImmutableMessage()), 4);
}

TEST_F(MessageQueueTest, queueDequeueMessages) {
    // add messages to the queue
    MutableMessage mutableMsg1;
    mutableMsg1.setRecipient("TEST1");
    mutableMsg1.setExpiryDate(expiryDate);
    messageQueue.queueMessage(mutableMsg1.getImmutableMessage());

    MutableMessage mutableMsg2;
    mutableMsg2.setRecipient("TEST2");
    mutableMsg2.setExpiryDate(expiryDate);
    messageQueue.queueMessage(mutableMsg2.getImmutableMessage());
    EXPECT_EQ(messageQueue.getQueueLength(), 2);

    // get messages from queue
    auto item = messageQueue.getNextMessageForParticipant("TEST1");
    compareMutableImmutableMessage(mutableMsg1, item->getContent());
    EXPECT_EQ(messageQueue.getQueueLength(), 1);

    item = messageQueue.getNextMessageForParticipant("TEST2");
    compareMutableImmutableMessage(mutableMsg2, item->getContent());
    EXPECT_EQ(messageQueue.getQueueLength(), 0);
}

TEST_F(MessageQueueTest, queueDequeueMultipleMessagesForOneParticipant) {
    // add messages to the queue
    MutableMessage mutableMessage;
    mutableMessage.setRecipient("TEST");
    mutableMessage.setExpiryDate(expiryDate);
    messageQueue.queueMessage(mutableMessage.getImmutableMessage());
    messageQueue.queueMessage(mutableMessage.getImmutableMessage());
    EXPECT_EQ(messageQueue.getQueueLength(), 2);

    // get messages from queue
    auto item = messageQueue.getNextMessageForParticipant("TEST");
    compareMutableImmutableMessage(mutableMessage, item->getContent());
    EXPECT_EQ(messageQueue.getQueueLength(), 1);

    item = messageQueue.getNextMessageForParticipant("TEST");
    compareMutableImmutableMessage(mutableMessage, item->getContent());
    EXPECT_EQ(messageQueue.getQueueLength(), 0);
}

TEST_F(MessageQueueTest, dequeueInvalidParticipantId) {
    EXPECT_FALSE(messageQueue.getNextMessageForParticipant("TEST"));
}
