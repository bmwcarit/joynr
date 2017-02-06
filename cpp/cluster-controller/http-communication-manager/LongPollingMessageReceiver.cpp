/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <algorithm>

#include <boost/lexical_cast.hpp>

#include "cluster-controller/http-communication-manager/LongPollingMessageReceiver.h"
#include "cluster-controller/httpnetworking/HttpNetworking.h"
#include "joynr/Util.h"
#include "joynr/DispatcherUtils.h"
#include "cluster-controller/httpnetworking/HttpResult.h"
#include "joynr/Future.h"
#include "joynr/JoynrMessage.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "joynr/JoynrMessage.h"

namespace joynr
{

INIT_LOGGER(LongPollingMessageReceiver);

LongPollingMessageReceiver::LongPollingMessageReceiver(
        const BrokerUrl& brokerUrl,
        const std::string& channelId,
        const std::string& receiverId,
        const LongPollingMessageReceiverSettings& settings,
        std::shared_ptr<Semaphore> channelCreatedSemaphore,
        std::function<void(const std::string&)> onTextMessageReceived)
        : Thread("LongPollRecv"),
          brokerUrl(brokerUrl),
          channelId(channelId),
          receiverId(receiverId),
          settings(settings),
          interrupted(false),
          interruptedMutex(),
          interruptedWait(),
          channelCreatedSemaphore(channelCreatedSemaphore),
          onTextMessageReceived(onTextMessageReceived),
          currentRequest()
{
}

LongPollingMessageReceiver::~LongPollingMessageReceiver()
{
    stop();
}

void LongPollingMessageReceiver::interrupt()
{
    std::unique_lock<std::mutex> lock(interruptedMutex);
    if (currentRequest) {
        currentRequest->interrupt();
    }
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
    Thread::stop();
}

void LongPollingMessageReceiver::run()
{
    checkServerTime();
    std::string createChannelUrl = brokerUrl.getCreateChannelUrl(channelId).toString();
    JOYNR_LOG_DEBUG(logger, "Running lpmr with channelId {}", channelId);
    std::shared_ptr<IHttpPostBuilder> createChannelRequestBuilder(
            HttpNetworking::getInstance()->createHttpPostBuilder(createChannelUrl));
    currentRequest.reset(
            createChannelRequestBuilder->addHeader("X-Atmosphere-tracking-id", receiverId)
                    ->withContentType("application/json")
                    ->withTimeout(settings.brokerTimeout)
                    ->build());

    std::string channelUrl;
    while (channelUrl.empty() && !isInterrupted()) {
        JOYNR_LOG_TRACE(logger, "sending create channel request");
        HttpResult createChannelResult = currentRequest->execute();
        if (createChannelResult.getStatusCode() == 201) {
            const std::unordered_multimap<std::string, std::string>& headers =
                    createChannelResult.getHeaders();
            auto it = headers.find("Location");
            channelUrl = it->second;
            JOYNR_LOG_DEBUG(logger, "channel creation successfull; channel url: {}", channelUrl);
            channelCreatedSemaphore->notify();
        } else {
            JOYNR_LOG_WARN(logger,
                           "channel creation failed); status code: {}",
                           createChannelResult.getStatusCode());
            std::unique_lock<std::mutex> lock(interruptedMutex);
            interruptedWait.wait_for(lock, settings.createChannelRetryInterval);
        }
    }

    // TODO: The received URL must be forwarded in such a way that ChannelAddress objects use it.

    while (!isInterrupted()) {

        std::shared_ptr<IHttpGetBuilder> longPollRequestBuilder(
                HttpNetworking::getInstance()->createHttpGetBuilder(channelUrl));

        currentRequest.reset(longPollRequestBuilder->acceptGzip()
                                     ->addHeader("Accept", "application/json")
                                     ->addHeader("X-Atmosphere-tracking-id", receiverId)
                                     ->withTimeout(settings.longPollTimeout)
                                     ->build());

        JOYNR_LOG_TRACE(logger, "sending long polling request; url: {}", channelUrl);
        HttpResult longPollingResult = currentRequest->execute();
        if (!isInterrupted()) {
            // TODO: remove HttpErrorCodes and use constants.
            // there is a bug in atmosphere, which currently gives back 503 instead of 200 as a
            // result to longpolling.
            // Accepting 503 is a temporary workaround for this bug. As soon as atmosphere is fixed,
            // this should be removed
            // 200 does nott refect the state of the message body! It could be empty.
            if (longPollingResult.getStatusCode() == 200 ||
                longPollingResult.getStatusCode() == 503) {
                util::logSerializedMessage(
                        logger, "long polling successful; contents: ", longPollingResult.getBody());
                processReceivedInput(longPollingResult.getBody());
                // Atmosphere currently cannot return 204 when a long poll times out, so this code
                // is currently never executed (2.2.2012)
            } else if (longPollingResult.getStatusCode() == 204) {
                JOYNR_LOG_TRACE(logger, "long polling successfull);full; no data");
            } else {
                std::string body("NULL");
                if (!longPollingResult.getBody().empty()) {
                    body = longPollingResult.getBody();
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

void LongPollingMessageReceiver::processReceivedInput(const std::string& receivedInput)
{
    std::vector<std::string> jsonObjects = util::splitIntoJsonObjects(receivedInput);
    for (std::size_t i = 0; i < jsonObjects.size(); i++) {
        processReceivedJsonObjects(jsonObjects.at(i));
    }
}

void LongPollingMessageReceiver::processReceivedJsonObjects(const std::string& jsonObject)
{
    if (onTextMessageReceived) {
        onTextMessageReceived(jsonObject);
    } else {
        JOYNR_LOG_ERROR(
                logger,
                "Discarding received message, since onTextMessageReceived callback is empty.");
    }
}

void LongPollingMessageReceiver::checkServerTime()
{
    std::string timeCheckUrl = brokerUrl.getTimeCheckUrl().toString();

    std::unique_ptr<IHttpGetBuilder> timeCheckRequestBuilder(
            HttpNetworking::getInstance()->createHttpGetBuilder(timeCheckUrl));
    std::shared_ptr<HttpRequest> timeCheckRequest(
            timeCheckRequestBuilder->addHeader("Accept", "text/plain")
                    ->withTimeout(settings.brokerTimeout)
                    ->build());
    JOYNR_LOG_TRACE(logger, "CheckServerTime: sending request to Bounce Proxy ({})", timeCheckUrl);
    std::chrono::system_clock::time_point localTimeBeforeRequest = std::chrono::system_clock::now();
    HttpResult timeCheckResult = timeCheckRequest->execute();
    std::chrono::system_clock::time_point localTimeAfterRequest = std::chrono::system_clock::now();
    std::uint64_t localTime = (util::toMilliseconds(localTimeBeforeRequest) +
                               util::toMilliseconds(localTimeAfterRequest)) /
                              2;
    if (timeCheckResult.getStatusCode() != 200) {
        JOYNR_LOG_ERROR(logger,
                        "CheckServerTime: Bounce Proxy not reached [statusCode={}] [body={}]",
                        timeCheckResult.getStatusCode(),
                        timeCheckResult.getBody());
    } else {
        JOYNR_LOG_TRACE(logger,
                        "CheckServerTime: reply received [statusCode={}] [body={}]",
                        timeCheckResult.getStatusCode(),
                        timeCheckResult.getBody());

        std::uint64_t serverTime;

        try {
            serverTime = boost::lexical_cast<std::uint64_t>(timeCheckResult.getBody());
        } catch (const boost::bad_lexical_cast& exception) {
            JOYNR_LOG_ERROR(logger,
                            ": Failed to cast received server time [statusCode={}] [body={}]",
                            timeCheckResult.getStatusCode(),
                            timeCheckResult.getBody());
            return;
        }

        auto minMaxTime = std::minmax(serverTime, localTime);
        std::uint64_t diff = minMaxTime.second - minMaxTime.first;

        JOYNR_LOG_DEBUG(logger,
                        "CheckServerTime [server time={}] [local time={}] [diff={} ms]",
                        util::toDateString(JoynrTimePoint(std::chrono::milliseconds(serverTime))),
                        util::toDateString(JoynrTimePoint(std::chrono::milliseconds(localTime))),
                        diff);

        if (diff > 500) {
            JOYNR_LOG_WARN(logger, "CheckServerTime: time difference to server is {} ms", diff);
        }
    }
}

} // namespace joynr
