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
#include "joynr/JsonSerializer.h"
#include "cluster-controller/httpnetworking/HttpResult.h"
#include "cluster-controller/http-communication-manager/IChannelUrlSelector.h"
#include "cluster-controller/http-communication-manager/ChannelUrlSelector.h"
#include "joynr/MessagingSettings.h"
#include "joynr/RequestStatus.h"
#include "joynr/TypeUtil.h"

#include <algorithm>
#include <chrono>

using namespace std::chrono;

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
          delayedScheduler(6, "MessageSender"),
          channelUrlContactorDelayedScheduler(3,
                                              "ChannelUrlContator",
                                              std::chrono::milliseconds(messageSendRetryInterval))
{
}

void HttpSender::init(std::shared_ptr<ILocalChannelUrlDirectory> channelUrlDirectory,
                      const MessagingSettings& settings)
{
    channelUrlCache->init(channelUrlDirectory, settings);
}

HttpSender::~HttpSender()
{
    delayedScheduler.shutdown();
    channelUrlContactorDelayedScheduler.shutdown();

    delete channelUrlCache;
}

void HttpSender::sendMessage(const std::string& channelId, const JoynrMessage& message)
{

    LOG_TRACE(logger, "sendMessage: ...");
    std::string&& serializedMessage = JsonSerializer::serialize(message);
    /** Potential issue: needs second threadpool to call the ChannelUrlDir so a deadlock cannot
     * occur
      */
    DelayedScheduler* scheduler;
    std::string channelUrlDirChannelId =
            MessagingSettings::SETTING_CHANNEL_URL_DIRECTORY_CHANNELID();
    if (channelId == channelUrlDirChannelId) {
        scheduler = &channelUrlContactorDelayedScheduler;
    } else {
        scheduler = &delayedScheduler;
    }

    /**
     * NOTE: passing std::string by value into the runnable and thereby relying on the fact that the
     * std::string internally
     * uses QSharedDataPointer to manage the string's data, which when copied by value only copies
     * the pointer (but safely)
     */
    scheduler->schedule(new SendMessageRunnable(this,
                                                channelId,
                                                message.getHeaderExpiryDate(),
                                                std::move(serializedMessage),
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
                                                     const std::string& channelId,
                                                     const JoynrTimePoint& decayTime,
                                                     std::string&& data,
                                                     DelayedScheduler& delayedScheduler,
                                                     qint64 maxAttemptTtl_ms)
        : joynr::Runnable(true),
          ObjectWithDecayTime(decayTime),
          channelId(channelId),
          data(std::move(data)),
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

void HttpSender::SendMessageRunnable::shutdown()
{
}

void HttpSender::SendMessageRunnable::run()
{
    qint64 startTime = TypeUtil::toQt(DispatcherUtils::nowInMilliseconds());
    if (isExpired()) {
        LOG_DEBUG(logger,
                  FormatString("Message expired, expiration time: %1")
                          .arg(DispatcherUtils::convertAbsoluteTimeToTtlString(decayTime))
                          .str());
        return;
    }

    LOG_TRACE(logger,
              FormatString("messageRunnableCounter: + %1 SMR existing. ")
                      .arg(SendMessageRunnable::messageRunnableCounter)
                      .str());

    assert(messageSender->channelUrlCache != NULL);
    // A channelId can have several Url's. Hence, we cannot use up all the time we have for testing
    // just one (in case it is not available). So we use just a fraction, yet at least MIN... and
    // at most MAX... seconds.
    qint64 curlTimeout = std::max(
            getRemainingTtl_ms() / HttpSender::FRACTION_OF_MESSAGE_TTL_USED_PER_CONNECTION_TRIAL(),
            HttpSender::MIN_ATTEMPT_TTL());
    std::string url = resolveUrlForChannelId(curlTimeout);
    LOG_TRACE(logger, "going to buildRequest");
    HttpResult sendMessageResult = buildRequestAndSend(url, curlTimeout);

    // Delay the next request if an error occurs
    qint64 currentTime = TypeUtil::toQt(DispatcherUtils::nowInMilliseconds());
    qint64 delay = messageSender->messageSendRetryInterval - (currentTime - startTime);
    if (delay < 0)
        delay = 10;

    if (sendMessageResult.getStatusCode() != 201) {
        messageSender->channelUrlCache->feedback(false, channelId, url);
        delayedScheduler.schedule(new SendMessageRunnable(messageSender,
                                                          channelId,
                                                          decayTime,
                                                          std::move(data),
                                                          delayedScheduler,
                                                          maxAttemptTtl_ms),
                                  OptionalDelay(std::chrono::milliseconds(delay)));
        std::string body("NULL");
        if (!sendMessageResult.getBody().isNull()) {
            body = std::string(sendMessageResult.getBody().data());
        }
        LOG_ERROR(logger,
                  FormatString(
                          "sending message - fail; error message %1; contents %2; scheduling for "
                          "retry...")
                          .arg(sendMessageResult.getErrorMessage())
                          .arg(body)
                          .str());
    } else {
        LOG_DEBUG(logger,
                  FormatString("sending message - success; url: %1 status code: %2 at %3")
                          .arg(url)
                          .arg(sendMessageResult.getStatusCode())
                          .arg(DispatcherUtils::nowInMilliseconds())
                          .str());
    }
}
std::string HttpSender::SendMessageRunnable::resolveUrlForChannelId(qint64 curlTimeout)
{
    LOG_TRACE(logger,
              FormatString("obtaining Url with a curlTimeout of : %1").arg(curlTimeout).str());
    RequestStatus status;
    // we also use the curl timeout here, to prevent long blocking during shutdown.
    std::string url = messageSender->channelUrlCache->obtainUrl(channelId, status, curlTimeout);
    if (!status.successful()) {
        LOG_ERROR(
                logger,
                FormatString("Issue while trying to obtained URl from the ChannelUrlDirectory: %1")
                        .arg(status.toString())
                        .str());
    }
    if (url.empty()) {
        LOG_DEBUG(logger,
                  "Url for channelId could not be obtained from the ChannelUrlDirectory"
                  "... EXITING ...");
        assert(false); // OR: url = messageSender->bounceProxyUrl.getSendUrl(channelId).toString();
    }

    LOG_TRACE(logger,
              FormatString("Sending message; url: %1, time  left: %2")
                      .arg(url)
                      .arg(DispatcherUtils::convertAbsoluteTimeToTtlString(decayTime).c_str())
                      .str());
    return url;
}

HttpResult HttpSender::SendMessageRunnable::buildRequestAndSend(const std::string& url,
                                                                qint64 curlTimeout)
{
    std::shared_ptr<IHttpPostBuilder> sendMessageRequestBuilder(
            HttpNetworking::getInstance()->createHttpPostBuilder(url));

    std::shared_ptr<HttpRequest> sendMessageRequest(
            sendMessageRequestBuilder->withContentType("application/json")
                    ->withTimeout_ms(std::min(maxAttemptTtl_ms, curlTimeout))
                    ->postContent(QString::fromStdString(data).toUtf8())
                    ->build());
    LOG_TRACE(logger, "builtRequest");

    Util::logSerializedMessage(logger, "Sending Message: ", data);

    return sendMessageRequest->execute();
}

} // namespace joynr
