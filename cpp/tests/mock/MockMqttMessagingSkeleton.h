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
#ifndef TESTS_MOCK_MOCKMQTTMESSAGINGSKELETON_H
#define TESTS_MOCK_MOCKMQTTMESSAGINGSKELETON_H

#include "tests/utils/Gmock.h"

#include "joynr/AbstractGlobalMessagingSkeleton.h"

class MockMqttMessagingSkeleton : public joynr::AbstractGlobalMessagingSkeleton
{
public:
    MOCK_METHOD2(transmit,
                 void(std::shared_ptr<joynr::ImmutableMessage> message,
                      const std::function<void(const joynr::exceptions::JoynrRuntimeException&)>&
                              onFailure));

    // GoogleMock does not support mocking functions with r-value references as parameters
    MOCK_METHOD1(onMessageReceivedMock, void(smrf::ByteVector& rawMessage));
    void onMessageReceived(smrf::ByteVector&& rawMessage) override
    {
        onMessageReceivedMock(rawMessage);
    }

    MOCK_METHOD1(registerMulticastSubscription, void(const std::string& multicastId));
    MOCK_METHOD1(unregisterMulticastSubscription, void(const std::string& multicastId));
};

#endif // TESTS_MOCK_MOCKMQTTMESSAGINGSKELETON_H
