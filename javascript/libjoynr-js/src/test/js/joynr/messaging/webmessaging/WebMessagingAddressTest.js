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
var WebMessagingAddress = require("../../../../classes/joynr/messaging/webmessaging/WebMessagingAddress");

describe("libjoynr-js.joynr.messaging.webmessaging.WebMessagingAddress", function() {
    var window, origin, webMessagingAddress;

    beforeEach(function() {
        window = {
            key: "window"
        };
        origin = "origin";
        webMessagingAddress = new WebMessagingAddress({
            window: window,
            origin: origin
        });
    });

    it("is instantiable and of correct type", function() {
        expect(WebMessagingAddress).toBeDefined();
        expect(typeof WebMessagingAddress === "function").toBeTruthy();
        expect(webMessagingAddress).toBeDefined();
        expect(webMessagingAddress instanceof WebMessagingAddress).toBeTruthy();
        expect(webMessagingAddress.getWindow).toBeDefined();
        expect(typeof webMessagingAddress.getWindow === "function").toBeTruthy();
        expect(webMessagingAddress.getOrigin).toBeDefined();
        expect(typeof webMessagingAddress.getOrigin === "function").toBeTruthy();
    });

    it("throws on missing or wrongly typed arguments in transmit", function() {
        expect(function() {
            webMessagingAddress = new WebMessagingAddress({
                // correct arguments
                window: window,
                origin: origin
            });
        }).not.toThrow();

        expect(function() {
            webMessagingAddress = new WebMessagingAddress({
                // window is of wrong type
                window: "",
                origin: origin
            });
        }).toThrow();

        expect(function() {
            webMessagingAddress = new WebMessagingAddress({
                // origin is of wrong type
                window: window,
                origin: {}
            });
        }).toThrow();

        expect(function() {
            webMessagingAddress = new WebMessagingAddress({
                // missing window argument
                origin: origin
            });
        }).toThrow();

        expect(function() {
            webMessagingAddress = new WebMessagingAddress({
                // missing origin argument
                window: window
            });
        }).toThrow();

        expect(function() {
            webMessagingAddress = new WebMessagingAddress({});
        }).toThrow(); // all arguments are missing

        expect(function() {
            webMessagingAddress = new WebMessagingAddress("");
        }).toThrow(); // settings is of wrong type

        expect(function() {
            webMessagingAddress = new WebMessagingAddress();
        }).toThrow(); // settings is undefined
    });

    it("retrieves window correctly", function() {
        expect(webMessagingAddress.getWindow()).toEqual(window);
    });

    it("retrieves origin correctly", function() {
        expect(webMessagingAddress.getOrigin()).toEqual(origin);
    });
});
