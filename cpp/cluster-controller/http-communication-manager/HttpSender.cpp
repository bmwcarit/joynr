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
#include "joynr/JoynrMessage.h"
#include "cluster-controller/http-communication-manager/HttpSender.h"
#include "joynr/Util.h"
#include "cluster-controller/httpnetworking/HttpNetworking.h"
#include "joynr/JsonSerializer.h"
#include "cluster-controller/httpnetworking/HttpResult.h"
#include "joynr/MessagingSettings.h"
#include "joynr/QtTypeUtil.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"

#include <algorithm>
#include <chrono>

namespace joynr
{

std::chrono::milliseconds HttpSender::MIN_ATTEMPT_TTL()
{
    return std::chrono::seconds(2);
}

const std::int64_t& HttpSender::FRACTION_OF_MESSAGE_TTL_USED_PER_CONNECTION_TRIAL()
{
    static std::int64_t value = 3;
    return value;
}

INIT_LOGGER(HttpSender);

HttpSender::HttpSender(const BrokerUrl& brokerUrl,
                       std::chrono::milliseconds maxAttemptTtl,
                       std::chrono::milliseconds messageSendRetryInterval)
        : brokerUrl(brokerUrl),
          maxAttemptTtl(maxAttemptTtl),
          messageSendRetryInterval(messageSendRetryInterval),
          delayedScheduler(6, "MessageSender")
{
}

void HttpSender::init(const MessagingSettings& settings)
{
    std::ignore = settings;
}

HttpSender::~HttpSender()
{
    delayedScheduler.shutdown();
}

void HttpSender::sendMessage(
        const system::RoutingTypes::Address& destinationAddress,
        const JoynrMessage& message,
        const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure)
{
    if (dynamic_cast<const system::RoutingTypes::ChannelAddress*>(&destinationAddress) == nullptr) {
        JOYNR_LOG_DEBUG(logger, "Invalid destination address type provided");
        onFailure(exceptions::JoynrRuntimeException("Invalid destination address type provided"));
        return;
    }

    auto channelAddress =
            dynamic_cast<const system::RoutingTypes::ChannelAddress&>(destinationAddress);

    // TODO pass back http errors to message router / calling MessageRunnable via onFailure
    std::ignore = onFailure;
    JOYNR_LOG_TRACE(logger, "sendMessage: ...");
    std::string&& serializedMessage = JsonSerializer::serialize(message);

    delayedScheduler.schedule(new SendMessageRunnable(this,
                                                      channelAddress,
                                                      message.getHeaderExpiryDate(),
                                                      std::move(serializedMessage),
                                                      delayedScheduler,
                                                      maxAttemptTtl));
}

/**
  * Implementation of SendMessageRunnable
  *
  */
INIT_LOGGER(HttpSender::SendMessageRunnable);

int HttpSender::SendMessageRunnable::messageRunnableCounter = 0;

HttpSender::SendMessageRunnable::SendMessageRunnable(
        HttpSender* messageSender,
        const system::RoutingTypes::ChannelAddress& channelAddress,
        const JoynrTimePoint& decayTime,
        std::string&& data,
        DelayedScheduler& delayedScheduler,
        std::chrono::milliseconds maxAttemptTtl)
        : Runnable(true),
          ObjectWithDecayTime(decayTime),
          channelAddress(channelAddress),
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

    // A channelId can have several Url's. Hence, we cannot use up all the time we have for
    // testing
    // just one (in case it is not available). So we use just a fraction, yet at least MIN...
    // and
    // at most MAX... seconds.
    std::int64_t curlTimeout =
            std::max(getRemainingTtl().count() /
                             HttpSender::FRACTION_OF_MESSAGE_TTL_USED_PER_CONNECTION_TRIAL(),
                     HttpSender::MIN_ATTEMPT_TTL().count());
    JOYNR_LOG_TRACE(logger, "going to buildRequest");
    HttpResult sendMessageResult =
            buildRequestAndSend(toUrl(channelAddress), std::chrono::milliseconds(curlTimeout));

    // Delay the next request if an error occurs
    auto now = std::chrono::system_clock::now();
    auto timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);
    std::chrono::milliseconds delay;
    if (messageSender->messageSendRetryInterval < timeDiff) {
        delay = std::chrono::milliseconds(10);
    } else {
        delay = messageSender->messageSendRetryInterval - timeDiff;
    }

    JOYNR_LOG_TRACE(
            logger, "sendMessageResult.getStatusCode() = {}", sendMessageResult.getStatusCode());
    if (sendMessageResult.getStatusCode() != 201) {
        delayedScheduler.schedule(new SendMessageRunnable(messageSender,
                                                          channelAddress,
                                                          decayTime,
                                                          std::move(data),
                                                          delayedScheduler,
                                                          maxAttemptTtl),
                                  std::chrono::milliseconds(delay));
        std::string body("NULL");
        if (!sendMessageResult.getBody().isNull()) {
            body = std::string(sendMessageResult.getBody().data());
        }
        JOYNR_LOG_ERROR(logger,
                        "sending message - fail; error message {}; contents {}; rescheduling "
                        "for retry...",
                        sendMessageResult.getErrorMessage(),
                        body);
    } else {
        JOYNR_LOG_DEBUG(logger,
                        "sending message - success; url: {} status code: {} at ",
                        toUrl(channelAddress),
                        sendMessageResult.getStatusCode(),
                        DispatcherUtils::nowInMilliseconds());
    }
}

HttpResult HttpSender::SendMessageRunnable::buildRequestAndSend(
        const std::string& url,
        std::chrono::milliseconds curlTimeout)
{
    JOYNR_LOG_TRACE(logger, "buildRequestAndSend.createHttpPostBuilder...");
    std::shared_ptr<IHttpPostBuilder> sendMessageRequestBuilder(
            HttpNetworking::getInstance()->createHttpPostBuilder(url));

    JOYNR_LOG_TRACE(logger, "buildRequestAndSend.sendMessageRequestBuilder...");
    std::shared_ptr<HttpRequest> sendMessageRequest(
            sendMessageRequestBuilder->withContentType("application/json")
                    ->withTimeout(std::min(maxAttemptTtl, curlTimeout))
                    ->postContent(QString::fromStdString(data).toUtf8())
                    ->build());
    JOYNR_LOG_TRACE(logger, "builtRequest");

    util::logSerializedMessage(logger, "Sending Message: ", data);

    return sendMessageRequest->execute();
}

std::string HttpSender::SendMessageRunnable::toUrl(
        const system::RoutingTypes::ChannelAddress& channelAddress) const
{
    std::string result;
    result.reserve(256);

    result.append(channelAddress.getMessagingEndpointUrl());

    if (result.length() > 0 && result.back() != '/') {
        result.append("/");
    }

    result.append("message/");

    return result;
}

void HttpSender::registerReceiveQueueStartedCallback(
        std::function<void(void)> waitForReceiveQueueStarted)
{
    std::ignore = waitForReceiveQueueStarted;
}

} // namespace joynr
