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
var JoynrRuntimeException = require("../joynr/exceptions/JoynrRuntimeException");
var Util = require("../joynr/util/UtilInternal.js");
var MessageSerializer = require("../joynr/messaging/MessageSerializer");

function useWebSocketNode() {
    if (typeof Buffer !== "function") {
        throw new JoynrRuntimeException("Decoding of binary websocket messages not possible. Buffer not available.");
    }

    /*
     * try to load the native C++ websocket implementation first; only if this fails
     * fall back to JS implementation. Temporarily silence error output for first
     * load attempt.
     */
    var ws;
    try {
        ws = require("wscpp");
    } catch (e) {
        ws = require("ws");
    }

    function WebSocketNodeWrapper(remoteUrl, keychain, useUnencryptedTls) {
        var clientOptions = keychain
            ? {
                  cert: keychain.tlsCert,
                  key: keychain.tlsKey,
                  ca: keychain.tlsCa,
                  rejectUnauthorized: true,
                  useUnencryptedTls: useUnencryptedTls
              }
            : undefined;

        var webSocketObj = new ws(remoteUrl, clientOptions);

        webSocketObj.encodeString = function(string) {
            return Buffer.from(string);
        };
        webSocketObj.decodeEventData = function(data) {
            return data;
        };

        var signingCallback = keychain
            ? function() {
                  // set the signature to just be the ownerID
                  return Buffer.from(keychain.ownerId);
              }
            : undefined;

        webSocketObj.marshalJoynrMessage = function(data) {
            return MessageSerializer.stringify(data, signingCallback);
        };
        webSocketObj.unmarshalJoynrMessage = function(event, callback) {
            var joynrMessage = MessageSerializer.parse(event.data);
            if (joynrMessage) {
                callback(joynrMessage);
            }
        };

        return webSocketObj;
    }

    Util.extend(WebSocketNodeWrapper, ws);

    return WebSocketNodeWrapper;
}

module.exports = global.window !== undefined ? require("./WebSocket") : useWebSocketNode();
