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
 * @name WebSocketMessagingStub
 * @constructor
 * @param {Object}
 *            settings
 * @param {SharedWebSocket}
 *            settings.sharedWebSocket to which messages are sent on the clustercontroller.
 */
var WebSocketMessagingStub = function WebSocketMessagingStub(settings) {
    var sharedWebSocket = settings.sharedWebSocket;

    /**
     * @name WebSocketMessagingStub#transmit
     * @function
     * @param {JoynrMessage}
     *            joynrMessage the joynr message to transmit
     */
    this.transmit = function transmit(joynrMessage) {
        return sharedWebSocket.send(joynrMessage);
    };
};

module.exports = WebSocketMessagingStub;
