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
#ifndef WEBSOCKETPPSENDER_H
#define WEBSOCKETPPSENDER_H

#include <functional>

#include <websocketpp/error.hpp>

#include "joynr/IWebSocketSendInterface.h"
#include "joynr/Logger.h"

namespace joynr
{

template <typename Endpoint>
class WebSocketPpSender : public IWebSocketSendInterface
{
protected:
    using ConnectionHandle = websocketpp::connection_hdl;

public:
    WebSocketPpSender(Endpoint& endpoint) : endpoint(endpoint), connectionHandle()
    {
    }

    ~WebSocketPpSender() final = default;

    void send(
            const std::string& msg,
            const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure) override
    {
        JOYNR_LOG_TRACE(logger, "outgoing binary message of size {}", msg.size());
        websocketpp::lib::error_code websocketError;
        endpoint.send(connectionHandle, msg, websocketpp::frame::opcode::binary, websocketError);
        if (websocketError) {
            onFailure(exceptions::JoynrDelayMessageException(
                    "Error sending binary message via WebSocketPpBase: " +
                    websocketError.message() + ", message: " + msg));
        }
    }

    /**
     * @brief Returns whether the socket is initialized or not
     * @return Initialization flag
     */
    bool isInitialized() const override
    {
        return isConnected();
    }

    /**
     * @brief Returns whether the socket is connected or not
     * @return Connection flag
     */
    bool isConnected() const override
    {
        try {
            if (typename Endpoint::connection_ptr connection =
                        endpoint.get_con_from_hdl(connectionHandle)) {
                return connection->get_state() == websocketpp::session::state::open;
            }
        } catch (websocketpp::exception e) {
            JOYNR_LOG_ERROR(
                    logger, "websocket not connected (websocketpp error message: %1)", e.what());
        }

        return false;
    }

    void setConnectionHandle(ConnectionHandle connection)
    {
        this->connectionHandle = connection;
    }

    void resetConnectionHandle()
    {
        this->connectionHandle.reset();
    }

private:
    Endpoint& endpoint;
    ConnectionHandle connectionHandle;
    ADD_LOGGER(WebSocketPpSender);
};

template <typename Endpoint>
INIT_LOGGER(SINGLE_MACRO_ARG(WebSocketPpSender<Endpoint>));

} // namespace joynr

#endif // WEBSOCKETPPSENDER_H
