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
require("../../../node-unit-test-helper");
var SharedWebSocket = require("../../../../../main/js/joynr/messaging/websocket/SharedWebSocket");
var JoynrMessage = require("../../../../../main/js/joynr/messaging/JoynrMessage");
var WebSocketAddress = require("../../../../../main/js/generated/joynr/system/RoutingTypes/WebSocketAddress");
var WebSocketClientAddress = require("../../../../../main/js/generated/joynr/system/RoutingTypes/WebSocketClientAddress");
var WebSocket = require("../../../../../test/js/global/WebSocketMock");

describe("libjoynr-js.joynr.messaging.webmessaging.SharedWebSocket", function() {
    var window = null;
    var localAddress;
    var ccAddress;
    var websocket = null;
    var sharedWebSocket = null;
    var listener1 = null;
    var listener2 = null;
    var data = null;
    var event = null;
    var joynrMessage = null;

    beforeEach(function(done) {
        function JoynrMessage() {}
        joynrMessage = new JoynrMessage();

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
            localAddress: localAddress,
            remoteAddress: ccAddress
        });

        listener1 = jasmine.createSpy("listener1");
        listener2 = jasmine.createSpy("listener2");
        function MessageEvent() {}
        event = new MessageEvent();
        data = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST
        });
        event.data = JSON.stringify(data);
        done();
    });

    it("is of correct type and has all members", function(done) {
        expect(SharedWebSocket).toBeDefined();
        expect(typeof SharedWebSocket === "function").toBeTruthy();
        expect(sharedWebSocket).toBeDefined();
        expect(sharedWebSocket instanceof SharedWebSocket).toBeTruthy();
        expect(sharedWebSocket.send).toBeDefined();
        expect(typeof sharedWebSocket.send === "function").toBeTruthy();
        done();
    });

    it("throws if arguments are missing or of wrong type", function(done) {
        expect(function() {
            sharedWebSocket = new SharedWebSocket({
                localAddress: localAddress,
                remoteAddress: ccAddress
            });
        }).not.toThrow(); // correct
        // call

        expect(function() {
            sharedWebSocket.send(new JoynrMessage({}));
        }).not.toThrow(); // correct call
        expect(function() {
            sharedWebSocket.onmessage = undefined;
        }).toThrow(); // callback must be a function

        expect(function() {
            sharedWebSocket.onmessage = function() {};
        }).not.toThrow(); // correct call

        expect(function() {
            sharedWebSocket.onmessage = {};
        }).toThrow(); // callback must be a function
        done();
    });

    it("calls websocket.send correctly", function(done) {
        websocket.readyState = WebSocket.OPEN;
        sharedWebSocket.send(joynrMessage);
        expect(websocket.send).toHaveBeenCalledWith(websocket.marshalJoynrMessage(joynrMessage), {
            binary: true
        });

        websocket.send.calls.reset();
        websocket.readyState = WebSocket.CLOSING;
        sharedWebSocket.send(joynrMessage);
        expect(websocket.send).not.toHaveBeenCalled();
        done();
    });
});
