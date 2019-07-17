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

import WebSocketProtocol from "../../../../../main/js/generated/joynr/system/RoutingTypes/WebSocketProtocol";
import WebSocketAddress from "../../../../../main/js/generated/joynr/system/RoutingTypes/WebSocketAddress";
import WebSocketClientAddress from "../../../../../main/js/generated/joynr/system/RoutingTypes/WebSocketClientAddress";
import JoynrMessage = require("../../../../../main/js/joynr/messaging/JoynrMessage");

const WebSocket = {
    CONNECTING: 0,
    OPEN: 1,
    CLOSING: 2,
    CLOSED: 3
};

const webSocketMock = {
    send: jest.fn().mockImplementation((_value, _settings, cb) => {
        if (cb && typeof cb === "function") cb();
    }),
    readyState: WebSocket.CONNECTING,
    marshalJoynrMessage: jest.fn().mockImplementation((joynrMessage: any) => JSON.stringify(joynrMessage))
};
const webSocketConstructor: any = jest.fn().mockImplementation(() => webSocketMock);
webSocketConstructor.CONNECTING = WebSocket.CONNECTING;
webSocketConstructor.OPEN = WebSocket.OPEN;
webSocketConstructor.CLOSING = WebSocket.CLOSING;
webSocketConstructor.CLOSED = WebSocket.CLOSED;

jest.doMock("../../../../../main/js/global/WebSocketNode", () => webSocketConstructor);

import SharedWebSocket from "../../../../../main/js/joynr/messaging/websocket/SharedWebSocket";

describe("libjoynr-js.joynr.messaging.webmessaging.SharedWebSocket", () => {
    let localAddress: any;
    let ccAddress: any;
    let sharedWebSocket: SharedWebSocket;
    const joynrMessage = new JoynrMessage({ payload: "hi", type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST });

    beforeEach(done => {
        localAddress = new WebSocketClientAddress({
            id: "1234"
        });
        ccAddress = new WebSocketAddress({
            protocol: WebSocketProtocol.WS,
            host: "host",
            port: 1234,
            path: "/test"
        });

        sharedWebSocket = new SharedWebSocket({
            localAddress,
            remoteAddress: ccAddress,
            keychain: undefined,
            provisioning: {}
        });
        done();
    });

    it("is of correct type and has all members", () => {
        expect(SharedWebSocket).toBeDefined();
        expect(typeof SharedWebSocket === "function").toBeTruthy();
        expect(sharedWebSocket).toBeDefined();
        expect(sharedWebSocket instanceof SharedWebSocket).toBeTruthy();
        expect(sharedWebSocket.send).toBeDefined();
        expect(typeof sharedWebSocket.send === "function").toBeTruthy();
    });

    it("calls websocket.send correctly", async () => {
        webSocketMock.readyState = WebSocket.OPEN;
        await sharedWebSocket.send(joynrMessage);
        expect(webSocketMock.marshalJoynrMessage).toHaveBeenCalled();
        expect(webSocketMock.send).toHaveBeenCalledWith(webSocketMock.marshalJoynrMessage(joynrMessage), {
            binary: true
        });

        webSocketMock.send.mockClear();
        webSocketMock.readyState = WebSocket.CLOSING;
        await sharedWebSocket.send(joynrMessage);
        expect(webSocketMock.send).not.toHaveBeenCalled();
    });

    it("uses callbacks after sharedWebSocket.send", async () => {
        webSocketMock.readyState = WebSocket.OPEN;
        sharedWebSocket.enableShutdownMode();
        await sharedWebSocket.send(joynrMessage);
        expect(webSocketMock.marshalJoynrMessage).toHaveBeenCalled();
        expect(webSocketMock.send).toHaveBeenCalledWith(
            webSocketMock.marshalJoynrMessage(joynrMessage),
            {
                binary: true
            },
            expect.any(Function)
        );

        webSocketMock.send.mockClear();
        webSocketMock.readyState = WebSocket.CLOSING;
        await sharedWebSocket.send(joynrMessage);
        expect(webSocketMock.send).not.toHaveBeenCalled();
    });
});
