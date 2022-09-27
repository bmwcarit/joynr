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
#ifndef WEBSOCKETCCMESSAGINGSKELETONNONTLS_H
#define WEBSOCKETCCMESSAGINGSKELETONNONTLS_H

#include "libjoynrclustercontroller/websocket/WebSocketCcMessagingSkeleton.h"

#include <websocketpp/config/asio_no_tls.hpp>

namespace joynr
{

class WebSocketCcMessagingSkeletonNonTLS
        : public WebSocketCcMessagingSkeleton<websocketpp::config::asio>
{
public:
    WebSocketCcMessagingSkeletonNonTLS(
            boost::asio::io_service& ioService,
            std::shared_ptr<IMessageRouter> messageRouter,
            std::shared_ptr<WebSocketMessagingStubFactory> messagingStubFactory,
            const system::RoutingTypes::WebSocketAddress& serverAddress)
            : WebSocketCcMessagingSkeleton<websocketpp::config::asio>(ioService,
                                                                      messageRouter,
                                                                      messagingStubFactory,
                                                                      serverAddress.getPort())
    {
    }

    void init() override
    {
        WebSocketCcMessagingSkeleton<websocketpp::config::asio>::init();
        startAccept();
    }

protected:
    bool validateIncomingMessage(const ConnectionHandle& hdl,
                                 std::shared_ptr<ImmutableMessage> message) final
    {
        std::ignore = hdl;
        std::ignore = message;
        return true;
    }

    bool preprocessIncomingMessage(std::shared_ptr<ImmutableMessage> message) final
    {
        std::ignore = message;
        return true;
    }
};

} // namespace joynr
#endif // WEBSOCKETCCMESSAGINGSKELETON_H
