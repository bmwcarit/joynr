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

const JoynrMessage = require("../joynr/messaging/JoynrMessage");
const JSONSerializer = require("../joynr/util/JSONSerializer");
const JoynrRuntimeException = require("../joynr/exceptions/JoynrRuntimeException");
const LoggingManager = require("../joynr/system/LoggingManager");

if (typeof TextDecoder !== "function") {
    throw new JoynrRuntimeException(
        "Encoding/Decoding of binary websocket messages not possible. TextEncoder/TextDecoder not available."
    );
}
const log = LoggingManager.getLogger("joynr.messaging.websocket.WebSocket");
const fileReader = new FileReader();
fileReader.onError = function(error) {
    log.error("Decoding of binary message failed: " + error);
};
const textDecoder = new TextDecoder();
const textEncoder = new TextEncoder();

WebSocket.encodeString = function(string) {
    return textEncoder.encode(string);
};
WebSocket.decodeEventData = function(event, callback) {
    if (event.target.binaryType.toLocaleLowerCase() === "blob") {
        fileReader.onload = function(loadEvent) {
            // TODO error handling, unit test for WebSocket+WebSocketNode
            callback(loadEvent.target.result);
        };
        fileReader.readAsText(event.data);
    } else if (event.target.binaryType.toLocaleLowerCase() === "arraybuffer") {
        callback(textDecoder.decode(event.data));
    }
    log.error("Received invalid/unsupported message from websocket.");
};

WebSocket.marshalJoynrMessage = function(joynrMessage) {
    return WebSocket.encodeString(JSONSerializer.stringify(joynrMessage));
};

WebSocket.unmarshalJoynrMessage = function(event, callback) {
    if (typeof event.data === "object") {
        const callbackWrapper = function(joynrMessageData) {
            if (joynrMessageData !== null && joynrMessageData !== undefined) {
                callback(JoynrMessage.parseMessage(JSON.parse(joynrMessageData)));
            }
        };
        WebSocket.decodeEventData(event, callbackWrapper);
    } else {
        log.error("Received unsupported message from websocket.");
    }
};

module.exports = WebSocket;
