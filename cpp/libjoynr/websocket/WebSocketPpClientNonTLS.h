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
#ifndef WEBSOCKETPPCLIENTNONTLS_H
#define WEBSOCKETPPCLIENTNONTLS_H

#include "libjoynr/websocket/WebSocketPpClient.h"

namespace joynr
{

class WebSocketPpClientNonTLS : public WebSocketPpClient<websocketpp::config::asio_client>
{
public:
    using WebSocketPpClient<websocketpp::config::asio_client>::WebSocketPpClient;
};

} // namespace joynr

#endif // WEBSOCKETPPCLIENTNONTLS_H
