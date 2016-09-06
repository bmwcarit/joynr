/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
#ifndef QWEBSOCKETSENDWRAPPER_H_
#define QWEBSOCKETSENDWRAPPER_H_

#include <functional>
#include <string>

#include <QtCore/QObject>
#include <QString>

#include "joynr/PrivateCopyAssign.h"
#include "joynr/IWebSocketSendInterface.h"

class QWebSocket;

namespace joynr
{

/**
 * @class QWebSocketSendWrapper
 * @brief A wrapper to send messages over a QWebSocket to be used in libJoynr
 */
class QWebSocketSendWrapper : public QObject, public IWebSocketSendInterface
{
    Q_OBJECT
public:
    /**
     * @brief Constructor
     * @param websocket Hosted web socket
     */
    explicit QWebSocketSendWrapper(QWebSocket* websocket);

    /**
     * @brief Destructor
     */
    ~QWebSocketSendWrapper() override;

    void send(const std::string& message,
              const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure)
            override;

    void registerDisconnectCallback(std::function<void()> onWebSocketDisconnected) override;

    void registerReceiveCallback(
            std::function<void(const std::string&)> onTextMessageReceived) override;

    bool isInitialized() const override;

    bool isConnected() const override;

Q_SIGNALS:
    void queueTextMessage(QString message);

private Q_SLOTS:
    void onSocketDisconnected();
    void sendTextMessage(const QString& message);

private:
    DISALLOW_COPY_AND_ASSIGN(QWebSocketSendWrapper);
    /*! Websocket to send data */
    QWebSocket* websocket;
    /*! Callback if websocket got closed */
    std::function<void()> onConnectionClosed;
};

} // namespace joynr

#endif // QWEBSOCKETSENDWRAPPER_H_
