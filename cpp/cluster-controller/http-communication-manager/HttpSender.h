/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
#ifndef HTTPSENDER_H
#define HTTPSENDER_H
#include "joynr/PrivateCopyAssign.h"

#include "joynr/IMessageSender.h"

#include "joynr/ContentWithDecayTime.h"
#include "joynr/BrokerUrl.h"
#include "joynr/Logger.h"
#include "joynr/DispatcherUtils.h"
#include "joynr/ThreadPoolDelayedScheduler.h"
#include "joynr/Runnable.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"

#include <string>
#include <memory>
#include <chrono>

namespace joynr
{

class JoynrMessage;
class MessagingSettings;
class HttpResult;

class HttpSender : public IMessageSender
{
public:
    static std::chrono::milliseconds MIN_ATTEMPT_TTL();
    static const std::int64_t& FRACTION_OF_MESSAGE_TTL_USED_PER_CONNECTION_TRIAL();

    HttpSender(const BrokerUrl& brokerUrl,
               std::chrono::milliseconds maxAttemptTtl,
               std::chrono::milliseconds messageSendRetryInterval);
    ~HttpSender() override;
    /**
    * @brief Sends the message to the given channel.
    */
    void sendMessage(const joynr::system::RoutingTypes::Address& destinationAddress,
                     const JoynrMessage& message,
                     const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure)
            override;

    void init(const MessagingSettings& settings) override;

    void registerReceiveQueueStartedCallback(
            std::function<void(void)> waitForReceiveQueueStarted) override;

private:
    DISALLOW_COPY_AND_ASSIGN(HttpSender);
    const BrokerUrl brokerUrl;
    const std::chrono::milliseconds maxAttemptTtl;
    const std::chrono::milliseconds messageSendRetryInterval;
    ADD_LOGGER(HttpSender);

    /**
     * @brief @ref ThreadPool used to send messages once an URL is known
     * @todo Replace by @ref ThreadPool instead of @ref ThreadPoolDelayedScheduler
     */
    ThreadPoolDelayedScheduler delayedScheduler;

    class SendMessageRunnable : public Runnable, public ObjectWithDecayTime
    {
    public:
        SendMessageRunnable(HttpSender* messageSender,
                            const system::RoutingTypes::ChannelAddress& channelAddress,
                            const JoynrTimePoint& decayTime,
                            std::string&& data,
                            DelayedScheduler& delayedScheduler,
                            std::chrono::milliseconds maxAttemptTtl);
        ~SendMessageRunnable() override;

        void shutdown() override;

        /**
         * @brief run
         * Tries a HTTP request on the URL which is stored in channelAddress.
         */
        void run() override;

    private:
        DISALLOW_COPY_AND_ASSIGN(SendMessageRunnable);
        HttpResult buildRequestAndSend(const std::string& url,
                                       std::chrono::milliseconds curlTimeout);
        std::string toUrl(const system::RoutingTypes::ChannelAddress& channelAddress) const;
        system::RoutingTypes::ChannelAddress channelAddress;
        std::string data;
        DelayedScheduler& delayedScheduler;
        HttpSender* messageSender;
        std::chrono::milliseconds maxAttemptTtl;

        ADD_LOGGER(SendMessageRunnable);
        static int messageRunnableCounter;
    };
};

} // namespace joynr
#endif // HTTPSENDER_H
