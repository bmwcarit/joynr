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

import WebSocketMessagingStub from "../../../../../main/js/joynr/messaging/websocket/WebSocketMessagingStub";

describe("libjoynr-js.joynr.messaging.webmessaging.WebSocketMessagingStub", () => {
    let webSocketMessagingStub: any;
    let joynrMessage: any;
    let sharedWebSocket: any;

    beforeEach(() => {
        sharedWebSocket = { send: jest.fn(), addEventListener: jest.fn() };

        webSocketMessagingStub = new WebSocketMessagingStub({
            sharedWebSocket
        });
        joynrMessage = {};
    });

    it("is of correct type and has all members", () => {
        expect(WebSocketMessagingStub).toBeDefined();
        expect(typeof WebSocketMessagingStub === "function").toBeTruthy();
        expect(webSocketMessagingStub).toBeDefined();
        expect(webSocketMessagingStub instanceof WebSocketMessagingStub).toBeTruthy();
        expect(webSocketMessagingStub.transmit).toBeDefined();
        expect(typeof webSocketMessagingStub.transmit === "function").toBeTruthy();
    });

    it("calls websocket.send correctly", () => {
        webSocketMessagingStub.transmit(joynrMessage);
        expect(sharedWebSocket.send).toHaveBeenCalledWith(joynrMessage);
    });
});
