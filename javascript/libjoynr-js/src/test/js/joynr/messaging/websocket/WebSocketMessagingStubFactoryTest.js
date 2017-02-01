/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

define([
    "joynr/messaging/websocket/WebSocketMessagingStubFactory",
    "joynr/system/RoutingTypes/WebSocketAddress",
    "joynr/system/RoutingTypes/WebSocketClientAddress",
    "joynr/messaging/websocket/SharedWebSocket",
    "global/WebSocket"
], function(
        WebSocketMessagingStubFactory,
        WebSocketAddress,
        WebSocketClientAddress,
        SharedWebSocket,
        WebSocket) {

    describe("libjoynr-js.joynr.messaging.webmessaging.WebSocketMessagingStubFactory", function() {
        var webSocketMessagingStubFactory = null;
        var websocket = null;
        var joynrMessage = null;
        var localAddress = new WebSocketClientAddress({
            id : "1234"
        });
        var ccAddress = new WebSocketAddress({
            protocol : "ws",
            host : "host",
            port : 1234,
            path : "/test"
        });

        var sharedWebSocket = new SharedWebSocket({
            localAddress : localAddress,
            remoteAddress : ccAddress
        });

        beforeEach(function() {
            websocket = new WebSocket("ws://test");
            websocket.send = jasmine.createSpy("send");

            webSocketMessagingStubFactory = new WebSocketMessagingStubFactory({
                address : ccAddress,
                sharedWebSocket : sharedWebSocket
            });

            function JoynrMessage() {}
            joynrMessage = new JoynrMessage();
        });

        it("is instantiable and of correct type", function() {
            expect(WebSocketMessagingStubFactory).toBeDefined();
            expect(typeof WebSocketMessagingStubFactory === "function").toBeTruthy();
            expect(webSocketMessagingStubFactory).toBeDefined();
            expect(webSocketMessagingStubFactory instanceof WebSocketMessagingStubFactory)
                    .toBeTruthy();
            expect(webSocketMessagingStubFactory.build).toBeDefined();
            expect(typeof webSocketMessagingStubFactory.build === "function").toBeTruthy();
        });

        it("creates a websocket messaging stub and uses it correctly", function() {
            websocket.readyState = WebSocket.OPEN;

            var webSocketMessagingStub = webSocketMessagingStubFactory.build(ccAddress);
            webSocketMessagingStub.transmit(joynrMessage);
            expect(websocket.send).toHaveBeenCalledWith(
                    WebSocket.marshalJoynrMessage(joynrMessage),
                    {
                        binary : true
                    });
        });

        it("reuses existing websocket messaging stub", function() {
            var webSocketMessagingStub1, webSocketMessagingStub2;
            webSocketMessagingStub1 = webSocketMessagingStubFactory.build(ccAddress);
            webSocketMessagingStub2 = webSocketMessagingStubFactory.build(ccAddress);
            expect(webSocketMessagingStub1).toBe(webSocketMessagingStub2);
        });

    });

});
