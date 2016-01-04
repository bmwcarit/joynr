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
#include <stdint.h>
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

namespace joynr
{

using namespace joynr_logging;

Logger* LongPollingMessageReceiver::logger =
        Logging::getInstance()->getLogger("MSG", "LongPollingMessageReceiver");

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
    LOG_INFO(logger, FormatString("Running lpmr with channelId %1").arg(channelId).str());
    std::shared_ptr<IHttpPostBuilder> createChannelRequestBuilder(
            HttpNetworking::getInstance()->createHttpPostBuilder(createChannelUrl));
    std::shared_ptr<HttpRequest> createChannelRequest(
            createChannelRequestBuilder->addHeader("X-Atmosphere-tracking-id", receiverId)
                    ->withContentType("application/json")
                    ->withTimeout(settings.bounceProxyTimeout)
                    ->build());

    std::string channelUrl;
    while (channelUrl.empty() && !isInterrupted()) {
        LOG_DEBUG(logger, "sending create channel request");
        HttpResult createChannelResult = createChannelRequest->execute();
        if (createChannelResult.getStatusCode() == 201) {
            channelUrl = *createChannelResult.getHeaders().find("Location");
            LOG_INFO(logger,
                     FormatString("channel creation successfull; channel url:%1")
                             .arg(channelUrl)
                             .str());
            channelCreatedSemaphore->notify();
        } else {
            LOG_INFO(logger,
                     FormatString("channel creation failed; status code:%1")
                             .arg(createChannelResult.getStatusCode())
                             .str());

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
    LOG_INFO(logger,
             FormatString("Adding channelId and Url of cluster controller to remote "
                          "ChannelUrlDirectory%1")
                     .arg(channelUrl)
                     .str());
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

        LOG_DEBUG(logger,
                  FormatString("sending long polling request; url: %1").arg(channelUrl).str());
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
                LOG_DEBUG(logger, "long polling successfull; no data");
            } else {
                std::string body("NULL");
                if (!longPollingResult.getBody().isNull()) {
                    body = QString(longPollingResult.getBody().data()).toStdString();
                }
                LOG_ERROR(logger,
                          FormatString("long polling failed; error message: %1; contents: %2")
                                  .arg(longPollingResult.getErrorMessage())
                                  .arg(body)
                                  .str());

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
        LOG_ERROR(logger,
                  FormatString("Unable to deserialize message. Raw message: %1")
                          .arg(jsonObject)
                          .str());
        return;
    }
    if (msg->getType().empty()) {
        LOG_ERROR(logger, "received empty message - dropping Messages");
        return;
    }
    if (!msg->containsHeaderExpiryDate()) {
        LOG_ERROR(logger,
                  FormatString("received message [msgId=%1] without decay time - dropping message")
                          .arg(msg->getHeaderMessageId())
                          .str());
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
    LOG_DEBUG(logger,
              FormatString("CheckServerTime: sending request to Bounce Proxy (%1)")
                      .arg(timeCheckUrl)
                      .str());
    std::chrono::system_clock::time_point localTimeBeforeRequest = std::chrono::system_clock::now();
    HttpResult timeCheckResult = timeCheckRequest->execute();
    system_clock::time_point localTimeAfterRequest = std::chrono::system_clock::now();
    uint64_t localTime = (TypeUtil::toMilliseconds(localTimeBeforeRequest) +
                          TypeUtil::toMilliseconds(localTimeAfterRequest)) /
                         2;
    if (timeCheckResult.getStatusCode() != 200) {
        LOG_ERROR(
                logger,
                FormatString("CheckServerTime: Bounce Proxy not reached [statusCode=%1] [body=%2]")
                        .arg(timeCheckResult.getStatusCode())
                        .arg(QString(timeCheckResult.getBody()).toStdString())
                        .str());
    } else {
        LOG_TRACE(logger,
                  FormatString("CheckServerTime: reply received [statusCode=%1] [body=%2]")
                          .arg(timeCheckResult.getStatusCode())
                          .arg(QString(timeCheckResult.getBody()).toStdString())
                          .str());
        uint64_t serverTime =
                TypeUtil::toStdUInt64(QString(timeCheckResult.getBody()).toLongLong());

        auto minMaxTime = std::minmax(serverTime, localTime);
        uint64_t diff = minMaxTime.second - minMaxTime.first;

        LOG_INFO(logger,
                 FormatString("CheckServerTime [server time=%1] [local time=%2] [diff=%3 ms]")
                         .arg(TypeUtil::toDateString(JoynrTimePoint(milliseconds(serverTime))))
                         .arg(TypeUtil::toDateString(JoynrTimePoint(milliseconds(localTime))))
                         .arg(diff)
                         .str());
        if (diff > 500) {
            LOG_ERROR(logger,
                      FormatString("CheckServerTime: time difference to server is %1 ms")
                              .arg(diff)
                              .str());
        }
    }
}

} // namespace joynr
