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

import WebSocketMessagingSkeleton from "../../../../../main/js/joynr/messaging/websocket/WebSocketMessagingSkeleton";
import JoynrMessage from "../../../../../main/js/joynr/messaging/JoynrMessage";

describe("libjoynr-js.joynr.messaging.websocket.WebSocketMessagingSkeleton", () => {
    let sharedWebSocket: any;
    let webSocketMessagingSkeleton: any;
    let listener: any;
    let joynrMessage: any;
    let multicastJoynrMessage: any;

    beforeEach(() => {
        sharedWebSocket = { send: jest.fn(), addEventListener: jest.fn() };

        webSocketMessagingSkeleton = new WebSocketMessagingSkeleton({
            sharedWebSocket,
            mainTransport: true
        });

        listener = jest.fn();
        joynrMessage = JoynrMessage.parseMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST
        });
        multicastJoynrMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST,
            payload: ""
        });
    });

    it("is of correct type and has all members", () => {
        expect(WebSocketMessagingSkeleton).toBeDefined();
        expect(typeof WebSocketMessagingSkeleton === "function").toBeTruthy();
        expect(webSocketMessagingSkeleton).toBeDefined();
        expect(webSocketMessagingSkeleton instanceof WebSocketMessagingSkeleton).toBeTruthy();
        expect(webSocketMessagingSkeleton.registerListener).toBeDefined();
        expect(typeof webSocketMessagingSkeleton.registerListener === "function").toBeTruthy();
        expect(webSocketMessagingSkeleton.unregisterListener).toBeDefined();
        expect(typeof webSocketMessagingSkeleton.unregisterListener === "function").toBeTruthy();
    });

    it("event calls through to registered listener", () => {
        webSocketMessagingSkeleton.registerListener(listener);
        expect(listener).not.toHaveBeenCalled();

        sharedWebSocket.onmessage(joynrMessage);

        expect(listener).toHaveBeenCalledWith(joynrMessage);
        expect(listener.mock.calls.length).toBe(1);
    });

    it("event does not call through to unregistered listener", () => {
        webSocketMessagingSkeleton.registerListener(listener);
        sharedWebSocket.onmessage(joynrMessage);
        expect(listener).toHaveBeenCalled();
        expect(listener.mock.calls.length).toBe(1);
        webSocketMessagingSkeleton.unregisterListener(listener);
        sharedWebSocket.onmessage(joynrMessage);
        expect(listener.mock.calls.length).toBe(1);
    });

    function receiveMessageAndCheckForIsReceivedFromGlobalFlag(expectedValue: any) {
        webSocketMessagingSkeleton.registerListener(listener);

        sharedWebSocket.onmessage(multicastJoynrMessage);

        expect(listener).toHaveBeenCalled();
        expect(listener.mock.calls.length).toBe(1);
        expect(listener.mock.calls[0][0].isReceivedFromGlobal).toBe(expectedValue);
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
