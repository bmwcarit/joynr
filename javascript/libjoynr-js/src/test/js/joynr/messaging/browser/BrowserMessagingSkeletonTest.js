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
const BrowserMessagingSkeleton = require("../../../../../main/js/joynr/messaging/browser/BrowserMessagingSkeleton");
const JoynrMessage = require("../../../../../main/js/joynr/messaging/JoynrMessage");

describe("libjoynr-js.joynr.messaging.browser.BrowserMessagingSkeleton", () => {
    let webMessagingSkeleton, browserMessagingSkeleton, listener1, listener2;
    let windowId, joynrMessage, untypedJoynrMessage, browserMessage;

    beforeEach(done => {
        webMessagingSkeleton = jasmine.createSpyObj("webMessagingSkeleton", ["registerListener"]);

        browserMessagingSkeleton = new BrowserMessagingSkeleton({
            webMessagingSkeleton
        });

        listener1 = jasmine.createSpy("listener1");
        listener2 = jasmine.createSpy("listener2");

        //TODO: check if we hand over the source address of joynr messages to the receiver (message router)
        windowId = "mywindowId";
        function JoynrMessage() {}
        joynrMessage = new JoynrMessage();
        untypedJoynrMessage = JSON.parse(JSON.stringify(joynrMessage));
        browserMessage = {
            windowId,
            message: untypedJoynrMessage
        };
        done();
    });

    it("is of correct type and has all members", done => {
        expect(BrowserMessagingSkeleton).toBeDefined();
        expect(typeof BrowserMessagingSkeleton === "function").toBeTruthy();
        expect(browserMessagingSkeleton).toBeDefined();
        expect(browserMessagingSkeleton instanceof BrowserMessagingSkeleton).toBeTruthy();
        expect(browserMessagingSkeleton.registerListener).toBeDefined();
        expect(typeof browserMessagingSkeleton.registerListener === "function").toBeTruthy();
        expect(browserMessagingSkeleton.unregisterListener).toBeDefined();
        expect(typeof browserMessagingSkeleton.unregisterListener === "function").toBeTruthy();
        done();
    });

    it("throws if arguments are missing or of wrong type", done => {
        expect(() => {
            browserMessagingSkeleton = new BrowserMessagingSkeleton({
                webMessagingSkeleton
            });
        }).not.toThrow(); // correct call
        expect(() => {
            browserMessagingSkeleton = new BrowserMessagingSkeleton({});
        }).toThrow(); // webMessagingSkeleton is missing
        expect(() => {
            browserMessagingSkeleton = new BrowserMessagingSkeleton({
                webMessagingSkeleton: ""
            });
        }).toThrow(); // webMessagingSkeleton is of wrong type

        expect(() => {
            browserMessagingSkeleton.registerListener(() => {});
        }).not.toThrow(); // correct call
        expect(() => {
            browserMessagingSkeleton.registerListener("");
        }).toThrow(); // listener is of wrong type
        expect(() => {
            browserMessagingSkeleton.registerListener({});
        }).toThrow(); // listener is of wrong type

        expect(() => {
            browserMessagingSkeleton.unregisterListener(() => {});
        }).not.toThrow(); // correct call
        expect(() => {
            browserMessagingSkeleton.unregisterListener("");
        }).toThrow(); // listener is of wrong type
        expect(() => {
            browserMessagingSkeleton.unregisterListener({});
        }).toThrow(); // listener is of wrong type
        done();
    });

    function callAllRegisteredListeners(calls, browserMessage) {
        let i;

        for (i = 0; i < calls.count(); ++i) {
            calls.argsFor(i)[0](browserMessage);
        }
    }

    it("event calls through to registered listeners", done => {
        browserMessagingSkeleton.registerListener(listener1);
        browserMessagingSkeleton.registerListener(listener2);
        expect(listener1).not.toHaveBeenCalled();
        expect(listener2).not.toHaveBeenCalled();
        callAllRegisteredListeners(webMessagingSkeleton.registerListener.calls, browserMessage);
        expect(listener1).toHaveBeenCalledWith(jasmine.any(JoynrMessage));
        expect(listener2).toHaveBeenCalledWith(jasmine.any(JoynrMessage));
        expect(listener1.calls.count()).toBe(1);
        expect(listener2.calls.count()).toBe(1);
        done();
    });

    it("event does not call through to unregistered listeners", done => {
        browserMessagingSkeleton.registerListener(listener1);
        browserMessagingSkeleton.registerListener(listener2);
        browserMessagingSkeleton.unregisterListener(listener1);
        callAllRegisteredListeners(webMessagingSkeleton.registerListener.calls, browserMessage);
        browserMessagingSkeleton.unregisterListener(listener2);
        callAllRegisteredListeners(webMessagingSkeleton.registerListener.calls, browserMessage);

        expect(listener1).not.toHaveBeenCalled();
        expect(listener2).toHaveBeenCalled();
        expect(listener2.calls.count()).toBe(1);
        done();
    });
});
