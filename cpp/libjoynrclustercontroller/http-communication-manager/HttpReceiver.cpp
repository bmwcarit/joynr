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
#include "libjoynrclustercontroller/http-communication-manager/HttpReceiver.h"

#include "joynr/Future.h"
#include "joynr/Util.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "libjoynrclustercontroller/http-communication-manager/LongPollingMessageReceiver.h"
#include "libjoynrclustercontroller/httpnetworking/HttpNetworking.h"
#include "libjoynrclustercontroller/httpnetworking/HttpResult.h"

namespace joynr
{

INIT_LOGGER(HttpReceiver);

HttpReceiver::HttpReceiver(const MessagingSettings& settings,
                           const std::string& channelId,
                           const std::string& receiverId)
        : channelCreatedSemaphore(std::make_shared<Semaphore>(0)),
          channelId(channelId),
          receiverId(receiverId),
          globalClusterControllerAddress(),
          settings(settings),
          messageReceiver(nullptr),
          onMessageReceived(nullptr)
{
    JOYNR_LOG_DEBUG(logger, "Print settings... ");
    settings.printSettings();
    updateSettings();
    JOYNR_LOG_DEBUG(logger, "Init finished.");

    system::RoutingTypes::ChannelAddress receiverChannelAddress(
            settings.getBrokerUrl().getBrokerChannelsBaseUrl().toString() + channelId + "/",
            channelId);

    globalClusterControllerAddress = joynr::serializer::serializeToJson(receiverChannelAddress);

    // Remove any existing curl handles
    HttpNetworking::getInstance()->getCurlHandlePool()->reset();
}

void HttpReceiver::updateSettings()
{
    // Setup the proxy to use
    if (settings.getLocalProxyHost().empty()) {
        HttpNetworking::getInstance()->setGlobalProxy(std::string());
    } else {
        HttpNetworking::getInstance()->setGlobalProxy(settings.getLocalProxyHost() + ":" +
                                                      settings.getLocalProxyPort());
    }

    // Turn on HTTP debug
    if (settings.getHttpDebug()) {
        HttpNetworking::getInstance()->setHTTPDebugOn();
    }

    // Set the connect timeout
    HttpNetworking::getInstance()->setConnectTimeout(
            std::chrono::milliseconds(settings.getHttpConnectTimeout()));

    // HTTPS settings
    HttpNetworking::getInstance()->setCertificateAuthority(settings.getCertificateAuthority());
    HttpNetworking::getInstance()->setClientCertificate(settings.getClientCertificate());
    HttpNetworking::getInstance()->setClientCertificatePassword(
            settings.getClientCertificatePassword());
}

HttpReceiver::~HttpReceiver()
{
    JOYNR_LOG_TRACE(logger, "destructing HttpCommunicationManager");
}

void HttpReceiver::startReceiveQueue()
{
    if (!onMessageReceived) {
        JOYNR_LOG_FATAL(logger, "FAIL::receiveQueue started with no onMessageReceived.");
    }

    // Get the settings specific to long polling
    LongPollingMessageReceiverSettings longPollSettings = {
            std::chrono::milliseconds(settings.getBrokerTimeout()),
            std::chrono::milliseconds(settings.getLongPollTimeout()),
            std::chrono::milliseconds(settings.getLongPollRetryInterval()),
            std::chrono::milliseconds(settings.getCreateChannelRetryInterval())};

    JOYNR_LOG_DEBUG(logger, "startReceiveQueue");
    messageReceiver = std::make_unique<LongPollingMessageReceiver>(settings.getBrokerUrl(),
                                                                   channelId,
                                                                   receiverId,
                                                                   longPollSettings,
                                                                   channelCreatedSemaphore,
                                                                   onMessageReceived);
    messageReceiver->start();
}

bool HttpReceiver::isConnected()
{
    return channelCreatedSemaphore->waitFor(std::chrono::milliseconds::zero());
}

void HttpReceiver::stopReceiveQueue()
{
    // currently channelCreatedSemaphore is not released here. This would be necessary if
    // stopReceivequeue is called, before channel is created.
    JOYNR_LOG_DEBUG(logger, "stopReceiveQueue");
    if (messageReceiver) {
        messageReceiver.reset();
    }
}

const std::string& HttpReceiver::getGlobalClusterControllerAddress() const
{
    return globalClusterControllerAddress;
}

bool HttpReceiver::tryToDeleteChannel()
{
    // If more than one attempt is needed, create a deleteChannelRunnable and move this to
    // messageSender.
    // TODO channelUrl is known only to the LongPollingMessageReceiver!
    std::string deleteChannelUrl = settings.getBrokerUrl()
                                           .getDeleteChannelUrl(getGlobalClusterControllerAddress())
                                           .toString();
    std::shared_ptr<IHttpDeleteBuilder> deleteChannelRequestBuilder(
            HttpNetworking::getInstance()->createHttpDeleteBuilder(deleteChannelUrl));
    std::shared_ptr<HttpRequest> deleteChannelRequest(
            deleteChannelRequestBuilder->withTimeout(std::chrono::seconds(20))->build());
    JOYNR_LOG_TRACE(logger, "sending delete channel request to {}", deleteChannelUrl);
    HttpResult deleteChannelResult = deleteChannelRequest->execute();
    std::int64_t statusCode = deleteChannelResult.getStatusCode();
    if (statusCode == 200) {
        channelCreatedSemaphore->waitFor(
                std::chrono::seconds(5)); // Reset the channel created Semaphore.
        JOYNR_LOG_DEBUG(logger, "channel deletion successfull");

        return true;
    } else if (statusCode == 204) {
        JOYNR_LOG_DEBUG(logger, "channel did not exist: {}", statusCode);
        return true;
    } else {
        JOYNR_LOG_DEBUG(logger,
                        "channel deletion failed with status code: {}",
                        deleteChannelResult.getStatusCode());
        return false;
    }
}

void HttpReceiver::registerReceiveCallback(
        std::function<void(smrf::ByteVector&&)> onMessageReceived)
{
    this->onMessageReceived = std::move(onMessageReceived);
}

} // namespace joynr
