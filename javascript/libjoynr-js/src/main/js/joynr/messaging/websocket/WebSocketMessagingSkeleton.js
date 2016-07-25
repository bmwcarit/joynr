/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

define("joynr/messaging/websocket/WebSocketMessagingSkeleton", [
    "joynr/messaging/JoynrMessage",
    "joynr/util/UtilInternal",
    "joynr/system/LoggerFactory"
], function(JoynrMessage, Util, LoggerFactory) {

    /**
     * @constructor WebSocketMessagingSkeleton
     * @param {Object}
     *            settings
     * @param {SharedWebSocket}
     *            settings.sharedWebSocket
     */
    var WebSocketMessagingSkeleton = function WebSocketMessagingSkeleton(settings) {
        Util.checkProperty(settings, "Object", "settings");
        Util.checkProperty(settings.sharedWebSocket, "SharedWebSocket", "sharedWebSocket");

        var sharedWebSocket = settings.sharedWebSocket;
        var receiverCallbacks = [];

        sharedWebSocket.onmessage = function(event) {
            var received = event.data;
            if (typeof event.data === "string") {
                received = JSON.parse(event.data);
                var joynrMessage = Util.extendDeep(new JoynrMessage(received), received);

                Util.fire(receiverCallbacks, joynrMessage);
            }
        };

        /**
         * Registers the listener function
         * @function WebSocketMessagingSkeleton#registerListener
         *
         * @param {Function}
         *            listener a listener function that should be added and should receive messages
         */
        this.registerListener = function registerListener(listener) {
            Util.checkProperty(listener, "Function", "listener");

            receiverCallbacks.push(listener);
        };

        /**
         * Unregisters the listener function
         * @function WebSocketMessagingSkeleton#unregisterListener
         *
         * @param {Function}
         *            listener the listener function that should re removed and shouldn't receive
         *            messages any more
         */
        this.unregisterListener = function unregisterListener(listener) {
            Util.checkProperty(listener, "Function", "listener");

            Util.removeElementFromArray(receiverCallbacks, listener);
        };

        /**
         * @function WebSocketMessagingSkeleton#shutdown
         */
        this.shutdown = function shutdown() {
            sharedWebSocket.close();
        };
    };

    return WebSocketMessagingSkeleton;

});