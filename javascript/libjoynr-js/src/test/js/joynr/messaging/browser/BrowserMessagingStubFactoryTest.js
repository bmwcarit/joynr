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

define([
    "joynr/util/Util",
    "joynr/messaging/browser/BrowserMessagingStubFactory"
], function(Util, BrowserMessagingStubFactory) {

    describe("libjoynr-js.joynr.messaging.browser.BrowserMessagingStubFactory", function() {
        var returnValue, webMessagingStub, browserMessagingStubFactory;
        var windowId, browserAddress, joynrMessage;

        beforeEach(function() {
            returnValue = {
                key : "returnValue"
            };
            function WebMessagingStub() {}
            webMessagingStub = new WebMessagingStub();
            webMessagingStub.transmit = jasmine.createSpy("transmit");
            webMessagingStub.transmit.andReturn(returnValue);
            browserMessagingStubFactory = new BrowserMessagingStubFactory({
                webMessagingStub : webMessagingStub
            });
            windowId = "windowId";
            function BrowserAddress() {}
            browserAddress = new BrowserAddress();
            browserAddress.windowId = windowId;
            function JoynrMessage() {}
            joynrMessage = new JoynrMessage();
        });

        it(
                "is instantiable and of correct type",
                function() {
                    expect(BrowserMessagingStubFactory).toBeDefined();
                    expect(typeof BrowserMessagingStubFactory === "function").toBeTruthy();
                    expect(browserMessagingStubFactory).toBeDefined();
                    expect(browserMessagingStubFactory instanceof BrowserMessagingStubFactory)
                            .toBeTruthy();
                    expect(browserMessagingStubFactory.build).toBeDefined();
                    expect(typeof browserMessagingStubFactory.build === "function").toBeTruthy();
                });

        it("throws on missing or wrongly typed arguments in constructor", function() {
            expect(function() {
                browserMessagingStubFactory = new BrowserMessagingStubFactory();
            }).toThrow(); // settings is undefined
            expect(function() {
                browserMessagingStubFactory = new BrowserMessagingStubFactory("");
            }).toThrow(); // settings is of wrong type
            expect(function() {
                browserMessagingStubFactory = new BrowserMessagingStubFactory({});
            }).toThrow(); // webMessagingStub is missing
            expect(function() {
                browserMessagingStubFactory = new BrowserMessagingStubFactory({
                    webMessagingStub : {}
                });
            }).toThrow(); // webMessagingStub is of wrong type
            expect(function() {
                browserMessagingStubFactory = new BrowserMessagingStubFactory({
                    webMessagingStub : ""
                });
            }).toThrow(); // webMessagingStub is of wrong type
            expect(function() {
                browserMessagingStubFactory = new BrowserMessagingStubFactory({
                    webMessagingStub : webMessagingStub
                });
            }).not.toThrow(); // correct call
        });

        it("throws on missing or wrongly typed arguments in build", function() {
            expect(function() {
                browserMessagingStubFactory.build();
            }).toThrow(); // address is undefined
            expect(function() {
                browserMessagingStubFactory.build("");
            }).toThrow(); // address is of wrong type
            expect(function() {
                browserMessagingStubFactory.build({});
            }).toThrow(); // address is of wrong type
            expect(function() {
                browserMessagingStubFactory.build(browserAddress);
            }).not.toThrow(); // correct call
        });

        it("creates a messaging stub and uses it correctly", function() {
            var browserMessagingStub = browserMessagingStubFactory.build(browserAddress);
            //expect(browserAddress.getTabId).toHaveBeenCalledWith();

            var result = browserMessagingStub.transmit(joynrMessage);
            expect(webMessagingStub.transmit).toHaveBeenCalledWith({
                windowId : windowId,
                message : joynrMessage
            });
            expect(result).toEqual(returnValue);
        });

    });

});
