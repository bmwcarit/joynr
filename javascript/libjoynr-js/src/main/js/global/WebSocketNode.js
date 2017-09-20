/*jslint es5: true, node: true, newcap: true */
/*global Buffer: true, requirejs: true */

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
    "global/Smrf",
    "joynr/messaging/JoynrMessage",
    "joynr/exceptions/JoynrRuntimeException",
    "joynr/system/LoggerFactory"
], function(smrf, JoynrMessage, JoynrRuntimeException, LoggerFactory) {

    function WebSocketNodeWrapper(remoteUrl, keychain) {

        if (typeof Buffer !== "function") {
            throw new JoynrRuntimeException(
                    "Decoding of binary websocket messages not possible. Buffer not available.");
        }
        var log = LoggerFactory.getLogger("joynr.messaging.websocket.WebSocket");

        var ws;

        /*
         * try to load the native C++ websocket implementation first; only if this fails
         * fall back to JS implementation. Temporarily silence error output for first
         * load attempt.
         */
        var savedOnError = requirejs.onError;
        requirejs.onError = function() {};
        ws = requirejs("wscpp");
        requirejs.onError = savedOnError;
        if (!ws) {
            ws = requirejs("ws");
            if (!ws) {
                throw new Error("No websocket module available");
            }
        }
        var webSocketObj = new ws(remoteUrl);

        webSocketObj.encodeString = function(string) {
            return Buffer.from(string);
        };
        webSocketObj.decodeEventData = function(data) {
            return data;
        };

        var skipJoynrHeaderKeys = {
            "contentType" : true,
            "creator" : true,
            "effort" : true,
            "from" : true,
            "msgId" : true,
            "replyChannelId" : true,
            "to" : true,
            "expiryDate" : true
        };

        webSocketObj.marshalJoynrMessage = function(joynrMessage) {
            var smrfMsg = {};
            var headerKey;
            smrfMsg.sender = joynrMessage.header.from;
            smrfMsg.recipient = joynrMessage.header.to;
            smrfMsg.ttlMs = joynrMessage.header.expiryDate;
            smrfMsg.isTtlAbsolute = true;
            smrfMsg.isCompressed = joynrMessage.compress;
            smrfMsg.body = new Buffer(joynrMessage.payload);
            smrfMsg.encryptionCert = null;
            smrfMsg.signingCert = null;
            smrfMsg.signingKey = null;
            smrfMsg.headers = {};
            smrfMsg.headers.t = joynrMessage.type;
            smrfMsg.headers.id = joynrMessage.header.msgId;
            if (joynrMessage.header.replyChannelId) {
                smrfMsg.headers.re = joynrMessage.header.replyChannelId;
            }
            if (joynrMessage.header.effort) {
                smrfMsg.headers.ef = joynrMessage.header.effort;
            }
            for (headerKey in joynrMessage.header) {
                if (joynrMessage.header.hasOwnProperty(headerKey)) {
                    if (!skipJoynrHeaderKeys[headerKey]) {
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

        var skipSmrfHeaderKeys = {
            // headers already converted manually
            't' : true,
            'id' : true,
            're' : true,
            'ef' : true,
            // reserved headers, prevent overwriting
            'from' : true,
            'to' : true,
            'msgId' : true,
            'replyChannelId' : true,
            'expiryDate' : true,
            'effort' : true
        };

        webSocketObj.unmarshalJoynrMessage = function(event, callback) {
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
                if (smrfMsg.headers.re) {
                    convertedMsg.header.replyChannelId = smrfMsg.headers.re;
                }
                convertedMsg.type = smrfMsg.headers.t;
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
                convertedMsg.header.effort = smrfMsg.headers.ef;
                convertedMsg.payload = smrfMsg.body.toString();
                for (headerKey in smrfMsg.headers) {
                    if (smrfMsg.headers.hasOwnProperty(headerKey)) {
                        if (!skipSmrfHeaderKeys[headerKey]) {
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

        return webSocketObj;
    }

    return WebSocketNodeWrapper;
});
