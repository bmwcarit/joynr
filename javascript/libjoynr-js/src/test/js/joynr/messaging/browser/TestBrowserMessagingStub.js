/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

define([ "joynr/messaging/browser/BrowserMessagingStub"
], function(BrowserMessagingStub) {

    describe("libjoynr-js.joynr.messaging.browser.BrowserMessagingStub", function() {

        var webMessagingStub, browserMessagingStub, windowId, joynrMessage;

        beforeEach(function() {
            function WebMessagingStub() {}
            webMessagingStub = new WebMessagingStub();
            webMessagingStub.transmit = jasmine.createSpy("transmit");

            browserMessagingStub = new BrowserMessagingStub({
                webMessagingStub : webMessagingStub
            });

            windowId = "mywindowId";
            function JoynrMessage() {}
            joynrMessage = new JoynrMessage();
        });

        it("is of correct type and has all members", function() {
            expect(BrowserMessagingStub).toBeDefined();
            expect(typeof BrowserMessagingStub === "function").toBeTruthy();
            expect(browserMessagingStub).toBeDefined();
            expect(browserMessagingStub instanceof BrowserMessagingStub).toBeTruthy();
            expect(browserMessagingStub.transmit).toBeDefined();
            expect(typeof browserMessagingStub.transmit === "function").toBeTruthy();
        });

        it("throws on missing or wrongly typed arguments in constructur", function() {
            expect(function() {
                browserMessagingStub = new BrowserMessagingStub(); // settings object is undefined
            }).toThrow();

            expect(function() {
                browserMessagingStub = new BrowserMessagingStub(""); // settings object is of wrong type
            }).toThrow();

            expect(function() {
                browserMessagingStub = new BrowserMessagingStub({}); // webMessagingStub is missing
            }).toThrow();

            expect(function() {
                browserMessagingStub = new BrowserMessagingStub({
                    webMessagingStub : "" // webMessagingStub is of wrong type
                });
            }).toThrow();

            expect(function() {
                browserMessagingStub = new BrowserMessagingStub({ // everything's fine here
                    webMessagingStub : webMessagingStub
                });
            }).not.toThrow();
        });

        it("throws on missing or wrongly typed arguments in transmit", function() {
            expect(function() {
                browserMessagingStub.transmit(undefined);
            }).toThrow();
            expect(function() {
                browserMessagingStub.transmit(null);
            }).toThrow();
            expect(function() {
                browserMessagingStub.transmit("");
            }).toThrow();
            expect(function() {
                browserMessagingStub.transmit({});
            }).toThrow();
            expect(function() {
                browserMessagingStub.transmit(joynrMessage);
            }).not.toThrow();
        });

        it("calls correctly webMessagingStub.transmit correctly", function() {
            browserMessagingStub.transmit(joynrMessage);
            expect(webMessagingStub.transmit).toHaveBeenCalledWith({
                message : joynrMessage
            });
        });

        it("calls correctly webMessagingStub.transmit with windowId correctly", function() {
            browserMessagingStub = new BrowserMessagingStub({
                windowId : windowId,
                webMessagingStub : webMessagingStub
            });

            browserMessagingStub.transmit(joynrMessage);
            expect(webMessagingStub.transmit).toHaveBeenCalledWith({
                windowId : windowId,
                message : joynrMessage
            });
        });

    });

});
