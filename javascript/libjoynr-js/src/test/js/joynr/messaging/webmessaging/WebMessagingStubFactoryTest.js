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

define([ "joynr/messaging/webmessaging/WebMessagingStubFactory"
], function(WebMessagingStubFactory) {

    describe("libjoynr-js.joynr.messaging.webmessaging.WebMessagingStubFactory", function() {
        var returnValue;
        var webMessagingSender;
        var webMessagingStubFactory;
        var window;
        var origin;
        var webMessagingAddress;
        var joynrMessage;

        beforeEach(function() {
            returnValue = {
                key : "returnValue"
            };
            webMessagingStubFactory = new WebMessagingStubFactory();

            function Window() {}
            window = new Window();
            window.postMessage = jasmine.createSpy("postMessage");

            origin = "origin";
            webMessagingAddress = jasmine.createSpyObj("webMessagingAddress", [
                "getWindow",
                "getOrigin"
            ]);
            webMessagingAddress.getWindow.andReturn(window);
            webMessagingAddress.getOrigin.andReturn(origin);
            function JoynrMessage() {}
            joynrMessage = new JoynrMessage();
        });

        it("is instantiable and of correct type", function() {
            expect(WebMessagingStubFactory).toBeDefined();
            expect(typeof WebMessagingStubFactory === "function").toBeTruthy();
            expect(webMessagingStubFactory).toBeDefined();
            expect(webMessagingStubFactory instanceof WebMessagingStubFactory).toBeTruthy();
            expect(webMessagingStubFactory.build).toBeDefined();
            expect(typeof webMessagingStubFactory.build === "function").toBeTruthy();
        });

        it("creates a messaging stub and uses it correctly", function() {
            var webMessagingStub = webMessagingStubFactory.build(webMessagingAddress);
            expect(webMessagingAddress.getWindow).toHaveBeenCalledWith();
            expect(webMessagingAddress.getOrigin).toHaveBeenCalledWith();

            var param = {
                message : joynrMessage
            };
            var result = webMessagingStub.transmit(param);
            expect(window.postMessage).toHaveBeenCalledWith(param, origin);
        });

    });

});
