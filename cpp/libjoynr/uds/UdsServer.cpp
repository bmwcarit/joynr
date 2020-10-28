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
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>

#include "joynr/ImmutableMessage.h"
#include "joynr/UdsServer.h"
#include "joynr/Logger.h"

#include "UdsFrameBufferV1.h"
#include "UdsSendQueue.h"

namespace joynr
{

constexpr int UdsServer::_threadsPerServer;

UdsServer::UdsServer(const UdsSettings& settings)
        : _ioContext(std::make_shared<boost::asio::io_service>(_threadsPerServer)),
          _registry{std::make_shared<Connection::Active>()},
          _openSleepTime{settings.getConnectSleepTimeMs()},
          _endpoint(settings.getSocketPath()),
          _acceptor(*_ioContext),
          _newClientSocket(*_ioContext),
          _started{false}
{
    _remoteConfig._maxSendQueueSize = settings.getSendingQueueSize();
}

UdsServer::~UdsServer()
{
    _started.store(false);
    _ioContext->stop(); // Cancel all pending do-actions
    if (_worker.valid()) {
        _worker.get(); // Assure that no connection interferes with further cleanup of destructor
    }
    _registry->clear(); // Delete all clients not hold by a user
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
            mode_t oldMask = umask(S_IROTH | S_IWOTH); // do not allow access to others
            _acceptor.open(_endpoint.protocol());
            _acceptor.bind(_endpoint);
            umask(oldMask); // restore original umask
            _acceptor.listen();
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
        _acceptor.close(ignore);
    }
}

void UdsServer::doAcceptClient() noexcept
{
    _acceptor.async_accept(_newClientSocket, [this](boost::system::error_code acceptFailure) {
        if (acceptFailure) {
            JOYNR_LOG_ERROR(logger(), "Failed to accept new client: {}", acceptFailure.message());
        } else {
            auto newConnection =
                    std::make_shared<Connection>(std::move(_newClientSocket),
                                                 _ioContext,
                                                 std::weak_ptr<Connection::Active>(_registry),
                                                 _remoteConfig);
            _registry->insert(newConnection);
            JOYNR_LOG_INFO(
                    logger(),
                    "Connection request received from new client. {:d} clients are registered.",
                    _registry->size());
            newConnection->doReadInitHeader();
        }
        doAcceptClient();
    });
}

UdsServer::Connection::Connection(uds::socket&& socket,
                                  std::shared_ptr<boost::asio::io_service> ioContext,
                                  std::weak_ptr<Active> registry,
                                  const ConnectionConfig& config) noexcept
        : _socket(std::move(socket)),
          _ioContext(std::move(ioContext)),
          _registry(std::move(registry)),
          _connectedCallback{config._connectedCallback},
          _disconnectedCallback{config._disconnectedCallback},
          _receivedCallback{config._receivedCallback},
          _closedDueToError{false},
          _sendQueue(std::make_unique<UdsSendQueue<UdsFrameBufferV1>>(config._maxSendQueueSize)),
          _readBuffer(std::make_unique<UdsFrameBufferV1>())
{
    _username = getUserName();
    JOYNR_LOG_DEBUG(logger(), "Established new connection with username '{}'", _username);
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
                logger(), "Could not obtain peer credentials from socket, errno {}", storedErrno);
    }
    return username;
}

UdsServer::Connection::~Connection()
{
    // Ignore any errors caused by the following socket closure.
    const auto closedDueToError = _closedDueToError.load();

    // Assure that all enqueued actions are removed from the pool which might us the send queue
    boost::system::error_code ignore;
    _socket.close(ignore);
    if (closedDueToError) {
        try {
            _sendQueue->emptyQueueAndNotify("Connection lost to " + _address.getId());
        } catch (const std::exception& e) {
            JOYNR_LOG_ERROR(logger(), "Failed to process message notifications: {}", e.what());
        }
    }
}

void UdsServer::Connection::doReadInitHeader() noexcept
{
    boost::asio::async_read(_socket,
                            _readBuffer->header(),
                            [this](boost::system::error_code readFailure, std::size_t /*length*/) {
        if (doCheck(readFailure)) {
            doReadInitBody();
        }
    });
}

void UdsServer::Connection::doReadInitBody() noexcept
{
    try {
        boost::asio::async_read(
                _socket,
                _readBuffer->body(),
                [this](boost::system::error_code readFailure, std::size_t /*length*/) {
                    if (doCheck(readFailure)) {
                        try {
                            _address = _readBuffer->readInit();
                            JOYNR_LOG_INFO(logger(),
                                           "Initialize connection for client with ID: {}",
                                           _address.getId());
                            _connectedCallback(_address, this->shared_from_this());
                            doReadHeader();
                        } catch (const std::exception& e) {
                            doClose("Initialization processing failed", e);
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
                            [this](boost::system::error_code readFailure, std::size_t /*length*/) {
        if (doCheck(readFailure)) {
            doReadBody();
        }
    });
}

void UdsServer::Connection::doReadBody() noexcept
{
    try {
        boost::asio::async_read(
                _socket,
                _readBuffer->body(),
                [this](boost::system::error_code readFailure, std::size_t /*length*/) {
                    if (doCheck(readFailure)) {
                        try {
                            _receivedCallback(_address, _readBuffer->readMessage(), _username);
                        } catch (const std::exception& e) {
                            doClose("Failed to process message", e);
                        }
                        doReadHeader();
                    }
                });
    } catch (const std::exception& e) {
        doClose("Failed to read message", e);
    }
}

void UdsServer::Connection::send(const smrf::ByteArrayView& msg,
                                 const IUdsSender::SendFailed& callback)
{
    try {
        _ioContext->post([ this, frame = UdsFrameBufferV1(msg), callback ]() mutable {
            try {
                if (_sendQueue->pushBack(std::move(frame), callback)) {
                    doWrite();
                }
            } catch (const std::exception& e) {
                doClose("Failed to insert new message", e);
            }
        });
    } catch (const std::exception& e) {
        doClose("Failed to insert new message", e);
    }
}

void UdsServer::Connection::doWrite() noexcept
{
    boost::asio::async_write(_socket,
                             _sendQueue->showFront(),
                             [this](boost::system::error_code writeFailed, std::size_t /*length*/) {
        doCheck(writeFailed);
        if (_sendQueue->popFrontOnSuccess(writeFailed)) {
            doWrite();
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
    if (!_closedDueToError.exchange(true)) {
        const std::string clientId = _address.getId().empty() ? "[unknown ID]" : _address.getId();
        JOYNR_LOG_FATAL(logger(), "Connection to {} corrupted: {}", clientId, errorMessage);
        if (!_address.getId().empty()) {
            try {
                _disconnectedCallback(_address);
            } catch (const std::exception& e) {
                JOYNR_LOG_ERROR(logger(), "Failed to process disconnection: {}", e.what());
            }
        }
        boost::system::error_code ignore;
        _socket.close(ignore);
        _ioContext->post([ removeMe = shared_from_this(), registry = _registry ]() {
            auto registryLock = registry.lock();
            if (registryLock) {
                registryLock->erase(removeMe);
            }
        });
    }
}

} // namespace joynr
