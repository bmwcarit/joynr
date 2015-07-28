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
#include "cluster-controller/httpnetworking/HttpResult.h"
#include "joynr/ILocalChannelUrlDirectory.h"
#include "joynr/Future.h"
#include "joynr/types/QtChannelUrlInformation.h"
#include "joynr/JoynrMessage.h"
#include "joynr/system/QtChannelAddress.h"
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
        const QString& channelId,
        const QString& receiverId,
        const LongPollingMessageReceiverSettings& settings,
        QSemaphore* channelCreatedSemaphore,
        QSharedPointer<ILocalChannelUrlDirectory> channelUrlDirectory,
        QSharedPointer<MessageRouter> messageRouter)
        : bounceProxyUrl(bounceProxyUrl),
          channelId(channelId),
          receiverId(receiverId),
          settings(settings),
          interruptedMutex(),
          interrupted(false),
          channelUrlDirectory(channelUrlDirectory),
          channelCreatedSemaphore(channelCreatedSemaphore),
          messageRouter(messageRouter)
{
}

void LongPollingMessageReceiver::interrupt()
{
    QMutexLocker lock(&interruptedMutex);
    interrupted = true;
}

bool LongPollingMessageReceiver::isInterrupted()
{
    QMutexLocker lock(&interruptedMutex);
    return (interrupted) ? true : false;
}

void LongPollingMessageReceiver::run()
{
    checkServerTime();
    QString createChannelUrl = bounceProxyUrl.getCreateChannelUrl(channelId).toString();
    LOG_INFO(logger, "Running lpmr with channelId " + channelId);
    QSharedPointer<IHttpPostBuilder> createChannelRequestBuilder(
            HttpNetworking::getInstance()->createHttpPostBuilder(createChannelUrl));
    QSharedPointer<HttpRequest> createChannelRequest(
            createChannelRequestBuilder->addHeader("X-Atmosphere-tracking-id", receiverId)
                    ->withContentType("application/json")
                    ->withTimeout_ms(settings.bounceProxyTimeout_ms)
                    ->build());

    QString channelUrl;
    while (channelUrl.isEmpty() && !isInterrupted()) {
        LOG_DEBUG(logger, "sending create channel request");
        HttpResult createChannelResult = createChannelRequest->execute();
        if (createChannelResult.getStatusCode() == 201) {
            channelUrl = *createChannelResult.getHeaders().find("Location");
            LOG_INFO(logger, "channel creation successfull; channel url:" + channelUrl);
            channelCreatedSemaphore->release(1);
        } else {
            LOG_INFO(logger,
                     "channel creation failed; status code:" +
                             QString::number(createChannelResult.getStatusCode()));
            usleep(settings.createChannelRetryInterval_ms * 1000);
        }
    }
    /**
      * register the channelUrl with the ChannelUrlDirectory (asynchronously)
      */
    assert(channelUrlDirectory != NULL);
    types::StdChannelUrlInformation urlInformation;
    std::vector<std::string> urls = {channelUrl.toStdString()};
    urlInformation.setUrls(urls);
    LOG_INFO(logger,
             "Adding channelId and Url of cluster controller to remote ChannelUrlDirectory" +
                     channelUrl);
    channelUrlDirectory->registerChannelUrls(channelId.toStdString(), urlInformation);

    while (!isInterrupted()) {

        QSharedPointer<IHttpGetBuilder> longPollRequestBuilder(
                HttpNetworking::getInstance()->createHttpGetBuilder(channelUrl));

        QSharedPointer<HttpRequest> longPollRequest(
                longPollRequestBuilder->acceptGzip()
                        ->addHeader("Accept", "application/json")
                        ->addHeader("X-Atmosphere-tracking-id", receiverId)
                        ->withTimeout_ms(settings.longPollTimeout_ms)
                        ->build());

        LOG_DEBUG(logger, QString("sending long polling request; url: %1").arg(channelUrl));
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
                                           QString("long polling successful; contents: "),
                                           longPollingResult.getBody().data());
                processReceivedInput(longPollingResult.getBody());
                // Atmosphere currently cannot return 204 when a long poll times out, so this code
                // is currently never executed (2.2.2012)
            } else if (longPollingResult.getStatusCode() == 204) {
                LOG_DEBUG(logger, "long polling successfull; no data");
            } else {
                QString body("NULL");
                if (!longPollingResult.getBody().isNull()) {
                    body = QString(longPollingResult.getBody().data());
                }
                LOG_ERROR(logger,
                          QString("long polling failed; error message: %1; contents: %2")
                                  .arg(longPollingResult.getErrorMessage())
                                  .arg(body));
                usleep(settings.longPollRetryInterval_ms * 1000);
            }
        }
    }
}

