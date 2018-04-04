/*jslint node: true */

/*global Buffer: true, FileReader: true, TextDecoder: true, TextEncoder: true */
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
var JoynrMessage = require("../../../main/js/joynr/messaging/JoynrMessage");
var JSONSerializer = require("../../../main/js/joynr/util/JSONSerializer");
var JoynrRuntimeException = require("../../../main/js/joynr/exceptions/JoynrRuntimeException");
var LoggingManager = require("../../../main/js/joynr/system/LoggingManager");
if (typeof Buffer !== "function" && typeof TextDecoder !== "function") {
    throw new JoynrRuntimeException(
        "Encoding/Decoding of binary websocket messages not possible. Buffer and TextEncoder/TextDecoder not available."
    );
}
var log = LoggingManager.getLogger("joynr.messaging.websocket.WebSocketMock");

var websocket = {
    mock: true,
    send: function() {}
};
var WebSocket = function WebSocket(newUrl) {
    websocket.url = newUrl;
    websocket.readyState = WebSocket.CONNECTING;
    return websocket;
};

WebSocket.CONNECTING = 0;
WebSocket.OPEN = 1;
WebSocket.CLOSING = 2;
WebSocket.CLOSED = 3;

websocket.encodeString = function(string) {
    if (typeof Buffer !== "function" && typeof TextDecoder === "function") {
        var textEncoder = new TextEncoder();
        return textEncoder.encode(string);
    }
    return string;
};
websocket.decodeEventData = function(event, callback) {
    if (typeof Buffer !== "function" && typeof TextDecoder === "function") {
        var textDecoder = new TextDecoder();
        callback(textDecoder.decode(event.data));
    } else {
        callback(event.data);
    }
};

websocket.marshalJoynrMessage = function(joynrMessage) {
    return this.encodeString(JSONSerializer.stringify(joynrMessage));
};

websocket.unmarshalJoynrMessage = function(event, callback) {
    if (typeof event.data === "object") {
        if (typeof Buffer === "function") {
            callback(JoynrMessage.parseMessage(JSON.parse(event.data.toString())));
        } else {
            var callbackWrapper = function(joynrMessageData) {
                if (joynrMessageData !== null && joynrMessageData !== undefined) {
                    callback(JoynrMessage.parseMessage(JSON.parse(joynrMessageData)));
                }
            };
            this.decodeEventData(event, callbackWrapper);
        }
    } else {
        log.error("Received unsupported message from websocket.");
    }
};

websocket.close = function() {};

module.exports = WebSocket;
