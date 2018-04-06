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
var JoynrMessage = require("../JoynrMessage");
var Typing = require("../../util/Typing");
var LoggingManager = require("../../system/LoggingManager");
var log = LoggingManager.getLogger("joynr.messaging.websocket.WebSocketMessagingSkeleton");

/**
 * @constructor WebSocketMessagingSkeleton
 * @param {Object}
 *            settings
 * @param {SharedWebSocket}
 *            settings.sharedWebSocket
 */
var WebSocketMessagingSkeleton = function WebSocketMessagingSkeleton(settings) {
    Typing.checkProperty(settings, "Object", "settings");
    Typing.checkProperty(settings.sharedWebSocket, "SharedWebSocket", "sharedWebSocket");
    Typing.checkProperty(settings.mainTransport, "Boolean", "settings.mainTransport");

    var sharedWebSocket = settings.sharedWebSocket;
    var listener;

    settings.sharedWebSocket.onmessage = function(joynrMessage) {
        log.debug("<<< INCOMING <<< message with ID " + joynrMessage.msgId);
        if (listener !== undefined) {
            if (joynrMessage.type === JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST && settings.mainTransport) {
                joynrMessage.isReceivedFromGlobal = true;
            }
            listener(joynrMessage);
        }
    };

    /**
     * Registers the listener function
     * @function WebSocketMessagingSkeleton#registerListener
     *
     * @param {Function}
     *            listener a listener function that should be added and should receive messages
     */
    this.registerListener = function registerListener(listenerToAdd) {
        Typing.checkProperty(listenerToAdd, "Function", "listenerToAdd");

        listener = listenerToAdd;
    };

    /**
     * Unregisters the listener function
     * @function WebSocketMessagingSkeleton#unregisterListener
     *
     * @param {Function}
     *            listener the listener function that should re removed and shouldn't receive
     *            messages any more
     */
    this.unregisterListener = function unregisterListener(listenerToRemove) {
        Typing.checkProperty(listenerToRemove, "Function", "listenerToRemove");

        listener = undefined;
    };

    /**
     * @function WebSocketMessagingSkeleton#shutdown
     */
    this.shutdown = function shutdown() {
        sharedWebSocket.close();
    };
};

module.exports = WebSocketMessagingSkeleton;
