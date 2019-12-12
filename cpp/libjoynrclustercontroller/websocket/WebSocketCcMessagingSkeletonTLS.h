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
#ifndef WEBSOCKETCCMESSAGINGSKELETONTLS_H
#define WEBSOCKETCCMESSAGINGSKELETONTLS_H

#include <memory>
#include <string>

#include <websocketpp/config/asio.hpp>

#include "WebSocketCcMessagingSkeleton.h"

namespace joynr
{

class WebSocketCcMessagingSkeletonTLS
        : public WebSocketCcMessagingSkeleton<websocketpp::config::asio_tls>
{
    using SSLContext = websocketpp::lib::asio::ssl::context;

public:
    WebSocketCcMessagingSkeletonTLS(
            boost::asio::io_service& ioService,
            std::shared_ptr<IMessageRouter> messageRouter,
            std::shared_ptr<WebSocketMessagingStubFactory> messagingStubFactory,
            const system::RoutingTypes::WebSocketAddress& serverAddress,
            const std::string& caPemFile,
            const std::string& certPemFile,
            const std::string& privateKeyPemFile,
            bool /*useEncryptedTls*/);

    virtual void init() override;

private:
    bool validateIncomingMessage(const ConnectionHandle& hdl,
                                 std::shared_ptr<ImmutableMessage> message) final;
    bool preprocessIncomingMessage(std::shared_ptr<ImmutableMessage> message) final;

    std::shared_ptr<SSLContext> createSSLContext(const std::string& _caPemFile,
                                                 const std::string& _certPemFile,
                                                 const std::string& _privateKeyPemFile,
                                                 ConnectionHandle hdl);

    const std::string _caPemFile;
    const std::string _certPemFile;
    const std::string _privateKeyPemFile;
};

} // namespace joynr
#endif // WEBSOCKETCCMESSAGINGSKELETON_H
