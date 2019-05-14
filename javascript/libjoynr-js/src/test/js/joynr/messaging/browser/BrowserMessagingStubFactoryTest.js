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
const BrowserMessagingStubFactory = require("../../../../../main/js/joynr/messaging/browser/BrowserMessagingStubFactory");

describe("libjoynr-js.joynr.messaging.browser.BrowserMessagingStubFactory", () => {
    let returnValue, webMessagingStub, browserMessagingStubFactory;
    let windowId, browserAddress, joynrMessage;

    beforeEach(done => {
        returnValue = {
            key: "returnValue"
        };
        function WebMessagingStub() {}
        webMessagingStub = new WebMessagingStub();
        webMessagingStub.transmit = jasmine.createSpy("transmit");
        webMessagingStub.transmit.and.returnValue(returnValue);
        browserMessagingStubFactory = new BrowserMessagingStubFactory({
            webMessagingStub
        });
        windowId = "windowId";
        function BrowserAddress() {}
        browserAddress = new BrowserAddress();
        browserAddress.windowId = windowId;
        function JoynrMessage() {}
        joynrMessage = new JoynrMessage();
        done();
    });

    it("is instantiable and of correct type", done => {
        expect(BrowserMessagingStubFactory).toBeDefined();
        expect(typeof BrowserMessagingStubFactory === "function").toBeTruthy();
        expect(browserMessagingStubFactory).toBeDefined();
        expect(browserMessagingStubFactory instanceof BrowserMessagingStubFactory).toBeTruthy();
        expect(browserMessagingStubFactory.build).toBeDefined();
        expect(typeof browserMessagingStubFactory.build === "function").toBeTruthy();
        done();
    });

    it("throws on missing or wrongly typed arguments in constructor", done => {
        expect(() => {
            browserMessagingStubFactory = new BrowserMessagingStubFactory();
        }).toThrow(); // settings is undefined
        expect(() => {
            browserMessagingStubFactory = new BrowserMessagingStubFactory("");
        }).toThrow(); // settings is of wrong type
        expect(() => {
            browserMessagingStubFactory = new BrowserMessagingStubFactory({});
        }).toThrow(); // webMessagingStub is missing
        expect(() => {
            browserMessagingStubFactory = new BrowserMessagingStubFactory({
                webMessagingStub: {}
            });
        }).toThrow(); // webMessagingStub is of wrong type
        expect(() => {
            browserMessagingStubFactory = new BrowserMessagingStubFactory({
                webMessagingStub: ""
            });
        }).toThrow(); // webMessagingStub is of wrong type
        expect(() => {
            browserMessagingStubFactory = new BrowserMessagingStubFactory({
                webMessagingStub
            });
        }).not.toThrow(); // correct call
        done();
    });

    it("throws on missing or wrongly typed arguments in build", done => {
        expect(() => {
            browserMessagingStubFactory.build();
        }).toThrow(); // address is undefined
        expect(() => {
            browserMessagingStubFactory.build(browserAddress);
        }).not.toThrow(); // correct call
        done();
    });

    it("creates a messaging stub and uses it correctly", done => {
        const browserMessagingStub = browserMessagingStubFactory.build(browserAddress);
        //expect(browserAddress.getTabId).toHaveBeenCalledWith();

        const result = browserMessagingStub.transmit(joynrMessage);
        expect(webMessagingStub.transmit).toHaveBeenCalledWith({
            windowId,
            message: joynrMessage
        });
        expect(result).toEqual(returnValue);
        done();
    });
});
