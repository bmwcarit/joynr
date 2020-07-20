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
#ifndef TESTS_MOCK_MOCKIMESSAGINGSKELETON_H
#define TESTS_MOCK_MOCKIMESSAGINGSKELETON_H

#include <functional>
#include <memory>

#include <gmock/gmock.h>
#include <smrf/ByteVector.h>

#include "joynr/exceptions/JoynrException.h"
#include "joynr/IMessagingSkeleton.h"

class MockIMessagingSkeleton : public joynr::IMessagingSkeleton
{
public:
    MOCK_METHOD0(dtorCalled, void());
    ~MockIMessagingSkeleton() override
    {
        dtorCalled();
    }

    MOCK_METHOD2(transmit, void(std::shared_ptr<joynr::ImmutableMessage>, const std::function<void(const joynr::exceptions::JoynrRuntimeException&)>&));

    // GoogleMock does not support mocking functions with r-value references as parameters
    MOCK_METHOD2(onMessageReceivedMock,void(smrf::ByteVector& rawMessage, const std::string& creator));
    void onMessageReceived(smrf::ByteVector&& rawMessage, const std::string& creator) override
    {
        onMessageReceivedMock(rawMessage, creator);
    }
};

#endif // TESTS_MOCK_MOCKIMESSAGINGSKELETON_H
