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
#ifndef TESTS_MOCK_MOCKUDSCLIENTCALLBACKS_H
#define TESTS_MOCK_MOCKUDSCLIENTCALLBACKS_H

#include <utility>

#include <gmock/gmock.h>

#include "smrf/ByteVector.h"

#include "joynr/exceptions/JoynrException.h"

class UdsClientCallbackInterface
{
public:
    virtual void connected() = 0;
    virtual void disconnected() = 0;
    virtual void receivedMock(smrf::ByteVector) = 0;
    void received(smrf::ByteVector&& msg)
    {
        receivedMock(std::move(msg));
    }
    virtual void sendFailed(const joynr::exceptions::JoynrRuntimeException&) = 0;
};

class MockUdsClientCallbacks : public UdsClientCallbackInterface
{
public:
    MOCK_METHOD0(connected, void());
    MOCK_METHOD0(disconnected, void());
    MOCK_METHOD1(receivedMock, void(smrf::ByteVector));
    MOCK_METHOD1(sendFailed, void(const joynr::exceptions::JoynrRuntimeException&));
};
#endif // TESTS_MOCK_MOCKUDSCLIENTCALLBACKS_H
