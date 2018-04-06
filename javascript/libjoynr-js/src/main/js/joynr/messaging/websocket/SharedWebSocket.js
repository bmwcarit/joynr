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
var Promise = require("../../../global/Promise");
var WebSocket = require("../../../global/WebSocketNode");
var Typing = require("../../util/Typing");
var JSONSerializer = require("../../util/JSONSerializer");
var LongTimer = require("../../util/LongTimer");
var Util = require("../../util/UtilInternal");
var LoggingManager = require("../../system/LoggingManager");
var log = LoggingManager.getLogger("joynr.messaging.websocket.SharedWebSocket");
/**
 * @param address
 * @param {WebSocketAddress}
 *            address to which messages are sent on the clustercontroller.
 * @returns {String} a url
 */
function webSocketAddressToUrl(address) {
    var url = address.protocol + "://" + address.host + ":" + address.port;
    if (address.path) {
        url += address.path;
    }
    return url;
}

/**
 * @param {WebSocket}
 *            websocket to use for the initialization
 * @param {String}
 *            localAddress address used by the cluster controller to contact this
 *            libjoynr
 */
function initializeConnection(websocket, localAddress) {
    websocket.send(websocket.encodeString(JSON.stringify(localAddress)), { binary: true });
}

/**
 * @param {WebSocket}
 *            websocket to use for sending messages
 * @param {Array}
 *            queuedMessages
 */
function sendQueuedMessages(websocket, queuedMessages) {
    var queued;
    while (queuedMessages.length) {
        queued = queuedMessages.shift();
        try {
            websocket.send(websocket.marshalJoynrMessage(queued), { binary: true });
            // Error is thrown if the socket is no longer open
        } catch (e) {
            // so add the message back to the front of the queue
            queuedMessages.unshift(queued);
            throw e;
        }
    }
}

function sendMessage(websocket, joynrMessage, queuedMessages) {
    if (websocket.readyState === WebSocket.OPEN) {
        try {
            websocket.send(websocket.marshalJoynrMessage(joynrMessage), { binary: true });
            // Error is thrown if the socket is no longer open, so requeue to the front
        } catch (e) {
            // add the message back to the front of the queue
            queuedMessages.unshift(joynrMessage);
            log.error("could not send joynrMessage: " + joynrMessage.msgId + " requeuing message. Error: " + e);
        }
    } else {
        // push new messages onto the back of the queue
        queuedMessages.push(joynrMessage);
    }
    return Promise.resolve();
}

/**
 * @name SharedWebSocket
 * @constructor
 * @param {Object}
 *            settings
 * @param {WebSocketClientAddress}
 *            settings.localAddress address used by the websocket server to contact this
 *            client. This address is used in in the init phase on the websocket, and
 *            then registered with the message router on the remote side
 * @param {WebSocketAddress}
 *            settings.remoteAddress to which messages are sent on the websocket server.
 * @param {Object} settings.provisioning
 * @param {Number} settings.provisioning.reconnectSleepTimeMs
 * @param {Object} settings.keychain
 */
