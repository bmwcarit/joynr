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
#include <memory>

#include "tests/utils/Gmock.h"
#include "tests/utils/Gtest.h"

#include "joynr/ReplyCaller.h"

#include "tests/JoynrTest.h"
#include "tests/mock/MockCallback.h"

using ::testing::_;
using ::testing::ByRef;
using ::testing::Eq;
using ::testing::Property;
using namespace ::testing;

MATCHER(timeoutException, "")
{
    return (std::dynamic_pointer_cast<joynr::exceptions::JoynrTimeOutException>(arg) != nullptr) &&
           arg->getMessage() == "timeout waiting for the response";
}

using namespace joynr;

/*
 * This tests the ReplyCaller class, for all template specifications i.e. void and not void.
 */

class ReplyCallerTest : public ::testing::Test
{
public:
    ReplyCallerTest()
            : intCallback(new MockCallbackWithJoynrException<int>()),
              intFixture(std::bind(&MockCallbackWithJoynrException<int>::onSuccess,
                                   intCallback,
                                   std::placeholders::_1),
                         std::bind(&MockCallbackWithJoynrException<int>::onError,
                                   intCallback,
                                   std::placeholders::_1)),
              voidCallback(new MockCallbackWithJoynrException<void>()),
              voidFixture(std::bind(&MockCallbackWithJoynrException<void>::onSuccess, voidCallback),
                          std::bind(&MockCallbackWithJoynrException<void>::onError,
                                    voidCallback,
                                    std::placeholders::_1))
    {
    }

    std::shared_ptr<MockCallbackWithJoynrException<int>> intCallback;
    ReplyCaller<int> intFixture;
    std::shared_ptr<MockCallbackWithJoynrException<void>> voidCallback;
    ReplyCaller<void> voidFixture;
};

typedef ReplyCallerTest ReplyCallerDeathTest;

TEST_F(ReplyCallerTest, timeOut)
{
    EXPECT_CALL(*intCallback, onSuccess(_)).Times(0);
    EXPECT_CALL(*intCallback, onError(timeoutException())).Times(1);
    intFixture.timeOut();
}

TEST_F(ReplyCallerTest, timeOutForVoid)
{
    EXPECT_CALL(*voidCallback, onSuccess()).Times(0);
    EXPECT_CALL(*voidCallback, onError(timeoutException())).Times(1);
    voidFixture.timeOut();
}

TEST_F(ReplyCallerTest, errorReceived)
{
    std::string errorMsg = "errorMsgFromProvider";
    EXPECT_CALL(*intCallback,
                onError(Pointee(joynrException(
                        joynr::exceptions::ProviderRuntimeException::TYPE_NAME(), errorMsg))))
            .Times(1);
    EXPECT_CALL(*intCallback, onSuccess(_)).Times(0);
    intFixture.returnError(std::make_shared<exceptions::ProviderRuntimeException>(errorMsg));
}

TEST_F(ReplyCallerTest, errorReceivedForVoid)
{
    std::string errorMsg = "errorMsgFromProvider";
    EXPECT_CALL(*voidCallback,
                onError(Pointee(joynrException(
                        joynr::exceptions::ProviderRuntimeException::TYPE_NAME(), errorMsg))))
            .Times(1);
    EXPECT_CALL(*voidCallback, onSuccess()).Times(0);
    voidFixture.returnError(std::make_shared<exceptions::ProviderRuntimeException>(errorMsg));
}

TEST_F(ReplyCallerTest, resultReceived)
{
    EXPECT_CALL(*intCallback, onSuccess(7));
    EXPECT_CALL(*intCallback, onError(_)).Times(0);
    intFixture.returnValue(7);
}

TEST_F(ReplyCallerTest, resultReceivedForVoid)
{
    EXPECT_CALL(*voidCallback, onSuccess());
    EXPECT_CALL(*voidCallback, onError(_)).Times(0);
    voidFixture.returnValue();
}
