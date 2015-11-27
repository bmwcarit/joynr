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
#include "joynr/BounceProxyUrl.h"
#include "joynr/joynrlogging.h"
#include "cluster-controller/http-communication-manager/IChannelUrlSelector.h"
#include "joynr/ILocalChannelUrlDirectory.h"

#include <QString>
#include <QByteArray>
#include <QThreadPool>
#include "joynr/DispatcherUtils.h"
#include <memory>

namespace joynr
{

class JoynrMessage;
class DelayedScheduler;
class MessagingSettings;
class HttpResult;

class HttpSender : public IMessageSender
{
public:
    static const qint64& MIN_ATTEMPT_TTL();
    static const qint64& MAX_ATTEMPT_TTL();
    static const qint64& FRACTION_OF_MESSAGE_TTL_USED_PER_CONNECTION_TRIAL();

    HttpSender(const BounceProxyUrl& bounceProxyUrl,
               qint64 maxAttemptTtl_ms,
               int messageSendRetryInterval); // int messageSendRetryInterval
    virtual ~HttpSender();
    /**
    * @brief Sends the message to the given channel.
    */
    void sendMessage(const QString& channelId, const JoynrMessage& message);
    /**
    * @brief The MessageSender needs the localChannelUrlDirectory to obtain Url's for
    * the channelIds.
    */
    void init(std::shared_ptr<ILocalChannelUrlDirectory> channelUrlDirectory,
              const MessagingSettings& settings);

private:
    DISALLOW_COPY_AND_ASSIGN(HttpSender);
    const BounceProxyUrl bounceProxyUrl;
    IChannelUrlSelector* channelUrlCache;
    const qint64 maxAttemptTtl_ms;
    const int messageSendRetryInterval;
    static joynr_logging::Logger* logger;

    QThreadPool threadPool; // used to send messages once an Url is known
    DelayedScheduler* delayedScheduler;
    QThreadPool channelUrlContactorThreadPool; // obtaining an url must be in a different threadpool
    DelayedScheduler* channelUrlContactorDelayedScheduler;

    class SendMessageRunnable : public QRunnable, public ObjectWithDecayTime
    {
    public:
        SendMessageRunnable(HttpSender* messageSender,
                            const QString& channelId,
                            const JoynrTimePoint& decayTime,
                            std::string&& data,
                            DelayedScheduler& delayedScheduler,
                            qint64 maxAttemptTtl_ms);
        ~SendMessageRunnable();
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
        void run();

    private:
        DISALLOW_COPY_AND_ASSIGN(SendMessageRunnable);
        HttpResult buildRequestAndSend(const QString& url, qint64 curlTimeout);
        QString resolveUrlForChannelId(qint64 curlTimeout);
        QString channelId;
        std::string data;
        DelayedScheduler& delayedScheduler;
        HttpSender* messageSender;
        qint64 maxAttemptTtl_ms;

        static joynr_logging::Logger* logger;
        static int messageRunnableCounter;
    };
};

} // namespace joynr
#endif // HTTPSENDER_H_
