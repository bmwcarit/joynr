/*global Buffer: true */

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

/**
 * Adapts the nodejs websocket lib WebSocket-Node to the WebSocket client API.
 * See: http://dev.w3.org/html5/websockets/#the-websocket-interface
 *
 */
define([
    "ws",
    "global/Smrf",
    "joynr/messaging/JoynrMessage",
    "joynr/util/JSONSerializer",
    "joynr/exceptions/JoynrRuntimeException",
    "joynr/system/LoggerFactory"
], function(ws, smrf, JoynrMessage, JSONSerializer, JoynrRuntimeException, LoggerFactory) {
    if (typeof Buffer !== "function") {
        throw new JoynrRuntimeException(
                "Decoding of binary websocket messages not possible. Buffer not available.");
    }
    var log = LoggerFactory.getLogger("joynr.messaging.websocket.WebSocket");

    ws.encodeString = function(string) {
        return string;
    };
    ws.decodeEventData = function(data) {
        return data;
    };

    ws.marshalJoynrMessage =
            function(joynrMessage) {
                var smrfMsg = {};
                var headerKey;
                smrfMsg.sender = joynrMessage.header.from;
                smrfMsg.recipient = joynrMessage.header.to;
                smrfMsg.ttlMs = joynrMessage.header.expiryDate;
                smrfMsg.isTtlAbsolute = true;
                smrfMsg.isCompressed = false;
                smrfMsg.body = new Buffer(joynrMessage.payload);
                smrfMsg.encryptionCert = null;
                smrfMsg.signingCert = null;
                smrfMsg.signingKey = null;
                smrfMsg.headers = {};
                smrfMsg.headers.type = joynrMessage.type;
                smrfMsg.headers.id = joynrMessage.header.msgId;
                if (joynrMessage.header.replyChannelId) {
                    smrfMsg.headers.replyTo = joynrMessage.header.replyChannelId;
                }
                // no special handling required for 'creator' and 'effort'
                // since there names are identical in header fields of JoynrMessage
                // and smrfMessage
                for (headerKey in joynrMessage.header) {
                    if (joynrMessage.header.hasOwnProperty(headerKey)) {
                        if (headerKey !== "contentType"
                            && headerKey !== "from"
                            && headerKey !== "msgId"
                            && headerKey !== "replyChannelId"
                            && headerKey !== "to"
                            && headerKey !== "expiryDate") {
                            smrfMsg.headers[headerKey] = joynrMessage.header[headerKey];
                        }
                    }
                }
                var serializedMsg;
                try {
                    serializedMsg = smrf.serialize(smrfMsg);
                } catch (e) {
                    throw new Error("ws.marshalJoynrMessage: got exception " + e);
                }
                return serializedMsg;
            };

    ws.unmarshalJoynrMessage = function(event, callback) {
        if (typeof event.data === "object") {
            var headerKey;
            var smrfMsg;
            try {
                smrfMsg = smrf.deserialize(event.data);
            } catch (e) {
                throw new Error("ws.marshalJoynrMessage: got exception " + e);
            }
            var convertedMsg = {};
            convertedMsg.header = {};
            convertedMsg.header.from = smrfMsg.sender;
            convertedMsg.header.to = smrfMsg.recipient;
            convertedMsg.header.msgId = smrfMsg.headers.id;
            if (smrfMsg.headers.replyTo) {
                convertedMsg.header.replyChannelId = smrfMsg.headers.replyTo;
            }
            convertedMsg.type = smrfMsg.headers.type;
            // ignore for now:
            //   smrfMsg.headers.isCompressed
            //   smrfMsg.headers.encryptionCert
            //   smrfMsg.headers.signingKey
            //   smrfMsg.headers.signingCert
            if (smrfMsg.isTtlAbsolute === true) {
                convertedMsg.header.expiryDate = smrfMsg.ttlMs;
            } else {
                convertedMsg.header.expiryDate = smrfMsg.ttlMs + Date.now();
            }
            convertedMsg.header.effort = smrfMsg.effort;
            convertedMsg.payload = smrfMsg.body.toString();
            for (headerKey in smrfMsg.headers) {
                if (smrfMsg.headers.hasOwnProperty(headerKey)) {
                    if (headerKey !== 'type' && headerKey !== 'id' && headerKey !== 'replyTo') {
                        convertedMsg.header[headerKey] = smrfMsg.headers[headerKey];
                    }
                }
            }
            var joynrMessage = new JoynrMessage(convertedMsg);
            callback(joynrMessage);
        } else {
            log.error("Received unsupported message from websocket.");
        }
    };

    return ws;
});
