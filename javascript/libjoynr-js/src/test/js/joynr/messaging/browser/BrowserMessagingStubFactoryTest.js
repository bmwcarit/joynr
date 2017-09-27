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
var Util = require('../../../../classes/joynr/util/Util');
var BrowserMessagingStubFactory =
        require('../../../../classes/joynr/messaging/browser/BrowserMessagingStubFactory');

    describe("libjoynr-js.joynr.messaging.browser.BrowserMessagingStubFactory", function() {
        var returnValue, webMessagingStub, browserMessagingStubFactory;
        var windowId, browserAddress, joynrMessage;

        beforeEach(function(done) {
            returnValue = {
                key : "returnValue"
            };
            function WebMessagingStub() {}
            webMessagingStub = new WebMessagingStub();
            webMessagingStub.transmit = jasmine.createSpy("transmit");
            webMessagingStub.transmit.and.returnValue(returnValue);
            browserMessagingStubFactory = new BrowserMessagingStubFactory({
                webMessagingStub : webMessagingStub
            });
            windowId = "windowId";
            function BrowserAddress() {}
            browserAddress = new BrowserAddress();
            browserAddress.windowId = windowId;
            function JoynrMessage() {}
            joynrMessage = new JoynrMessage();
            done();
        });

        it(
                "is instantiable and of correct type",
                function(done) {
                    expect(BrowserMessagingStubFactory).toBeDefined();
                    expect(typeof BrowserMessagingStubFactory === "function").toBeTruthy();
                    expect(browserMessagingStubFactory).toBeDefined();
                    expect(browserMessagingStubFactory instanceof BrowserMessagingStubFactory)
                            .toBeTruthy();
                    expect(browserMessagingStubFactory.build).toBeDefined();
                    expect(typeof browserMessagingStubFactory.build === "function").toBeTruthy();
                    done();
                });

        it("throws on missing or wrongly typed arguments in constructor", function(done) {
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
            done();
        });

        it("throws on missing or wrongly typed arguments in build", function(done) {
            expect(function() {
                browserMessagingStubFactory.build();
            }).toThrow(); // address is undefined
            expect(function() {
                browserMessagingStubFactory.build(browserAddress);
            }).not.toThrow(); // correct call
            done();
        });

        it("creates a messaging stub and uses it correctly", function(done) {
            var browserMessagingStub = browserMessagingStubFactory.build(browserAddress);
            //expect(browserAddress.getTabId).toHaveBeenCalledWith();

            var result = browserMessagingStub.transmit(joynrMessage);
            expect(webMessagingStub.transmit).toHaveBeenCalledWith({
                windowId : windowId,
                message : joynrMessage
            });
            expect(result).toEqual(returnValue);
            done();
        });

    });
