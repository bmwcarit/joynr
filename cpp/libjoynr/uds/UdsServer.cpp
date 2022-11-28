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
#include <cstdio>

#include <boost/format.hpp>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "joynr/ImmutableMessage.h"
#include "joynr/Logger.h"
#include "joynr/UdsServer.h"

#include "UdsFrameBufferV1.h"
#include "UdsSendQueue.h"

namespace joynr
{

constexpr int UdsServer::_threadsPerServer;

UdsServer::UdsServer(const UdsSettings& settings)
        : _ioContext(std::make_shared<boost::asio::io_service>(_threadsPerServer)),
          _openSleepTime{settings.getConnectSleepTimeMs()},
          _endpoint(settings.getSocketPath()),
          _acceptor(*_ioContext),
          _started{false},
          _acceptorMutex(),
          _connectionMap(),
          _connectionIndex(0),
          _workGuard(std::make_shared<boost::asio::io_service::work>(*_ioContext))
{
    _remoteConfig._maxSendQueueSize = settings.getSendingQueueSize();
}

UdsServer::~UdsServer()
{
    // Skip clean-up if UdsServer::start has not been called.
    if (_started.exchange(false)) {
        boost::system::error_code ignore;
        std::unique_lock<std::mutex> acceptorLock(_acceptorMutex);
        _acceptor.cancel(ignore); // Acceptor will not create further connections

        // shutdown all still existing connections
        while (!_connectionMap.empty()) {
            auto it = _connectionMap.cbegin();
            std::weak_ptr<Connection> connection = it->second;
            _connectionMap.erase(it);
            if (auto connectionSharedPtr = connection.lock()) {
                _acceptorMutex.unlock();
                // shutdown must be invoked without lock to avoid
                // potential deadlock situation
                connectionSharedPtr->shutdown();
                connectionSharedPtr.reset();
                // wait until the object has been destructed
                while (connection.lock()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(25));
                }
                _acceptorMutex.lock();
            }
        }

        _workGuard.reset();
        acceptorLock.unlock();

        _ioContext->stop();
        // Wait for worker before destructing members
        try {
            _worker.get();
        } catch (const std::exception& e) {
            JOYNR_LOG_ERROR(logger(), "Unexpected exception when terminating worker.", e.what());
        }
    }
}

void UdsServer::setConnectCallback(const Connected& callback)
{
    if (callback) {
        _remoteConfig._connectedCallback = callback;
    }
}

void UdsServer::setDisconnectCallback(const Disconnected& callback)
{
    if (callback) {
        _remoteConfig._disconnectedCallback = callback;
    }
}

void UdsServer::setReceiveCallback(const Received& callback)
{
    if (callback) {
        _remoteConfig._receivedCallback = callback;
    }
}

void UdsServer::start()
{
    if (_started.exchange(true)) {
        JOYNR_LOG_ERROR(logger(), "Server already started.");
        return;
    }
    _worker = std::async(&UdsServer::run, this);
}

void UdsServer::run()
{
    bool isRetry = false;
    while (_started.load()) {
        if (isRetry) {
            // Wait before retry to open socket so that worker does not block everything
            std::this_thread::sleep_for(_openSleepTime);
            _ioContext->reset();
        }
        if (0 == remove(_endpoint.path().c_str())) {
            JOYNR_LOG_WARN(
                    logger(), "Path {} already exists. Replacing existing path.", _endpoint.path());
        }
        isRetry = true;
        try {
            // no access to others and no permission to execute
            mode_t oldMask = umask(S_IXUSR | S_IXGRP | S_IRWXO);
            std::unique_lock<std::mutex> acceptorLock(_acceptorMutex);
            _acceptor.open(_endpoint.protocol());
            _acceptor.bind(_endpoint);
            umask(oldMask); // restore original umask
            _acceptor.listen();
            acceptorLock.unlock();
            JOYNR_LOG_INFO(logger(), "Waiting for connections on path {}.", _endpoint.path());
            doAcceptClient();
            _ioContext->run();
        } catch (const boost::system::system_error& error) {
            JOYNR_LOG_ERROR(logger(),
                            "Encountered an error on path {} and will restart: {}",
                            _endpoint.path(),
                            error.what());
        }
        boost::system::error_code ignore;
        std::unique_lock<std::mutex> acceptorLock(_acceptorMutex);
        _acceptor.close(ignore);
    }
}

