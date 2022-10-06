/*
 * #%L
 * %%
 * Copyright (C) 2016 - 2017 BMW Car IT GmbH
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
#ifndef WEBSOCKETPPRECEIVER_H
#define WEBSOCKETPPRECEIVER_H

#include <functional>

#include <websocketpp/error.hpp>

#include <smrf/ByteVector.h>

#include "joynr/Logger.h"

namespace joynr
{

template <typename Endpoint>
class WebSocketPpReceiver
{
protected:
    using ConnectionHandle = websocketpp::connection_hdl;
    using MessagePtr = typename Endpoint::message_ptr;

public:
    WebSocketPpReceiver() : onMessageReceivedCallback()
    {
    }

    ~WebSocketPpReceiver() = default;

    void registerReceiveCallback(
            std::function<void(ConnectionHandle&&, smrf::ByteVector&&)> callback)
    {
        onMessageReceivedCallback = std::move(callback);
    }

    void onMessageReceived(ConnectionHandle hdl, MessagePtr message)
    {
        using websocketpp::frame::opcode::value;
        const value mode = message->get_opcode();
        if (mode == value::binary) {
            JOYNR_LOG_TRACE(
                    logger(), "incoming binary message of size {}", message->get_payload().size());
            if (onMessageReceivedCallback) {
                // TODO can this copy be avoided?
                const std::string& messageStr = message->get_payload();
                smrf::ByteVector rawMessage(messageStr.begin(), messageStr.end());
                onMessageReceivedCallback(std::move(hdl), std::move(rawMessage));
            }
        } else {
            JOYNR_LOG_ERROR(
                    logger(), "received unsupported message type {}, dropping message", mode);
        }
    }

private:
    std::function<void(ConnectionHandle&&, smrf::ByteVector&&)> onMessageReceivedCallback;

    ADD_LOGGER(WebSocketPpReceiver)
};

} // namespace joynr

#endif // WEBSOCKETPPRECEIVER_H
