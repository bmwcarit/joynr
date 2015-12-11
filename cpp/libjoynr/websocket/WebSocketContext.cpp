/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
#include "IWebSocketContextCallback.h"
#include "WebSocketContext.h"
#include "joynr/exceptions/JoynrException.h"

#include <sstream>
#include <vector>

extern "C" {
#include <private-libwebsockets.h>
}

#include "joynr/joynrlogging.h"

// FIXME: libwebsocket includes syslog.h defining LOG_* symbols
#undef LOG_INFO
#undef LOG_DEBUG
#undef JOYNRLOGGINGMACROS_H_
#include "joynr/joynrloggingmacros.h"

namespace joynr
{

joynr_logging::Logger* WebSocketContext::logger =
        joynr_logging::Logging::getInstance()->getLogger("MSG", "WebSocketContext");

WebSocketContext::WebSocketContext(IWebSocketContextCallback* webSocketServer,
                                   const std::string& protocolName,
                                   const std::string& interface,
                                   int32_t port,
                                   const std::string& certPath,
                                   const std::string& keyPath)
        : serverPort(port),
          interface(interface),
          certPath(certPath),
          keyPath(keyPath),
          keepRunning(true),
          websocketContext(nullptr),
          handleCounter(0),
          websocketHandles(),
          serverCallback(webSocketServer),
          thread(nullptr),
          protocolName(protocolName),
          initializeCondition(),
          intitializeMutex()
{
    // loglevel to be set by bitwise OR: LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO | LLL_DEBUG
    lws_set_log_level(LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO | LLL_DEBUG,
                      WebSocketContext::libWebSocketLog);
    websocketHandles.clear();
}

WebSocketContext::~WebSocketContext()
{
    stop();
    assert(websocketContext == nullptr);
    assert(websocketHandles.empty());
    assert(!thread->joinable());
}

void WebSocketContext::start()
{
    assert(thread == nullptr);
    std::unique_lock<std::mutex> lock(intitializeMutex);
    thread = new std::thread(&joynr::WebSocketContext::libWebSocketContextLoop, this);
    initializeCondition.wait(lock);
}

void WebSocketContext::stop()
{
    std::unique_lock<std::mutex> lock(intitializeMutex);
    LOG_TRACE(logger, "Signaling to stop context thread");
    keepRunning = false;
    if (thread->joinable()) {
        // Throw assertion if stop is called by the running thread
        // Calling stop from the thread itself will block here
        assert(std::this_thread::get_id() != thread->get_id());
        thread->join();
    }
}

void WebSocketContext::onOutgoingDataAvailable(WebSocketConnectionHandle handle)
{
    std::unique_lock<std::mutex> lock(intitializeMutex);
    if (websocketContext == nullptr) {
        LOG_WARN(logger, "Not yet initialized");
        return;
    }
    auto it = websocketHandles.find(handle);
    if (it == websocketHandles.end()) {
        LOG_WARN(logger, "Not yet connected");
        return;
    }

    libwebsocket_callback_on_writable(websocketContext, it->second);
}

int WebSocketContext::serviceCallbackHandle(libwebsocket_context* context,
                                            libwebsocket* wsi,
                                            libwebsocket_callback_reasons reason,
                                            void* data,
                                            size_t dataLength)
{
    IWebSocketContextCallback* callback = nullptr;
    if (wsi != nullptr) {
        const struct libwebsocket_protocols* proto = libwebsockets_get_protocol(wsi);
        if (proto != nullptr) {
            callback = static_cast<IWebSocketContextCallback*>(proto->user);
        }
    }

    switch (reason) {
    // The following events are directly forwarded to the user
    case LWS_CALLBACK_SERVER_WRITEABLE:
    case LWS_CALLBACK_CLIENT_WRITEABLE: {
        LOG_TRACE(logger, "WebSocket got writable");
        assert(wsi != nullptr);
        assert(context != nullptr);
        assert(callback != nullptr);

        WebSocketConnectionHandle handle = findHandle(wsi);
        assert(handle > 0);

        callback->onWebSocketWriteable(handle, [this, wsi](const std::string& message) {
            return this->write(wsi, message);
        });
        break;
    }
    case LWS_CALLBACK_ESTABLISHED:
    case LWS_CALLBACK_CLIENT_ESTABLISHED: {
        LOG_DEBUG(logger, "WebSocket connection established");
        assert(callback != nullptr);
        // WebSocketConnectionHandle handle = findHandle(wsi);
        callback->onConnectionEstablished(/*handle*/);
        break;
    }
    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR: {
        LOG_WARN(logger, "WebSocket connection error");
        assert(callback != nullptr);
        callback->onErrorOccured(IWebSocketContextCallback::WebSocketError_ConnectionError);
        break;
    }
    case LWS_CALLBACK_PROTOCOL_DESTROY: {
        // WSI is not available anymore. So there is no callback available
        LOG_TRACE(logger, "Protocol destroyed");
        break;
    }
    case LWS_CALLBACK_WSI_DESTROY: {
        LOG_TRACE(logger, "Connection handle and/or protocol deleted");
        // Using the global callback because WSI is not available anymore
        serverCallback->onConnectionClosed();
        break;
    }
    case LWS_CALLBACK_CLOSED:
    case LWS_CALLBACK_CLOSED_HTTP: {
        LOG_DEBUG(logger, "WebSocket connection closed");
        assert(callback != nullptr);
        callback->onConnectionClosed(/*handle*/);
        break;
    }
    case LWS_CALLBACK_RECEIVE:
    case LWS_CALLBACK_RECEIVE_PONG:
    case LWS_CALLBACK_CLIENT_RECEIVE:
    case LWS_CALLBACK_CLIENT_RECEIVE_PONG: {
        assert(callback != nullptr);
        char name[255];
        char host[255];
        libwebsockets_get_peer_addresses(context,
                                         wsi,
                                         libwebsocket_get_socket_fd(wsi),
                                         name,
                                         sizeof(name),
                                         host,
                                         sizeof(host));
        LOG_DEBUG(logger,
                  FormatString("Message received by: %1 / %2 / %3")
                          .arg(name)
                          .arg(host)
                          //.arg(wsi)
                          .str());

        callback->onMessageReceived(std::string(name),
                                    std::string(host),
                                    std::string(static_cast<char*>(data), dataLength));
        break;
    }
    case LWS_CALLBACK_PROTOCOL_INIT: {
        assert(context != nullptr);
        LOG_TRACE(
                logger,
                FormatString("Protocol initialized: \"%1\"").arg(context->protocols[0].name).str());
    }
    case LWS_CALLBACK_WSI_CREATE:
        LOG_TRACE(logger, "WebSocket created.");
        break;
    case LWS_CALLBACK_GET_THREAD_ID:
        break;
    case LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED: {
        LOG_DEBUG(logger, "New client connection instantiated");
        assert(wsi != nullptr);
        char name[30];
        char host[30];
        libwebsockets_get_peer_addresses(context,
                                         wsi,
                                         libwebsocket_get_socket_fd(wsi),
                                         name,
                                         sizeof(name),
                                         host,
                                         sizeof(host));
        LOG_DEBUG(logger,
                  FormatString("New client: %1 / %2 / %3")
                          .arg(name)
                          .arg(host)
                          //.arg(wsi)
                          .str());

        WebSocketConnectionHandle handle = ++handleCounter;
        websocketHandles.emplace(std::make_pair(handle, wsi));
        callback->onNewConnection(handle, std::string(host), std::string(name));
        break;
    }
    case LWS_CALLBACK_ADD_POLL_FD:
    case LWS_CALLBACK_DEL_POLL_FD:
    case LWS_CALLBACK_CHANGE_MODE_POLL_FD:
    case LWS_CALLBACK_LOCK_POLL:
    case LWS_CALLBACK_UNLOCK_POLL:
        // We dont use polling. This is only for multiple servers / contexts
        break;
    case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER:
        // Possibility to modify the header before sending (e.g. coockie data)
        LOG_TRACE(logger, "Handshake header");
        break;
    case LWS_CALLBACK_CLIENT_FILTER_PRE_ESTABLISH:
        // this is the last chance for the client user code to examine the http
        // headers and decide to reject the connection. If the content in the
        // headers is interesting to the client (url, etc) it needs to copy it
        // out at this point since it will be destroyed before the
        // CLIENT_ESTABLISHED call
        LOG_TRACE(logger, "Pre establish filter called");
        break;
    case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
        // Possiblitity to reject incoming connections on protocol level. Return > 0 to drop
        // connection
        // e.g. not "joynr"
        break;
    case LWS_CALLBACK_FILTER_NETWORK_CONNECTION:
        // Possiblitity to reject incoming connections on network level. Return > 0 to drop
        // connection
        // e.g. not "localhost"
        break;
    case LWS_CALLBACK_HTTP:
    case LWS_CALLBACK_HTTP_BODY:
    case LWS_CALLBACK_HTTP_BODY_COMPLETION:
    case LWS_CALLBACK_HTTP_FILE_COMPLETION:
    case LWS_CALLBACK_HTTP_WRITEABLE:
    case LWS_CALLBACK_FILTER_HTTP_CONNECTION:
        // No support for HTTP
        LOG_TRACE(logger, FormatString("HTTP callback: %1").arg(reason).str());
        //        return -1;
        break;
    case LWS_CALLBACK_OPENSSL_LOAD_EXTRA_CLIENT_VERIFY_CERTS:
    case LWS_CALLBACK_OPENSSL_LOAD_EXTRA_SERVER_VERIFY_CERTS:
    case LWS_CALLBACK_OPENSSL_PERFORM_CLIENT_CERT_VERIFICATION:
    case LWS_CALLBACK_OPENSSL_CONTEXT_REQUIRES_PRIVATE_KEY:
    // Process OpenSSL!
    // break;
    case LWS_CALLBACK_CONFIRM_EXTENSION_OKAY:
    case LWS_CALLBACK_CLIENT_CONFIRM_EXTENSION_SUPPORTED:
    case LWS_CALLBACK_USER:
    default:
        LOG_TRACE(logger, FormatString("Unhandled callback reason received: %1").arg(reason).str());
        break;
    }
    return 0;
}

int WebSocketContext::write(struct libwebsocket* handle, const std::string& message)
{
    std::unique_lock<std::mutex> lock(intitializeMutex);
    if (websocketContext == nullptr || handle == nullptr) {
        LOG_WARN(logger, "Not yet connected");
        return -1;
    }

    if (message.empty()) {
        return -1;
    }

    int len = message.size();

    std::vector<char> buffer(sizeof(char) *
                             (LWS_SEND_BUFFER_PRE_PADDING + len + LWS_SEND_BUFFER_POST_PADDING));
    std::vector<char>::iterator startPosition = buffer.begin() + LWS_SEND_BUFFER_PRE_PADDING;
    buffer.insert(startPosition, message.begin(), message.end());
    int n = libwebsocket_write(handle,
                               (unsigned char*)(buffer.data() + LWS_SEND_BUFFER_PRE_PADDING),
                               len,
                               LWS_WRITE_TEXT);
    LOG_TRACE(logger, FormatString("Wrote %1 characters to websocket").arg(n).str());
    return n;
}

WebSocketContext::WebSocketConnectionHandle WebSocketContext::connectToServer(
        const system::RoutingTypes::WebSocketAddress& address,
        WebSocketSsl sslUsage)
{
    assert(websocketContext != nullptr);
    assert(address.getPort() > 0);

    // attempt connection
    LOG_TRACE(logger, FormatString("Attempting to connect to %1").arg(address.toString()).str());

    std::stringstream uriStream;
    uriStream << address.getHost() << ":" << address.getPort();

    const char* path = ((address.getPath().empty()) ? "/" : address.getPath()).c_str();
    const char* proto = (protocolName.empty()) ? nullptr : protocolName.c_str();

    struct libwebsocket* websocketHandle = libwebsocket_client_connect(
            websocketContext,
            address.getHost().c_str(), // the location of the server
            address.getPort(),         // server port
            sslUsage,                  // ssl
            path,                      // resource path
            uriStream.str().c_str(),   // the host (may be also the location) with the port
            nullptr,                   // origin, the host of the client - optional
            proto,                     // protocol name
            -1                         // latest supported protocol
            );

    if (websocketHandle == nullptr) {
        LOG_ERROR(logger,
                  FormatString("Connection over websocket with server %1 not possible")
                          .arg(address.toString())
                          .str());
        return -1;
    }

    const WebSocketConnectionHandle handle = ++handleCounter;
    websocketHandles.emplace(std::make_pair(handle, websocketHandle));

    // service the context only if connection attempt returned non-null wsi
    LOG_TRACE(logger, FormatString("Successfully connected to %1").arg(address.toString()).str());

    return handle;
}

int WebSocketContext::libWebSocketCallback(libwebsocket_context* context,
                                           libwebsocket* wsi,
                                           libwebsocket_callback_reasons reason,
                                           void*,
                                           void* in,
                                           size_t len)
{
    WebSocketContext* tmpContext =
            static_cast<WebSocketContext*>(libwebsocket_context_user(context));
    assert(tmpContext != nullptr);
    return tmpContext->serviceCallbackHandle(context, wsi, reason, in, len);
}

void WebSocketContext::libWebSocketLog(int level, const char* message)
{
    switch (level) {
    case LLL_ERR:
        LOG_ERROR(logger, message);
        break;
    case LLL_WARN:
        LOG_WARN(logger, message);
        break;
    case LLL_NOTICE:
    case LLL_INFO:
        LOG_INFO(logger, message);
        break;
    case LLL_DEBUG:
        LOG_DEBUG(logger, message);
        break;
    case LLL_PARSER:
    case LLL_HEADER:
    case LLL_EXT:
    case LLL_CLIENT:
    case LLL_LATENCY:
    default:
        LOG_TRACE(logger, message);
        break;
    }
}

void WebSocketContext::libWebSocketContextLoop()
{

    struct libwebsocket_protocols protocols[2];
    struct lws_context_creation_info info;

    if (!initContext(protocols, info)) {
        LOG_ERROR(logger, "Creating context failed");
        return;
    }

    initializeCondition.notify_all();

    int timeout = 0;
    int i = 0;
    while (keepRunning && timeout >= 0) {
        timeout = libwebsocket_service(websocketContext, 100);
        ++i;
    }

    if (timeout >= 0) {
        LOG_TRACE(logger, "End context loop due to normal shutdown");
    } else {
        LOG_WARN(logger, "Context loop exits. Maybe a connection error.");
    }

    deinitContext();
}

bool WebSocketContext::initContext(struct libwebsocket_protocols* protocols,
                                   struct lws_context_creation_info& info)
{
    // Check that connection was not already established
    assert(websocketContext == nullptr);
    assert(websocketHandles.empty());

