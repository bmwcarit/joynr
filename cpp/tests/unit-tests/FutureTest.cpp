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
#include "joynr/Future.h"
#include "tests/utils/MockObjects.h"

using ::testing::Property;
using ::testing::Eq;
using ::testing::ByRef;
using ::testing::NotNull;
using ::testing::_;
using namespace ::testing;
using namespace joynr;

/*
 * This tests the Future class.
 */


class FutureTest : public ::testing::Test {
public:

    FutureTest()
        : intFuture(),
          voidFuture() {
    }

    void SetUp() {
    }

protected:
    Future<int> intFuture;
    Future<void> voidFuture;
};

TEST_F(FutureTest, getValueAndStatusAfterResultReceived) {
    intFuture.onSuccess(RequestStatus(RequestStatusCode::OK), 10);
    int actualValue;
    intFuture.getValues(actualValue);
    ASSERT_EQ(10, actualValue);
    ASSERT_EQ(RequestStatusCode::OK, intFuture.getStatus().getCode());

    // try retrieving the values a second time
    intFuture.getValues(actualValue);
    ASSERT_EQ(10, actualValue);
    ASSERT_EQ(RequestStatusCode::OK, intFuture.getStatus().getCode());

}

TEST_F(FutureTest, isOKReturnsTrueWhenStatusIsOk) {
    ASSERT_FALSE(intFuture.isOk());
    intFuture.onSuccess(RequestStatus(RequestStatusCode::OK), 10);
    ASSERT_TRUE(intFuture.isOk());
}

TEST_F(FutureTest, getValueAndStatusAfterFailiureReceived) {
    intFuture.onFailure(RequestStatus(RequestStatusCode::ERROR_TIME_OUT_WAITING_FOR_RESPONSE));
    ASSERT_EQ(RequestStatusCode::ERROR_TIME_OUT_WAITING_FOR_RESPONSE, intFuture.getStatus().getCode());
    int actualValue;

    ASSERT_DEATH_IF_SUPPORTED(intFuture.getValues(actualValue), ".");
}

TEST_F(FutureTest, getValueAndStatusBeforeOperationFinishes) {
    ASSERT_EQ(RequestStatusCode::IN_PROGRESS, intFuture.getStatus().getCode());
    int actualValue;
    ASSERT_DEATH_IF_SUPPORTED(intFuture.getValues(actualValue), ".");
}

TEST_F(FutureTest, getStatusForVoidAfterResultReceived) {
    voidFuture.onSuccess(RequestStatusCode::OK);
    ASSERT_EQ(RequestStatusCode::OK, voidFuture.getStatus().getCode());
}

TEST_F(FutureTest, getStatusForVoidAfterFailureReceived) {
    voidFuture.onFailure(RequestStatus(RequestStatusCode::ERROR_TIME_OUT_WAITING_FOR_RESPONSE));
    ASSERT_EQ(RequestStatusCode::ERROR_TIME_OUT_WAITING_FOR_RESPONSE, voidFuture.getStatus().getCode());
}

TEST_F(FutureTest, getStatusForVoidBeforeOperationFinishes) {
    ASSERT_EQ(RequestStatusCode::IN_PROGRESS, voidFuture.getStatus().getCode());
}

TEST_F(FutureTest, waitForFinishWithTimer) {
    RequestStatus requestStatus = intFuture.waitForFinished(5);
    EXPECT_EQ(RequestStatusCode::IN_PROGRESS, requestStatus.getCode());
    int actualValue;
    ASSERT_DEATH_IF_SUPPORTED(intFuture.getValues(actualValue), ".");
}

TEST_F(FutureTest, waitForFinishWithTimerForVoid) {
    RequestStatus requestStatus = voidFuture.waitForFinished(5);
    EXPECT_EQ(RequestStatusCode::IN_PROGRESS, requestStatus.getCode());
}
