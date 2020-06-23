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
#ifndef UDSSERVER_H
#define UDSSERVER_H

#include <atomic>
#include <future>
#include <memory>
#include <set>

#include <boost/asio.hpp>

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

class UdsServer
{
public:
    using Connected = std::function<
            void(const system::RoutingTypes::UdsClientAddress&, std::shared_ptr<IUdsSender>)>;
    using Disconnected = std::function<void(const system::RoutingTypes::UdsClientAddress&)>;
    using Received =
            std::function<void(const system::RoutingTypes::UdsClientAddress&, smrf::ByteVector&&)>;

    /** The initial serialized message must start with the following byte stream, otherwise the
     * connection is rejected. */
    static constexpr char _initMessageStart[] =
            "{\"_typeName\":\"joynr.system.RoutingTypes.UdsClientAddress\"";

    explicit UdsServer(const UdsSettings& settings) noexcept;
    virtual ~UdsServer() final;

    // Server cannot be copied since it has internal threads
    DISALLOW_COPY_AND_ASSIGN(UdsServer);

    UdsServer(UdsServer&&) = default;
    UdsServer& operator=(UdsServer&&) = default;

    /**
     * @brief Sets callback for sucuessful connection of a client. Connection is successful if
     * initial message has been received.
     * The provided sender must not exist longer than the UDS server.
     * @param callback Callback
     */
    void setConnectCallback(const Connected& callback) noexcept;

    /**
     * @brief Sets callback for disconnection of the client (not called if server is stopped before
     * client disconnection)
     * @param callback Callback
     */
    void setDisconnectCallback(const Disconnected& callback) noexcept;

    /**
     * @brief Sets callback for message reception
     * @param callback Callback
     */
    void setReceiveCallback(const Received& callback) noexcept;

    /** Opens an UNIX domain socket asynchronously and starts the IO thread pool. */
    void start();

private:
    using uds = boost::asio::local::stream_protocol;
    // Default config basically does nothing, everything is just eaten
    struct ConnectionConfig
    {
        std::size_t _maxSendQueueSize = 0;
        Connected _connectedCallback =
                [](const system::RoutingTypes::UdsClientAddress&, std::shared_ptr<IUdsSender>) {};
        Disconnected _disconnectedCallback = [](const system::RoutingTypes::UdsClientAddress&) {};
        Received _receivedCallback =
                [](const system::RoutingTypes::UdsClientAddress&, smrf::ByteVector&&) {};
    };

    // Connection to remote client, lifetime is decoupled from server
    class Connection : public std::enable_shared_from_this<Connection>, public IUdsSender
    {
    public:
        using Active = std::set<std::shared_ptr<Connection>>;
        Connection(uds::socket&& socket,
                   std::shared_ptr<boost::asio::io_service> ioContext,
                   std::weak_ptr<Active> registry,
                   const ConnectionConfig& config) noexcept;
        /** Notifies sender if the connection got lost (error occured) */
        virtual ~Connection();

        // remote clients are shared
        DISALLOW_COPY_AND_ASSIGN(Connection);
        DISALLOW_MOVE_AND_ASSIGN(Connection);

        void send(const smrf::ByteArrayView& msg, const IUdsSender::SendFailed& callback) override;
        // All do-actions are executed by the UdsServer thread pool. If the pool is stopped, the
        // actions are dropped
        void doReadInitHeader();

    private:
        // I/O context functions
        void doReadInitBody();
        void doReadHeader();
        void doReadBody();
        void doWrite();
        bool doCheck(const boost::system::error_code& ec);
        void doClose();

        // Socket is always available, hence sending does not require any locks
        uds::socket _socket;
        // Socket is dependent on the _ioContext
        std::shared_ptr<boost::asio::io_service> _ioContext;
        // Registry is only available as long as the server exists
        std::weak_ptr<Active> _registry;

        Connected _connectedCallback;
        Disconnected _disconnectedCallback;
        Received _receivedCallback;

        std::atomic_bool _closedDueToError;

        system::RoutingTypes::UdsClientAddress _address; // Only accessed by read-strand

        // PIMPL to keep includes clean
        std::unique_ptr<UdsSendQueue<UdsFrameBufferV1>> _sendQueue;
        std::unique_ptr<UdsFrameBufferV1> _readBuffer;

        ADD_LOGGER(Connection)
    };

    void run();

    // I/O context functions
    void doAcceptClient();

    /** Currently one thread handles server socket and all client sockets. Therefore no strands are
     * implemented for remote-client read/write */
    static constexpr int _threadsPerServer = 1;
    std::shared_ptr<boost::asio::io_service> _ioContext;
    std::shared_ptr<Connection::Active> _registry;
    ConnectionConfig _remoteConfig;
    std::chrono::milliseconds _openSleepTime;
    uds::endpoint _endpoint;
    uds::acceptor _acceptor;
    uds::socket _newClientSocket;
    std::future<void> _worker;
    std::atomic_bool _started;
    ADD_LOGGER(UdsServer)
};

} // namespace joynr

#endif // UDSSERVER_H