void UdsServer::doAcceptClient() noexcept
{
    std::unique_lock<std::mutex> acceptorLock(_acceptorMutex);
    if (!_started.load()) {
        JOYNR_LOG_INFO(logger(),
                       "Stop accepting new clients because UdsServer destructor has been invoked.");
        return;
    }

    // check if some of the existing entries can be removed
    // which is the case when the weak_ptr has expired since the
    // connection object got already destructed
    std::unordered_set<std::uint64_t> expiredEntries;
    for (const auto& connectionsPair : _connectionMap) {
        auto connectionIndex = connectionsPair.first;
        auto connection = connectionsPair.second;
        if (connection.expired()) {
            expiredEntries.insert(connectionIndex);
        }
    }
    for (const auto& connectionIndex : expiredEntries) {
        _connectionMap.erase(connectionIndex);
    }

    uint64_t index = _connectionIndex++;
    auto newConnection = std::make_shared<Connection>(_ioContext, _remoteConfig, index);
    _connectionMap[index] = newConnection;

    _acceptor.async_accept(
            newConnection->getSocket(),
            [this, newConnection, index](boost::system::error_code acceptFailure) {
                if (acceptFailure) {
                    JOYNR_LOG_ERROR(
                            logger(), "Failed to accept new client: {}", acceptFailure.message());
                } else {
                    JOYNR_LOG_INFO(logger(),
                                   "Connection index {} request received from new client.",
                                   index);
                    newConnection->doReadInitHeader();
                    doAcceptClient();
                }
            });
}

UdsServer::Connection::Connection(std::shared_ptr<boost::asio::io_service>& ioContext,
                                  const ConnectionConfig& config,
                                  std::uint64_t connectionIndex) noexcept
        : _ioContext{ioContext},
          _socket(*ioContext),
          _connectedCallback{config._connectedCallback},
          _disconnectedCallback{config._disconnectedCallback},
          _receivedCallback{config._receivedCallback},
          _isClosed{false},
          _username("connection not established"),
          _sendQueue(std::make_unique<UdsSendQueue<UdsFrameBufferV1>>(config._maxSendQueueSize)),
          _readBuffer(std::make_unique<UdsFrameBufferV1>()),
          _connectionIndex(connectionIndex)
{
}

std::string UdsServerUtil::getUserNameByUid(uid_t uid)
{
    std::string username;
    struct passwd passwd;
    struct passwd* result;
    char buf[1024];
    int rc;

    if (!(rc = getpwuid_r(uid, &passwd, buf, sizeof(buf), &result))) {
        if (result) {
            username = std::string(passwd.pw_name, strnlen(passwd.pw_name, 256UL));
        } else {
            JOYNR_LOG_INFO(logger(), "Could not find username for uid {}", uid);
        }
    } else {
        JOYNR_LOG_ERROR(logger(), "Could not find username for uid {}, errno {}", uid, rc);
    }
    if (username.empty()) {
        username = std::to_string(uid);
    }
    return username;
}

std::string UdsServer::Connection::getUserName()
{
    std::string username;
    struct ucred ucred;
    socklen_t len = sizeof(ucred);
    int sockfd = _socket.native_handle();
    if (!getsockopt(sockfd, SOL_SOCKET, SO_PEERCRED, &ucred, &len)) {
        username = UdsServerUtil::getUserNameByUid(ucred.uid);
    } else {
        username = std::string("anonymous");
        int storedErrno = errno;
        JOYNR_LOG_ERROR(
                logger(),
                "Connection index {} could not obtain peer credentials from socket, errno {}",
                _connectionIndex,
                storedErrno);
    }
    return username;
}

UdsServer::uds::socket& UdsServer::Connection::getSocket()
{
    return _socket;
}

void UdsServer::Connection::send(const smrf::ByteArrayView& msg,
                                 const IUdsSender::SendFailed& callback)
{
    if (_isClosed.load()) {
        throw std::runtime_error("Connection already closed.");
    }
    try {
        // UdsFrameBufferV1 first since it can cause exception
        _ioContext->post(
                [frame = UdsFrameBufferV1(msg), self = shared_from_this(), callback]() mutable {
                    try {
                        if (self->_sendQueue->pushBack(std::move(frame), callback)) {
                            self->doWrite();
                        }
                    } catch (const std::exception& e) {
                        self->doClose("Failed to insert new message", e);
                    }
                });
    } catch (const joynr::exceptions::JoynrRuntimeException& e) {
        // In case generation of frame buffer failed, close connection
        _ioContext->post([self = shared_from_this(), e]() mutable {
            self->doClose("Failed to construct message", e);
        });
        throw;
    }
}

void UdsServer::Connection::shutdown()
{
    if (_isClosed.load()) {
        return;
    }
    const std::string clientId = _address.getId().empty() ? "[unknown ID]" : _address.getId();
    JOYNR_LOG_INFO(logger(),
                   "Closing connection index {} to {} ({}).",
                   _connectionIndex,
                   clientId,
                   _username);
    if (_ioContext) {
        _ioContext->dispatch([self = shared_from_this()]() { self->doClose(); });
    }
    while (!_isClosed.load()) {
        std::this_thread::yield();
    }
}

void UdsServer::Connection::doReadInitHeader() noexcept
{
    boost::asio::async_read(_socket,
                            _readBuffer->header(),
                            [self = shared_from_this()](
                                    boost::system::error_code readFailure, std::size_t /*length*/) {
                                if (self->doCheck(readFailure)) {
                                    self->doReadInitBody();
                                }
                            });
}

