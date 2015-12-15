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
#include "joynr/ReplyCaller.h"
#include "tests/utils/MockObjects.h"

using ::testing::Property;
using ::testing::Eq;
using ::testing::ByRef;
using ::testing::NotNull;
using ::testing::_;
using namespace ::testing;

MATCHER(timeoutException, "") {
    return (dynamic_cast<const joynr::exceptions::JoynrTimeOutException*>(&arg) != nullptr)
            && arg.getMessage() == "timeout waiting for the response";
}

using namespace joynr;

/*
 * This tests the ReplyCaller class, for all template specifications i.e. void and not void.
 */

class ReplyCallerTest : public ::testing::Test {
public:
    ReplyCallerTest()
        : intCallback(new MockCallback<int>()),
          intFixture(std::bind(&MockCallback<int>::onSuccess, intCallback, std::placeholders::_2),
                     std::bind(&MockCallback<int>::onError, intCallback, std::placeholders::_1, std::placeholders::_2)),
          voidCallback(new MockCallback<void>()),
          voidFixture(std::bind(&MockCallback<void>::onSuccess, voidCallback),
                      std::bind(&MockCallback<void>::onError, voidCallback, std::placeholders::_1, std::placeholders::_2)) {}

    std::shared_ptr<MockCallback<int>> intCallback;
    ReplyCaller<int> intFixture;
    std::shared_ptr<MockCallback<void>> voidCallback;
    ReplyCaller<void> voidFixture;
};

typedef ReplyCallerTest ReplyCallerDeathTest;

TEST_F(ReplyCallerTest, getType) {
    ASSERT_EQ(Util::getTypeId<int>(), intFixture.getTypeId());
}

TEST_F(ReplyCallerTest, getTypeInt64_t) {
    std::shared_ptr<MockCallback<int64_t>> callback(new MockCallback<int64_t>());
    ReplyCaller<int64_t> int64_tReplyCaller(
                [callback](const RequestStatus& status, const int64_t& value) {
                    callback->onSuccess(value);
                },
                [](const RequestStatus& status, const exceptions::JoynrException& error){
                });
    ASSERT_EQ(Util::getTypeId<int64_t>(), int64_tReplyCaller.getTypeId());
}

TEST_F(ReplyCallerTest, getTypeInt8_t) {
    std::shared_ptr<MockCallback<int8_t>> callback(new MockCallback<int8_t>());
    ReplyCaller<int8_t> int8_tReplyCaller(
                [callback](const RequestStatus& status, const int8_t& value) {
                    callback->onSuccess(value);
                },
                [](const RequestStatus& status, const exceptions::JoynrException& error){
                });
    ASSERT_EQ(Util::getTypeId<int8_t>(), int8_tReplyCaller.getTypeId());
}

TEST_F(ReplyCallerTest, getTypeForVoid) {
    int typeId = voidFixture.getTypeId();
    ASSERT_EQ(Util::getTypeId<void>(), typeId);
}


TEST_F(ReplyCallerTest, timeOut) {
    EXPECT_CALL(*intCallback, onError(
                    Property(&RequestStatus::getCode, RequestStatusCode::ERROR_TIMEOUT_WAITING_FOR_RESPONSE),timeoutException()));
    intFixture.timeOut();
}

TEST_F(ReplyCallerTest, timeOutForVoid) {
    EXPECT_CALL(*voidCallback, onError(
                    Property(&RequestStatus::getCode, RequestStatusCode::ERROR_TIMEOUT_WAITING_FOR_RESPONSE),timeoutException()));
    voidFixture.timeOut();
}

TEST_F(ReplyCallerTest, resultReceived) {
    EXPECT_CALL(*intCallback, onSuccess(7));
    intFixture.returnValue(7);
}

TEST_F(ReplyCallerTest, resultReceivedForVoid) {
    EXPECT_CALL(*voidCallback, onSuccess());
    voidFixture.returnValue();
}

