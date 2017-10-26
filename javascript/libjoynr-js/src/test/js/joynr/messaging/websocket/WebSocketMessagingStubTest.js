/*jslint node: true */

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
var WebSocketMessagingStub = require("../../../../classes/joynr/messaging/websocket/WebSocketMessagingStub");
var WebSocketMessagingStubFactory = require("../../../../classes/joynr/messaging/websocket/WebSocketMessagingStubFactory");
var WebSocketAddress = require("../../../../classes/joynr/system/RoutingTypes/WebSocketAddress");
var WebSocketProtocol = require("../../../../classes/joynr/system/RoutingTypes/WebSocketProtocol");
var WebSocketClientAddress = require("../../../../classes/joynr/system/RoutingTypes/WebSocketClientAddress");
var SharedWebSocket = require("../../../../classes/joynr/messaging/websocket/SharedWebSocket");

describe("libjoynr-js.joynr.messaging.webmessaging.WebSocketMessagingStub", function() {
    var webSocketMessagingStub = null;
    var factory = null;
    var joynrMessage = null;
    var sharedWebSocket = null;
    var ccAddress = new WebSocketAddress({
        protocol: WebSocketProtocol.WS,
        host: "host",
        port: 1234,
        path: "/test"
    });
    var localAddress = new WebSocketClientAddress({
        id: "1234"
    });

    beforeEach(function(done) {
        sharedWebSocket = new SharedWebSocket({
            remoteAddress: ccAddress,
            localAddress: localAddress
        });
        spyOn(sharedWebSocket, "send").and.callThrough();
        sharedWebSocket.addEventListener = jasmine.createSpy("addEventListener");

        factory = new WebSocketMessagingStubFactory({
            address: ccAddress,
            sharedWebSocket: sharedWebSocket
        });

        webSocketMessagingStub = factory.build(ccAddress);

        function JoynrMessage() {}
        joynrMessage = new JoynrMessage();
        done();
    });

    it("is of correct type and has all members", function(done) {
        expect(WebSocketMessagingStub).toBeDefined();
        expect(typeof WebSocketMessagingStub === "function").toBeTruthy();
        expect(webSocketMessagingStub).toBeDefined();
        expect(webSocketMessagingStub instanceof WebSocketMessagingStub).toBeTruthy();
        expect(webSocketMessagingStub.transmit).toBeDefined();
        expect(typeof webSocketMessagingStub.transmit === "function").toBeTruthy();
        done();
    });

    it("calls websocket.send correctly", function(done) {
        webSocketMessagingStub.transmit(joynrMessage);
        expect(sharedWebSocket.send).toHaveBeenCalledWith(joynrMessage);
        done();
    });
});
