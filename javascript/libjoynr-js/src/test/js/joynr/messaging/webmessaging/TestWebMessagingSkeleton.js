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

define([ "joynr/messaging/webmessaging/WebMessagingSkeleton"
], function(WebMessagingSkeleton) {

    describe("libjoynr-js.joynr.messaging.webmessaging.WebMessagingSkeleton", function() {

        var window, webMessagingSkeleton, listener1, listener2, data, event;

        beforeEach(function() {
            function Window() {}
            window = new Window();
            window.addEventListener = jasmine.createSpy("addEventListener");
            window.removeEventListener = jasmine.createSpy("removeEventListener");

            webMessagingSkeleton = new WebMessagingSkeleton({
                window : window
            });

            listener1 = jasmine.createSpy();
            listener2 = jasmine.createSpy();

            data = {
                key : "myData"
            };
            event = {
                data : data
            };
        });

        it("is of correct type and has all members", function() {
            expect(WebMessagingSkeleton).toBeDefined();
            expect(typeof WebMessagingSkeleton === "function").toBeTruthy();
            expect(webMessagingSkeleton).toBeDefined();
            expect(webMessagingSkeleton instanceof WebMessagingSkeleton).toBeTruthy();
            expect(webMessagingSkeleton.registerListener).toBeDefined();
            expect(typeof webMessagingSkeleton.registerListener === "function").toBeTruthy();
            expect(webMessagingSkeleton.unregisterListener).toBeDefined();
            expect(typeof webMessagingSkeleton.unregisterListener === "function").toBeTruthy();
        });

        it("throws if arguments are missing or of wrong type", function() {
            expect(function() {
                webMessagingSkeleton = new WebMessagingSkeleton({
                    window : window
                });
            }).not.toThrow(); // correct call
            expect(function() {
                webMessagingSkeleton = new WebMessagingSkeleton({});
            }).toThrow(); // window is missing
            expect(function() {
                webMessagingSkeleton = new WebMessagingSkeleton({
                    window : ""
                });
            }).toThrow(); // window is of wrong type

            expect(function() {
                webMessagingSkeleton = new WebMessagingSkeleton({
                    window : {}
                });
            }).toThrow(); // window does not provide the expected functions

            expect(function() {
                webMessagingSkeleton = new WebMessagingSkeleton({
                    window : {
                        addEventListener : function() {}
                    }
                });
            }).toThrow(); // window does not provide the expected functions

            expect(function() {
                webMessagingSkeleton = new WebMessagingSkeleton({
                    window : {
                        addEventListener : function() {},
                        removeEventListener : function() {}
                    }
                });
            }).not.toThrow(); // window does not provide the expected functions

            expect(function() {
                webMessagingSkeleton.registerListener(function() {});
            }).not.toThrow(); // correct call
            expect(function() {
                webMessagingSkeleton.registerListener("");
            }).toThrow(); // listener is of wrong type
            expect(function() {
                webMessagingSkeleton.registerListener({});
            }).toThrow(); // listener is of wrong type

            expect(function() {
                webMessagingSkeleton.unregisterListener(function() {});
            }).not.toThrow(); // correct call
            expect(function() {
                webMessagingSkeleton.unregisterListener("");
            }).toThrow(); // listener is of wrong type
            expect(function() {
                webMessagingSkeleton.unregisterListener({});
            }).toThrow(); // listener is of wrong type
        });

        function callAllRegisteredListeners(calls, event) {
            var i;

            for (i = 0; i < calls.length; ++i) {
                calls[i].args[1](event);
            }
        }

        it("event calls through to registered listeners", function() {
            webMessagingSkeleton.registerListener(listener1);
            webMessagingSkeleton.registerListener(listener2);
            expect(listener1).not.toHaveBeenCalled();
            expect(listener2).not.toHaveBeenCalled();
            callAllRegisteredListeners(window.addEventListener.calls, event);
            expect(listener1).toHaveBeenCalledWith(data);
            expect(listener2).toHaveBeenCalledWith(data);
            expect(listener1.calls.length).toBe(1);
            expect(listener2.calls.length).toBe(1);
        });

        it("event does not call through to unregistered listeners", function() {
            webMessagingSkeleton.registerListener(listener1);
            webMessagingSkeleton.registerListener(listener2);
            webMessagingSkeleton.unregisterListener(listener1);
            callAllRegisteredListeners(window.addEventListener.calls, event);
            webMessagingSkeleton.unregisterListener(listener2);
            callAllRegisteredListeners(window.addEventListener.calls, event);

            expect(listener1).not.toHaveBeenCalled();
            expect(listener2).toHaveBeenCalled();
            expect(listener2.calls.length).toBe(1);
        });

    });
});
