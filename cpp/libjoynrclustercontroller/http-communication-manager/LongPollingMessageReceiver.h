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
#ifndef LONGPOLLINGMESSAGERECEIVER_H_
#define LONGPOLLINGMESSAGERECEIVER_H_
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>

#include <smrf/ByteVector.h>

#include "joynr/BrokerUrl.h"
#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/Semaphore.h"

namespace joynr
{

class HttpRequest;
/**
 * Structure used for configuring the long poll message receiver
 */
struct LongPollingMessageReceiverSettings
{
    std::chrono::milliseconds brokerTimeout;
    std::chrono::milliseconds longPollTimeout;
    std::chrono::milliseconds longPollRetryInterval;
    std::chrono::milliseconds createChannelRetryInterval;
};

/**
 * Class that makes long polling requests to the bounce proxy
 */
class LongPollingMessageReceiver
{
public:
    LongPollingMessageReceiver(const BrokerUrl& brokerUrl,
                               const std::string& channelId,
                               const std::string& receiverId,
                               const LongPollingMessageReceiverSettings& settings,
                               std::shared_ptr<Semaphore> channelCreatedSemaphore,
                               std::function<void(smrf::ByteVector&&)> onMessageReceived);

    ~LongPollingMessageReceiver();

    void start();
    void stop();
    void run();
    void interrupt();
    bool isInterrupted();

    void processReceivedInput(const std::string& receivedInput);

private:
    void checkServerTime();
    DISALLOW_COPY_AND_ASSIGN(LongPollingMessageReceiver);
    const BrokerUrl _brokerUrl;
    const std::string _channelId;
    const std::string _receiverId;
    const LongPollingMessageReceiverSettings _settings;

    bool _interrupted;
    std::mutex _interruptedMutex;
    std::condition_variable _interruptedWait;

    ADD_LOGGER(LongPollingMessageReceiver)

    // Ownership shared between this and HttpReceiver
    std::shared_ptr<Semaphore> _channelCreatedSemaphore;

    /*! On message received callback */
    std::function<void(smrf::ByteVector&&)> _onMessageReceived;
    std::unique_ptr<HttpRequest> _currentRequest;

    std::unique_ptr<std::thread> _thread;
};

} // namespace joynr
#endif // LONGPOLLINGMESSAGERECEIVER_H_
