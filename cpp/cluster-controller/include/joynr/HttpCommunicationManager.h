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
#ifndef HTTPCOMMUNICATIONMANAGER_H_
#define HTTPCOMMUNICATIONMANAGER_H_
#include "joynr/PrivateCopyAssign.h"

#include "joynr/JoynrClusterControllerExport.h"

#include "joynr/ICommunicationManager.h"
#include "joynr/MessagingSettings.h"
#include "joynr/joynrlogging.h"
#include "joynr/ILocalChannelUrlDirectory.h"

#include <QString>
#include <QSettings>
#include <QSemaphore>

class DispatcherIntegrationTest;
class CapabilitiesClientTest;

namespace joynr {

class JoynrMessage;
class IMessageReceiver;
class LongPollingMessageReceiver;
class IMessageSender;


/**
  * \class HttpCommunicationManager
  * \brief Implements HTTP communication to the bounce proxy (backend)
  *
  * Implements the ICommunicationManager interface using the httpnetworking
  * subproject that uses libcurl.
  */
class JOYNRCLUSTERCONTROLLER_EXPORT HttpCommunicationManager : public ICommunicationManager {

public:
    explicit HttpCommunicationManager(const MessagingSettings& settings);
    virtual ~HttpCommunicationManager();

    /**
     * Sets the IMessageReceiver that handles the incoming Message. This has to be called before startReceiveQueue.
     * This function is not thread safe.
     */
    void setMessageDispatcher(IMessageReceiver* messageDispatcher);

    /**
      * Gets the channel ID of the receive channel for incoming messages.
      */
    virtual const QString& getReceiveChannelId() const;

    /**
      * Checks the MessageSettings and updates the configuration.
      * Can be called at any time to read settings.
      */
    virtual void updateSettings();

    /**
      * Deletes the channel on the bounceproxy. Will only try once
      */
    virtual bool tryToDeleteChannel();

    /**
      * Blocks until the ReceiveQue is started.
      */
    virtual void waitForReceiveQueueStarted();

    virtual void sendMessage(const QString& channelId, const qint64& ttl_ms, const JoynrMessage& message);
    virtual void startReceiveQueue();


    /**
      * stops the receiveQue. This might ungracefully terminate the thread of the LongPollingMessageReceiver.
      */
    virtual void stopReceiveQueue();

    virtual void init(QSharedPointer<ILocalChannelUrlDirectory> channelUrlDirectory);

    IMessageSender* getMessageSender();

private:
    DISALLOW_COPY_AND_ASSIGN(HttpCommunicationManager);
    void init();
    void initMessageSender();

    /* This semaphore keeps track of the status of the channel. On creation no resources are available.
       Once the channel is created, one resource will be released. WaitForReceiveQueueStarted will try to
       acquire a resource from this semaphore, and block until it gets one.
       On Channel deletion, the semaphore tries to acquire a resource again, so that the next cycle of
       createChannel and waitForReceiveQueueStarted works as well. */
    QSemaphore* channelCreatedSemaphore;
    QString channelId; //currently channelid is used to construct the channelUrl or channelLocation.
    // Receiver ID is used to uniquely identify a message receiver (X-Atmosphere-tracking-id).
    // Allows for registering multiple receivers for a single channel.
    QString receiverId;

    IMessageReceiver* messageDispatcher;
    IMessageSender* messageSender;
    MessagingSettings settings;
    LongPollingMessageReceiver* messageReceiver;
    QSharedPointer<ILocalChannelUrlDirectory> channelUrlDirectory;

    friend class ::DispatcherIntegrationTest;
    friend class ::CapabilitiesClientTest;

    static joynr_logging::Logger* logger;
};



} // namespace joynr
#endif // HTTPCOMMUNICATIONMANAGER_H_
