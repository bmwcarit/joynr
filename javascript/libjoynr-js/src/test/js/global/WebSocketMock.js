/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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
const JoynrRuntimeException = require("../../../main/js/joynr/exceptions/JoynrRuntimeException");
if (typeof Buffer !== "function" && typeof TextDecoder !== "function") {
    throw new JoynrRuntimeException(
        "Encoding/Decoding of binary websocket messages not possible. Buffer and TextEncoder/TextDecoder not available."
    );
}

const websocket = {
    mock: true,
    send() {}
};
const WebSocket = function WebSocket(newUrl) {
    if (WebSocket.constructorReturnsNull) {
        return null;
    }
    websocket.url = newUrl;
    websocket.readyState = WebSocket.CONNECTING;
    return websocket;
};

WebSocket.CONNECTING = 0;
WebSocket.OPEN = 1;
WebSocket.CLOSING = 2;
WebSocket.CLOSED = 3;

websocket.close = function() {};
WebSocket.constructorReturnsNull = false;

module.exports = WebSocket;
