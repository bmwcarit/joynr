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
#ifndef WEBSOCKETMESSAGINSTUB_H
#define WEBSOCKETMESSAGINSTUB_H

#include <functional>
#include <memory>

#include "joynr/IMessagingStub.h"
#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"

namespace joynr
{

class IWebSocketSendInterface;

namespace exceptions
{
class JoynrRuntimeException;
} // namespace exceptions
  /**
   * @class WebSocketMessagingStub
   * @brief Represents an outgoing WebSocket connection
   */
class WebSocketMessagingStub : public IMessagingStub
{
public:
    /**
     * @brief Constructor
     * @param webSocket Interface to be used to send data
     * @param onStubClosed Function to be called on close
     */
    WebSocketMessagingStub(std::shared_ptr<IWebSocketSendInterface> webSocket);

    /**
     * @brief Destructor
     */
    ~WebSocketMessagingStub() final = default;
    void transmit(
            std::shared_ptr<ImmutableMessage> message,
            const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure) final;

private:
    DISALLOW_COPY_AND_ASSIGN(WebSocketMessagingStub);

    /*! Message sender for outgoing messages over WebSocket */
    std::shared_ptr<IWebSocketSendInterface> _webSocket;

    ADD_LOGGER(WebSocketMessagingStub)
};

} // namespace joynr
#endif // WEBSOCKETMESSAGINSTUB_H
