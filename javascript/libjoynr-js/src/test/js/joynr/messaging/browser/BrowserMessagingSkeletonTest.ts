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

import BrowserMessagingSkeleton from "../../../../../main/js/joynr/messaging/browser/BrowserMessagingSkeleton";
import JoynrMessage from "../../../../../main/js/joynr/messaging/JoynrMessage";

describe("libjoynr-js.joynr.messaging.browser.BrowserMessagingSkeleton", () => {
    let webMessagingSkeleton: any, browserMessagingSkeleton: BrowserMessagingSkeleton, listener1: any, listener2: any;
    let windowId: any, untypedJoynrMessage: any, browserMessage: any;

    beforeEach(done => {
        webMessagingSkeleton = {
            registerListener: jest.fn()
        };

        browserMessagingSkeleton = new BrowserMessagingSkeleton({
            webMessagingSkeleton
        });

        listener1 = jest.fn();
        listener2 = jest.fn();

        //TODO: check if we hand over the source address of joynr messages to the receiver (message router)
        windowId = "mywindowId";
        untypedJoynrMessage = {};
        browserMessage = {
            windowId,
            message: untypedJoynrMessage
        };
        done();
    });

    it("is of correct type and has all members", () => {
        expect(BrowserMessagingSkeleton).toBeDefined();
        expect(typeof BrowserMessagingSkeleton === "function").toBeTruthy();
        expect(browserMessagingSkeleton).toBeDefined();
        expect(browserMessagingSkeleton.registerListener).toBeDefined();
        expect(typeof browserMessagingSkeleton.registerListener === "function").toBeTruthy();
        expect(browserMessagingSkeleton.unregisterListener).toBeDefined();
        expect(typeof browserMessagingSkeleton.unregisterListener === "function").toBeTruthy();
    });

    it("throws if arguments are missing or of wrong type", () => {
        expect(() => {
            browserMessagingSkeleton = new BrowserMessagingSkeleton({
                webMessagingSkeleton
            });
        }).not.toThrow(); // correct call
        expect(() => {
            browserMessagingSkeleton = new BrowserMessagingSkeleton({} as any);
        }).toThrow(); // webMessagingSkeleton is missing
        expect(() => {
            browserMessagingSkeleton = new BrowserMessagingSkeleton({
                webMessagingSkeleton: ""
            } as any);
        }).toThrow(); // webMessagingSkeleton is of wrong type

        expect(() => {
            browserMessagingSkeleton.registerListener(() => {
                // do nothing
            });
        }).not.toThrow(); // correct call
        expect(() => {
            browserMessagingSkeleton.registerListener("" as any);
        }).toThrow(); // listener is of wrong type
        expect(() => {
            browserMessagingSkeleton.registerListener({} as any);
        }).toThrow(); // listener is of wrong type

        expect(() => {
            browserMessagingSkeleton.unregisterListener(() => {
                // do nothing
            });
        }).not.toThrow(); // correct call
        expect(() => {
            browserMessagingSkeleton.unregisterListener("" as any);
        }).toThrow(); // listener is of wrong type
        expect(() => {
            browserMessagingSkeleton.unregisterListener({} as any);
        }).toThrow(); // listener is of wrong type
    });

    function callAllRegisteredListeners(mock: jest.Mock, browserMessage: any) {
        for (let i = 0; i < mock.mock.calls.length; ++i) {
            mock.mock.calls[i][0](browserMessage);
        }
    }

    it("event calls through to registered listeners", () => {
        browserMessagingSkeleton.registerListener(listener1);
        browserMessagingSkeleton.registerListener(listener2);
        expect(listener1).not.toHaveBeenCalled();
        expect(listener2).not.toHaveBeenCalled();
        callAllRegisteredListeners(webMessagingSkeleton.registerListener, browserMessage);
        expect(listener1).toHaveBeenCalledWith(expect.any(JoynrMessage));
        expect(listener2).toHaveBeenCalledWith(expect.any(JoynrMessage));
        expect(listener1.mock.calls.length).toBe(1);
        expect(listener2.mock.calls.length).toBe(1);
    });

    it("event does not call through to unregistered listeners", () => {
        browserMessagingSkeleton.registerListener(listener1);
        browserMessagingSkeleton.registerListener(listener2);
        browserMessagingSkeleton.unregisterListener(listener1);
        callAllRegisteredListeners(webMessagingSkeleton.registerListener, browserMessage);
        browserMessagingSkeleton.unregisterListener(listener2);
        callAllRegisteredListeners(webMessagingSkeleton.registerListener, browserMessage);

        expect(listener1).not.toHaveBeenCalled();
        expect(listener2).toHaveBeenCalled();
        expect(listener2.mock.calls.length).toBe(1);
    });
});
