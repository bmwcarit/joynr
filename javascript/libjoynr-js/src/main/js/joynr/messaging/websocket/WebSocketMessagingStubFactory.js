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

define("joynr/messaging/websocket/WebSocketMessagingStubFactory", [
    "joynr/util/Util",
    "joynr/messaging/websocket/WebSocketMessagingStub",
    "joynr/system/routingtypes/WebSocketAddress"
], function(Util, WebSocketMessagingStub, WebSocketAddress) {

    /**
     * @constructor
     * @name WebSocketMessagingStubFactory
     * @param {Object}
     *            settings
     * @param {SharedWebSocket}
     *            settings.sharedWebSocket to the websocket server
     * @param {WebSocketAddress}
     *            settings.address of the websocket for the websocket server
     */
    var WebSocketMessagingStubFactory = function WebSocketMessagingStubFactory(settings) {
        Util.checkProperty(settings, "Object", "settings");
        Util.checkProperty(settings.address, "WebSocketAddress", "address");
        Util.checkProperty(settings.sharedWebSocket, "SharedWebSocket", "sharedWebSocket");

        var addresses = {};
        addresses[settings.address] = new WebSocketMessagingStub({
            sharedWebSocket : settings.sharedWebSocket
        });

        /**
         * @name WebSocketMessagingStubFactory#build
         * @function
         */
        this.build = function build(address) {
            Util.checkProperty(address, "WebSocketAddress", "address");

            if (addresses[address] === undefined) {
                addresses[address] = new WebSocketMessagingStub({
                    address : address
                });
            }

            return addresses[address];
        };
    };

    return WebSocketMessagingStubFactory;

});