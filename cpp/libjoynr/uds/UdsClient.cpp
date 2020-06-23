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

UdsClient::UdsClient(const UdsSettings& settings) noexcept
        : IUdsSender(),
          _connectedCallback{[]() {}},
          _disconnectedCallback{[]() {}},
          _receivedCallback{[](smrf::ByteVector&&) {}},
          _id{settings.getClientId()},
          _connectSleepTime{settings.getConnectSleepTimeMs()},
          _sendQueue(
                  std::make_unique<UdsSendQueue<UdsFrameBufferV1>>(settings.getSendingQueueSize())),
          _readBuffer(std::make_unique<UdsFrameBufferV1>()),
          _endpoint(settings.getSocketPath()),
          _ioContext(_threadsPerConnection),
          _socket(_ioContext),
          _state{State::STOP}
{
    _sendQueue->pushBack(UdsFrameBufferV1(
            joynr::system::RoutingTypes::UdsClientAddress(settings.getClientId())));
}

UdsClient::~UdsClient()
{
    _state.store(State::STOP);
    _ioContext.stop();
    if (_worker.valid()) {
        _worker.get();
    } else {
        JOYNR_LOG_WARN(
                logger(), "UDS client {} ({}) stopped by before starting.", _id, _endpoint.path());
    }
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
        JOYNR_LOG_ERROR(logger(), "UDS client {} ({}) alreay started.", _id, _endpoint.path());
        return;
    }
    _state.store(State::START);
    _worker = std::async(&UdsClient::run, this);
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
                                _id,
                                _endpoint.path(),
                                _connectSleepTime.count());
            } else {
                if (State::START == _state.exchange(State::CONNECTED)) {
                    _connectedCallback();
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
        // Currently the framework does not provide any means to report a fatal problem to the user,
        // hence it is only logged.
        JOYNR_LOG_FATAL(
                logger(), "UDS client {} ({}) stopped unexpectedly.", _id, _endpoint.path());
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
                            _id,
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
                                        _id,
                                        _endpoint.path(),
                                        readFailure.message());
                    } else {
                        try {
                            _receivedCallback(_readBuffer->readMessage());
                            doReadHeader();
                        } catch (const std::exception& e) {
                            JOYNR_LOG_FATAL(logger(),
                                            "UDS client {} failed to process message-frame: {}",
                                            _id,
                                            e.what());
                        }
                    }
                });
    } catch (const std::exception& e) {
        JOYNR_LOG_FATAL(logger(), "UDS client {} failed to read-message frame: {}", _id, e.what());
    }
}

void UdsClient::send(const smrf::ByteArrayView& msg, const IUdsSender::SendFailed& callback)
{
    _ioContext.post([ this, frame = UdsFrameBufferV1(msg), callback ]() mutable {
        if (_sendQueue->pushBack(std::move(frame), callback)) {
            doWrite();
        }
    });
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

} // namespace joynr
