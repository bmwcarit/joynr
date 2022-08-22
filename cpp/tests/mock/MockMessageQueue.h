/*
 * #%L
 * %%
 * Copyright (C) 2021 BMW Car IT GmbH
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
#ifndef TESTS_MOCK_MOCKMESSAGEQUEUE_H
#define TESTS_MOCK_MOCKMESSAGEQUEUE_H

#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <string>

#include "tests/utils/Gmock.h"

#include "joynr/ImmutableMessage.h"
#include "joynr/MessageQueue.h"

template <typename T>
class MockMessageQueue : public joynr::MessageQueue<T> {
public:
    MockMessageQueue(std::uint64_t messageQueueLimit = 0,
                     std::uint64_t perKeyMessageQueueLimit = 0,
                     std::uint64_t messageQueueLimitBytes = 0) :
        joynr::MessageQueue<T>(messageQueueLimit,
                     perKeyMessageQueueLimit,
                     messageQueueLimitBytes) {}

    MOCK_METHOD1(setOnMsgsDropped, void(std::function<void(std::deque<std::shared_ptr<joynr::ImmutableMessage>>&)> onMsgsDroppedFunc));
    MOCK_CONST_METHOD0(getQueueLength, std::size_t());
    MOCK_CONST_METHOD0(getQueueSizeBytes, std::size_t());
    MOCK_METHOD2_T(queueMessage, std::deque<std::shared_ptr<joynr::ImmutableMessage>>(const T key, std::shared_ptr<joynr::ImmutableMessage> message));
    MOCK_METHOD1_T(getNextMessageFor, std::shared_ptr<joynr::ImmutableMessage>(const T& key));
    MOCK_METHOD0(removeOutdatedMessages, void());

};

#endif // TESTS_MOCK_MOCKMESSAGEQUEUE_H
