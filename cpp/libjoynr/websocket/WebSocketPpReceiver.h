/*
 * #%L
 * %%
 * Copyright (C) 2016 - 2016 BMW Car IT GmbH
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
    WebSocketPpReceiver() : onTextMessageReceivedCallback(), onBinaryMessageReceived()
    {
    }

    ~WebSocketPpReceiver() = default;

    void registerReceiveCallback(std::function<void(const std::string&)> callback)
    {
        onTextMessageReceivedCallback = std::move(callback);
    }

    void onMessageReceived(ConnectionHandle hdl, MessagePtr message)
    {
        std::ignore = hdl;
        using websocketpp::frame::opcode::value;
        const value mode = message->get_opcode();
        if (mode == value::text) {
            JOYNR_LOG_TRACE(logger, "incoming text message \"{}\"", message->get_payload());
            if (onTextMessageReceivedCallback) {
                onTextMessageReceivedCallback(message->get_payload());
            }
        } else if (mode == value::binary) {
            JOYNR_LOG_TRACE(
                    logger, "incoming binary message of size {}", message->get_payload().size());
            if (onBinaryMessageReceived) {
                onBinaryMessageReceived(message->get_payload());
            }
        } else {
            JOYNR_LOG_ERROR(logger, "received unsupported message type {}, dropping message", mode);
        }
    }

private:
    std::function<void(const std::string&)> onTextMessageReceivedCallback;
    // reserved for future use
    std::function<void(const std::string&)> onBinaryMessageReceived;

    ADD_LOGGER(WebSocketPpReceiver);
};

template <typename Endpoint>
INIT_LOGGER(SINGLE_MACRO_ARG(WebSocketPpReceiver<Endpoint>));

} // namespace joynr

#endif // WEBSOCKETPPRECEIVER_H
