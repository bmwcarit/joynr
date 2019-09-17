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
#ifndef HTTPRECEIVER_H
#define HTTPRECEIVER_H

#include <memory>
#include <string>

#include "joynr/ITransportMessageReceiver.h"
#include "joynr/JoynrClusterControllerExport.h"
#include "joynr/Logger.h"
#include "joynr/MessagingSettings.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/Semaphore.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"

namespace joynr
{

class LongPollingMessageReceiver;

/**
  * @class HttpReceiver
  * @brief Implements HTTP communication to the bounce proxy (backend)
  *
  * Implements the ITransportMessageReceiver interface using the httpnetworking
  * subproject that uses libcurl.
  */
class JOYNRCLUSTERCONTROLLER_EXPORT HttpReceiver : public ITransportMessageReceiver
{

public:
    explicit HttpReceiver(const MessagingSettings& settings,
                          const std::string& channelId,
                          const std::string& receiverId);
    ~HttpReceiver() override;

    /**
      * Returns the serialized (json) receiver address
      */
    const std::string getSerializedGlobalClusterControllerAddress() const override;

    /**
      * Returns the receiver address
      */
    const system::RoutingTypes::ChannelAddress& getGlobalClusterControllerAddress() const override;

    /**
      * Checks the MessageSettings and updates the configuration.
      * Can be called at any time to read settings.
      */
    void updateSettings() override;

    /**
      * Deletes the channel on the broker. Will only try once
      */
    bool tryToDeleteChannel() override;

    bool isConnected() override;

    void startReceiveQueue() override;

    /**
      * stops the receiveQue. This might ungracefully terminate the thread of the
     * LongPollingMessageReceiver.
      */
    void stopReceiveQueue() override;

    void registerReceiveCallback(
            std::function<void(smrf::ByteVector&&)> onMessageReceived) override;

private:
    DISALLOW_COPY_AND_ASSIGN(HttpReceiver);

    /* This semaphore keeps track of the status of the channel. On creation no resources are
       available.
       Once the channel is created, one resource will be released.
       On Channel deletion, the semaphore tries to acquire a resource again, so that the next cycle
       of createChannel works as well. */
    std::shared_ptr<Semaphore> _channelCreatedSemaphore;
    std::string _channelId; // currently channelid is used to construct the channelUrl or
                            // channelLocation.
    // Receiver ID is used to uniquely identify a message receiver (X-Atmosphere-tracking-id).
    // Allows for registering multiple receivers for a single channel.
    std::string _receiverId;

    system::RoutingTypes::ChannelAddress _globalClusterControllerAddress;

    MessagingSettings _settings;
    std::unique_ptr<LongPollingMessageReceiver> _messageReceiver;

    /*! On text message received callback */
    std::function<void(smrf::ByteVector&&)> _onMessageReceived;

    ADD_LOGGER(HttpReceiver)
};

} // namespace joynr
#endif // HTTPRECEIVER_H