void LongPollingMessageReceiver::processReceivedInput(const QByteArray& receivedInput)
{
    QList<QByteArray> jsonObjects = Util::splitIntoJsonObjects(receivedInput);
    for (int i = 0; i < jsonObjects.size(); i++) {
        processReceivedQjsonObjects(jsonObjects.at(i));
    }
}

void LongPollingMessageReceiver::processReceivedQjsonObjects(const QByteArray& jsonObject)
{
    JoynrMessage* msg = JsonSerializer::deserialize<JoynrMessage>(jsonObject);
    if (msg == Q_NULLPTR) {
        LOG_ERROR(logger,
                  QString("Unable to deserialize message. Raw message: %1")
                          .arg(QString::fromUtf8(jsonObject)));
        return;
    }
    if (msg->getType().isEmpty()) {
        LOG_ERROR(logger, "received empty message - dropping Messages");
        return;
    }
    if (!msg->containsHeaderExpiryDate()) {
        LOG_ERROR(logger,
                  QString("received message [msgId=%1] without decay time - dropping message")
                          .arg(msg->getHeaderMessageId()));
    }

    if (msg->getType() == JoynrMessage::VALUE_MESSAGE_TYPE_REQUEST ||
        msg->getType() == JoynrMessage::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST ||
        msg->getType() == JoynrMessage::VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST) {
        // TODO ca: check if replyTo header info is available?
        QString replyChannelId = msg->getHeaderReplyChannelId();
        QSharedPointer<system::QtChannelAddress> address(
                new system::QtChannelAddress(replyChannelId));
        messageRouter->addNextHop(msg->getHeaderFrom().toStdString(), address);
    }

    // messageRouter.route passes the message reference to the MessageRunnable, which copies it.
    // TODO would be nicer if the pointer would be passed to messageRouter, on to MessageRunnable,
    // and runnable should delete it.
    messageRouter->route(*msg);
    delete msg;
}

void LongPollingMessageReceiver::checkServerTime()
{
    QString timeCheckUrl = bounceProxyUrl.getTimeCheckUrl().toString();

    QSharedPointer<IHttpGetBuilder> timeCheckRequestBuilder(
            HttpNetworking::getInstance()->createHttpGetBuilder(timeCheckUrl));
    QSharedPointer<HttpRequest> timeCheckRequest(
            timeCheckRequestBuilder->addHeader("Accept", "text/plain")
                    ->withTimeout_ms(settings.bounceProxyTimeout_ms)
                    ->build());
    LOG_DEBUG(logger,
              QString("CheckServerTime: sending request to Bounce Proxy (") + timeCheckUrl +
                      QString(")"));
    QDateTime localTimeBeforeRequest = QDateTime::currentDateTime();
    HttpResult timeCheckResult = timeCheckRequest->execute();
    QDateTime localTimeAfterRequest = QDateTime::currentDateTime();
    QDateTime localTime =
            QDateTime::fromMSecsSinceEpoch((localTimeBeforeRequest.toMSecsSinceEpoch() +
                                            localTimeAfterRequest.toMSecsSinceEpoch()) /
                                           2);
    if (timeCheckResult.getStatusCode() != 200) {
        LOG_ERROR(logger,
                  QString("CheckServerTime: Bounce Proxy not reached [statusCode=%1] [body=%2]")
                          .arg(QString::number(timeCheckResult.getStatusCode()))
                          .arg(QString(timeCheckResult.getBody())));
    } else {
        LOG_TRACE(logger,
                  QString("CheckServerTime: reply received [statusCode=%1] [body=%2]")
                          .arg(QString::number(timeCheckResult.getStatusCode()))
                          .arg(QString(timeCheckResult.getBody())));
        QDateTime serverTime =
                QDateTime::fromMSecsSinceEpoch(QString(timeCheckResult.getBody()).toLongLong());
        qint64 diff = qAbs(serverTime.msecsTo(localTime));

        LOG_INFO(logger,
                 QString("CheckServerTime [server time=%1] [local time=%2] [diff=%3 ms]")
                         .arg(serverTime.toString())
                         .arg(localTime.toString())
                         .arg(QString::number(diff)));
        if (diff > 500) {
            LOG_ERROR(logger,
                      QString("CheckServerTime: time difference to server is %1 ms")
                              .arg(QString::number(diff)));
        }
    }
}

} // namespace joynr
