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
require("../../../node-unit-test-helper");
const WebSocketMessagingStub = require("../../../../../main/js/joynr/messaging/websocket/WebSocketMessagingStub");
const WebSocketMessagingStubFactory = require("../../../../../main/js/joynr/messaging/websocket/WebSocketMessagingStubFactory");
const WebSocketAddress = require("../../../../../main/js/generated/joynr/system/RoutingTypes/WebSocketAddress");
const WebSocketProtocol = require("../../../../../main/js/generated/joynr/system/RoutingTypes/WebSocketProtocol");
const WebSocketClientAddress = require("../../../../../main/js/generated/joynr/system/RoutingTypes/WebSocketClientAddress");
const SharedWebSocket = require("../../../../../main/js/joynr/messaging/websocket/SharedWebSocket");

describe("libjoynr-js.joynr.messaging.webmessaging.WebSocketMessagingStub", () => {
    let webSocketMessagingStub = null;
    let factory = null;
    let joynrMessage = null;
    let sharedWebSocket = null;
    const ccAddress = new WebSocketAddress({
        protocol: WebSocketProtocol.WS,
        host: "host",
        port: 1234,
        path: "/test"
    });
    const localAddress = new WebSocketClientAddress({
        id: "1234"
    });

    beforeEach(done => {
        sharedWebSocket = new SharedWebSocket({
            remoteAddress: ccAddress,
            localAddress
        });
        spyOn(sharedWebSocket, "send").and.callThrough();
        sharedWebSocket.addEventListener = jasmine.createSpy("addEventListener");

        factory = new WebSocketMessagingStubFactory({
            address: ccAddress,
            sharedWebSocket
        });

        webSocketMessagingStub = factory.build(ccAddress);

        function JoynrMessage() {}
        joynrMessage = new JoynrMessage();
        done();
    });

    it("is of correct type and has all members", done => {
        expect(WebSocketMessagingStub).toBeDefined();
        expect(typeof WebSocketMessagingStub === "function").toBeTruthy();
        expect(webSocketMessagingStub).toBeDefined();
        expect(webSocketMessagingStub instanceof WebSocketMessagingStub).toBeTruthy();
        expect(webSocketMessagingStub.transmit).toBeDefined();
        expect(typeof webSocketMessagingStub.transmit === "function").toBeTruthy();
        done();
    });

    it("calls websocket.send correctly", done => {
        webSocketMessagingStub.transmit(joynrMessage);
        expect(sharedWebSocket.send).toHaveBeenCalledWith(joynrMessage);
        done();
    });
});
