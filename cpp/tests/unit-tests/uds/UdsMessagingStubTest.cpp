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
#include <memory>

#include "tests/utils/Gtest.h"
#include "tests/utils/Gmock.h"

#include "joynr/MutableMessage.h"
#include "joynr/ImmutableMessage.h"
#include "joynr/Semaphore.h"

#include "libjoynr/uds/UdsMessagingStub.h"

#include "tests/JoynrTest.h"
#include "tests/mock/MockCallback.h"
#include "tests/mock/MockIUdsSender.h"

using namespace ::testing;
using namespace joynr;

class UdsMessagingStubTest : public ::testing::Test
{
public:
    UdsMessagingStubTest()
            : _mockIUdsSender(std::make_shared<MockIUdsSender>()),
              _iUdsSender(_mockIUdsSender),
              _udsMessagingStub(std::make_shared<UdsMessagingStub>(_iUdsSender)),
              _semaphore(0),
              _mutableMessage(),
              _immutableMessage(nullptr)
    {
        _mutableMessage.setSender("sender");
        _mutableMessage.setRecipient("recipient");
        _mutableMessage.setExpiryDate(TimePoint::fromRelativeMs(1234567));
        _mutableMessage.setReplyTo("replyTo");
        _mutableMessage.setEffort("effort");
        _mutableMessage.setPayload("payload");
        _immutableMessage = _mutableMessage.getImmutableMessage();
    }

    ~UdsMessagingStubTest() = default;

protected:
    std::shared_ptr<MockIUdsSender> _mockIUdsSender = std::make_shared<MockIUdsSender>();
    std::shared_ptr<IUdsSender> _iUdsSender;
    std::shared_ptr<UdsMessagingStub> _udsMessagingStub;
    Semaphore _semaphore;
    MutableMessage _mutableMessage;
    std::shared_ptr<ImmutableMessage> _immutableMessage;
};

TEST_F(UdsMessagingStubTest, transmitCallsIUdsSenderSendWithCorrectArgs)
{
    smrf::ByteArrayView byteArrayView(_immutableMessage->getSerializedMessage());
    std::string expectedData(byteArrayView.data(), byteArrayView.data() + byteArrayView.size());

    smrf::ByteArrayView capturedByteArrayView;
    EXPECT_CALL(*_mockIUdsSender,
            send(A<const smrf::ByteArrayView &>(),
                A<const std::function<
                void(const joynr::exceptions::JoynrRuntimeException&)>&>()))
        .WillOnce(DoAll(::testing::SaveArg<0>(&capturedByteArrayView), ReleaseSemaphore(&_semaphore)));

    EXPECT_CALL(*_mockIUdsSender, dtorCalled()).Times(1);

    _udsMessagingStub->transmit(_immutableMessage,
            [](const exceptions::JoynrRuntimeException&) { FAIL() << "onFailure called"; }
    );
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::seconds(2)));
    std::string capturedData(capturedByteArrayView.data(), capturedByteArrayView.data() + capturedByteArrayView.size());
    EXPECT_EQ(expectedData, capturedData);
}

TEST_F(UdsMessagingStubTest, transmitOnErrorIsInvokedWhenCalledFromUdsSenderSend)
{
    exceptions::JoynrRuntimeException expectedException("send exception");
    EXPECT_CALL(*_mockIUdsSender,
            send(A<const smrf::ByteArrayView &>(),
                A<const std::function<
                void(const joynr::exceptions::JoynrRuntimeException&)>&>()))
        .WillOnce(DoAll(InvokeArgument<1>(expectedException), ReleaseSemaphore(&_semaphore)));

    auto callback = std::make_shared<MockCallback<void>>();
    EXPECT_CALL(*callback, onError(Eq(expectedException))).Times(1);
    EXPECT_CALL(*_mockIUdsSender, dtorCalled()).Times(1);

    _udsMessagingStub->transmit(_immutableMessage,
            [callback](const exceptions::JoynrRuntimeException& error) {
                callback->onError(error);
            }
    );
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::seconds(2)));
}
