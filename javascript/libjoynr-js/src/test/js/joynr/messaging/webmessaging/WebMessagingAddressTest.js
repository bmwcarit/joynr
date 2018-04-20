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
const WebMessagingAddress = require("../../../../../main/js/joynr/messaging/webmessaging/WebMessagingAddress");

describe("libjoynr-js.joynr.messaging.webmessaging.WebMessagingAddress", () => {
    let window, origin, webMessagingAddress;

    beforeEach(() => {
        window = {
            key: "window"
        };
        origin = "origin";
        webMessagingAddress = new WebMessagingAddress({
            window,
            origin
        });
    });

    it("is instantiable and of correct type", () => {
        expect(WebMessagingAddress).toBeDefined();
        expect(typeof WebMessagingAddress === "function").toBeTruthy();
        expect(webMessagingAddress).toBeDefined();
        expect(webMessagingAddress instanceof WebMessagingAddress).toBeTruthy();
        expect(webMessagingAddress.getWindow).toBeDefined();
        expect(typeof webMessagingAddress.getWindow === "function").toBeTruthy();
        expect(webMessagingAddress.getOrigin).toBeDefined();
        expect(typeof webMessagingAddress.getOrigin === "function").toBeTruthy();
    });

    it("throws on missing or wrongly typed arguments in transmit", () => {
        expect(() => {
            webMessagingAddress = new WebMessagingAddress({
                // correct arguments
                window,
                origin
            });
        }).not.toThrow();

        expect(() => {
            webMessagingAddress = new WebMessagingAddress({
                // window is of wrong type
                window: "",
                origin
            });
        }).toThrow();

        expect(() => {
            webMessagingAddress = new WebMessagingAddress({
                // origin is of wrong type
                window,
                origin: {}
            });
        }).toThrow();

        expect(() => {
            webMessagingAddress = new WebMessagingAddress({
                // missing window argument
                origin
            });
        }).toThrow();

        expect(() => {
            webMessagingAddress = new WebMessagingAddress({
                // missing origin argument
                window
            });
        }).toThrow();

        expect(() => {
            webMessagingAddress = new WebMessagingAddress({});
        }).toThrow(); // all arguments are missing

        expect(() => {
            webMessagingAddress = new WebMessagingAddress("");
        }).toThrow(); // settings is of wrong type

        expect(() => {
            webMessagingAddress = new WebMessagingAddress();
        }).toThrow(); // settings is undefined
    });

    it("retrieves window correctly", () => {
        expect(webMessagingAddress.getWindow()).toEqual(window);
    });

    it("retrieves origin correctly", () => {
        expect(webMessagingAddress.getOrigin()).toEqual(origin);
    });
});
