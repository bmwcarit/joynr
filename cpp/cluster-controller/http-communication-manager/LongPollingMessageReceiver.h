/*
 * #%L
 * joynr::C++
 * $Id:$
 * $HeadURL:$
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
#ifndef LONGPOLLINGMESSAGERECEIVER_H_
#define LONGPOLLINGMESSAGERECEIVER_H_
#include "joynr/PrivateCopyAssign.h"

#include "joynr/ContentWithDecayTime.h"
#include "cluster-controller/http-communication-manager/BounceProxyUrl.h"
#include "joynr/joynrlogging.h"

#include <QThread>
#include <QSemaphore>
#include <QMutex>

namespace joynr {

class LocalChannelUrlDirectory;

class IMessageReceiver;

/**
 * Structure used for configuring the long poll message receiver
 */
struct LongPollingMessageReceiverSettings {
    qint64 bounceProxyTimeout_ms;
    qint64 longPollTimeout_ms;
    int longPollRetryInterval_ms;
    int createChannelRetryInterval_ms;
};

/**
 * Class that makes long polling requests to the bounce proxy
 */
class LongPollingMessageReceiver : public QThread {
    Q_OBJECT

public:
    LongPollingMessageReceiver(const BounceProxyUrl& bounceProxyUrl,
                               const QString& channelId,
                               const QString& receiverId,
                               IMessageReceiver* messageReceiver,
                               const LongPollingMessageReceiverSettings& settings,
                               QSemaphore* channelCreatedSemaphore,
                               QSharedPointer<LocalChannelUrlDirectory> channelUrlDirectory);
    void run();
    void interrupt();
    bool isInterrupted();

    void processReceivedInput(const QByteArray& receivedInput);
    void processReceivedQjsonObjects(const QByteArray &jsonObject);


private:
    void checkServerTime();
    DISALLOW_COPY_AND_ASSIGN(LongPollingMessageReceiver);
    const BounceProxyUrl bounceProxyUrl;
    const QString channelId;
    const QString receiverId;
    const LongPollingMessageReceiverSettings settings;

    QMutex interruptedMutex;
    bool interrupted;
    IMessageReceiver* messageReceiver;
    QSharedPointer<LocalChannelUrlDirectory> channelUrlDirectory;

    static joynr_logging::Logger* logger;
    QSemaphore* channelCreatedSemaphore;
};


} // namespace joynr
#endif //LONGPOLLINGMESSAGERECEIVER_H_
