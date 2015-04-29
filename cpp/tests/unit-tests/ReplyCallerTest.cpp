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
#include "joynr/ICallback.h"
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
          intFixture([this] (const RequestStatus& status, const int& input) { intCallback->callbackFct(status, input);}),
          voidCallback(new MockCallback<void>()),
          voidFixture([this] (const RequestStatus& status) { voidCallback->callbackFct(status);}) {}

    QSharedPointer<MockCallback<int>> intCallback;
    ReplyCaller<int> intFixture;
    QSharedPointer<MockCallback<void>> voidCallback;
    ReplyCaller<void> voidFixture;
};

typedef ReplyCallerTest ReplyCallerDeathTest;

TEST_F(ReplyCallerTest, getType) {
    ASSERT_EQ(qMetaTypeId<int>(), intFixture.getTypeId());
}

TEST_F(ReplyCallerTest, getTypeQInt64) {
    QSharedPointer<MockCallback<qint64>> callback(new MockCallback<qint64>());
    ReplyCaller<qint64> qint64ReplyCaller(
                [callback](const RequestStatus& status, const qint64& value) {
                    callback->callbackFct(status, value);
                });
    ASSERT_EQ(qMetaTypeId<qint64>(), qint64ReplyCaller.getTypeId());
}

TEST_F(ReplyCallerTest, getTypeQInt8) {
    QSharedPointer<MockCallback<qint8>> callback(new MockCallback<qint8>());
    ReplyCaller<qint8> qint8ReplyCaller(
                [callback](const RequestStatus& status, const qint8& value) {
                    callback->callbackFct(status, value);
                });
    ASSERT_EQ(qMetaTypeId<qint8>(), qint8ReplyCaller.getTypeId());
}

TEST_F(ReplyCallerTest, getTypeForVoid) {
    QString type = voidFixture.getTypeName();
    ASSERT_EQ("void", type);
}


TEST_F(ReplyCallerTest, timeOut) {
    EXPECT_CALL(*intCallback, callbackFct(
                    Property(&RequestStatus::getCode, RequestStatusCode::ERROR_TIME_OUT_WAITING_FOR_RESPONSE), _));
    intFixture.timeOut();
}

TEST_F(ReplyCallerTest, timeOutForVoid) {
    EXPECT_CALL(*voidCallback, callbackFct(
                    Property(&RequestStatus::getCode, RequestStatusCode::ERROR_TIME_OUT_WAITING_FOR_RESPONSE)));
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

