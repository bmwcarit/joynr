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
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "libjoynr/uds/UdsSendQueue.h"
#include "libjoynr/uds/UdsFrameBufferV1.h"

#include "tests/PrettyPrint.h"

using namespace joynr;
using namespace testing;

/** Checks the correct UdsSendQueue using UdsFrameBufferV1 as template argument. */
class UdsSendQueueTest : public ::testing::Test
{
public:
    // For simplicity, the UDS frames hold only one byte of payload data
    using ErrorValues = std::pair<smrf::Byte, exceptions::JoynrRuntimeException>;
    std::vector<smrf::Byte> _queuedFrameData;
    std::vector<ErrorValues> _queuedErrorCallbacks;

    /*
     * For the UdsSendQueue, the frame serialization (cookie, different frame types, ...) is
     * transparent, hence only UdsFrameBufferV1 message frames are checked (using one byte of
     * payload).
     * @param data Payload data
     */
    UdsFrameBufferV1 createFrame(const smrf::Byte& data)
    {
        _queuedFrameData.push_back(data);
        const smrf::ByteVector dataVector(1, data);
        return UdsFrameBufferV1(smrf::ByteArrayView(dataVector));
    }

    std::vector<smrf::Byte> getErrorCallbackData()
    {
        std::vector<smrf::Byte> result(_queuedErrorCallbacks.size());
        std::transform(_queuedErrorCallbacks.begin(),
                       _queuedErrorCallbacks.end(),
                       result.begin(),
                       [](const ErrorValues& values) { return values.first; });
        return result;
    }

    static smrf::Byte extractBodyData(const boost::asio::const_buffer& buffer)
    {
        static constexpr auto bodyPos =
                sizeof(UdsFrameBufferV1::Cookie) + sizeof(UdsFrameBufferV1::BodyLength);
        static constexpr auto expectedSize = bodyPos + sizeof(smrf::Byte);
        if (expectedSize != buffer.size()) {
            throw std::invalid_argument(
                    "Internal test error. ASIO buffer size does not match expectation.");
        }
        static_assert(1 == sizeof(smrf::Byte), "Pointer arithmetic will fail.");
        const auto* bodyPtr = static_cast<const smrf::Byte*>(buffer.data()) + bodyPos;
        return *bodyPtr;
    }
};

TEST_F(UdsSendQueueTest, insertRemove)
{
    constexpr std::size_t hugeLimitNeverReached = 10;
    UdsSendQueue<UdsFrameBufferV1> test(hugeLimitNeverReached);

    EXPECT_EQ(test.showFront().size(), 0)
            << "In case queue is emtpy, show front should return empty message";

    const IUdsSender::SendFailed noErrors = [](const exceptions::JoynrRuntimeException&) {
        throw std::invalid_argument("No errors expected, since a send failure is only triggered in "
                                    "case the queue is full.");
    };
    // The return value true means that another state can be added to the state machine
    EXPECT_EQ(test.pushBack(createFrame(1), noErrors), true)
            << "'true' expected since new write state required";
    EXPECT_EQ(extractBodyData(test.showFront()), 1) << "Queue element not inserted.";
    EXPECT_EQ(test.pushBack(createFrame(2), noErrors), false)
            << "'false' expected since new write state pending";
    EXPECT_EQ(extractBodyData(test.showFront()), 1)
            << "Front queue element not the one inserted first.";
    EXPECT_EQ(test.popFrontOnSuccess(
                      boost::system::error_code(42, boost::system::generic_category())),
              false)
            << "'false' expected since write failed";
    EXPECT_EQ(extractBodyData(test.showFront()), 1) << "Front queue element removed on failure.";
    EXPECT_EQ(test.popFrontOnSuccess(boost::system::error_code()), true)
            << "'true' expected since write succeeded and other itmes are pending";
    EXPECT_EQ(extractBodyData(test.showFront()), 2)
            << "Front queue element not removed on success.";
    EXPECT_EQ(test.popFrontOnSuccess(boost::system::error_code()), false)
            << "''false' expected since write succeeded but no more itmes are pending";
    EXPECT_EQ(test.showFront().size(), 0) << "Last element not removed on succcess.";
}

TEST_F(UdsSendQueueTest, queueLimitExceeded)
{
    constexpr smrf::Byte testLimit{3};
    UdsSendQueue<UdsFrameBufferV1> test(testLimit);

    for (smrf::Byte i = 0; i < testLimit; i++) {
        test.pushBack(createFrame(i), [this, i](const exceptions::JoynrRuntimeException& ex) {
            _queuedErrorCallbacks.push_back({i, ex});
        });
    }
    EXPECT_EQ(_queuedErrorCallbacks.size(), 0)
            << "Error callbacks executed, though queue limit has not been exceeded.";

    constexpr smrf::Byte latestValue{testLimit + 1};
    test.pushBack(createFrame(latestValue),
                  [this, latestValue](const exceptions::JoynrRuntimeException& ex) {
        _queuedErrorCallbacks.push_back({latestValue, ex});
    });

    EXPECT_EQ(latestValue, extractBodyData(test.showFront()))
            << "Front queue element not the one inserted after queue limit reached.";
    EXPECT_EQ(testLimit, _queuedErrorCallbacks.size())
            << "Error callbacks not executed though queue limit has been exceeded.";
    _queuedFrameData.pop_back();
    EXPECT_EQ(getErrorCallbackData(), _queuedFrameData)
            << "Unexpected data reported by error call backs";
}

TEST_F(UdsSendQueueTest, zeroLimit)
{
    constexpr smrf::Byte testIterations{3};
    UdsSendQueue<UdsFrameBufferV1> test(0);

    for (smrf::Byte i = 0; i < testIterations; i++) {
        test.pushBack(createFrame(i), [this, i](const exceptions::JoynrRuntimeException& ex) {
            _queuedErrorCallbacks.push_back({i, ex});
        });
        EXPECT_EQ(_queuedErrorCallbacks.size(), i);
    }
}
