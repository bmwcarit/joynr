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
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "joynr/ReplyCaller.h"
#include "tests/utils/MockObjects.h"

using ::testing::Property;
using ::testing::Eq;
using ::testing::ByRef;
using ::testing::NotNull;
using ::testing::_;
using namespace ::testing;


using namespace joynr;

/*
 * This tests the ReplyCaller class, for all template specifications i.e. void and not void.
 */

class ReplyCallerTest : public ::testing::Test {
public:
    ReplyCallerTest()
        : intCallback(new MockCallback<int>()),
          intFixture(std::bind(&MockCallback<int>::callbackFct, intCallback, std::placeholders::_1, std::placeholders::_2),
                     std::bind(&MockCallback<int>::errorFct, intCallback, std::placeholders::_1)),
          voidCallback(new MockCallback<void>()),
          voidFixture(std::bind(&MockCallback<void>::callbackFct, voidCallback, std::placeholders::_1),
                      std::bind(&MockCallback<void>::errorFct, voidCallback, std::placeholders::_1)) {}

    QSharedPointer<MockCallback<int>> intCallback;
    ReplyCaller<int> intFixture;
    QSharedPointer<MockCallback<void>> voidCallback;
    ReplyCaller<void> voidFixture;
};

typedef ReplyCallerTest ReplyCallerDeathTest;

TEST_F(ReplyCallerTest, getType) {
    ASSERT_EQ(Util::getTypeId<int>(), intFixture.getTypeId());
}

TEST_F(ReplyCallerTest, getTypeQInt64) {
    QSharedPointer<MockCallback<qint64>> callback(new MockCallback<qint64>());
    ReplyCaller<qint64> qint64ReplyCaller(
                [callback](const RequestStatus& status, const qint64& value) {
                    callback->callbackFct(status, value);
                },
                [](const RequestStatus& status){
                });
    ASSERT_EQ(Util::getTypeId<qint64>(), qint64ReplyCaller.getTypeId());
}

TEST_F(ReplyCallerTest, getTypeQInt8) {
    QSharedPointer<MockCallback<qint8>> callback(new MockCallback<qint8>());
    ReplyCaller<qint8> qint8ReplyCaller(
                [callback](const RequestStatus& status, const qint8& value) {
                    callback->callbackFct(status, value);
                },
                [](const RequestStatus& status){
                });
    ASSERT_EQ(Util::getTypeId<qint8>(), qint8ReplyCaller.getTypeId());
}

TEST_F(ReplyCallerTest, getTypeForVoid) {
    int typeId = voidFixture.getTypeId();
    ASSERT_EQ(Util::getTypeId<void>(), typeId);
}


TEST_F(ReplyCallerTest, timeOut) {
    EXPECT_CALL(*intCallback, errorFct(
                    Property(&RequestStatus::getCode, RequestStatusCode::ERROR_TIMEOUT_WAITING_FOR_RESPONSE)));
    intFixture.timeOut();
}

TEST_F(ReplyCallerTest, timeOutForVoid) {
    EXPECT_CALL(*voidCallback, errorFct(
                    Property(&RequestStatus::getCode, RequestStatusCode::ERROR_TIMEOUT_WAITING_FOR_RESPONSE)));
    voidFixture.timeOut();
}

TEST_F(ReplyCallerTest, resultReceived) {
    EXPECT_CALL(*intCallback, callbackFct(
                    Property(&RequestStatus::getCode, RequestStatusCode::OK),
                    7));
    intFixture.returnValue(7);
}

TEST_F(ReplyCallerTest, resultReceivedForVoid) {
    EXPECT_CALL(*voidCallback, callbackFct(
                    Property(&RequestStatus::getCode, RequestStatusCode::OK)));
    voidFixture.returnValue();
}

