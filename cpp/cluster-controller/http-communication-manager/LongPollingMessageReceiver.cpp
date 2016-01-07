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
#include "cluster-controller/http-communication-manager/LongPollingMessageReceiver.h"
#include "cluster-controller/httpnetworking/HttpNetworking.h"
#include "joynr/Util.h"
#include "joynr/JsonSerializer.h"
#include "joynr/DispatcherUtils.h"
#include <cstdlib>
#include <cstdint>
#include "joynr/TypeUtil.h"
#include "cluster-controller/httpnetworking/HttpResult.h"
#include "joynr/ILocalChannelUrlDirectory.h"
#include "joynr/Future.h"
#include "joynr/types/ChannelUrlInformation.h"
#include "joynr/JoynrMessage.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "joynr/MessageRouter.h"
#include "joynr/JoynrMessage.h"

#include <algorithm>
#include <QString>

namespace joynr
{

INIT_LOGGER(LongPollingMessageReceiver);

LongPollingMessageReceiver::LongPollingMessageReceiver(
        const BounceProxyUrl& bounceProxyUrl,
        const std::string& channelId,
        const std::string& receiverId,
        const LongPollingMessageReceiverSettings& settings,
        joynr::Semaphore* channelCreatedSemaphore,
        std::shared_ptr<ILocalChannelUrlDirectory> channelUrlDirectory,
        std::shared_ptr<MessageRouter> messageRouter)
        : joynr::Thread("LongPollRecv"),
          bounceProxyUrl(bounceProxyUrl),
          channelId(channelId),
          receiverId(receiverId),
          settings(settings),
          interrupted(false),
          interruptedMutex(),
          interruptedWait(),
          channelUrlDirectory(channelUrlDirectory),
          channelCreatedSemaphore(channelCreatedSemaphore),
          messageRouter(messageRouter)
{
}

void LongPollingMessageReceiver::interrupt()
{
    interrupted = true;
    interruptedWait.notify_all();
}

bool LongPollingMessageReceiver::isInterrupted()
{
    return interrupted;
}

void LongPollingMessageReceiver::stop()
{
    interrupt();
    joynr::Thread::stop();
}

void LongPollingMessageReceiver::run()
{
    checkServerTime();
    std::string createChannelUrl = bounceProxyUrl.getCreateChannelUrl(channelId).toString();
    JOYNR_LOG_INFO(logger, "Running lpmr with channelId {}", channelId);
    std::shared_ptr<IHttpPostBuilder> createChannelRequestBuilder(
            HttpNetworking::getInstance()->createHttpPostBuilder(createChannelUrl));
    std::shared_ptr<HttpRequest> createChannelRequest(
            createChannelRequestBuilder->addHeader("X-Atmosphere-tracking-id", receiverId)
                    ->withContentType("application/json")
                    ->withTimeout(settings.bounceProxyTimeout)
                    ->build());

    std::string channelUrl;
    while (channelUrl.empty() && !isInterrupted()) {
        JOYNR_LOG_DEBUG(logger, "sending create channel request");
        HttpResult createChannelResult = createChannelRequest->execute();
        if (createChannelResult.getStatusCode() == 201) {
            channelUrl = *createChannelResult.getHeaders().find("Location");
            JOYNR_LOG_INFO(logger, "channel creation successfull; channel url: {}", channelUrl);
            channelCreatedSemaphore->notify();
        } else {
            JOYNR_LOG_INFO(logger,
                           "channel creation failed); status code: {}",
                           createChannelResult.getStatusCode());
            std::unique_lock<std::mutex> lock(interruptedMutex);
            interruptedWait.wait_for(lock, settings.createChannelRetryInterval);
        }
    }
    /**
      * register the channelUrl with the ChannelUrlDirectory (asynchronously)
      */
    assert(channelUrlDirectory != nullptr);
    types::ChannelUrlInformation urlInformation;
    std::vector<std::string> urls = {channelUrl};
    urlInformation.setUrls(urls);
    JOYNR_LOG_INFO(
            logger,
            "Adding channelId and Url of cluster controller to remote ChannelUrlDirectory {}",
            channelUrl);
    channelUrlDirectory->registerChannelUrlsAsync(channelId, urlInformation);

    while (!isInterrupted()) {

        std::shared_ptr<IHttpGetBuilder> longPollRequestBuilder(
                HttpNetworking::getInstance()->createHttpGetBuilder(channelUrl));

        std::shared_ptr<HttpRequest> longPollRequest(
                longPollRequestBuilder->acceptGzip()
                        ->addHeader("Accept", "application/json")
                        ->addHeader("X-Atmosphere-tracking-id", receiverId)
                        ->withTimeout(settings.longPollTimeout)
                        ->build());

        JOYNR_LOG_DEBUG(logger, "sending long polling request; url: {}", channelUrl);
        HttpResult longPollingResult = longPollRequest->execute();
        if (!isInterrupted()) {
            // TODO: remove HttpErrorCodes and use constants.
            // there is a bug in atmosphere, which currently gives back 503 instead of 200 as a
            // result to longpolling.
            // Accepting 503 is a temporary workaround for this bug. As soon as atmosphere is fixed,
            // this should be removed
            // 200 does nott refect the state of the message body! It could be empty.
            if (longPollingResult.getStatusCode() == 200 ||
                longPollingResult.getStatusCode() == 503) {
                Util::logSerializedMessage(logger,
                                           "long polling successful; contents: ",
                                           longPollingResult.getBody().data());
                processReceivedInput(longPollingResult.getBody());
                // Atmosphere currently cannot return 204 when a long poll times out, so this code
                // is currently never executed (2.2.2012)
            } else if (longPollingResult.getStatusCode() == 204) {
                JOYNR_LOG_DEBUG(logger, "long polling successfull);full; no data");
            } else {
                std::string body("NULL");
                if (!longPollingResult.getBody().isNull()) {
                    body = QString(longPollingResult.getBody().data()).toStdString();
                }
                JOYNR_LOG_ERROR(logger,
                                "long polling failed; error message: {}; contents: {}",
                                longPollingResult.getErrorMessage(),
                                body);
                std::unique_lock<std::mutex> lock(interruptedMutex);
                interruptedWait.wait_for(lock, settings.createChannelRetryInterval);
            }
        }
    }
}

void LongPollingMessageReceiver::processReceivedInput(const QByteArray& receivedInput)
{
    std::vector<std::string> jsonObjects =
            Util::splitIntoJsonObjects(QString(receivedInput).toStdString());
    for (std::size_t i = 0; i < jsonObjects.size(); i++) {
        processReceivedJsonObjects(jsonObjects.at(i));
    }
}

void LongPollingMessageReceiver::processReceivedJsonObjects(const std::string& jsonObject)
{
    JoynrMessage* msg = JsonSerializer::deserialize<JoynrMessage>(jsonObject);
    if (msg == nullptr) {
        JOYNR_LOG_ERROR(logger, "Unable to deserialize message. Raw message: {}", jsonObject);
        return;
    }
    if (msg->getType().empty()) {
        JOYNR_LOG_ERROR(logger, "received empty message - dropping Messages");
        return;
    }
    if (!msg->containsHeaderExpiryDate()) {
        JOYNR_LOG_ERROR(logger,
                        "received message [msgId=[{}] without decay time - dropping message",
                        msg->getHeaderMessageId());
    }

    if (msg->getType() == JoynrMessage::VALUE_MESSAGE_TYPE_REQUEST ||
        msg->getType() == JoynrMessage::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST ||
        msg->getType() == JoynrMessage::VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST) {
        // TODO ca: check if replyTo header info is available?
        std::string replyChannelId = msg->getHeaderReplyChannelId();
        std::shared_ptr<system::RoutingTypes::ChannelAddress> address(
                new system::RoutingTypes::ChannelAddress(replyChannelId));
        messageRouter->addNextHop(msg->getHeaderFrom(), address);
    }

    // messageRouter.route passes the message reference to the MessageRunnable, which copies it.
    // TODO would be nicer if the pointer would be passed to messageRouter, on to MessageRunnable,
    // and runnable should delete it.
    messageRouter->route(*msg);
    delete msg;
}

void LongPollingMessageReceiver::checkServerTime()
{
    std::string timeCheckUrl = bounceProxyUrl.getTimeCheckUrl().toString();

    std::shared_ptr<IHttpGetBuilder> timeCheckRequestBuilder(
            HttpNetworking::getInstance()->createHttpGetBuilder(timeCheckUrl));
    std::shared_ptr<HttpRequest> timeCheckRequest(
            timeCheckRequestBuilder->addHeader("Accept", "text/plain")
                    ->withTimeout(settings.bounceProxyTimeout)
                    ->build());
    JOYNR_LOG_DEBUG(logger, "CheckServerTime: sending request to Bounce Proxy ({})", timeCheckUrl);
    std::chrono::system_clock::time_point localTimeBeforeRequest = std::chrono::system_clock::now();
    HttpResult timeCheckResult = timeCheckRequest->execute();
    std::chrono::system_clock::time_point localTimeAfterRequest = std::chrono::system_clock::now();
    std::uint64_t localTime = (TypeUtil::toMilliseconds(localTimeBeforeRequest) +
                               TypeUtil::toMilliseconds(localTimeAfterRequest)) /
                              2;
    if (timeCheckResult.getStatusCode() != 200) {
        JOYNR_LOG_ERROR(logger,
                        "CheckServerTime: Bounce Proxy not reached [statusCode={}] [body={}]",
                        timeCheckResult.getStatusCode(),
                        QString(timeCheckResult.getBody()).toStdString());
    } else {
        JOYNR_LOG_TRACE(logger,
                        "CheckServerTime: reply received [statusCode={}] [body={}]",
                        timeCheckResult.getStatusCode(),
                        QString(timeCheckResult.getBody()).toStdString());
        std::uint64_t serverTime =
                TypeUtil::toStdUInt64(QString(timeCheckResult.getBody()).toLongLong());

        auto minMaxTime = std::minmax(serverTime, localTime);
        std::uint64_t diff = minMaxTime.second - minMaxTime.first;

        JOYNR_LOG_INFO(
                logger,
                "CheckServerTime [server time={}] [local time={}] [diff={} ms]",
                TypeUtil::toDateString(JoynrTimePoint(std::chrono::milliseconds(serverTime))),
                TypeUtil::toDateString(JoynrTimePoint(std::chrono::milliseconds(localTime))),
                diff);

        if (diff > 500) {
            JOYNR_LOG_ERROR(logger, "CheckServerTime: time difference to server is {} ms", diff);
        }
    }
}

} // namespace joynr
