/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
const Typing = require("../../util/Typing");
const WebSocketMessagingStub = require("./WebSocketMessagingStub");

class WebSocketMessagingStubFactory {
    /**
     * @constructor
     * @name WebSocketMessagingStubFactory
     * @param {Object} settings
     * @param {SharedWebSocket} settings.sharedWebSocket to the websocket server
     * @param {WebSocketAddress} settings.address of the websocket for the websocket server
     */
    constructor(settings) {
        Typing.checkProperty(settings, "Object", "settings");
        Typing.checkProperty(settings.address, "WebSocketAddress", "address");
        Typing.checkProperty(settings.sharedWebSocket, "SharedWebSocket", "sharedWebSocket");

        const addresses = {};
        addresses[settings.address] = new WebSocketMessagingStub({
            sharedWebSocket: settings.sharedWebSocket
        });

        /**
         * @name WebSocketMessagingStubFactory#build
         * @function
         */
        this.build = function build(address) {
            return addresses[address];
        };
    }
}

module.exports = WebSocketMessagingStubFactory;
