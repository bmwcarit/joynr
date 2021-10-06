/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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
#ifndef UDSCLIENT_H
#define UDSCLIENT_H

#include <atomic>
#include <chrono>
#include <functional>
#include <future>
#include <memory>

#include <boost/asio.hpp>

#include <smrf/ByteArrayView.h>
#include <smrf/ByteVector.h>

#include "joynr/BoostIoserviceForwardDecl.h"
#include "joynr/IUdsSender.h"
#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/UdsSettings.h"
#include "joynr/system/RoutingTypes/UdsClientAddress.h"

namespace joynr
{
namespace exceptions
{
class JoynrRuntimeException;
} // namespace exceptions

class UdsFrameBufferV1;

template <typename FRAME>
class UdsSendQueue;

class UdsClient : public IUdsSender
{
public:
    using Connected = std::function<void()>;
    using Disconnected = std::function<void()>;
    using Received = std::function<void(smrf::ByteVector&&)>;

    explicit UdsClient(const UdsSettings& settings,
                       const std::function<void(const exceptions::JoynrRuntimeException&)>&
                               onFatalRuntimeError);
    virtual ~UdsClient();

    // Client cannot be copied since it has internal threads
    DISALLOW_COPY_AND_ASSIGN(UdsClient);

    // io_context cannot be moved
    DISALLOW_MOVE_AND_ASSIGN(UdsClient);

    /** @return Address of this client */
    system::RoutingTypes::UdsClientAddress getAddress() const noexcept;

    /**
     * Called uppon sucessful connection
     * @param callback Callback
     */
    void setConnectCallback(const Connected& callback);

    /**
     * Called uppon disconnection (regardless whether stopped by user or a fatal error occured)
     * @param callback Callback
     */
    void setDisconnectCallback(const Disconnected& callback);

    /**
     * Called when a new message has been received
     * @param callback Callback
     */
    void setReceiveCallback(const Received& callback);

    /** Starts the UDS client asynchronously and triggers the connected callback as soon as the
     * connection has been established. */
    void start();

    /**
     * Thread safe method to stop all internal threads.
     * This method should only be used, if external garbage collection is used
     * in combination with external thread management. Since the client manages its
     * own thread pool, it provides external access in case client lifetime
     * is managed by garbage collector.
     */
    void shutdown() noexcept;

    void send(const smrf::ByteArrayView& msg, const IUdsSender::SendFailed& callback) override;

private:
    // Internal worker thread
    void run();
    void abortOnSocketConfigurationError() noexcept;

    // I/O context functions
    void doReadHeader() noexcept;
    void doReadBody() noexcept;
    void doWriteInit() noexcept;
    void doWrite() noexcept;
    void doHandleFatalError(const std::string& errorMessage, const std::exception& error) noexcept;
    void doHandleFatalError(const std::string& errorMessage) noexcept;

    bool hasFatalErrorAlreadyBeenReported() noexcept;

    static constexpr int _threadsPerConnection = 1;
    std::function<void(const exceptions::JoynrRuntimeException&)> _fatalRuntimeErrorCallback;
    Connected _connectedCallback;
    Disconnected _disconnectedCallback;
    Received _receivedCallback;
    system::RoutingTypes::UdsClientAddress _address;
    std::chrono::milliseconds _connectSleepTime;

    // PIMPL to keep includes clean
    std::unique_ptr<UdsSendQueue<UdsFrameBufferV1>> _sendQueue;
    std::unique_ptr<UdsFrameBufferV1> _readBuffer;

    boost::asio::local::stream_protocol::endpoint _endpoint;
    boost::asio::io_service _ioContext;
    boost::asio::local::stream_protocol::socket _socket;
    enum class State : unsigned char { START, CONNECTED, STOP, FAILED };
    std::atomic<State> _state;
    std::mutex _asyncShutdownMutex;
    std::future<void> _worker;

    ADD_LOGGER(UdsClient)
};

} // namespace joynr

#endif // UDSCLIENT_H
