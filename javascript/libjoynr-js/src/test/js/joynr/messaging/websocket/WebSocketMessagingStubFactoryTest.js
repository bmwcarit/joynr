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
const WebSocketMessagingStubFactory = require("../../../../../main/js/joynr/messaging/websocket/WebSocketMessagingStubFactory");
const WebSocketAddress = require("../../../../../main/js/generated/joynr/system/RoutingTypes/WebSocketAddress");
const WebSocketClientAddress = require("../../../../../main/js/generated/joynr/system/RoutingTypes/WebSocketClientAddress");
const SharedWebSocket = require("../../../../../main/js/joynr/messaging/websocket/SharedWebSocket");
const WebSocket = require("../../../../../test/js/global/WebSocketMock");

describe("libjoynr-js.joynr.messaging.webmessaging.WebSocketMessagingStubFactory", () => {
    let webSocketMessagingStubFactory = null;
    let websocket = null;
    let joynrMessage = null;
    const localAddress = new WebSocketClientAddress({
        id: "1234"
    });
    const ccAddress = new WebSocketAddress({
        protocol: "ws",
        host: "host",
        port: 1234,
        path: "/test"
    });

    const sharedWebSocket = new SharedWebSocket({
        localAddress,
        remoteAddress: ccAddress
    });

    beforeEach(() => {
        websocket = new WebSocket("ws://test");
        websocket.send = jasmine.createSpy("send");

        webSocketMessagingStubFactory = new WebSocketMessagingStubFactory({
            address: ccAddress,
            sharedWebSocket
        });

        function JoynrMessage() {}
        joynrMessage = new JoynrMessage();
    });

    it("is instantiable and of correct type", () => {
        expect(WebSocketMessagingStubFactory).toBeDefined();
        expect(typeof WebSocketMessagingStubFactory === "function").toBeTruthy();
        expect(webSocketMessagingStubFactory).toBeDefined();
        expect(webSocketMessagingStubFactory instanceof WebSocketMessagingStubFactory).toBeTruthy();
        expect(webSocketMessagingStubFactory.build).toBeDefined();
        expect(typeof webSocketMessagingStubFactory.build === "function").toBeTruthy();
    });

    it("creates a websocket messaging stub and uses it correctly", () => {
        websocket.readyState = WebSocket.OPEN;

        const webSocketMessagingStub = webSocketMessagingStubFactory.build(ccAddress);
        webSocketMessagingStub.transmit(joynrMessage);
        expect(websocket.send).toHaveBeenCalledWith(websocket.marshalJoynrMessage(joynrMessage), {
            binary: true
        });
    });

    it("reuses existing websocket messaging stub", () => {
        let webSocketMessagingStub1, webSocketMessagingStub2;
        webSocketMessagingStub1 = webSocketMessagingStubFactory.build(ccAddress);
        webSocketMessagingStub2 = webSocketMessagingStubFactory.build(ccAddress);
        expect(webSocketMessagingStub1).toBe(webSocketMessagingStub2);
    });
});
