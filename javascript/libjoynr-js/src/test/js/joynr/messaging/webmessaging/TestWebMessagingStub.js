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

define([ "joynr/messaging/webmessaging/WebMessagingStub"
], function(WebMessagingStub) {

    describe("libjoynr-js.joynr.messaging.webmessaging.WebMessagingStub", function() {

        var window, origin, webMessagingStub, joynrMessage;

        beforeEach(function() {
            function Window() {}
            window = new Window();
            window.postMessage = jasmine.createSpy("postMessage");
            origin = "defaultOrigin";

            webMessagingStub = new WebMessagingStub({
                window : window,
                origin : origin
            });

            function JoynrMessage() {}
            joynrMessage = new JoynrMessage();
        });

        it("is of correct type and has all members", function() {
            expect(WebMessagingStub).toBeDefined();
            expect(typeof WebMessagingStub === "function").toBeTruthy();
            expect(webMessagingStub).toBeDefined();
            expect(webMessagingStub instanceof WebMessagingStub).toBeTruthy();
            expect(webMessagingStub.transmit).toBeDefined();
            expect(typeof webMessagingStub.transmit === "function").toBeTruthy();
        });

        it("throws on missing or wrongly typed arguments in constructur", function() {
            expect(function() {
                webMessagingStub = new WebMessagingStub(); // settings object is undefined
            }).toThrow();

            expect(function() {
                webMessagingStub = new WebMessagingStub({}); // both arguments, window and origin are missing
            }).toThrow();

            expect(function() {
                webMessagingStub = new WebMessagingStub({
                    window : window
                // origin argument is missing
                });
            }).toThrow();

            expect(function() {
                webMessagingStub = new WebMessagingStub({
                    origin : origin
                // window argument is missing
                });
            }).toThrow();

            expect(function() {
                webMessagingStub = new WebMessagingStub({
                    window : "", // window is of wrong type
                    origin : origin
                });
            }).toThrow();

            expect(function() {
                webMessagingStub = new WebMessagingStub({
                    window : {}, // window does not provide the expected functions
                    origin : origin
                });
            }).toThrow();

            expect(function() {
                webMessagingStub = new WebMessagingStub({ // everything's fine here
                    window : {
                        postMessage : function() {}
                    },
                    origin : origin
                // origin is of wrong type
                });
            }).not.toThrow();

            expect(function() {
                webMessagingStub = new WebMessagingStub({ // everything's fine here
                    window : window,
                    origin : {}
                // origin is of wrong type
                });
            }).toThrow();

            expect(function() {
                webMessagingStub = new WebMessagingStub({ // everything's fine here
                    window : window,
                    origin : origin
                });
            }).not.toThrow();
        });

        it("throws on missing or wrongly typed arguments in transmit", function() {
            expect(function() {
                webMessagingStub.transmit(undefined);
            }).toThrow();
            expect(function() {
                webMessagingStub.transmit(null);
            }).toThrow();
            expect(function() {
                webMessagingStub.transmit("");
            }).toThrow();
            expect(function() {
                webMessagingStub.transmit({});
            }).toThrow();
            expect(function() {
                webMessagingStub.transmit({
                    message : joynrMessage
                });
            }).not.toThrow();
        });

        it("calls correctly window.postMessage correctly", function() {
            var param = {
                message : joynrMessage
            };
            webMessagingStub.transmit(param);
            expect(window.postMessage).toHaveBeenCalledWith(param, origin);
        });

    });
});
