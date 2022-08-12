/*
 * #%L
 * %%
 * Copyright (C) 2019 - 2020 BMW Car IT GmbH
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

#ifndef TEST_MOCK_TESTJOYNRCLUSTERCONTROLLERRUNTIME_H
#define TEST_MOCK_TESTJOYNRCLUSTERCONTROLLERRUNTIME_H

#include <string>

#include "tests/utils/Gmock.h"

#include "joynr/ITransportMessageReceiver.h"
#include "joynr/ITransportMessageSender.h"
#include "joynr/JoynrClusterControllerMqttConnectionData.h"
#include "joynr/JoynrClusterControllerRuntime.h"
#include "libjoynrclustercontroller/mqtt/MosquittoConnection.h"

using namespace joynr;

class MockJoynrClusterControllerMqttConnectionData
        : public joynr::JoynrClusterControllerMqttConnectionData
{
public:
    MockJoynrClusterControllerMqttConnectionData() = default;
    virtual ~MockJoynrClusterControllerMqttConnectionData() = default;

    MOCK_CONST_METHOD0(getMosquittoConnection, std::shared_ptr<MosquittoConnection>());
    MOCK_METHOD1(setMosquittoConnection, void(const std::shared_ptr<MosquittoConnection>& value));
    MOCK_CONST_METHOD0(getMqttMessageReceiver, std::shared_ptr<ITransportMessageReceiver>());
    MOCK_METHOD1(setMqttMessageReceiver,
                 void(const std::shared_ptr<ITransportMessageReceiver>& value));
    MOCK_CONST_METHOD0(getMqttMessageSender, std::shared_ptr<ITransportMessageSender>());
    MOCK_METHOD1(setMqttMessageSender, void(const std::shared_ptr<ITransportMessageSender>& value));
};

/** Helper to apply TestJoynrClusterControllerRuntimeto CC */
class TestJoynrClusterControllerRuntime : public JoynrClusterControllerRuntime
{
public:
    TestJoynrClusterControllerRuntime(
            std::unique_ptr<Settings> settings,
            std::function<void(const exceptions::JoynrRuntimeException&)>&& onFatalRuntimeError,
            std::shared_ptr<IKeychain> keyChain = nullptr,
            MqttMessagingSkeletonFactory mqttMessagingSkeletonFactory = nullptr,
            std::int64_t removeStaleDelayMs = _defaultRemoveStaleDelayMs)
            : JoynrClusterControllerRuntime(std::move(settings),
                                            std::move(onFatalRuntimeError),
                                            keyChain,
                                            mqttMessagingSkeletonFactory,
                                            removeStaleDelayMs){};

    TestJoynrClusterControllerRuntime(
            const std::shared_ptr<MosquittoConnection>& mockMosquittoConnection,
            const std::shared_ptr<ITransportMessageReceiver>& mockMqttMessageReceiver,
            const std::shared_ptr<ITransportMessageSender>& mockMqttMessageSender,
            std::unique_ptr<Settings> settings,
            std::function<void(const exceptions::JoynrRuntimeException&)>&& onFatalRuntimeError,
            std::shared_ptr<IKeychain> keyChain = nullptr,
            MqttMessagingSkeletonFactory mqttMessagingSkeletonFactory = nullptr,
            std::int64_t removeStaleDelayMs = _defaultRemoveStaleDelayMs,
            std::function<void(
                    std::vector<std::shared_ptr<MockJoynrClusterControllerMqttConnectionData>>&)>
                    testsToExecute = nullptr)
            : JoynrClusterControllerRuntime(std::move(settings),
                                            std::move(onFatalRuntimeError),
                                            keyChain,
                                            mqttMessagingSkeletonFactory,
                                            removeStaleDelayMs)
    {
        const std::size_t numberOfGBIDs = 1U + _messagingSettings.getAdditionalBackendsCount();
        _mqttConnectionDataVector.clear();

        std::vector<std::shared_ptr<MockJoynrClusterControllerMqttConnectionData>> createdMocks;
        for (std::size_t i = 0; i < numberOfGBIDs; i++) {
            auto mockMqttConnectionData =
                    std::make_shared<MockJoynrClusterControllerMqttConnectionData>();
            createdMocks.push_back(mockMqttConnectionData);
            _mqttConnectionDataVector.push_back(mockMqttConnectionData);

            ON_CALL(*mockMqttConnectionData, getMqttMessageReceiver())
                    .WillByDefault(testing::Return(mockMqttMessageReceiver));

            ON_CALL(*mockMqttConnectionData, getMqttMessageSender())
                    .WillByDefault(testing::Return(mockMqttMessageSender));

            ON_CALL(*mockMqttConnectionData, getMosquittoConnection())
                    .WillByDefault(testing::Return(mockMosquittoConnection));
        }

        if (testsToExecute != nullptr) {
            testsToExecute(createdMocks);
        }
    };
};

#endif // TEST_MOCK_TESTJOYNRCLUSTERCONTROLLERRUNTIME_H
