/*jslint es5: true, node: true, node: true */
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
var Promise = require('../../../global/Promise');
var WebSocket = require('../../../global/WebSocketNode');
var Typing = require('../../util/Typing');
var JsonSerializer = require('../../util/JSONSerializer');
var LongTimer = require('../../util/LongTimer');
var LoggerFactory = require('../../system/LoggerFactory');
module.exports = (function (Promise, WebSocket, Typing, JSONSerializer, LongTimer, LoggerFactory) {
            var log = LoggerFactory.getLogger("joynr.messaging.websocket.SharedWebSocket");
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
                websocket.send(WebSocket.encodeString(JSON.stringify(localAddress)), {binary: true});
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
                        websocket.send(WebSocket.marshalJoynrMessage(queued.message), {binary: true});
                        queued.resolve();
                        // Error is thrown if the socket is no longer open
                    } catch (e) {
                        // so add the message back to the front of the queue
                        queuedMessages.unshift(queued);
                        throw e;
                    }
                }
            }

            function sendMessage(websocket, joynrMessage, queuedMessages) {
                return new Promise(function(resolve, reject){
                    if (websocket.readyState === WebSocket.OPEN) {
                        try {
                            websocket.send(WebSocket.marshalJoynrMessage(joynrMessage), {binary: true});
                            resolve();
                            // Error is thrown if the socket is no longer open, so requeue to the front
                        } catch (e) {
                            // add the message back to the front of the queue
                            queuedMessages.unshift({
                                message : joynrMessage,
                                resolve : resolve
                            });
                            throw e;
                        }
                    } else {
                        // push new messages onto the back of the queue
                        queuedMessages.push({
                            message : joynrMessage,
                            resolve : resolve
                        });
                    }
                });
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
             */
            var SharedWebSocket =
                    function SharedWebSocket(settings) {
                        Typing.checkProperty(settings, "Object", "settings");
                        Typing.checkProperty(
                                settings.localAddress,
                                "WebSocketClientAddress",
                                "localAddress");
                        Typing.checkProperty(
                                settings.remoteAddress,
                                "WebSocketAddress",
                                "remoteAddress");

                        var websocket = null;
                        var provisioning = settings.provisioning || {};
                        var reconnectSleepTimeMs = provisioning.reconnectSleepTimeMs || 1000; // default value = 1000ms
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
                            websocket = new WebSocket(remoteUrl);
                            websocket.onopen = onOpen;
                            websocket.onclose = onClose;
                            websocket.onerror = onError;
                            if (onmessageCallback !== null) {
                                websocket.onmessage = onmessageCallback;
                            }
                        };

                        onError = function onError(event) {
                            log.error("error in websocket: "
                                    + event.code
                                    + (event.reason !== undefined ? " reason: "
                                    + event.reason : "")
                                    + ". Resetting connection.");
                            if (reconnectTimer !== undefined) {
                                LongTimer.clearTimeout(reconnectTimer);
                            }
                            reconnectTimer = LongTimer.setTimeout(resetConnection, reconnectSleepTimeMs);
                        };

                        onClose = function onClose(event) {
                            if (event.code !== SharedWebSocket.EVENT_CODE_SHUTDOWN ) {
                                log.info("connection closed unexpectedly. code: "
                                    + event.code
                                    + " reason: "
                                    + event.reason
                                    + ". Trying to reconnect...");
                                if (reconnectTimer !== undefined) {
                                    LongTimer.clearTimeout(reconnectTimer);
                                }
                                reconnectTimer = LongTimer.setTimeout(resetConnection, reconnectSleepTimeMs);
                            } else {
                                log.info("connection closed. reason: " + event.reason);
                            }
                        };

                        // send all queued messages, requeuing to the front in case of a problem
                        onOpen = function onOpen() {
                            try {
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
                            return sendMessage(websocket, joynrMessage, queuedMessages).catch(function(e1) {
                                    resetConnection();
                                });
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
                            }
                        };

                        // using the defineProperty syntax for onmessage to be able to keep
                        // the same API as WebSocket but have a setter function called when
                        // the attribute is set.
                        Object.defineProperty(this, "onmessage", {
                            set : function(newCallback) {
                                if (typeof newCallback === "function") {
                                    onmessageCallback = function(data) {
                                        WebSocket.unmarshalJoynrMessage(data, newCallback);
                                    };
                                    websocket.onmessage = onmessageCallback;
                                } else {
                                    throw new Error(
                                            "onmessage callback must be a function, but instead was of type "
                                                + typeof newCallback);
                                }
                            },
                            get : function() {
                                return onmessageCallback;
                            },
                            enumerable : false,
                            configurable : false
                        });

                    };
            SharedWebSocket.EVENT_CODE_SHUTDOWN = 4000;
            return SharedWebSocket;
}(Promise, WebSocket, Typing, JsonSerializer, LongTimer, LoggerFactory));
