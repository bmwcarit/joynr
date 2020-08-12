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
#include "joynr/UdsClient.h"

#include <string>

#include "joynr/Util.h"

#include "UdsFrameBufferV1.h"
#include "UdsSendQueue.h"

namespace joynr
{

constexpr int UdsClient::_threadsPerConnection;

UdsClient::UdsClient(const UdsSettings& settings,
                     const std::function<void(const exceptions::JoynrRuntimeException&)>&
                             onFatalRuntimeError) noexcept
        : IUdsSender(),
          _fatalRuntimeErrorCallback{onFatalRuntimeError},
          _connectedCallback{[]() {}},
          _disconnectedCallback{[]() {}},
          _receivedCallback{[](smrf::ByteVector&&) {}},
          _address{settings.createClientMessagingAddress()},
          _connectSleepTime{settings.getConnectSleepTimeMs()},
          _sendQueue(
                  std::make_unique<UdsSendQueue<UdsFrameBufferV1>>(settings.getSendingQueueSize())),
          _readBuffer(std::make_unique<UdsFrameBufferV1>()),
          _endpoint(settings.getSocketPath()),
          _ioContext(_threadsPerConnection),
          _socket(_ioContext),
          _state{State::STOP}
{
    try {
        _sendQueue->pushBack(UdsFrameBufferV1(_address));
    } catch (const std::exception& e) {
        doHandleFatalError("Failed to insert INIT message to queue.", e);
    }
}

UdsClient::~UdsClient()
{
    shutdown();
}

joynr::system::RoutingTypes::UdsClientAddress UdsClient::getAddress() const noexcept
{
    return _address;
}

void UdsClient::setConnectCallback(const Connected& callback) noexcept
{
    _connectedCallback = callback;
}

void UdsClient::setDisconnectCallback(const Disconnected& callback) noexcept
{
    _disconnectedCallback = callback;
}

void UdsClient::setReceiveCallback(const Received& callback) noexcept
{
    _receivedCallback = callback;
}

void UdsClient::start()
{
    if (_worker.valid()) {
        JOYNR_LOG_ERROR(
                logger(), "UDS client {} ({}) alreay started.", _address.getId(), _endpoint.path());
        return;
    }
    _state.store(State::START);
    _worker = std::async(&UdsClient::run, this);
}

void UdsClient::shutdown() noexcept
{
    std::lock_guard<std::mutex> lockStop(_asyncShutdownMutex);
    _state.store(State::STOP);
    try {
        _ioContext.stop();
        if (_worker.valid()) {
            _worker.get();
        }
    } catch (std::exception& e) {
        JOYNR_LOG_ERROR(logger(),
                        "UDS client {} caused unexpected error when stopping worker: {}",
                        _address.getId(),
                        e.what());
    } catch (...) {
        JOYNR_LOG_ERROR(logger(),
                        "UDS client {} caused unknown error when stopping worker",
                        _address.getId());
    }
}

void UdsClient::run()
{
    bool isRetry = false;
    while (State::START == _state.load()) {
        if (isRetry) {
            // Wait before retry connection so that worker does not block everything
            std::this_thread::sleep_for(_connectSleepTime);
            _ioContext.reset();
        }
        isRetry = true;
        _socket.async_connect(_endpoint, [this](boost::system::error_code failedToConnect) {
            if (failedToConnect) {
                JOYNR_LOG_ERROR(logger(),
                                "UDS client {} ({}) failed to connect. Retry in {:d}ms.",
                                _address.getId(),
                                _endpoint.path(),
                                _connectSleepTime.count());
            } else {
                if (State::START == _state.exchange(State::CONNECTED)) {
                    try {
                        _connectedCallback();
                    } catch (const std::exception& e) {
                        doHandleFatalError("Error from connect callback", e);
                    }
                    _ioContext.post([this]() {
                        // Send messages currently in the queue, which contains at least the initial
                        // message
                        doWrite();
                    });
                    doReadHeader();
                }
            }
        });
        _ioContext.run();
        _socket.close();
    }
    if (State::CONNECTED == _state.load()) {
        doHandleFatalError("State machine stopped unexpectedly.");
    }
    _disconnectedCallback();
}

void UdsClient::doReadHeader()
{
    boost::asio::async_read(_socket,
                            _readBuffer->header(),
                            [this](boost::system::error_code readFailure, std::size_t /*length*/) {
        if (readFailure) {
            JOYNR_LOG_ERROR(logger(),
                            "UDS client {} ({}) failed to read header: {}",
                            _address.getId(),
                            _endpoint.path(),
                            readFailure.message());
        } else {
            doReadBody();
        }
    });
}

void UdsClient::doReadBody()
{
    try {
        boost::asio::async_read(
                _socket,
                _readBuffer->body(),
                [this](boost::system::error_code readFailure, std::size_t /*length*/) {
                    if (readFailure) {
                        JOYNR_LOG_ERROR(logger(),
                                        "UDS client {} ({}) failed to read body: {}",
                                        _address.getId(),
                                        _endpoint.path(),
                                        readFailure.message());
                    } else {
                        try {
                            _receivedCallback(_readBuffer->readMessage());
                            doReadHeader();
                        } catch (const std::exception& e) {
                            doHandleFatalError("Failed to process message-frame", e);
                        }
                    }
                });
    } catch (const std::exception& e) {
        doHandleFatalError("Failed to read message-frame", e);
    }
}

void UdsClient::send(const smrf::ByteArrayView& msg, const IUdsSender::SendFailed& callback)
{
    try {
        _ioContext.post([ this, frame = UdsFrameBufferV1(msg), callback ]() mutable {
            if (_sendQueue->pushBack(std::move(frame), callback)) {
                doWrite();
            }
        });
    } catch (const std::exception& e) {
        doHandleFatalError("Failed to queue message-frame", e);
    }
}

void UdsClient::doWrite()
{
    boost::asio::async_write(_socket,
                             _sendQueue->showFront(),
                             [this](boost::system::error_code writeFailed, std::size_t /*length*/) {
        if (_sendQueue->popFrontOnSuccess(writeFailed)) {
            doWrite();
        }
    });
}

void UdsClient::doHandleFatalError(const std::string& errorMessage,
                                   const std::exception& error) noexcept
{
    doHandleFatalError(errorMessage + " - " + error.what());
}

void UdsClient::doHandleFatalError(const std::string& errorMessage) noexcept
{
    JOYNR_LOG_FATAL(logger(),
                    "UDS client {} ({}) fatal runtime error, stopping all communication via UDS "
                    "permanently: {}",
                    _address.getId(),
                    _endpoint.path(),
                    errorMessage);
    if (State::FAILED != _state.exchange(State::FAILED)) {
        try {
            if (_fatalRuntimeErrorCallback) {
                _fatalRuntimeErrorCallback(exceptions::JoynrRuntimeException(errorMessage));
            }
        } catch (std::exception& e) {
            JOYNR_LOG_ERROR(logger(),
                            "UDS client {} received unexpected exception when informing user about "
                            "internal fatal runtime error: {}",
                            _address.getId(),
                            e.what());
        } catch (...) {
            JOYNR_LOG_ERROR(logger(),
                            "UDS client {} received an unkown error when informing user about "
                            "internal fatal runtime error.",
                            _address.getId());
        }
        boost::system::error_code ignore; // Maybe the socket is not open.
        _socket.close(ignore);
    }
}

} // namespace joynr