var SharedWebSocket = function SharedWebSocket(settings) {
    Typing.checkProperty(settings, "Object", "settings");
    Typing.checkProperty(settings.localAddress, "WebSocketClientAddress", "localAddress");
    Typing.checkProperty(settings.remoteAddress, "WebSocketAddress", "remoteAddress");

    var websocket = null;
    var provisioning = settings.provisioning || {};
    var reconnectSleepTimeMs = provisioning.reconnectSleepTimeMs || 1000; // default value = 1000ms
    var useUnencryptedTls = provisioning.useUnencryptedTls || true; // default to unencrypted Tls communication
    var localAddress = settings.localAddress;
    var remoteUrl = webSocketAddressToUrl(settings.remoteAddress);
    var onmessageCallback = null;
    var queuedMessages = [];
    var onOpen;
    var onError;
    var onClose;
    var closed = false;
    var reconnectTimer;

    var resetConnection = function resetConnection() {
        reconnectTimer = undefined;
        if (closed) {
            return;
        }
        websocket = new WebSocket(remoteUrl, settings.keychain, useUnencryptedTls);
        websocket.onopen = onOpen;
        websocket.onclose = onClose;
        websocket.onerror = onError;
        if (onmessageCallback !== null) {
            websocket.onmessage = onmessageCallback;
        }
    };

    onError = function onError(event) {
        if (closed) {
            return;
        }
        log.error(
            "error in websocket: " +
                event.code +
                (event.reason !== undefined ? " reason: " + event.reason : "") +
                ". Resetting connection."
        );
        if (reconnectTimer !== undefined) {
            LongTimer.clearTimeout(reconnectTimer);
        }
        if (websocket) {
            websocket.close(SharedWebSocket.EVENT_CODE_SHUTDOWN, "shutdown");
            websocket = null;
        }
        reconnectTimer = LongTimer.setTimeout(resetConnection, reconnectSleepTimeMs);
    };

    onClose = function onClose(event) {
        if (closed) {
            return;
        }
        if (event.code !== SharedWebSocket.EVENT_CODE_SHUTDOWN) {
            log.info(
                "connection closed unexpectedly. code: " +
                    event.code +
                    " reason: " +
                    event.reason +
                    ". Trying to reconnect..."
            );
            if (reconnectTimer !== undefined) {
                LongTimer.clearTimeout(reconnectTimer);
            }
            if (websocket) {
                websocket.close(SharedWebSocket.EVENT_CODE_SHUTDOWN, "shutdown");
                websocket = null;
            }
            reconnectTimer = LongTimer.setTimeout(resetConnection, reconnectSleepTimeMs);
        } else {
            log.info("connection closed. reason: " + event.reason);
            if (websocket) {
                websocket.close(SharedWebSocket.EVENT_CODE_SHUTDOWN, "shutdown");
                websocket = null;
            }
        }
    };

    // send all queued messages, requeuing to the front in case of a problem
    onOpen = function onOpen() {
        try {
            log.debug("connection opened.");
            initializeConnection(websocket, localAddress);
            sendQueuedMessages(websocket, queuedMessages);
        } catch (e) {
            resetConnection();
        }
    };

    resetConnection();

    /**
     * @name SharedWebSocket#send
     * @function
     * @param {JoynrMessage}
     *            joynrMessage the joynr message to transmit
     */
    this.send = function send(joynrMessage) {
        log.debug(">>> OUTGOING >>> message with ID " + joynrMessage.msgId);
        return sendMessage(websocket, joynrMessage, queuedMessages);
    };

    /**
     * @name SharedWebSocket#close
     * @function
     */
    this.close = function close() {
        closed = true;
        if (reconnectTimer !== undefined) {
            LongTimer.clearTimeout(reconnectTimer);
            reconnectTimer = undefined;
        }
        if (websocket !== null) {
            websocket.close(SharedWebSocket.EVENT_CODE_SHUTDOWN, "shutdown");
            websocket = null;
        }
    };

    // using the defineProperty syntax for onmessage to be able to keep
    // the same API as WebSocket but have a setter function called when
    // the attribute is set.
    Object.defineProperty(this, "onmessage", {
        set: function(newCallback) {
            if (typeof newCallback === "function") {
                onmessageCallback = function(data) {
                    websocket.unmarshalJoynrMessage(data, newCallback);
                };
                websocket.onmessage = onmessageCallback;
            } else {
                throw new Error("onmessage callback must be a function, but instead was of type " + typeof newCallback);
            }
        },
        get: function() {
            return onmessageCallback;
        },
        enumerable: false,
        configurable: false
    });
};
SharedWebSocket.EVENT_CODE_SHUTDOWN = 4000;
module.exports = SharedWebSocket;
