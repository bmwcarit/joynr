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

import WebMessagingSkeleton from "../../../../../main/js/joynr/messaging/webmessaging/WebMessagingSkeleton";

describe("libjoynr-js.joynr.messaging.webmessaging.WebMessagingSkeleton", () => {
    let window: any, webMessagingSkeleton: WebMessagingSkeleton, listener1: any, listener2: any, data: any, event: any;

    beforeEach(done => {
        window = {
            addEventListener: jest.fn(),
            removeEventListener: jest.fn()
        };

        webMessagingSkeleton = new WebMessagingSkeleton({
            window
        });

        listener1 = jest.fn();
        listener2 = jest.fn();

        data = {
            key: "myData"
        };
        event = {
            data
        };
        done();
    });

    it("is of correct type and has all members", () => {
        expect(WebMessagingSkeleton).toBeDefined();
        expect(typeof WebMessagingSkeleton === "function").toBeTruthy();
        expect(webMessagingSkeleton).toBeDefined();
        expect(webMessagingSkeleton.registerListener).toBeDefined();
        expect(typeof webMessagingSkeleton.registerListener === "function").toBeTruthy();
        expect(webMessagingSkeleton.unregisterListener).toBeDefined();
        expect(typeof webMessagingSkeleton.unregisterListener === "function").toBeTruthy();
    });

    function callAllRegisteredListeners(mock: jest.Mock, event: any) {
        for (let i = 0; i < mock.mock.calls.length; ++i) {
            mock.mock.calls[i][1](event);
        }
    }

    it("event calls through to registered listeners", () => {
        webMessagingSkeleton.registerListener(listener1);
        webMessagingSkeleton.registerListener(listener2);
        expect(listener1).not.toHaveBeenCalled();
        expect(listener2).not.toHaveBeenCalled();
        callAllRegisteredListeners(window.addEventListener, event);
        expect(listener1).toHaveBeenCalledWith(data);
        expect(listener2).toHaveBeenCalledWith(data);
        expect(listener1.mock.calls.length).toBe(1);
        expect(listener2.mock.calls.length).toBe(1);
    });

    it("event does not call through to unregistered listeners", () => {
        webMessagingSkeleton.registerListener(listener1);
        webMessagingSkeleton.registerListener(listener2);
        webMessagingSkeleton.unregisterListener(listener1);
        callAllRegisteredListeners(window.addEventListener, event);
        webMessagingSkeleton.unregisterListener(listener2);
        callAllRegisteredListeners(window.addEventListener, event);

        expect(listener1).not.toHaveBeenCalled();
        expect(listener2).toHaveBeenCalled();
        expect(listener2.mock.calls.length).toBe(1);
    });
});
