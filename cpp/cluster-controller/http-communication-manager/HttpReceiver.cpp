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
#include "cluster-controller/http-communication-manager/HttpReceiver.h"

#include "joynr/Util.h"
#include "cluster-controller/httpnetworking/HttpNetworking.h"
#include "cluster-controller/http-communication-manager/LongPollingMessageReceiver.h"
#include "joynr/ObjectWithDecayTime.h"
#include "joynr/JsonSerializer.h"
#include "joynr/JoynrMessage.h"
#include "cluster-controller/httpnetworking/HttpResult.h"
#include "cluster-controller/messaging/MessagingPropertiesPersistence.h"
#include "joynr/Future.h"
#include "joynr/TypeUtil.h"

#include <QtCore>

namespace joynr
{

using namespace joynr_logging;

Logger* HttpReceiver::logger = Logging::getInstance()->getLogger("MSG", "HttpReceiver");

HttpReceiver::HttpReceiver(const MessagingSettings& settings,
                           std::shared_ptr<MessageRouter> messageRouter)
        : channelCreatedSemaphore(new QSemaphore(0)),
          channelId(),
          receiverId(),
          settings(settings),
          messageReceiver(NULL),
          channelUrlDirectory(),
          messageRouter(messageRouter)
{
    MessagingPropertiesPersistence persist(settings.getMessagingPropertiesPersistenceFilename());
    channelId = TypeUtil::toQt(persist.getChannelId());
    receiverId = persist.getReceiverId();
    init();

    // Remove any existing curl handles
    HttpNetworking::getInstance()->getCurlHandlePool()->reset();
}

void HttpReceiver::init()
{
    LOG_DEBUG(logger, "Print settings... ");
    settings.printSettings();
    updateSettings();
    LOG_DEBUG(logger, "Init finished.");
}

void HttpReceiver::init(std::shared_ptr<ILocalChannelUrlDirectory> channelUrlDirectory)
{
    this->channelUrlDirectory = channelUrlDirectory;
}

void HttpReceiver::updateSettings()
{
    // Setup the proxy to use
    if (settings.getLocalProxyHost().empty()) {
        HttpNetworking::getInstance()->setGlobalProxy(QString());
    } else {
        HttpNetworking::getInstance()->setGlobalProxy(TypeUtil::toQt(settings.getLocalProxyHost()) +
                                                      ":" +
                                                      TypeUtil::toQt(settings.getLocalProxyPort()));
    }

    // Turn on HTTP debug
    if (settings.getHttpDebug()) {
        HttpNetworking::getInstance()->setHTTPDebugOn();
    }

    // Set the connect timeout
    HttpNetworking::getInstance()->setConnectTimeout_ms(settings.getHttpConnectTimeout());

    // HTTPS settings
    HttpNetworking::getInstance()->setCertificateAuthority(
            TypeUtil::toQt(settings.getCertificateAuthority()));
    HttpNetworking::getInstance()->setClientCertificate(
            TypeUtil::toQt(settings.getClientCertificate()));
    HttpNetworking::getInstance()->setClientCertificatePassword(
            TypeUtil::toQt(settings.getClientCertificatePassword()));
}

HttpReceiver::~HttpReceiver()
{
    LOG_TRACE(logger, "destructing HttpCommunicationManager");
}

void HttpReceiver::startReceiveQueue()
{

    if (!messageRouter || !channelUrlDirectory) {
        LOG_FATAL(logger, "FAIL::receiveQueue started with no messageRouter/channelUrlDirectory.");
    }

    // Get the settings specific to long polling
    LongPollingMessageReceiverSettings longPollSettings = {
            settings.getBounceProxyTimeout(),
            settings.getLongPollTimeout(),
            settings.getLongPollRetryInterval(),
            settings.getCreateChannelRetryInterval(),
    };

    LOG_DEBUG(logger, "startReceiveQueue");
    messageReceiver = new LongPollingMessageReceiver(settings.getBounceProxyUrl(),
                                                     channelId,
                                                     TypeUtil::toQt(receiverId),
                                                     longPollSettings,
                                                     channelCreatedSemaphore,
                                                     channelUrlDirectory,
                                                     messageRouter);
    messageReceiver->start();
}

void HttpReceiver::waitForReceiveQueueStarted()
{
    LOG_TRACE(logger, "waiting for ReceiveQueue to be started.");
    channelCreatedSemaphore->acquire(1);
    channelCreatedSemaphore->release(1);
}

void HttpReceiver::stopReceiveQueue()
{
    // currently channelCreatedSemaphore is not released here. This would be necessary if
    // stopReceivequeue is called, before channel is created.
    LOG_DEBUG(logger, "stopReceiveQueue");
    if (messageReceiver != NULL) {
        messageReceiver->stop();

        delete messageReceiver;
        messageReceiver = NULL;
    }
}

const QString& HttpReceiver::getReceiveChannelId() const
{
    return channelId;
}

bool HttpReceiver::tryToDeleteChannel()
{
    // If more than one attempt is needed, create a deleteChannelRunnable and move this to
    // messageSender.
    // TODO channelUrl is known only to the LongPlooMessageReceiver!
    QString deleteChannelUrl =
            settings.getBounceProxyUrl().getDeleteChannelUrl(getReceiveChannelId()).toString();
    std::shared_ptr<IHttpDeleteBuilder> deleteChannelRequestBuilder(
            HttpNetworking::getInstance()->createHttpDeleteBuilder(deleteChannelUrl));
    std::shared_ptr<HttpRequest> deleteChannelRequest(
            deleteChannelRequestBuilder->withTimeout_ms(20 * 1000)->build());
    LOG_DEBUG(logger, "sending delete channel request to " + deleteChannelUrl);
    HttpResult deleteChannelResult = deleteChannelRequest->execute();
    long statusCode = deleteChannelResult.getStatusCode();
    if (statusCode == 200) {
        channelCreatedSemaphore->tryAcquire(1, 5000); // Reset the channel created Semaphore.
        LOG_INFO(logger, "channel deletion successfull");
        channelUrlDirectory->unregisterChannelUrlsAsync(channelId.toStdString());
        LOG_INFO(logger, "Sendeing unregister request to ChannelUrlDirectory ...");

        return true;
    } else if (statusCode == 204) {
        LOG_INFO(logger, "channel did not exist: " + QString::number(statusCode));
        return true;
    } else {
        LOG_INFO(logger,
                 "channel deletion failed with status code: " +
                         QString::number(deleteChannelResult.getStatusCode()));
        return false;
    }
}

} // namespace joynr
