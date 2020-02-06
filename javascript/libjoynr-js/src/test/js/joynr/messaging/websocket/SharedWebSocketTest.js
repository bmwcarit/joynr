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
const SharedWebSocket = require("../../../../../main/js/joynr/messaging/websocket/SharedWebSocket");
const JoynrMessage = require("../../../../../main/js/joynr/messaging/JoynrMessage");
const MessageSerializer = require("../../../../../main/js/joynr/messaging/MessageSerializer");
const Request = require("../../../../../main/js/joynr/dispatching/types/Request");
const WebSocketAddress = require("../../../../../main/js/generated/joynr/system/RoutingTypes/WebSocketAddress");
const WebSocketClientAddress = require("../../../../../main/js/generated/joynr/system/RoutingTypes/WebSocketClientAddress");
const WebSocket = require("../../../../../test/js/global/WebSocketMock");

describe("libjoynr-js.joynr.messaging.webmessaging.SharedWebSocket", () => {
    let window = null;
    let localAddress;
    let ccAddress;
    let websocket = null;
    let sharedWebSocket = null;
    let data = null;
    let event = null;
    let joynrMessage = null;

    beforeEach(done => {
        const request = Request.create({
            methodName: "methodName"
        });

        joynrMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST,
            payload: JSON.stringify(request)
        });
        joynrMessage.from = "fromParticipantId";
        joynrMessage.to = "toParticipantId";
        joynrMessage.expiryDate = Date.now() + 10000;

        function Window() {}
        window = new Window();
        window.addEventListener = jasmine.createSpy("addEventListener");

        websocket = new WebSocket("ws://test");
        websocket.send = jasmine.createSpy("send");
        localAddress = new WebSocketClientAddress({
            id: "1234"
        });
        ccAddress = new WebSocketAddress({
            protocol: "ws",
            host: "host",
            port: 1234,
            path: "/test"
        });

        sharedWebSocket = new SharedWebSocket({
            localAddress,
            remoteAddress: ccAddress
        });

        function MessageEvent() {}
        event = new MessageEvent();
        data = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST
        });
        event.data = JSON.stringify(data);
        done();
    });

    it("is of correct type and has all members", done => {
        expect(SharedWebSocket).toBeDefined();
        expect(typeof SharedWebSocket === "function").toBeTruthy();
        expect(sharedWebSocket).toBeDefined();
        expect(sharedWebSocket instanceof SharedWebSocket).toBeTruthy();
        expect(sharedWebSocket.send).toBeDefined();
        expect(typeof sharedWebSocket.send === "function").toBeTruthy();
        done();
    });

    it("throws if arguments are missing or of wrong type", done => {
        expect(() => {
            sharedWebSocket = new SharedWebSocket({
                localAddress,
                remoteAddress: ccAddress
            });
        }).not.toThrow(); // correct
        // call

        expect(() => {
            sharedWebSocket.send(new JoynrMessage({}));
        }).not.toThrow(); // correct call
        expect(() => {
            sharedWebSocket.onmessage = undefined;
        }).toThrow(); // callback must be a function

        expect(() => {
            sharedWebSocket.onmessage = function() {};
        }).not.toThrow(); // correct call

        expect(() => {
            sharedWebSocket.onmessage = {};
        }).toThrow(); // callback must be a function
        done();
    });

    it("calls websocket.send correctly", done => {
        websocket.readyState = WebSocket.OPEN;
        sharedWebSocket.send(joynrMessage);
        expect(websocket.send).toHaveBeenCalledWith(MessageSerializer.stringify(joynrMessage), {
            binary: true
        });

        websocket.send.calls.reset();
        websocket.readyState = WebSocket.CLOSING;
        sharedWebSocket.send(joynrMessage);
        expect(websocket.send).not.toHaveBeenCalled();
        done();
    });

    it("uses callbacks after sharedWebSocket.send", done => {
        websocket.readyState = WebSocket.OPEN;
        sharedWebSocket.enableShutdownMode();
        sharedWebSocket.send(joynrMessage);
        expect(websocket.send).toHaveBeenCalledWith(
            MessageSerializer.stringify(joynrMessage),
            {
                binary: true
            },
            jasmine.any(Function)
        );

        websocket.send.calls.reset();
        websocket.readyState = WebSocket.CLOSING;
        sharedWebSocket.send(joynrMessage);
        expect(websocket.send).not.toHaveBeenCalled();
        done();
    });
});

describe("libjoynr-js.joynr.messaging.webmessaging.SharedWebSocket2", () => {
    let localAddress;
    let ccAddress;
    let sharedWebSocket = null;
    let joynrMessage = null;

    beforeEach(done => {
        const request = Request.create({
            methodName: "methodName"
        });

        joynrMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST,
            payload: JSON.stringify(request)
        });
        joynrMessage.from = "fromParticipantId";
        joynrMessage.to = "toParticipantId";
        joynrMessage.expiryDate = Date.now() + 10000;

        WebSocket.constructorReturnsNull = true;
        localAddress = new WebSocketClientAddress({
            id: "1234"
        });
        ccAddress = new WebSocketAddress({
            protocol: "ws",
            host: "host",
            port: 1234,
            path: "/test"
        });

        sharedWebSocket = new SharedWebSocket({
            localAddress,
            remoteAddress: ccAddress
        });
        done();
    });

    afterEach(done => {
        WebSocket.constructorReturnsNull = false;
        done();
    });

    it("queues messages when websocket connection is not available", done => {
        sharedWebSocket.send(joynrMessage);
        expect(sharedWebSocket.getNumberOfQueuedMessages()).toBe(1);
        done();
    });
});