    memset(protocols, 0x00U, 2 * sizeof(struct libwebsocket_protocols));
    protocols[0].name = protocolName.c_str();
    protocols[0].callback = WebSocketContext::libWebSocketCallback;
    protocols[0].per_session_data_size = 0; // sizeof(struct session_data);
                                            //    protocols[0].rx_buffer_size = 0;
                                            //    protocols[0].id = 1;
    protocols[0].user = serverCallback;

    protocols[1].name = nullptr;
    protocols[1].callback = nullptr;
    protocols[1].per_session_data_size = 0;

    memset(&info, 0x00U, sizeof(struct lws_context_creation_info));
    info.port = (serverPort <= 0) ? CONTEXT_PORT_NO_LISTEN : serverPort;
    info.iface = (interface.empty()) ? nullptr : interface.c_str();
    info.protocols = protocols;
#ifdef LWS_WITH_SSL
    info.ssl_cert_filepath = certPath.c_str();
    info.ssl_private_key_filepath = keyPath.c_str();
#endif
    //#ifdef LWS_WITHOUT_EXTENSIONS
    //    info.extensions = libwebsocket_get_internal_extensions();
    //#else
    //#endif
    info.extensions = nullptr;
    info.gid = -1;
    info.uid = -1;
    info.options = 0;
    info.user = this;

    websocketContext = libwebsocket_create_context(&info);
    LOG_TRACE(logger, "Context created");

    return websocketContext != nullptr;
}

void WebSocketContext::deinitContext()
{
    auto connection = websocketHandles.begin();
    while (connection != websocketHandles.end()) {
        libwebsocket_close_and_free_session(
                websocketContext, connection->second, LWS_CLOSE_STATUS_NORMAL);
        connection = websocketHandles.erase(connection);
    }

    if (websocketContext != nullptr) {
        libwebsocket_cancel_service(websocketContext);
        libwebsocket_context_destroy(websocketContext);
        websocketContext = nullptr;
    }

    LOG_DEBUG(logger, "Destroyed client context");
}

WebSocketContext::WebSocketConnectionHandle WebSocketContext::findHandle(
        const struct libwebsocket* handle) const
{
    auto it = websocketHandles.begin();
    while (it != websocketHandles.end()) {
        if (it->second == handle) {
            return it->first;
            break;
        }
    }
    return -1;
}

} // namespace joynr
