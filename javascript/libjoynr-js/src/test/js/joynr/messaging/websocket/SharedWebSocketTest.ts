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

import * as MessageSerializer from "../../../../../main/js/joynr/messaging/MessageSerializer";
import WebSocketProtocol from "../../../../../main/js/generated/joynr/system/RoutingTypes/WebSocketProtocol";
import WebSocketAddress from "../../../../../main/js/generated/joynr/system/RoutingTypes/WebSocketAddress";
import WebSocketClientAddress from "../../../../../main/js/generated/joynr/system/RoutingTypes/WebSocketClientAddress";
import JoynrMessage = require("../../../../../main/js/joynr/messaging/JoynrMessage");

const WebSocket = {
    WSN_CONNECTING: 0,
    WSN_OPEN: 1,
    WSN_CLOSING: 2,
    WSN_CLOSED: 3
};

const webSocketMock = {
    send: jest.fn().mockImplementation((_value, _settings, cb) => {
        if (cb && typeof cb === "function") cb();
    }),
    readyState: WebSocket.WSN_CONNECTING
};
const webSocketConstructor: any = jest.fn().mockImplementation(() => webSocketMock);
webSocketConstructor.WSN_CONNECTING = WebSocket.WSN_CONNECTING;
webSocketConstructor.WSN_OPEN = WebSocket.WSN_OPEN;
webSocketConstructor.WSN_CLOSING = WebSocket.WSN_CLOSING;
webSocketConstructor.WSN_CLOSED = WebSocket.WSN_CLOSED;

jest.doMock("../../../../../main/js/global/WebSocketNode", () => webSocketConstructor);

import SharedWebSocket from "../../../../../main/js/joynr/messaging/websocket/SharedWebSocket";

let sharedWebSocket: SharedWebSocket;
const joynrMessage = new JoynrMessage({ payload: "hi", type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST });
joynrMessage.from = "sender";
joynrMessage.to = "recipient";
joynrMessage.expiryDate = 1;
joynrMessage.compress = false;
const localAddress = new WebSocketClientAddress({
    id: "1234"
});
const ccAddress = new WebSocketAddress({
    protocol: WebSocketProtocol.WS,
    host: "host",
    port: 1234,
    path: "/test"
});

describe("libjoynr-js.joynr.messaging.webmessaging.SharedWebSocket-1", () => {
    beforeEach(done => {
        webSocketConstructor.mockImplementation(() => webSocketMock);

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
        webSocketMock.readyState = WebSocket.WSN_OPEN;
        await sharedWebSocket.send(joynrMessage);
        expect(webSocketMock.send).toHaveBeenCalledWith(MessageSerializer.stringify(joynrMessage), {
            binary: true
        });

        webSocketMock.send.mockClear();
        webSocketMock.readyState = WebSocket.WSN_CLOSING;
        await sharedWebSocket.send(joynrMessage);
        expect(webSocketMock.send).not.toHaveBeenCalled();
    });

    it("uses callbacks after sharedWebSocket.send", async () => {
        webSocketMock.readyState = WebSocket.WSN_OPEN;
        sharedWebSocket.enableShutdownMode();
        await sharedWebSocket.send(joynrMessage);
        expect(webSocketMock.send).toHaveBeenCalledWith(
            MessageSerializer.stringify(joynrMessage),
            {
                binary: true
            },
            expect.any(Function)
        );

        webSocketMock.send.mockClear();
        webSocketMock.readyState = WebSocket.WSN_CLOSING;
        await sharedWebSocket.send(joynrMessage);
        expect(webSocketMock.send).not.toHaveBeenCalled();
    });
});

describe("libjoynr-js.joynr.messaging.webmessaging.SharedWebSocket-2", () => {
    beforeEach(done => {
        webSocketConstructor.mockImplementation(() => null);

        sharedWebSocket = new SharedWebSocket({
            localAddress,
            remoteAddress: ccAddress,
            keychain: undefined,
            provisioning: {}
        });
        done();
    });

    it("queues messages when websocket connection is not available", async () => {
        await sharedWebSocket.send(joynrMessage);
        expect(sharedWebSocket.numberOfQueuedMessages).toBe(1);
    });
});
