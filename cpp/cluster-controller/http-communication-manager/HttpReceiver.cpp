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

#include <QtCore>

namespace joynr {

using namespace joynr_logging;

Logger* HttpReceiver::logger = Logging::getInstance()->getLogger("MSG", "HttpReveicer");

HttpReceiver::HttpReceiver(const MessagingSettings& settings, QSharedPointer<MessageRouter> messageRouter)
        : channelCreatedSemaphore(new QSemaphore(0)),
          channelId(),
          receiverId(),
          settings(settings),
          messageReceiver(NULL),
          channelUrlDirectory(),
          messageRouter(messageRouter)
{
    MessagingPropertiesPersistence persist(settings.getMessagingPropertiesPersistenceFilename());
    channelId = persist.getChannelId();
    receiverId = persist.getReceiverId();
    init();
}

void HttpReceiver::init(){
    LOG_DEBUG(logger, "Print settings... ");
    settings.printSettings();
    updateSettings();
    LOG_DEBUG(logger, "Init finished.");
}

void HttpReceiver::init(QSharedPointer<ILocalChannelUrlDirectory> channelUrlDirectory) {
    this->channelUrlDirectory = channelUrlDirectory;
}

void HttpReceiver::updateSettings() {
    // Setup the proxy to use
    if(settings.getLocalProxyHost().isEmpty()) {
        HttpNetworking::getInstance()->setGlobalProxy(QString());
    } else {
        HttpNetworking::getInstance()->setGlobalProxy(settings.getLocalProxyHost() + ":" + settings.getLocalProxyPort());
    }

    // Turn on HTTP debug
    if(settings.getHttpDebug()) {
        HttpNetworking::getInstance()->setHTTPDebugOn();
    }

    // Set the connect timeout
    HttpNetworking::getInstance()->setConnectTimeout_ms(settings.getHttpConnectTimeout());
}


HttpReceiver::~HttpReceiver() {
    LOG_TRACE(logger, "destructing HttpCommunicationManager");
}

void HttpReceiver::startReceiveQueue() {

    if (messageRouter.isNull() || channelUrlDirectory.isNull()) {
        LOG_FATAL(logger,"FAIL::receiveQueue started with no messageRouter/channelUrlDirectory.");
    }

    // Get the settings specific to long polling
    LongPollingMessageReceiverSettings longPollSettings = {
            settings.getBounceProxyTimeout(),
            settings.getLongPollTimeout(),
            settings.getLongPollRetryInterval(),
            settings.getCreateChannelRetryInterval(),
    };

    LOG_DEBUG(logger, "startReceiveQueue");
    messageReceiver = new LongPollingMessageReceiver(
                settings.getBounceProxyUrl(),
                channelId,
                receiverId,
                longPollSettings,
                channelCreatedSemaphore,
                channelUrlDirectory,
                messageRouter);
    messageReceiver->setObjectName(QString("HttpCommunicationManager-MessageReceiver"));
    messageReceiver->start();
}

void HttpReceiver::waitForReceiveQueueStarted()
{
    LOG_TRACE(logger, "waiting for ReceiveQueue to be started.");
    channelCreatedSemaphore->acquire(1);
    channelCreatedSemaphore->release(1);
}


void HttpReceiver::stopReceiveQueue() {
    //currently channelCreatedSemaphore is not released here. This would be necessary if stopReceivequeue is called, before channel is created.
    LOG_DEBUG(logger, "stopReceiveQueue");
    if (messageReceiver!=NULL){
        messageReceiver->interrupt();
        messageReceiver->wait(2 * 1000);
        if(!messageReceiver->isRunning()) {
            delete messageReceiver;
            messageReceiver = NULL;
        } else {
            messageReceiver->terminate();
            messageReceiver->wait();
            delete messageReceiver;
            messageReceiver = NULL;
        }
    }
}

const QString& HttpReceiver::getReceiveChannelId() const {
    return channelId;
}


bool HttpReceiver::tryToDeleteChannel() {
    // If more than one attempt is needed, create a deleteChannelRunnable and move this to messageSender.
    //TODO channelUrl is known only to the LongPlooMessageReceiver!
    QString deleteChannelUrl = settings.getBounceProxyUrl().getDeleteChannelUrl(getReceiveChannelId()).toString();
    QSharedPointer<IHttpDeleteBuilder> deleteChannelRequestBuilder(HttpNetworking::getInstance()->createHttpDeleteBuilder(deleteChannelUrl));
    QSharedPointer<HttpRequest> deleteChannelRequest(
                deleteChannelRequestBuilder
                ->withTimeout_ms(20 * 1000)
                ->build()
    );
    LOG_DEBUG(logger, "sending delete channel request to " + deleteChannelUrl);
    HttpResult deleteChannelResult = deleteChannelRequest->execute();
    long statusCode = deleteChannelResult.getStatusCode();
    if(statusCode == 200) {
        channelCreatedSemaphore->tryAcquire(1, 5000); //Reset the channel created Semaphore.
        LOG_INFO(logger, "channel deletion successfull");
        QSharedPointer<Future<void> > future(new Future<void>());
        channelUrlDirectory->unregisterChannelUrls(future,channelId);
        LOG_INFO(logger, "Sendeing unregister request to ChannelUrlDirectory ...");

        return true;
    } else if (statusCode == 204) {
        LOG_INFO(logger, "channel did not exist: " + QString::number(statusCode) );
        return true;
    } else {
        LOG_INFO(logger, "channel deletion failed with status code: " + QString::number(deleteChannelResult.getStatusCode()));
        return false;
    }


}

} // namespace joynr
