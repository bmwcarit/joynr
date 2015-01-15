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
#include "joynr/JoynrMessage.h"
#include "cluster-controller/http-communication-manager/HttpSender.h"
#include "joynr/Util.h"
#include "cluster-controller/httpnetworking/HttpNetworking.h"
#include "joynr/DelayedScheduler.h"
#include "joynr/JsonSerializer.h"
#include "joynr/DispatcherUtils.h"
#include "cluster-controller/httpnetworking/HttpResult.h"
#include "cluster-controller/http-communication-manager/ChannelUrlSelector.h"
#include "joynr/MessagingSettings.h"
#include "joynr/RequestStatus.h"

#include <QThread>
#include <algorithm>

namespace joynr
{

using namespace joynr_logging;

const qint64& HttpSender::MIN_ATTEMPT_TTL()
{
    static qint64 value = 2 * 1000;
    return value;
}

const qint64& HttpSender::FRACTION_OF_MESSAGE_TTL_USED_PER_CONNECTION_TRIAL()
{
    static qint64 value = 3;
    return value;
}

Logger* HttpSender::logger = Logging::getInstance()->getLogger("MSG", "HttpSender");

HttpSender::HttpSender(const BounceProxyUrl& bounceProxyUrl,
                       qint64 maxAttemptTtl_ms,
                       int messageSendRetryInterval)
        : bounceProxyUrl(bounceProxyUrl),
          channelUrlCache(new ChannelUrlSelector(this->bounceProxyUrl,
                                                 ChannelUrlSelector::TIME_FOR_ONE_RECOUPERATION(),
                                                 ChannelUrlSelector::PUNISHMENT_FACTOR())),
          maxAttemptTtl_ms(maxAttemptTtl_ms),
          messageSendRetryInterval(messageSendRetryInterval),
          threadPool(),
          delayedScheduler(NULL),
          channelUrlContactorThreadPool(),
          channelUrlContactorDelayedScheduler(NULL)
{
    threadPool.setMaxThreadCount(6);
    delayedScheduler = new ThreadPoolDelayedScheduler(threadPool,
                                                      QString("MessageSender-DelayedScheduler"),
                                                      0); // The default is to not delay messages

    // Create a different scheduler for the ChannelURL directory. Ideally, this should
    // not delay messages by default. However, a race condition exists that causes intermittent
    // errors in the system integration tests when the default delay is 0.
    channelUrlContactorThreadPool.setMaxThreadCount(3);
    channelUrlContactorDelayedScheduler =
            new ThreadPoolDelayedScheduler(channelUrlContactorThreadPool,
                                           QString("MessageSender-ChannelUrlContator"),
                                           messageSendRetryInterval);
}

void HttpSender::init(QSharedPointer<ILocalChannelUrlDirectory> channelUrlDirectory,
                      const MessagingSettings& settings)
{
    channelUrlCache->init(channelUrlDirectory, settings);
}

HttpSender::~HttpSender()
{
    channelUrlContactorThreadPool.waitForDone();
    threadPool.waitForDone();
    delete channelUrlContactorDelayedScheduler;
    delete delayedScheduler;
    delete channelUrlCache;
}

void HttpSender::sendMessage(const QString& channelId, const JoynrMessage& message)
{

    LOG_TRACE(logger, "sendMessage: ...");
    QByteArray serializedMessage = JsonSerializer::serialize(message);
    /** Potential issue: needs second threadpool to call the ChannelUrlDir so a deadlock cannot
     * occur
      */
    DelayedScheduler* scheduler;
    if (channelId == MessagingSettings::SETTING_CHANNEL_URL_DIRECTORY_CHANNELID()) {
        scheduler = channelUrlContactorDelayedScheduler;
    } else {
        scheduler = delayedScheduler;
    }

    /**
     * NOTE: passing QString by value into the runnable and thereby relying on the fact that the
     * QString internally
     * uses QSharedDataPointer to manage the string's data, which when copied by value only copies
     * the pointer (but safely)
     */
    scheduler->schedule(new SendMessageRunnable(this,
                                                channelId,
                                                message.getHeaderExpiryDate(),
                                                serializedMessage,
                                                *scheduler,
                                                maxAttemptTtl_ms));
}

/**
  * Implementation of SendMessageRunnable
  *
  */
Logger* HttpSender::SendMessageRunnable::logger =
        Logging::getInstance()->getLogger("MSG", "MessageSender::SendMessageRunnable");
int HttpSender::SendMessageRunnable::messageRunnableCounter = 0;

HttpSender::SendMessageRunnable::SendMessageRunnable(HttpSender* messageSender,
                                                     const QString& channelId,
                                                     const QDateTime& decayTime,
                                                     const QByteArray& data,
                                                     DelayedScheduler& delayedScheduler,
                                                     qint64 maxAttemptTtl_ms)
        : ObjectWithDecayTime(decayTime),
          channelId(channelId),
          data(data),
          delayedScheduler(delayedScheduler),
          messageSender(messageSender),
          maxAttemptTtl_ms(maxAttemptTtl_ms)
{
    messageRunnableCounter++;
}

HttpSender::SendMessageRunnable::~SendMessageRunnable()
{
    messageRunnableCounter--;
}

void HttpSender::SendMessageRunnable::run()
{
    qint64 startTime = QDateTime::currentMSecsSinceEpoch();
    if (isExpired()) {
        LOG_DEBUG(logger,
                  "Message expired, expiration time: " +
                          DispatcherUtils::convertAbsoluteTimeToTtlString(decayTime));
        return;
    }

    LOG_TRACE(logger,
              "messageRunnableCounter: + " +
                      QString::number(SendMessageRunnable::messageRunnableCounter) +
                      " SMR existing. ");

    assert(messageSender->channelUrlCache != NULL);
    // A channelId can have several Url's. Hence, we cannot use up all the time we have for testing
    // just one (in case it is not available). So we use just a fraction, yet at least MIN... and
    // at most MAX... seconds.
    qint64 curlTimeout = std::max(
            getRemainingTtl_ms() / HttpSender::FRACTION_OF_MESSAGE_TTL_USED_PER_CONNECTION_TRIAL(),
            HttpSender::MIN_ATTEMPT_TTL());
    QString url = resolveUrlForChannelId(curlTimeout);
    LOG_TRACE(logger, "going to buildRequest");
    HttpResult sendMessageResult = buildRequestAndSend(url, curlTimeout);

    // Delay the next request if an error occurs
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 delay = messageSender->messageSendRetryInterval - (currentTime - startTime);
    if (delay < 0)
        delay = 10;

    if (sendMessageResult.getStatusCode() != 201) {
        messageSender->channelUrlCache->feedback(false, channelId, url);
        delayedScheduler.schedule(new SendMessageRunnable(messageSender,
                                                          channelId,
                                                          decayTime,
                                                          data,
                                                          delayedScheduler,
                                                          maxAttemptTtl_ms),
                                  delay);
        QString body("NULL");
        if (!sendMessageResult.getBody().isNull()) {
            body = QString(sendMessageResult.getBody().data());
        }
        LOG_ERROR(logger,
                  QString("sending message - fail; error message %1; contents %2; scheduling for "
                          "retry...")
                          .arg(sendMessageResult.getErrorMessage())
                          .arg(body));
    } else {
        LOG_DEBUG(logger,
                  "sending message - success; url: " + url + " status code: " +
                          QString::number(sendMessageResult.getStatusCode()) + " at " +
                          QString::number(QDateTime::currentMSecsSinceEpoch()));
    }
}
QString HttpSender::SendMessageRunnable::resolveUrlForChannelId(qint64 curlTimeout)
{
    LOG_TRACE(logger, "obtaining Url with a curlTimeout of : " + QString::number(curlTimeout));
    RequestStatus status;
    // we also use the curl timeout here, to prevent long blocking during shutdown.
    QString url = messageSender->channelUrlCache->obtainUrl(channelId, status, curlTimeout);
    if (!status.successful()) {
        LOG_ERROR(logger,
                  "Issue while trying to obtained URl from the ChannelUrlDirectory: " +
                          status.toString());
    }
    if (url.isNull() || url.isEmpty()) {
        LOG_DEBUG(logger,
                  "Url for channelId could not be obtained from the ChannelUrlDirectory"
                  "... EXITING ...");
        assert(false); // OR: url = messageSender->bounceProxyUrl.getSendUrl(channelId).toString();
    }

    LOG_TRACE(logger,
              "Sending message; url: " + url + ", time  left: " +
                      DispatcherUtils::convertAbsoluteTimeToTtlString(decayTime));
    return url;
}

HttpResult HttpSender::SendMessageRunnable::buildRequestAndSend(const QString& url,
                                                                qint64 curlTimeout)
{
    QSharedPointer<IHttpPostBuilder> sendMessageRequestBuilder(
            HttpNetworking::getInstance()->createHttpPostBuilder(url));

    QSharedPointer<HttpRequest> sendMessageRequest(
            sendMessageRequestBuilder->withContentType("application/json")
                    ->withTimeout_ms(std::min(maxAttemptTtl_ms, curlTimeout))
                    ->postContent(data)
                    ->build());
    LOG_TRACE(logger, "builtRequest");

    Util::logSerializedMessage(logger, QString("Sending Message: "), data);

    return sendMessageRequest->execute();
}

} // namespace joynr
