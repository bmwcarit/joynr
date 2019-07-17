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

import WebSocketMessagingStubFactory from "../../../../../main/js/joynr/messaging/websocket/WebSocketMessagingStubFactory";
import WebSocketAddress from "../../../../../main/js/generated/joynr/system/RoutingTypes/WebSocketAddress";

describe("libjoynr-js.joynr.messaging.webmessaging.WebSocketMessagingStubFactory", () => {
    let webSocketMessagingStubFactory: WebSocketMessagingStubFactory;
    let joynrMessage: any;
    let sharedWebSocket: any;

    const ccAddress = new WebSocketAddress({
        protocol: "ws" as any,
        host: "host",
        port: 1234,
        path: "/test"
    });

    beforeEach(() => {
        sharedWebSocket = { send: jest.fn(), addEventListener: jest.fn() };

        webSocketMessagingStubFactory = new WebSocketMessagingStubFactory({
            address: ccAddress,
            sharedWebSocket
        });

        joynrMessage = {};
    });

    it("is instantiable and of correct type", () => {
        expect(typeof WebSocketMessagingStubFactory === "function").toBeTruthy();
        expect(webSocketMessagingStubFactory).toBeDefined();
        expect(webSocketMessagingStubFactory instanceof WebSocketMessagingStubFactory).toBeTruthy();
        expect(webSocketMessagingStubFactory.build).toBeDefined();
        expect(typeof webSocketMessagingStubFactory.build === "function").toBeTruthy();
    });

    it("creates a websocket messaging stub and uses it correctly", async () => {
        sharedWebSocket.send = jest.fn();
        sharedWebSocket.send.mockReturnValue(Promise.resolve());
        const webSocketMessagingStub = webSocketMessagingStubFactory.build(ccAddress);
        await webSocketMessagingStub.transmit(joynrMessage);
        expect(sharedWebSocket.send).toHaveBeenCalledWith(joynrMessage);
    });

    it("reuses existing websocket messaging stub", () => {
        const webSocketMessagingStub1 = webSocketMessagingStubFactory.build(ccAddress);
        const webSocketMessagingStub2 = webSocketMessagingStubFactory.build(ccAddress);
        expect(webSocketMessagingStub1).toBe(webSocketMessagingStub2);
    });
});
