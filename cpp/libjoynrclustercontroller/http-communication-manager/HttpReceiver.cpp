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
#include "HttpReceiver.h"

#include <chrono>
#include <cstdint>

#include "joynr/BrokerUrl.h"
#include "joynr/Semaphore.h"
#include "joynr/Url.h"
#include "joynr/serializer/Serializer.h"

#include "../httpnetworking/HttpNetworking.h"
#include "../httpnetworking/HttpResult.h"

#include "LongPollingMessageReceiver.h"

namespace joynr
{

HttpReceiver::HttpReceiver(const MessagingSettings& settings,
                           const std::string& channelId,
                           const std::string& receiverId)
        : _channelCreatedSemaphore(std::make_shared<Semaphore>(0)),
          _channelId(channelId),
          _receiverId(receiverId),
          _globalClusterControllerAddress(),
          _settings(settings),
          _messageReceiver(nullptr),
          _onMessageReceived(nullptr)
{
    JOYNR_LOG_DEBUG(logger(), "Print settings... ");
    settings.printSettings();
    updateSettings();
    JOYNR_LOG_DEBUG(logger(), "Init finished.");

    _globalClusterControllerAddress = system::RoutingTypes::ChannelAddress(
            settings.getBrokerUrl().getBrokerChannelsBaseUrl().toString() + channelId + "/",
            channelId);

    // Remove any existing curl handles
    HttpNetworking::getInstance()->getCurlHandlePool()->reset();
}

void HttpReceiver::updateSettings()
{
    // Setup the proxy to use
    if (_settings.getLocalProxyHost().empty()) {
        HttpNetworking::getInstance()->setGlobalProxy(std::string());
    } else {
        HttpNetworking::getInstance()->setGlobalProxy(_settings.getLocalProxyHost() + ":" +
                                                      _settings.getLocalProxyPort());
    }

    // Turn on HTTP debug
    if (_settings.getHttpDebug()) {
        HttpNetworking::getInstance()->setHTTPDebugOn();
    }

    // Set the connect timeout
    HttpNetworking::getInstance()->setConnectTimeout(
            std::chrono::milliseconds(_settings.getHttpConnectTimeout()));

    // HTTPS settings
    HttpNetworking::getInstance()->setCertificateAuthority(_settings.getCertificateAuthority());
    HttpNetworking::getInstance()->setClientCertificate(_settings.getClientCertificate());
    HttpNetworking::getInstance()->setClientCertificatePassword(
            _settings.getClientCertificatePassword());
}

HttpReceiver::~HttpReceiver()
{
    JOYNR_LOG_TRACE(logger(), "destructing HttpCommunicationManager");
}

void HttpReceiver::startReceiveQueue()
{
    if (!_onMessageReceived) {
        JOYNR_LOG_FATAL(logger(), "FAIL::receiveQueue started with no onMessageReceived.");
    }

    // Get the settings specific to long polling
    LongPollingMessageReceiverSettings longPollSettings = {
            std::chrono::milliseconds(_settings.getBrokerTimeoutMs()),
            std::chrono::milliseconds(_settings.getLongPollTimeoutMs()),
            std::chrono::milliseconds(_settings.getLongPollRetryInterval()),
            std::chrono::milliseconds(_settings.getCreateChannelRetryInterval())};

    JOYNR_LOG_DEBUG(logger(), "startReceiveQueue");
    _messageReceiver = std::make_unique<LongPollingMessageReceiver>(_settings.getBrokerUrl(),
                                                                    _channelId,
                                                                    _receiverId,
                                                                    longPollSettings,
                                                                    _channelCreatedSemaphore,
                                                                    _onMessageReceived);
    _messageReceiver->start();
}

bool HttpReceiver::isConnected()
{
    return _channelCreatedSemaphore->waitFor(std::chrono::milliseconds::zero());
}

void HttpReceiver::stopReceiveQueue()
{
    // currently channelCreatedSemaphore is not released here. This would be necessary if
    // stopReceivequeue is called, before channel is created.
    JOYNR_LOG_DEBUG(logger(), "stopReceiveQueue");
    if (_messageReceiver) {
        _messageReceiver.reset();
    }
}

const std::string HttpReceiver::getSerializedGlobalClusterControllerAddress() const
{
    return joynr::serializer::serializeToJson(_globalClusterControllerAddress);
}

const system::RoutingTypes::ChannelAddress& HttpReceiver::getGlobalClusterControllerAddress() const
{
    return _globalClusterControllerAddress;
}

bool HttpReceiver::tryToDeleteChannel()
{
    // If more than one attempt is needed, create a deleteChannelRunnable and move this to
    // messageSender.
    // TODO channelUrl is known only to the LongPollingMessageReceiver!
    std::string deleteChannelUrl =
            _settings.getBrokerUrl()
                    .getDeleteChannelUrl(getSerializedGlobalClusterControllerAddress())
                    .toString();
    std::shared_ptr<IHttpDeleteBuilder> deleteChannelRequestBuilder(
            HttpNetworking::getInstance()->createHttpDeleteBuilder(deleteChannelUrl));
    std::shared_ptr<HttpRequest> deleteChannelRequest(
            deleteChannelRequestBuilder->withTimeout(std::chrono::seconds(20))->build());
    JOYNR_LOG_TRACE(logger(), "sending delete channel request to {}", deleteChannelUrl);
    HttpResult deleteChannelResult = deleteChannelRequest->execute();
    std::int64_t statusCode = deleteChannelResult.getStatusCode();
    if (statusCode == 200) {
        _channelCreatedSemaphore->waitFor(
                std::chrono::seconds(5)); // Reset the channel created Semaphore.
        JOYNR_LOG_DEBUG(logger(), "channel deletion successfull");

        return true;
    } else if (statusCode == 204) {
        JOYNR_LOG_DEBUG(logger(), "channel did not exist: {}", statusCode);
        return true;
    } else {
        JOYNR_LOG_DEBUG(logger(),
                        "channel deletion failed with status code: {}",
                        deleteChannelResult.getStatusCode());
        return false;
    }
}

void HttpReceiver::registerReceiveCallback(
        std::function<void(smrf::ByteVector&&)> onMessageReceived)
{
    this->_onMessageReceived = std::move(onMessageReceived);
}

} // namespace joynr
