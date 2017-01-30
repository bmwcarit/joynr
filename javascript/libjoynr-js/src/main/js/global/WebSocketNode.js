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
    "joynr/messaging/JoynrMessage",
    "joynr/util/JSONSerializer",
    "joynr/exceptions/JoynrRuntimeException"
], function(ws, JoynrMessage, JSONSerializer, JoynrRuntimeException) {
    if (typeof Buffer !== "function") {
        throw new JoynrRuntimeException(
                "Decoding of binary websocket messages not possible. Buffer not available.");
    }
    ws.marshalJoynrMessage = function(joynrMessage) {
        return JSONSerializer.stringify(joynrMessage);
    };

    ws.unmarshalJoynrMessage = function(event) {
        if (typeof event.data === "object") {
            return new JoynrMessage(JSON.parse(event.data.toString()));
        }
        return undefined;
    };

    return ws;
});
