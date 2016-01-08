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
#include "joynr/QtTypeUtil.h"

#include <algorithm>
#include <chrono>

using namespace std::chrono;

namespace joynr
{

std::chrono::milliseconds HttpSender::MIN_ATTEMPT_TTL()
{
    return std::chrono::seconds(2);
}

const int64_t& HttpSender::FRACTION_OF_MESSAGE_TTL_USED_PER_CONNECTION_TRIAL()
{
    static int64_t value = 3;
    return value;
}

INIT_LOGGER(HttpSender);

HttpSender::HttpSender(const BounceProxyUrl& bounceProxyUrl,
                       std::chrono::milliseconds maxAttemptTtl,
                       std::chrono::milliseconds messageSendRetryInterval)
        : bounceProxyUrl(bounceProxyUrl),
          channelUrlCache(new ChannelUrlSelector(this->bounceProxyUrl,
                                                 ChannelUrlSelector::TIME_FOR_ONE_RECOUPERATION(),
                                                 ChannelUrlSelector::PUNISHMENT_FACTOR())),
          maxAttemptTtl(maxAttemptTtl),
          messageSendRetryInterval(messageSendRetryInterval),
          delayedScheduler(6, "MessageSender"),
          channelUrlContactorDelayedScheduler(3, "ChannelUrlContator", messageSendRetryInterval)
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

    JOYNR_LOG_TRACE(logger, "sendMessage: ...");
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
                                                maxAttemptTtl));
}

/**
  * Implementation of SendMessageRunnable
  *
  */
INIT_LOGGER(HttpSender::SendMessageRunnable);

int HttpSender::SendMessageRunnable::messageRunnableCounter = 0;

HttpSender::SendMessageRunnable::SendMessageRunnable(HttpSender* messageSender,
                                                     const std::string& channelId,
                                                     const JoynrTimePoint& decayTime,
                                                     std::string&& data,
                                                     DelayedScheduler& delayedScheduler,
                                                     std::chrono::milliseconds maxAttemptTtl)
        : joynr::Runnable(true),
          ObjectWithDecayTime(decayTime),
          channelId(channelId),
          data(std::move(data)),
          delayedScheduler(delayedScheduler),
          messageSender(messageSender),
          maxAttemptTtl(maxAttemptTtl)
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
    auto startTime = std::chrono::system_clock::now();
    if (isExpired()) {
        JOYNR_LOG_DEBUG(logger,
                        "Message expired, expiration time: {}",
                        DispatcherUtils::convertAbsoluteTimeToTtlString(decayTime));
        return;
    }

    JOYNR_LOG_TRACE(logger,
                    "messageRunnableCounter: + {}  SMR existing.",
                    SendMessageRunnable::messageRunnableCounter);

    assert(messageSender->channelUrlCache != nullptr);
    // A channelId can have several Url's. Hence, we cannot use up all the time we have for testing
    // just one (in case it is not available). So we use just a fraction, yet at least MIN... and
    // at most MAX... seconds.
    int64_t curlTimeout = std::max(
            getRemainingTtl_ms() / HttpSender::FRACTION_OF_MESSAGE_TTL_USED_PER_CONNECTION_TRIAL(),
            HttpSender::MIN_ATTEMPT_TTL().count());
    std::string url = resolveUrlForChannelId(std::chrono::milliseconds(curlTimeout));
    JOYNR_LOG_TRACE(logger, "going to buildRequest");
    HttpResult sendMessageResult = buildRequestAndSend(url, std::chrono::milliseconds(curlTimeout));

    // Delay the next request if an error occurs
    auto now = std::chrono::system_clock::now();
    auto timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);
    std::chrono::milliseconds delay;
    if (messageSender->messageSendRetryInterval < timeDiff) {
        delay = std::chrono::milliseconds(10);
    } else {
        delay = messageSender->messageSendRetryInterval - timeDiff;
    }

    if (sendMessageResult.getStatusCode() != 201) {
        messageSender->channelUrlCache->feedback(false, channelId, url);
        delayedScheduler.schedule(new SendMessageRunnable(messageSender,
                                                          channelId,
                                                          decayTime,
                                                          std::move(data),
                                                          delayedScheduler,
                                                          maxAttemptTtl),
                                  std::chrono::milliseconds(delay));
        std::string body("NULL");
        if (!sendMessageResult.getBody().isNull()) {
            body = std::string(sendMessageResult.getBody().data());
        }
        JOYNR_LOG_ERROR(
                logger,
                "sending message - fail; error message {}; contents {}; rescheduling for retry...",
                sendMessageResult.getErrorMessage(),
                body);
    } else {
        JOYNR_LOG_DEBUG(logger,
                        "sending message - success; url: {} status code: {} at ",
                        url,
                        sendMessageResult.getStatusCode(),
                        DispatcherUtils::nowInMilliseconds());
    }
}
std::string HttpSender::SendMessageRunnable::resolveUrlForChannelId(
        std::chrono::milliseconds curlTimeout)
{
    JOYNR_LOG_TRACE(logger, "obtaining Url with a curlTimeout of : {}", curlTimeout.count());
    RequestStatus status;
    // we also use the curl timeout here, to prevent long blocking during shutdown.
    std::string url = messageSender->channelUrlCache->obtainUrl(channelId, status, curlTimeout);
    if (!status.successful()) {
        JOYNR_LOG_ERROR(logger,
                        "Issue while trying to obtained URl from the ChannelUrlDirectory: {}",
                        status.toString());
    }
    if (url.empty()) {
        JOYNR_LOG_DEBUG(logger,
                        "Url for channelId could not be obtained from the "
                        "ChannelUrlDirectory ... EXITING ...");
        assert(false); // OR: url = messageSender->bounceProxyUrl.getSendUrl(channelId).toString();
    }

    JOYNR_LOG_TRACE(logger,
                    "Sending message; url: {}, time left: {}",
                    url,
                    DispatcherUtils::convertAbsoluteTimeToTtlString(decayTime));
    return url;
}

HttpResult HttpSender::SendMessageRunnable::buildRequestAndSend(
        const std::string& url,
        std::chrono::milliseconds curlTimeout)
{
    std::shared_ptr<IHttpPostBuilder> sendMessageRequestBuilder(
            HttpNetworking::getInstance()->createHttpPostBuilder(url));

    std::shared_ptr<HttpRequest> sendMessageRequest(
            sendMessageRequestBuilder->withContentType("application/json")
                    ->withTimeout(std::min(maxAttemptTtl, curlTimeout))
                    ->postContent(QString::fromStdString(data).toUtf8())
                    ->build());
    JOYNR_LOG_TRACE(logger, "builtRequest");

    Util::logSerializedMessage(logger, "Sending Message: ", data);

    return sendMessageRequest->execute();
}

} // namespace joynr
