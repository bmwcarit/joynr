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
#ifndef HTTPSENDER_H_
#define HTTPSENDER_H_
#include "joynr/PrivateCopyAssign.h"

#include "joynr/IMessageSender.h"

#include "joynr/ContentWithDecayTime.h"
#include "joynr/BrokerUrl.h"
#include "joynr/Logger.h"
#include "joynr/ILocalChannelUrlDirectory.h"
#include "joynr/DispatcherUtils.h"
#include "joynr/ThreadPoolDelayedScheduler.h"
#include "joynr/Runnable.h"

#include <string>
#include <memory>
#include <chrono>

namespace joynr
{

class JoynrMessage;
class MessagingSettings;
class HttpResult;
class IChannelUrlSelector;

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
    void sendMessage(const std::string& channelId, const JoynrMessage& message) override;
    /**
    * @brief The MessageSender needs the localChannelUrlDirectory to obtain Url's for
    * the channelIds.
    */
    void init(std::shared_ptr<ILocalChannelUrlDirectory> channelUrlDirectory,
              const MessagingSettings& settings) override;

private:
    DISALLOW_COPY_AND_ASSIGN(HttpSender);
    const BrokerUrl brokerUrl;
    IChannelUrlSelector* channelUrlCache;
    const std::chrono::milliseconds maxAttemptTtl;
    const std::chrono::milliseconds messageSendRetryInterval;
    ADD_LOGGER(HttpSender);

    /**
     * @brief @ref ThreadPool used to send messages once an URL is known
     * @todo Replace by @ref ThreadPool instead of @ref ThreadPoolDelayedScheduler
     */
    ThreadPoolDelayedScheduler delayedScheduler;

    /**
     * @brief ThreadPool to obtain an URL
     * @note Obtaining an URL must be in a different ThreadPool
     *
     * Create a different scheduler for the ChannelURL directory. Ideally,
     * this should not delay messages by default. However, a race condition
     * exists that causes intermittent errors in the system integration tests
     * when the default delay is 0.
     */
    ThreadPoolDelayedScheduler channelUrlContactorDelayedScheduler;

    class SendMessageRunnable : public Runnable, public ObjectWithDecayTime
    {
    public:
        SendMessageRunnable(HttpSender* messageSender,
                            const std::string& channelId,
                            const JoynrTimePoint& decayTime,
                            std::string&& data,
                            DelayedScheduler& delayedScheduler,
                            std::chrono::milliseconds maxAttemptTtl);
        ~SendMessageRunnable() override;

        void shutdown() override;

        /**
         * @brief run
         * 1) Obtains the 'best' Url for the channelId from the ChannelUrlSelector.
         * 2)  Sets the curl timeout to a fraction of the messaging TTL
         *     (yet always between MIN and MAX_CONNECTION_TTL()).
         * 3) Tries a HTTP request on this URL. If the result is negative,
         * feedback is provided to the ChannelUrlSelector (and back to 1). Otherwise: Done.
         * During this procedure, the ChannelUrlSelector decides if it is appropriate
         * to try an alternative Url (depending on the history of feedback).
         */
        void run() override;

    private:
        DISALLOW_COPY_AND_ASSIGN(SendMessageRunnable);
        HttpResult buildRequestAndSend(const std::string& url,
                                       std::chrono::milliseconds curlTimeout);
        std::string resolveUrlForChannelId(std::chrono::milliseconds curlTimeout);
        std::string channelId;
        std::string data;
        DelayedScheduler& delayedScheduler;
        HttpSender* messageSender;
        std::chrono::milliseconds maxAttemptTtl;

        ADD_LOGGER(SendMessageRunnable);
        static int messageRunnableCounter;
    };
};

} // namespace joynr
#endif // HTTPSENDER_H_
