/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
#ifndef HTTPRECEIVER_H
#define HTTPRECEIVER_H

#include <memory>
#include <string>

#include "joynr/PrivateCopyAssign.h"
#include "joynr/ILocalChannelUrlDirectory.h"
#include "joynr/IMessageReceiver.h"
#include "joynr/JoynrClusterControllerExport.h"
#include "joynr/Logger.h"
#include "joynr/MessagingSettings.h"
#include "joynr/Semaphore.h"

class DispatcherIntegrationTest;
class CapabilitiesClientTest;

namespace joynr
{

class LongPollingMessageReceiver;

/**
  * @class HttpReceiver
  * @brief Implements HTTP communication to the bounce proxy (backend)
  *
  * Implements the IMessageReceiver interface using the httpnetworking
  * subproject that uses libcurl.
  */
class JOYNRCLUSTERCONTROLLER_EXPORT HttpReceiver : public IMessageReceiver
{

public:
    explicit HttpReceiver(const MessagingSettings& settings);
    ~HttpReceiver() override;

    /**
      * Gets the channel ID of the receive channel for incoming messages.
      */
    const std::string& getReceiveChannelId() const override;

    /**
      * Checks the MessageSettings and updates the configuration.
      * Can be called at any time to read settings.
      */
    void updateSettings() override;

    /**
      * Deletes the channel on the broker. Will only try once
      */
    bool tryToDeleteChannel() override;

    /**
      * Blocks until the ReceiveQue is started.
      */
    void waitForReceiveQueueStarted() override;

    void startReceiveQueue() override;

    /**
      * stops the receiveQue. This might ungracefully terminate the thread of the
     * LongPollingMessageReceiver.
      */
    void stopReceiveQueue() override;

    void init(std::shared_ptr<ILocalChannelUrlDirectory> channelUrlDirectory) override;

    void registerReceiveCallback(
            std::function<void(const std::string&)> onTextMessageReceived) override;

private:
    DISALLOW_COPY_AND_ASSIGN(HttpReceiver);
    void init();

    const BrokerUrl getBrokerUrl();

    /* This semaphore keeps track of the status of the channel. On creation no resources are
       available.
       Once the channel is created, one resource will be released. WaitForReceiveQueueStarted will
       try to
       acquire a resource from this semaphore, and block until it gets one.
       On Channel deletion, the semaphore tries to acquire a resource again, so that the next cycle
       of
       createChannel and waitForReceiveQueueStarted works as well. */
    Semaphore* channelCreatedSemaphore;
    std::string channelId; // currently channelid is used to construct the channelUrl or
                           // channelLocation.
    // Receiver ID is used to uniquely identify a message receiver (X-Atmosphere-tracking-id).
    // Allows for registering multiple receivers for a single channel.
    std::string receiverId;

    MessagingSettings settings;
    LongPollingMessageReceiver* messageReceiver;
    std::shared_ptr<ILocalChannelUrlDirectory> channelUrlDirectory;

    /*! On text message received callback */
    std::function<void(const std::string&)> onTextMessageReceived;

    friend class ::DispatcherIntegrationTest;
    friend class ::CapabilitiesClientTest;

    ADD_LOGGER(HttpReceiver);
};

} // namespace joynr
#endif // HTTPRECEIVER_H