void UdsServer::Connection::doReadInitBody() noexcept
{
    try {
        boost::asio::async_read(
                _socket,
                _readBuffer->body(),
                [self = shared_from_this()](
                        boost::system::error_code readFailure, std::size_t /*length*/) {
                    if (self->doCheck(readFailure)) {
                        try {
                            self->_username = self->getUserName();
                            self->_address = self->_readBuffer->readInit();
                            JOYNR_LOG_INFO(logger(),
                                           "Initialize connection index {} for client with User / "
                                           "ID: {} / {}",
                                           self->_connectionIndex,
                                           self->_username,
                                           self->_address.getId());
                            self->_connectedCallback(self->_address,
                                                     std::make_unique<UdsServer::UdsSender>(
                                                             std::weak_ptr<Connection>(self)));
                            self->doReadHeader();
                        } catch (const std::exception& e) {
                            self->doClose("Initialization processing failed", e);
                        }
                    }
                });
    } catch (const std::exception& e) {
        doClose("Failed to read init-frame", e);
    }
}

void UdsServer::Connection::doReadHeader() noexcept
{
    boost::asio::async_read(_socket,
                            _readBuffer->header(),
                            [self = shared_from_this()](
                                    boost::system::error_code readFailure, std::size_t /*length*/) {
                                if (self->doCheck(readFailure)) {
                                    self->doReadBody();
                                }
                            });
}

void UdsServer::Connection::doReadBody() noexcept
{
    try {
        boost::asio::async_read(
                _socket,
                _readBuffer->body(),
                [self = shared_from_this()](
                        boost::system::error_code readFailure, std::size_t /*length*/) {
                    if (self->doCheck(readFailure)) {
                        try {
                            self->_receivedCallback(self->_address,
                                                    self->_readBuffer->readMessage(),
                                                    self->_username);
                        } catch (const std::exception& e) {
                            self->doClose("Failed to process message", e);
                        }
                        self->doReadHeader();
                    }
                });
    } catch (const std::exception& e) {
        doClose("Failed to read message", e);
    }
}

void UdsServer::Connection::doWrite() noexcept
{
    boost::asio::async_write(_socket,
                             _sendQueue->showFront(),
                             [self = shared_from_this()](boost::system::error_code writeFailed,
                                                         std::size_t /*length*/) {
                                 if (self->doCheck(writeFailed)) {
                                     if (self->_sendQueue->popFrontOnSuccess(writeFailed)) {
                                         self->doWrite();
                                     }
                                 }
                             });
}

bool UdsServer::Connection::doCheck(const boost::system::error_code& error) noexcept
{
    if (error) {
        doClose(error.message());
        return false;
    }
    return true;
}

void UdsServer::Connection::doClose(const std::string& errorMessage,
                                    const std::exception& error) noexcept
{
    doClose(errorMessage + " - " + error.what());
}

void UdsServer::Connection::doClose(const std::string& errorMessage) noexcept
{
    if (!_isClosed.load()) {
        const std::string clientId = _address.getId().empty() ? "[unknown ID]" : _address.getId();
        JOYNR_LOG_FATAL(logger(),
                        "Connection index {} to {} corrupted: {}",
                        _connectionIndex,
                        clientId,
                        errorMessage);
    }
    doClose();
}

void UdsServer::Connection::doClose() noexcept
{
    if (!_isClosed.exchange(true)) {
        if (!_address.getId().empty()) {
            try {
                _disconnectedCallback(_address);
            } catch (const std::exception& e) {
                JOYNR_LOG_ERROR(logger(),
                                "Connection index {} failed to process disconnection: {}",
                                _connectionIndex,
                                e.what());
            }
        }
        boost::system::error_code ignore;
        _socket.shutdown(boost::asio::socket_base::shutdown_both, ignore);
        _socket.close();
        try {
            _sendQueue->emptyQueueAndNotify("Connection closed.");
        } catch (const std::exception& e) {
            JOYNR_LOG_ERROR(logger(),
                            "Connection index {} failed to process send-failure: {}",
                            _connectionIndex,
                            e.what());
        }
    }
}

UdsServer::UdsSender::UdsSender(std::weak_ptr<Connection> connection) : _connection{connection}
{
}

UdsServer::UdsSender::~UdsSender()
{
    auto connection = _connection.lock();
    if (connection) {
        try {
            connection->shutdown();
        } catch (std::exception& e) {
            JOYNR_LOG_ERROR(logger(), "Failed to close connection: {}", e.what());
        }
    }
}

void UdsServer::UdsSender::send(const smrf::ByteArrayView& msg,
                                const IUdsSender::SendFailed& callback)
{
    auto connection = _connection.lock();
    auto safeCallback =
            callback ? callback : [](const joynr::exceptions::JoynrRuntimeException&) {};
    try {
        if (connection) {
            connection->send(msg, safeCallback);
        } else {
            throw std::runtime_error("Connection already closed.");
        }
    } catch (const std::exception& e) {
        try {
            safeCallback(joynr::exceptions::JoynrRuntimeException(e.what()));
        } catch (const std::exception& ee) {
            JOYNR_LOG_ERROR(logger(), "Failed to process send-failed: {}", e.what());
        }
    }
}

} // namespace joynr
