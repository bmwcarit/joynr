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
const WebSocketMessagingSkeleton = require("../../../../../main/js/joynr/messaging/websocket/WebSocketMessagingSkeleton");
const JoynrMessage = require("../../../../../main/js/joynr/messaging/JoynrMessage");
const JSONSerializer = require("../../../../../main/js/joynr/util/JSONSerializer");
const MessageSerializer = require("../../../../../main/js/joynr/messaging/MessageSerializer");
const MulticastPublication = require("../../../../../main/js/joynr/dispatching/types/MulticastPublication");
const Request = require("../../../../../main/js/joynr/dispatching/types/Request");
const WebSocketAddress = require("../../../../../main/js/generated/joynr/system/RoutingTypes/WebSocketAddress");
const WebSocketClientAddress = require("../../../../../main/js/generated/joynr/system/RoutingTypes/WebSocketClientAddress");
const SharedWebSocket = require("../../../../../main/js/joynr/messaging/websocket/SharedWebSocket");

describe("libjoynr-js.joynr.messaging.websocket.WebSocketMessagingSkeleton", () => {
    let window = null;
    let sharedWebSocket = null;
    let webSocketMessagingSkeleton = null;
    let listener = null;
    let data = null;
    let event = null;
    let multicastEvent;
    const ccAddress = new WebSocketAddress({
        protocol: "ws",
        host: "host",
        port: 1234,
        path: "/test"
    });
    const localAddress = new WebSocketClientAddress({
        id: "1234"
    });

    beforeEach(done => {
        function Window() {}
        window = new Window();
        window.addEventListener = jasmine.createSpy("addEventListener");

        sharedWebSocket = new SharedWebSocket({
            remoteAddress: ccAddress,
            localAddress
        });

        spyOn(sharedWebSocket, "send").and.callThrough();

        webSocketMessagingSkeleton = new WebSocketMessagingSkeleton({
            sharedWebSocket,
            mainTransport: true
        });

        listener = jasmine.createSpy("listener");
        function MessageEvent() {}
        event = new MessageEvent();
        const request = Request.create({
            methodName: "methodName"
        });
        const joynrMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST,
            payload: JSON.stringify(request)
        });
        joynrMessage.from = "fromParticipantId";
        joynrMessage.to = "toParticipantId";
        joynrMessage.expiryDate = Date.now() + 10000;
        data = joynrMessage;
        multicastEvent = new MessageEvent();
        if (typeof Buffer === "function") {
            // node environment
            event.data = MessageSerializer.stringify(joynrMessage);
            event.target = {
                binaryType: "arraybuffer"
            };
            const multicastId = "multicastId";
            const publication = MulticastPublication.create({
                response: ["test"],
                multicastId
            });

            const multicastJoynrMessage = new JoynrMessage({
                type: JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST,
                payload: JSONSerializer.stringify(publication)
            });
            multicastJoynrMessage.from = "fromParticipantId";
            multicastJoynrMessage.to = "toParticipantId";
            multicastJoynrMessage.expiryDate = Date.now() + 10000;
            multicastEvent.data = MessageSerializer.stringify(multicastJoynrMessage);
        } else if (typeof TextEncoder === "function") {
            // browser
            const textEncoder = new TextEncoder();
            event.data = textEncoder.encode(JSON.stringify(data));
            event.target = {
                binaryType: "arraybuffer"
            };
            multicastEvent.data = textEncoder.encode(
                JSON.stringify(
                    new JoynrMessage({
                        type: JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST
                    })
                )
            );
        } else {
            // browser without TextDecoder
            done.fail("error in beforeEach: Buffer/TextEncoder not supported");
        }
        done();
    });

    it("is of correct type and has all members", done => {
        expect(WebSocketMessagingSkeleton).toBeDefined();
        expect(typeof WebSocketMessagingSkeleton === "function").toBeTruthy();
        expect(webSocketMessagingSkeleton).toBeDefined();
        expect(webSocketMessagingSkeleton instanceof WebSocketMessagingSkeleton).toBeTruthy();
        expect(webSocketMessagingSkeleton.registerListener).toBeDefined();
        expect(typeof webSocketMessagingSkeleton.registerListener === "function").toBeTruthy();
        expect(webSocketMessagingSkeleton.unregisterListener).toBeDefined();
        expect(typeof webSocketMessagingSkeleton.unregisterListener === "function").toBeTruthy();
        done();
    });

    it("throws if arguments are missing or of wrong type", done => {
        expect(() => {
            webSocketMessagingSkeleton = new WebSocketMessagingSkeleton({
                sharedWebSocket,
                mainTransport: true
            });
        }).not.toThrow(); // correct call
        expect(() => {
            webSocketMessagingSkeleton = new WebSocketMessagingSkeleton({
                sharedWebSocket
            });
        }).toThrow(); // mainTransport missing
        expect(() => {
            webSocketMessagingSkeleton = new WebSocketMessagingSkeleton({});
        }).toThrow(); // incorrect call

        expect(() => {
            webSocketMessagingSkeleton.registerListener(() => {});
        }).not.toThrow(); // correct call
        expect(() => {
            webSocketMessagingSkeleton.registerListener("");
        }).toThrow(); // listener is of wrong type
        expect(() => {
            webSocketMessagingSkeleton.registerListener({});
        }).toThrow(); // listener is of wrong type

        expect(() => {
            webSocketMessagingSkeleton.unregisterListener(() => {});
        }).not.toThrow(); // correct call
        expect(() => {
            webSocketMessagingSkeleton.unregisterListener("");
        }).toThrow(); // listener is of wrong type
        expect(() => {
            webSocketMessagingSkeleton.unregisterListener({});
        }).toThrow(); // listener is of wrong type
        done();
    });

    it("event calls through to registered listener", () => {
        webSocketMessagingSkeleton.registerListener(listener);
        expect(listener).not.toHaveBeenCalled();

        sharedWebSocket.onmessage(event);

        expect(listener.calls.argsFor(0)[0].id).toBe(data.id);
        expect(listener.calls.argsFor(0)[0].from).toBe(data.from);
        expect(listener.calls.argsFor(0)[0].to).toBe(data.to);
        expect(listener.calls.count()).toBe(1);
    });

    it("event does not call through to unregistered listener", () => {
        webSocketMessagingSkeleton.registerListener(listener);
        sharedWebSocket.onmessage(event);
        expect(listener).toHaveBeenCalled();
        expect(listener.calls.count()).toBe(1);
        webSocketMessagingSkeleton.unregisterListener(listener);
        sharedWebSocket.onmessage(event);
        expect(listener.calls.count()).toBe(1);
    });

    function receiveMessageAndCheckForIsReceivedFromGlobalFlag(expectedValue) {
        webSocketMessagingSkeleton.registerListener(listener);

        sharedWebSocket.onmessage(multicastEvent);

        expect(listener).toHaveBeenCalled();
        expect(listener.calls.count()).toBe(1);
        expect(listener.calls.argsFor(0)[0].isReceivedFromGlobal).toBe(expectedValue);
    }

    it("sets isReceivedFromGlobal if web socket is main transport", () => {
        receiveMessageAndCheckForIsReceivedFromGlobalFlag(true);
    });

    it("does not set isReceivedFromGlobal if web socket is NOT main transport", () => {
        webSocketMessagingSkeleton = new WebSocketMessagingSkeleton({
            sharedWebSocket,
            mainTransport: false
        });

        receiveMessageAndCheckForIsReceivedFromGlobalFlag(false);
    });
});
