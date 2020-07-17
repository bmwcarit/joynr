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

#include "joynr/UdsServer.h"
#include "joynr/Logger.h"

#include "UdsFrameBufferV1.h"
#include "UdsSendQueue.h"

namespace joynr
{

constexpr char UdsServer::_initMessageStart[];
constexpr int UdsServer::_threadsPerServer;

UdsServer::UdsServer(const UdsSettings& settings) noexcept
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

void UdsServer::setConnectCallback(const Connected& callback) noexcept
{
    _remoteConfig._connectedCallback = callback;
}

void UdsServer::setDisconnectCallback(const Disconnected& callback) noexcept
{
    _remoteConfig._disconnectedCallback = callback;
}

void UdsServer::setReceiveCallback(const Received& callback) noexcept
{
    _remoteConfig._receivedCallback = callback;
}

void UdsServer::start()
{
    if (_started.exchange(true)) {
        JOYNR_LOG_ERROR(logger(), "UDS server alreay started.");
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
            JOYNR_LOG_WARN(logger(),
                           "UDS server path {} already exists. Replacing existing path.",
                           _endpoint.path());
        }
        isRetry = true;
        try {
            _acceptor.open(_endpoint.protocol());
            _acceptor.bind(_endpoint);
            _acceptor.listen();
            JOYNR_LOG_INFO(logger(), "Waiting for connections on path {}.", _endpoint.path());
            doAcceptClient();
            _ioContext->run();
        } catch (const boost::system::system_error& error) {
            JOYNR_LOG_ERROR(logger(),
                            "UDS server ({}) encountered an error and will restart: {}",
                            _endpoint.path(),
                            error.what());
        }
        boost::system::error_code ignore;
        _acceptor.close(ignore);
    }
}

void UdsServer::doAcceptClient()
{
    _acceptor.async_accept(_newClientSocket, [this](boost::system::error_code acceptFailure) {
        if (acceptFailure) {
            JOYNR_LOG_ERROR(logger(),
                            "UDS server failed to accept new client: {}",
                            acceptFailure.message());
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
}

UdsServer::Connection::~Connection()
{
    // Ignore any errors caused by the following socket closure.
    const auto closedDueToError = _closedDueToError.load();

    // Assure that all enqueued actions are removed from the pool which might us the send queue
    boost::system::error_code ignore;
    _socket.close(ignore);
    if (closedDueToError) {
        _sendQueue->emptyQueueAndNotify("Connection lost to " + _address.getId());
    }
}

void UdsServer::Connection::doReadInitHeader()
{
    boost::asio::async_read(_socket,
                            _readBuffer->header(),
                            [this](boost::system::error_code readFailure, std::size_t /*length*/) {
        if (doCheck(readFailure)) {
            doReadInitBody();
        }
    });
}

void UdsServer::Connection::doReadInitBody()
{
    try {
        boost::asio::async_read(_socket,
                                _readBuffer->body(),
                                [this](boost::system::error_code readFailure,
                                       std::size_t /*length*/) {
            if (doCheck(readFailure)) {
                try {
                    _address = _readBuffer->readInit();
                    JOYNR_LOG_INFO(logger(),
                                   "Initialize connection for UDS client id: {}",
                                   _address.getId());
                    _connectedCallback(_address, this->shared_from_this());
                    doReadHeader();
                } catch (const std::exception& e) {
                    std::string id = _address.getId();
                    if (id.empty()) {
                        id = "[unknown ID]";
                    }
                    JOYNR_LOG_FATAL(
                            logger(), "Failed to initialize UDS connection '{}': {}", id, e.what());
                    doClose();
                }
            }
        });
    } catch (const std::exception& e) {
        JOYNR_LOG_FATAL(logger(), "Failed to read init-frame for new UDS connection: {}", e.what());
        doClose();
    }
}

void UdsServer::Connection::doReadHeader()
{
    boost::asio::async_read(_socket,
                            _readBuffer->header(),
                            [this](boost::system::error_code readFailure, std::size_t /*length*/) {
        if (doCheck(readFailure)) {
            doReadBody();
        }
    });
}

void UdsServer::Connection::doReadBody()
{
    try {
        boost::asio::async_read(
                _socket,
                _readBuffer->body(),
                [this](boost::system::error_code readFailure, std::size_t /*length*/) {
                    if (doCheck(readFailure)) {
                        try {
                            _receivedCallback(_address, _readBuffer->readMessage());
                        } catch (const std::exception& e) {
                            JOYNR_LOG_FATAL(logger(),
                                            "Failed to process UDS message from '{}': {}",
                                            _address.getId(),
                                            e.what());
                            doClose();
                        }
                        doReadHeader();
                    }
                });
    } catch (const std::exception& e) {
        JOYNR_LOG_FATAL(logger(),
                        "Failed to read UDS message-frame from '{}': {}",
                        _address.getId(),
                        e.what());
        doClose();
    }
}

void UdsServer::Connection::send(const smrf::ByteArrayView& msg,
                                 const IUdsSender::SendFailed& callback)
{
    try {
        _ioContext->post([ this, frame = UdsFrameBufferV1(msg), callback ]() mutable {
            if (_sendQueue->pushBack(std::move(frame), callback)) {
                doWrite();
            }
        });
    } catch (const std::exception& e) {
        JOYNR_LOG_FATAL(
                logger(), "Failed insert UDS message for '{}': {}", _address.getId(), e.what());
        doClose();
    }
}

void UdsServer::Connection::doWrite()
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

bool UdsServer::Connection::doCheck(const boost::system::error_code& error)
{
    if (error) {
        JOYNR_LOG_INFO(logger(), "UDS remote client connection disconnected: {}", error.message());
        doClose();
        return false;
    }
    return true;
}

void UdsServer::Connection::doClose()
{
    if (!_closedDueToError.exchange(true)) {
        if (!_address.getId().empty()) {
            _disconnectedCallback(_address);
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
