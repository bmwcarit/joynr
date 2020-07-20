/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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

#include "joynr/MutableMessage.h"
#include "joynr/ImmutableMessage.h"
#include "joynr/Semaphore.h"
#include "tests/JoynrTest.h"
#include "tests/mock/MockCallback.h"
#include "tests/mock/MockIMessagingSkeleton.h"

using namespace ::testing;
using namespace joynr;

TEST(IMessagingSkeletonTest, mockIMessagingSkeletonAPIsCanBeCalled)
{
    MutableMessage mutableMessage;
    std::shared_ptr<ImmutableMessage> immutableMessage = mutableMessage.getImmutableMessage();
    auto mockIMessagingSkeleton = std::make_shared<MockIMessagingSkeleton>();
    std::shared_ptr<IMessagingSkeleton> iMessagingSkeleton = mockIMessagingSkeleton;

    EXPECT_CALL(*mockIMessagingSkeleton,
            transmit(immutableMessage,
                A<const std::function<
                void(const joynr::exceptions::JoynrRuntimeException&)>&>())).Times(1);
    iMessagingSkeleton->transmit(
            immutableMessage,
            [](const exceptions::JoynrRuntimeException& error) {
                std::ignore = error;
                FAIL() << "onError callback invoked";
            }
    );

    std::string creator("creator");
    EXPECT_CALL(*mockIMessagingSkeleton, onMessageReceivedMock(_, _)).Times(1);
    smrf::ByteVector smrfMessage = immutableMessage->getSerializedMessage();
    iMessagingSkeleton->onMessageReceived(std::move(smrfMessage), creator);
    EXPECT_CALL(*mockIMessagingSkeleton, dtorCalled());
}
