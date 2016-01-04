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
#ifndef LONGPOLLINGMESSAGERECEIVER_H_
#define LONGPOLLINGMESSAGERECEIVER_H_
#include "joynr/PrivateCopyAssign.h"

#include "joynr/ContentWithDecayTime.h"
#include "joynr/BounceProxyUrl.h"
#include "joynr/joynrlogging.h"
#include "joynr/Directory.h"
#include "joynr/Thread.h"

#include <mutex>
#include <condition_variable>

#include "joynr/Semaphore.h"
#include <memory>
#include <string>
#include <chrono>

#include <QByteArray>

namespace joynr
{

class ILocalChannelUrlDirectory;
class MessageRouter;

/**
 * Structure used for configuring the long poll message receiver
 */
struct LongPollingMessageReceiverSettings
{
    std::chrono::milliseconds bounceProxyTimeout;
    std::chrono::milliseconds longPollTimeout;
    std::chrono::milliseconds longPollRetryInterval;
    std::chrono::milliseconds createChannelRetryInterval;
};

/**
 * Class that makes long polling requests to the bounce proxy
 */
class LongPollingMessageReceiver : public Thread
{
public:
    LongPollingMessageReceiver(const BounceProxyUrl& bounceProxyUrl,
                               const std::string& channelId,
                               const std::string& receiverId,
                               const LongPollingMessageReceiverSettings& settings,
                               Semaphore* channelCreatedSemaphore,
                               std::shared_ptr<ILocalChannelUrlDirectory> channelUrlDirectory,
                               std::shared_ptr<MessageRouter> messageRouter);
    void stop() override;
    void run() override;
    void interrupt();
    bool isInterrupted();

    void processReceivedInput(const QByteArray& receivedInput);
    void processReceivedJsonObjects(const std::string& jsonObject);

private:
    void checkServerTime();
    DISALLOW_COPY_AND_ASSIGN(LongPollingMessageReceiver);
    const BounceProxyUrl bounceProxyUrl;
    const std::string channelId;
    const std::string receiverId;
    const LongPollingMessageReceiverSettings settings;

    bool interrupted;
    std::mutex interruptedMutex;
    std::condition_variable interruptedWait;

    std::shared_ptr<ILocalChannelUrlDirectory> channelUrlDirectory;

    static joynr_logging::Logger* logger;
    Semaphore* channelCreatedSemaphore;
    std::shared_ptr<MessageRouter> messageRouter;
};

} // namespace joynr
#endif // LONGPOLLINGMESSAGERECEIVER_H_
