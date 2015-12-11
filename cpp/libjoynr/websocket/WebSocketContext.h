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
#ifndef WEBSOCKETCONTEXT_H
#define WEBSOCKETCONTEXT_H

#include "joynr/PrivateCopyAssign.h"

#include "joynr/system/RoutingTypes/WebSocketAddress.h"

#include <libwebsockets.h>
#include <string>
#include <atomic>
#include <thread>
#include <unordered_map>
#include <condition_variable>

struct libwebsocket;
struct libwebsocket_context;

namespace joynr
{
namespace joynr_logging
{
class Logger;
}

class IWebSocketContextCallback;

/**
 * @brief The WebSocketContext class encapsulates boilerplate code needed for libwebsockets
 * initialization. It is a Runnable and run method is one segment that differs for client and server
 * (that is why run method has been made pure virtual). This class and its children classes have
 * been
 * used internally and only implementations of WebSocket may know about it.
 */
class WebSocketContext
{
public:
    /*! Handle representing a WebSocket connection */
    typedef int32_t WebSocketConnectionHandle;

    /**
     * @brief SSL configuration
     */
    enum WebSocketSsl {
        WebSocketSsl_NoSsl = 0,     //< Use no SSL
        WebSocketSsl_Ssl = 1,       //< Use SSL
        WebSocketSsl_SelfSigned = 2 //< Allow self signed certs
    };

    /**
     * @brief Constructor
     * @param eventCallback Callback for events
     * @param protocolName Name of the protocol to be used
     * @param interface Network interface to listen on
     * @param port Server listening port. Use @c -1 to disable listening
     * @param certPath Path to public key
     * @param keyPath Path to private key
     */
    WebSocketContext(IWebSocketContextCallback* eventCallback,
                     const std::string& protocolName,
                     const std::string& interface = "",
                     const int32_t port = -1,
                     const std::string& certPath = "",
                     const std::string& keyPath = "");

    /**
     * @brief Destructor
     */
    virtual ~WebSocketContext();

    /**
     * @brief Start the context
     * Will create a thread running the WebSocket and handling its events
     * @note This method will block till the context is ready
     */
    void start();

    /**
     * @brief Stops the context
     * @note This method will block until the containing thread got joined
     */
    void stop();

    /**
     * @brief Connects to a server
     * @param address Address of the server
     * @param sslUsage SSL behavior
     * @return The handle of the connection
     */
    WebSocketConnectionHandle connectToServer(const system::RoutingTypes::WebSocketAddress& address,
                                              WebSocketSsl sslUsage = WebSocketSsl_NoSsl);

    // void disconnectFromServer(const system::RoutingTypes::WebSocketAddress& address);

    /**
     * @brief Notify the WebSocket that outgoing data is available
     * @param handle Handle of the connection with outgoing data
     */
    void onOutgoingDataAvailable(WebSocketConnectionHandle handle);

private:
    DISALLOW_COPY_AND_ASSIGN(WebSocketContext);

    static int libWebSocketCallback(libwebsocket_context* context,
                                    libwebsocket* wsi,
                                    libwebsocket_callback_reasons reason,
                                    void* user,
                                    void* in,
                                    size_t len);

    static void libWebSocketLog(int level, const char* message);

    int write(struct libwebsocket* handle, const std::string& message);

    int serviceCallbackHandle(libwebsocket_context* context,
                              libwebsocket* wsi,
                              libwebsocket_callback_reasons reason,
                              void* data,
                              size_t dataLength);

    void libWebSocketContextLoop();

    bool initContext(struct libwebsocket_protocols* protocols,
                     struct lws_context_creation_info& info);

    void deinitContext();

    WebSocketConnectionHandle findHandle(const struct libwebsocket* handle) const;

    const int32_t serverPort;
    const std::string interface;
    const std::string certPath;
    const std::string keyPath;

    volatile bool keepRunning;

    struct libwebsocket_context* websocketContext;

    WebSocketConnectionHandle handleCounter;

    std::unordered_map<WebSocketConnectionHandle, struct libwebsocket*> websocketHandles;

    IWebSocketContextCallback* serverCallback;

    std::thread* thread;

    const std::string protocolName;

    std::condition_variable initializeCondition;
    std::mutex intitializeMutex;

    static joynr_logging::Logger* logger;
};

} // namespace joynr
#endif // WEBSOCKETCONTEXT_H
