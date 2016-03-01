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
#include "QWebSocketSendWrapper.h"

#include <QtWebSockets>

namespace joynr
{

QWebSocketSendWrapper::QWebSocketSendWrapper(QWebSocket* websocket)
        : QObject(websocket), IWebSocketSendInterface(), websocket(websocket), onConnectionClosed()
{
    connect(this,
            &QWebSocketSendWrapper::queueTextMessage,
            this,
            &QWebSocketSendWrapper::sendTextMessage,
            Qt::QueuedConnection);

    connect(websocket,
            &QWebSocket::disconnected,
            this,
            &QWebSocketSendWrapper::onSocketDisconnected,
            Qt::QueuedConnection);
}

QWebSocketSendWrapper::~QWebSocketSendWrapper()
{
    websocket->deleteLater();
}

void QWebSocketSendWrapper::send(const std::string& message)
{
    emit queueTextMessage(QString::fromStdString(message));
}

void QWebSocketSendWrapper::registerDisconnectCallback(
        std::function<void()> onWebSocketDisconnected)
{
    onConnectionClosed = onWebSocketDisconnected;
}

void QWebSocketSendWrapper::registerReceiveCallback(std::function<void(const std::string&)>)
{
    // Is already done by WebSocketCcMessagingSkeleton
}

bool QWebSocketSendWrapper::isInitialized() const
{
    return isConnected();
}

bool QWebSocketSendWrapper::isConnected() const
{
    return websocket->isValid();
}

void QWebSocketSendWrapper::sendTextMessage(const QString& message)
{
    websocket->sendTextMessage(message);
}

void QWebSocketSendWrapper::onSocketDisconnected()
{
    if (onConnectionClosed) {
        onConnectionClosed();
    }
}
}
