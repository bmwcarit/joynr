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
#ifndef WEBSOCKETPPCLIENTTLS_H
#define WEBSOCKETPPCLIENTTLS_H

#include "WebSocketPpClient.h"

#include "joynr/BoostIoserviceForwardDecl.h"

namespace joynr
{
class IKeychain;
class WebSocketSettings;

class WebSocketPpClientTLS : public WebSocketPpClient<websocketpp::config::asio_tls_client>
{
    using SSLContext = websocketpp::lib::asio::ssl::context;

public:
    WebSocketPpClientTLS(const WebSocketSettings& wsSettings,
                         boost::asio::io_service& ioService,
                         std::shared_ptr<IKeychain> keyChain);

private:
    std::shared_ptr<SSLContext> createSSLContext(std::shared_ptr<IKeychain> keyChain);

    bool useEncryptedTls;
};

} // namespace joynr

#endif // WEBSOCKETPPCLIENTTLS_H
