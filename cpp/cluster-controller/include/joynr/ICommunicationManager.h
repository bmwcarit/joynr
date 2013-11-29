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
#ifndef ICOMMUNICATIONMANAGER_H
#define ICOMMUNICATIONMANAGER_H

#include <QString>
#include <QSharedPointer>

namespace joynr {

class JoynrMessage;
class IMessageReceiver;

/**
  * \class ICommunicationManager
  * \brief Interface for communication manager
  *
  * This interface describes basic messaging functionality available in
  * the Joynr framework. Especially, it offers a method to send a message to
  * a given channel and a signal to notify about received messages.
  */
class ICommunicationManager {

public:
    virtual ~ICommunicationManager() {}

    /**
      * Gets the channel ID of the receive channel for incoming messages.
      */
    virtual const QString& getReceiveChannelId() const = 0;

    /**
     * Sets the IMessageReceiver that handles the incoming Message. This has to be called before startReceiveQueue.
     * This function is not thread safe.
     * TODO: See if this can be moved to the constructor of all ICommunicationManagers.
     */
    virtual void setMessageDispatcher(IMessageReceiver* messageDispatcher) = 0;


    /**
      * Send the given message to the channel with the given channel ID.
      */
    virtual void sendMessage(const QString &channelId, const qint64& ttl_ms, const JoynrMessage &message) = 0;

    /**
      * Starts processing incomming messages. This method must be called
      * after creation in order to receive incoming messages.
      */
    virtual void startReceiveQueue() = 0;

    /**
      * Blocks until the ReceiveQueue is actually started.
      */
    virtual void waitForReceiveQueueStarted() = 0;

    /**
      * Stops receiving messages over the specified channel.
      * The channel remains on the server.
      */
    virtual void stopReceiveQueue() = 0;

    /**
      * Update the settings of the Communication Manager
      */
    virtual void updateSettings() = 0;

    /**
      * Will try to delete the channel from the server. Returns true if successfull, false if not.
      */
    virtual bool tryToDeleteChannel() = 0;

};

} // namespace joynr
#endif // IMESSAGING_H
