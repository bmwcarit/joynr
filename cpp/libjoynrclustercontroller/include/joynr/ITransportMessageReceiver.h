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
#ifndef ITRANSPORTMESSAGERECEIVER_H
#define ITRANSPORTMESSAGERECEIVER_H

#include <functional>
#include <string>

#include <smrf/ByteVector.h>

namespace joynr
{
namespace system
{
namespace RoutingTypes
{
class Address;
} // namespace RoutingTypes
} // namespace system

/**
 * @class ITransportMessageReceiver
 * @brief Interface for message receiver
 *
 * This interface describes basic messaging functionality available in
 * the Joynr framework. Especially, it offers a method to notify about
 * received messages.
 */
class ITransportMessageReceiver
{

public:
    virtual ~ITransportMessageReceiver() = default;

    /**
     * Returns the serialized (json) receive address
     */
    virtual const std::string getSerializedGlobalClusterControllerAddress() const = 0;

    /**
     * Returns the receive address
     */
    virtual const system::RoutingTypes::Address& getGlobalClusterControllerAddress() const = 0;

    /**
     * Starts processing incoming messages. This method must be called
     * after creation in order to receive incoming messages.
     */
    virtual void startReceiveQueue() = 0;

    /**
     * Check if the receiver is connected
     */
    virtual bool isConnected() = 0;

    /**
     * Stops receiving messages over the specified channel.
     * The channel remains on the server.
     */
    virtual void stopReceiveQueue() = 0;

    /**
     * Update the settings of the Message Receiver
     */
    virtual void updateSettings() = 0;

    virtual void registerReceiveCallback(
            std::function<void(smrf::ByteVector&&)> onMessageReceived) = 0;
};

} // namespace joynr
#endif // ITRANSPORTMESSAGERECEIVER_H
