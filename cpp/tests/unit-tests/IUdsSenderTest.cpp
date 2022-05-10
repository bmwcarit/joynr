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

#include <memory>

#include "tests/utils/Gtest.h"
#include "tests/utils/Gmock.h"

#include "joynr/exceptions/JoynrException.h"
#include "joynr/MutableMessage.h"
#include "joynr/ImmutableMessage.h"
#include "tests/mock/MockIUdsSender.h"

using namespace ::testing;
using namespace joynr;

TEST(IUdsSenderTest, mockIUdsSenderAPIsCanBeCalled)
{
    MutableMessage mutableMessage;
    std::shared_ptr<ImmutableMessage> immutableMessage = mutableMessage.getImmutableMessage();
    smrf::ByteArrayView byteArrayView = immutableMessage->getUnencryptedBody();

    auto mockIUdsSender = std::make_shared<MockIUdsSender>();
    std::shared_ptr<IUdsSender> iUdsSender = mockIUdsSender;

    EXPECT_CALL(*mockIUdsSender,
            send(A<const smrf::ByteArrayView &>(),
                A<const std::function<
                void(const joynr::exceptions::JoynrRuntimeException&)>&>()));
    EXPECT_CALL(*mockIUdsSender, dtorCalled()).Times(1);
    iUdsSender->send(
            byteArrayView,
            [](const exceptions::JoynrRuntimeException& error) {
                std::ignore = error;
                FAIL() << "onError callback invoked";
            }
    );
}
