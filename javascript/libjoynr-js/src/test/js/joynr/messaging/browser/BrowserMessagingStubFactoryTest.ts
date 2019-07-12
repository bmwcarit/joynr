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

import BrowserMessagingStubFactory from "../../../../../main/js/joynr/messaging/browser/BrowserMessagingStubFactory";

describe("libjoynr-js.joynr.messaging.browser.BrowserMessagingStubFactory", () => {
    let returnValue: any, webMessagingStub: any, browserMessagingStubFactory: BrowserMessagingStubFactory;
    let windowId: any, browserAddress: any, joynrMessage: any;

    beforeEach(done => {
        returnValue = {
            key: "returnValue"
        };
        class WebMessagingStub {}
        webMessagingStub = new WebMessagingStub();
        webMessagingStub.transmit = jest.fn();
        webMessagingStub.transmit.mockReturnValue(returnValue);
        browserMessagingStubFactory = new BrowserMessagingStubFactory({
            webMessagingStub
        });
        windowId = "windowId";
        class BrowserAddress {}
        browserAddress = new BrowserAddress();
        browserAddress.windowId = windowId;
        class JoynrMessage {}
        joynrMessage = new JoynrMessage();
        done();
    });

    it("is instantiable and of correct type", () => {
        expect(BrowserMessagingStubFactory).toBeDefined();
        expect(typeof BrowserMessagingStubFactory === "function").toBeTruthy();
        expect(browserMessagingStubFactory).toBeDefined();
        expect(browserMessagingStubFactory instanceof BrowserMessagingStubFactory).toBeTruthy();
        expect(browserMessagingStubFactory.build).toBeDefined();
        expect(typeof browserMessagingStubFactory.build === "function").toBeTruthy();
    });

    it("throws on missing or wrongly typed arguments in constructor", () => {
        expect(() => {
            browserMessagingStubFactory = new BrowserMessagingStubFactory(undefined as any);
        }).toThrow(); // settings is undefined
        expect(() => {
            browserMessagingStubFactory = new BrowserMessagingStubFactory("" as any);
        }).toThrow(); // settings is of wrong type
        expect(() => {
            browserMessagingStubFactory = new BrowserMessagingStubFactory({} as any);
        }).toThrow(); // webMessagingStub is missing
        expect(() => {
            browserMessagingStubFactory = new BrowserMessagingStubFactory({
                webMessagingStub: {}
            } as any);
        }).toThrow(); // webMessagingStub is of wrong type
        expect(() => {
            browserMessagingStubFactory = new BrowserMessagingStubFactory({
                webMessagingStub: ""
            } as any);
        }).toThrow(); // webMessagingStub is of wrong type
        expect(() => {
            browserMessagingStubFactory = new BrowserMessagingStubFactory({
                webMessagingStub
            });
        }).not.toThrow(); // correct call
    });

    it("throws on missing or wrongly typed arguments in build", () => {
        expect(() => {
            browserMessagingStubFactory.build(undefined as any);
        }).toThrow(); // address is undefined
        expect(() => {
            browserMessagingStubFactory.build(browserAddress);
        }).not.toThrow(); // correct call
    });

    it("creates a messaging stub and uses it correctly", () => {
        const browserMessagingStub = browserMessagingStubFactory.build(browserAddress);
        //expect(browserAddress.getTabId).toHaveBeenCalledWith();

        const result = browserMessagingStub.transmit(joynrMessage);
        expect(webMessagingStub.transmit).toHaveBeenCalledWith({
            windowId,
            message: joynrMessage
        });
        expect(result).toEqual(returnValue);
    });
});
