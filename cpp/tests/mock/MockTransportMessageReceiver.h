/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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
#ifndef TESTS_MOCK_MOCKTRANSPORTMESSAGERECEIVER_H
#define TESTS_MOCK_MOCKTRANSPORTMESSAGERECEIVER_H

#include "tests/utils/Gmock.h"

#include "joynr/ITransportMessageReceiver.h"

class MockTransportMessageReceiver : public joynr::ITransportMessageReceiver
{
public:
    MockTransportMessageReceiver() = default;
    MOCK_METHOD0(init, void());
    MOCK_CONST_METHOD0(getSerializedGlobalClusterControllerAddress, const std::string());
    MOCK_CONST_METHOD0(getGlobalClusterControllerAddress, const joynr::system::RoutingTypes::Address&());
    MOCK_METHOD0(startReceiveQueue, void());
    MOCK_METHOD0(stopReceiveQueue, void());
    MOCK_METHOD0(updateSettings, void());
    MOCK_METHOD1(registerReceiveCallback, void(std::function<void(smrf::ByteVector&&)> onMessageReceived));
    MOCK_METHOD0(isConnected, bool());
};

#endif // TESTS_MOCK_MOCKTRANSPORTMESSAGERECEIVER_H
