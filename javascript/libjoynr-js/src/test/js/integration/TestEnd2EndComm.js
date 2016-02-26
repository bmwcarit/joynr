/*global joynrTestRequire: true, xit: true, console: true */
/*jslint es5: true, nomen: true */

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

joynrTestRequire(
        "integration/TestEnd2EndComm",
        [
            "global/Promise",
            "joynr",
            "joynr/vehicle/RadioProxy",
            "joynr/vehicle/radiotypes/RadioStation",
            "joynr/vehicle/radiotypes/ErrorList",
            "joynr/datatypes/exampleTypes/Country",
            "joynr/datatypes/exampleTypes/StringMap",
            "integration/IntegrationUtils",
            "joynr/provisioning/provisioning_cc",
            "integration/provisioning_end2end_common"
        ],
        function(
                Promise,
                joynr,
                RadioProxy,
                RadioStation,
                ErrorList,
                Country,
                StringMap,
                IntegrationUtils,
                provisioning,
                provisioning_end2end) {

            describe(
                    "libjoynr-js.integration.end2end.comm",
                    function() {

                        var radioProxy;
                        var provisioningSuffix;
                        var workerId;
                        var subscriptionLength = 2000;
                        var safetyTimeout = 200;
                        var subscriptionQosOnChange;
                        var subscriptionQosInterval;
                        var subscriptionQosMixed;

                        beforeEach(function() {
                            var testProvisioning = null;
                            radioProxy = undefined;

                            provisioningSuffix = this.description.replace(/ /g, "_").replace(/\(/g,"_").replace(/\)/g,"_") + "-" + Date.now();
                            testProvisioning =
                                    IntegrationUtils.getProvisioning(
                                            provisioning,
                                            provisioningSuffix);

                            runs(function() {
                                joynr.load(testProvisioning).then(function(newjoynr) {
                                    joynr = newjoynr;
                                    IntegrationUtils.initialize(joynr);

                                    subscriptionQosOnChange =
                                            new joynr.proxy.OnChangeSubscriptionQos({
                                                minIntervalMs : 50
                                            });

                                    subscriptionQosInterval =
                                            new joynr.proxy.PeriodicSubscriptionQos({
                                                periodMs : 1000
                                            });

                                    subscriptionQosMixed =
                                            new joynr.proxy.OnChangeWithKeepAliveSubscriptionQos({
                                                minIntervalMs : 100,
                                                maxIntervalMs : 1000
                                            });

                                    IntegrationUtils.initializeWebWorker(
                                            "TestEnd2EndCommProviderWorker",
                                            provisioningSuffix).then(function(newWorkerId) {
                                        workerId = newWorkerId;
                                        return IntegrationUtils.startWebWorker(workerId);
                                    }).then(function() {
                                        return IntegrationUtils.buildProxy(RadioProxy);
                                    }).then(function(newRadioProxy) {
                                        radioProxy = newRadioProxy;
                                    });
                                }).catch(function(error){
                                    throw error;
                                });
                            });
                            waitsFor(function() {
                                return radioProxy !== undefined;
                            }, "proxy to be resolved", testProvisioning.ttl);
                        });

                        /**
                         * Calls attribute set with a certain value and tests whether attribute get
                         * returns this value
                         *
                         * @param {ProxyAttribute}
                         *            attribute the proxy attribute to test
                         * @param {Number}
                         *            index the recursion index that goes down to zero
                         * @param {Function}
                         *            [resolve] function to be invoked once the set async function has been completed
                         * @param {Function}
                         *            [reject] function to be invoked once the set async function fails
                         */
                        function setAndTestAttributeRecursive(attribute, index, resolve, reject) {

                            // value is toggled between true/false on every increment of variable i,
                            // ending at false on i=0
                            var valueToBeSet = !!(index % 2);

                            // set the value
                            attribute.set({
                                value : valueToBeSet
                            }).then(
                                    function() {
                                        // get the value
                                        attribute.get().then(
                                                function(valueFromProvider) {
                                                    // check if provider set the value and returned
                                                    // the new one accordingly
                                                    IntegrationUtils.checkValueAndType(
                                                            valueFromProvider,
                                                            valueToBeSet);

                                                    // go on with the recursion
                                                    if (index === 0) {
                                                        // recursion should stop, index is decreased
                                                        // to 0
                                                        resolve(index);
                                                    } else {
                                                        // start next recursion level
                                                        setAndTestAttributeRecursive(
                                                                attribute,
                                                                index - 1,
                                                                resolve,
                                                                reject);
                                                    }
                                                }).catch(function(error) {
                                                    reject(new Error(
                                                            "Failed to retrieve attribute value (recursion index "
                                                                + index
                                                                + "): "
                                                                + error));
                                                });
                                    }).catch(function(error) {
                                        reject(new Error(
                                                "Failed to set attribute value (recursion index "
                                                    + index
                                                    + "): "
                                                    + error));
                                    });
                        }

                        /**
                         * Calls attribute set with a certain value and tests whether attribute get
                         * returns this value
                         *
                         * @param {ProxyAttribute}
                         *            attribute the proxy attribute to test
                         * @param {Number}
                         *            index the recursion index that goes down to zero
                         * @returns {Promise} a promise object that rejects on the first occuring
                         *          error or resolves with the value 0 if all tests finished
                         *          successfully
                         */
                        function setAndTestAttribute(attribute, index) {
                            return new Promise(function(resolve, reject) {
                                setAndTestAttributeRecursive(attribute, index, resolve, reject);
                            });
                        }

                        function callOperation(
                                operationName,
                                opArgs,
                                expectation) {
                            var onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");

                            runs(function() {
                                radioProxy[operationName](opArgs).then(
                                        onFulfilledSpy).catch(IntegrationUtils.outputPromiseError);
                            });

                            waitsFor(function() {
                                return onFulfilledSpy.callCount > 0;
                            }, "operation call to finish", provisioning.ttl);

                            runs(function() {
                                expect(onFulfilledSpy).toHaveBeenCalled();
                                if (expectation !== undefined) {
                                    if (typeof expectation === "function") {
                                        expectation(onFulfilledSpy);
                                    }
                                    else {
                                        expect(onFulfilledSpy).toHaveBeenCalledWith(expectation);
                                    }
                                }
                            });
                        }

                        function setupSubscriptionAndReturnSpy(subscribingEntity, subscriptionQos){
                            var promise, spy = jasmine.createSpyObj("spy", [
                                                                            "onFulfilled",
                                                                            "onReceive",
                                                                            "onError"
                                                                        ]);

                            runs(function() {
                                promise = radioProxy[subscribingEntity].subscribe({
                                    subscriptionQos : subscriptionQos,
                                    onReceive : spy.onReceive,
                                    onError : spy.onError
                                }).then(spy.onFulfilled).catch(IntegrationUtils.outputPromiseError);
                            });

                            waitsFor(function() {
                                return spy.onFulfilled.callCount > 0;
                            }, "subscription to be registered", provisioning.ttl);

                            return spy;
                        }
                        function expectPublication(spy, expectationFct){
                            waitsFor(
                                    function() {
                                        return (spy.onReceive.calls.length > 0 || spy.onError.calls.length > 0);
                                    },
                                    "first publication to occur",
                                    500 + provisioning.ttl);

                            runs(function() {
                                expect(spy.onReceive).toHaveBeenCalled();
                                expect(spy.onError).not.toHaveBeenCalled();
                                expectationFct(spy.onReceive.mostRecentCall);
                                spy.onReceive.reset();
                            });
                        }

                        function expectMultiplePublications(spy, expectedPublications, timeout, expectationFct){
                            waitsFor(
                                    function() {
                                        return (spy.onReceive.calls.length + spy.onError.calls.length >= expectedPublications);
                                    },
                                    expectedPublications + "publications to occur",
                                    timeout);

                            runs(function() {
                                expect(spy.onReceive.calls.length).toBe(expectedPublications);
                                expectationFct(spy.onReceive.calls);
                                spy.onReceive.reset();
                            });
                        }

                        function expectPublicationError(spy){
                            waitsFor(
                                    function() {
                                        return (spy.onReceive.calls.length > 0 || spy.onError.calls.length > 0);
                                    },
                                    "first error to occur",
                                    500 + provisioning.ttl);

                            runs(function() {
                                expect(spy.onReceive).not.toHaveBeenCalled();
                                expect(spy.onError).toHaveBeenCalled();
                                expect(spy.onError.calls[0].args[0]._typeName).toEqual("joynr.exceptions.ProviderRuntimeException");
                                spy.onError.reset();
                            });
                        }
                        function publishesValue(subscriptionQos) {
                            var spy = setupSubscriptionAndReturnSpy("numberOfStations", subscriptionQos);
                            expectPublication(spy, function(publicationCallback) {
                                expect(typeof publicationCallback.args[0] === "number").toBeTruthy();
                            });
                        }

                        function checkUnsubscribe(timeout, subscriptionQos) {
                            var spy, subscriptionId;

                            runs(function() {
                                spy = jasmine.createSpyObj("spy", [
                                    "onFulfilled",
                                    "onReceive",
                                    "onError"
                                ]);
                                radioProxy.numberOfStations.subscribe({
                                    subscriptionQos : subscriptionQos,
                                    onReceive : spy.onReceive,
                                    onError : spy.onError
                                }).then(function(id) {
                                    subscriptionId = id;
                                    spy.onFulfilled(id);
                                }).catch(IntegrationUtils.outputPromiseError);
                            });

                            waitsFor(
                                    function() {
                                        return spy.onFulfilled.callCount > 0
                                            && (spy.onReceive.calls.length > 0 || spy.onError.calls.length > 0);
                                    },
                                    "subscription to be registered and first publication to occur",
                                    provisioning.ttl);

                            var nrOfPublications;
                            runs(function() {
                                expect(spy.onFulfilled).toHaveBeenCalled();
                                spy.onFulfilled.reset();
                                nrOfPublications = spy.onReceive.calls.length;
                                radioProxy.numberOfStations.unsubscribe({
                                    subscriptionId : subscriptionId
                                }).then(spy.onFulfilled).catch(IntegrationUtils.outputPromiseError);
                            });

                            waitsFor(function() {
                                return spy.onFulfilled.callCount > 0;
                            }, "unsubscribe to complete successfully", provisioning.ttl);

                            runs(function() {
                                expect(spy.onFulfilled).toHaveBeenCalled();
                            });

                            // wait 2 times the publication interval
                            var waitForPublication = true;
                            runs(function() {
                                joynr.util.LongTimer.setTimeout(function() {
                                    waitForPublication = false;
                                }, 2 * timeout);
                            });

                            waitsFor(function() {
                                return !waitForPublication;
                            }, 4 * timeout);

                            // check that no publications occured since the unsubscribe
                            runs(function() {
                                expect(spy.onReceive.calls.length).toEqual(nrOfPublications);
                            });
                        }

                        var getAttribute = function(attributeName, expectedValue) {
                            var onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");

                            waitsFor(function() {
                                return radioProxy !== undefined;
                            }, "radioProxy is defined", provisioning.ttl);

                            runs(function() {
                                radioProxy[attributeName].get().then(function(value) {
                                    expect(value).toEqual(expectedValue);
                                    onFulfilledSpy(value);
                                }).catch(IntegrationUtils.outputPromiseError);
                            });

                            waitsFor(function() {
                                return onFulfilledSpy.callCount > 0;
                            }, "attribute " + attributeName + " is received", provisioning.ttl);

                            runs(function() {
                                expect(onFulfilledSpy).toHaveBeenCalled();
                            });
                        };

                        var getFailingAttribute = function(attributeName) {
                            var onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");
                            var catchSpy = jasmine.createSpy("catchSpy");

                            waitsFor(function() {
                                return radioProxy !== undefined;
                            }, "radioProxy is defined", provisioning.ttl);

                            runs(function() {
                                radioProxy[attributeName].get().then(function(value) {
                                    onFulfilledSpy(value);
                                }).catch(function(exception) {
                                    catchSpy(exception);
                                });
                            });

                            waitsFor(function() {
                                return catchSpy.callCount > 0;
                            }, "getter for attribute " + attributeName + " returns exception", provisioning.ttl);

                            runs(function() {
                                expect(catchSpy).toHaveBeenCalled();
                                expect(catchSpy.calls[0].args[0]._typeName).toBeDefined();
                                expect(catchSpy.calls[0].args[0]._typeName).toEqual("joynr.exceptions.ProviderRuntimeException");
                            });
                        };

                        var setAttribute = function(attributeName, value) {
                            var onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");

                            runs(function() {
                                radioProxy[attributeName].set({
                                    value : value
                                }).then(onFulfilledSpy).catch(IntegrationUtils.outputPromiseError);
                            });

                            waitsFor(function() {
                                return onFulfilledSpy.callCount > 0;
                            }, "attribute is set", provisioning.ttl);

                            runs(function() {
                                expect(onFulfilledSpy).toHaveBeenCalled();
                            });
                        };

                        it("gets the attribute", function() {
                            getAttribute("isOn", true);
                        });

                        it("gets the enumAttribute", function() {
                            getAttribute("enumAttribute", Country.GERMANY);
                        });

                        it("gets the enumArrayAttribute", function() {
                            getAttribute("enumArrayAttribute", [Country.GERMANY]);
                        });

                        it("gets an exception for failingSyncAttribute", function() {
                            getFailingAttribute("failingSyncAttribute");
                        });

                        it("gets an exception for failingAsyncAttribute", function() {
                            getFailingAttribute("failingAsyncAttribute");
                        });

                        it("sets the enumArrayAttribute", function() {
                            var value = [];
                            setAttribute("enumArrayAttribute", value);
                            getAttribute("enumArrayAttribute", value);
                            value = [Country.GERMANY, Country.AUSTRIA, Country.AUSTRALIA, Country.CANADA, Country.ITALY];
                            setAttribute("enumArrayAttribute", value);
                            getAttribute("enumArrayAttribute", value);
                        });

                        it("sets the typeDef attributes", function() {
                            var value = new RadioStation({
                                name: "TestEnd2EndComm.typeDefForStructAttribute.RadioStation",
                                byteBuffer: []
                            });
                            setAttribute("typeDefForStruct", value);
                            getAttribute("typeDefForStruct", value);
                            value = 1234543;
                            setAttribute("typeDefForPrimitive", value);
                            getAttribute("typeDefForPrimitive", value);
                        });

                        it("sets the attribute", function() {
                            setAttribute("isOn", true);
                            getAttribute("isOn", true);
                            setAttribute("isOn", false);
                            getAttribute("isOn", false);
                        });

                        it("sets the enumAttribute", function() {
                            setAttribute("enumAttribute", Country.AUSTRIA);
                            getAttribute("enumAttribute", Country.AUSTRIA);
                            setAttribute("enumAttribute", Country.AUSTRALIA);
                            getAttribute("enumAttribute", Country.AUSTRALIA);
                        });

                        it("sets the byteBufferAttribute", function() {
                            var testByteBuffer = [1,2,3,4];
                            setAttribute("byteBufferAttribute", testByteBuffer);
                            getAttribute("byteBufferAttribute", testByteBuffer);
                            setAttribute("byteBufferAttribute", testByteBuffer);
                            getAttribute("byteBufferAttribute", testByteBuffer);
                        });

                        it("sets the stringMapAttribute", function() {
                            var stringMap = new StringMap({key1: "value1"});
                            setAttribute("stringMapAttribute", stringMap);
                            getAttribute("stringMapAttribute", stringMap);
                            setAttribute("stringMapAttribute", stringMap);
                            getAttribute("stringMapAttribute", stringMap);
                        });

                        it("subscribe to failingSyncAttribute", function() {
                            var spy = setupSubscriptionAndReturnSpy("failingSyncAttribute", subscriptionQosInterval);
                            expectPublicationError(spy);
                        });

                        it("subscribe to failingAsyncAttribute", function() {
                            var spy = setupSubscriptionAndReturnSpy("failingAsyncAttribute", subscriptionQosInterval);
                            expectPublicationError(spy);
                        });

                        it("subscribe to enumAttribute", function() {
                            setAttribute("enumAttribute", Country.AUSTRIA);
                            var spy = setupSubscriptionAndReturnSpy("enumAttribute", subscriptionQosOnChange);
                            expectPublication(spy, function(call) {
                               expect(call.args[0]).toEqual(Country.AUSTRIA);
                            });
                            setAttribute("enumAttribute", Country.AUSTRALIA);
                            expectPublication(spy, function(call) {
                                expect(call.args[0]).toEqual(Country.AUSTRALIA);
                            });

                            setAttribute("enumAttribute", Country.ITALY);
                            expectPublication(spy, function(call) {
                                expect(call.args[0]).toEqual(Country.ITALY);
                            });
                        });

                        it("subscribe to complex typedef attribute", function() {
                            var value = new RadioStation({
                                name: "TestEnd2EndComm.typeDefForStructAttribute.RadioStation",
                                byteBuffer: []
                            });
                            setAttribute("typeDefForStruct", value);
                            var spy = setupSubscriptionAndReturnSpy("typeDefForStruct", subscriptionQosOnChange);
                            expectPublication(spy, function(call) {
                               expect(call.args[0]).toEqual(value);
                            });
                        });

                        it("subscribe to primitive typedef attribute", function() {
                            var value = 1234543;
                            setAttribute("typeDefForPrimitive", value);
                            var spy = setupSubscriptionAndReturnSpy("typeDefForPrimitive", subscriptionQosOnChange);
                            getAttribute("typeDefForPrimitive", value);
                            expectPublication(spy, function(call) {
                               expect(call.args[0]).toEqual(value);
                            });
                        });

                        it("subscribe to byteBufferAttribute", function() {
                            //initialize attribute
                            setAttribute("byteBufferAttribute", []);
                            var spy = setupSubscriptionAndReturnSpy("byteBufferAttribute", subscriptionQosOnChange);
                            expectPublication(spy, function(call) {
                                expect(call.args[0]).toEqual([]);
                            });
                            var i, byteBuffer10k = [], testByteBufferAttribute = function(expectedByteBuffer) {
                                setAttribute("byteBufferAttribute", expectedByteBuffer);
                                expectPublication(spy, function(call) {
                                   expect(call.args[0]).toEqual(expectedByteBuffer);
                                });
                            };
                            testByteBufferAttribute([0,1,2,3,4,5,6,7,8,9,8,7,6,5,4,3,2,1,0]);
                            testByteBufferAttribute([255]);
                            testByteBufferAttribute([2,2,2,2]);
                            for (i = 0; i < 10000; i++) {
                                byteBuffer10k.push(i % 256);
                            }
                            testByteBufferAttribute(byteBuffer10k);
                        });

                        it("subscribe to weakSignal broadcast having ByteBuffer as output parameter", function() {
                            var spy = setupSubscriptionAndReturnSpy("weakSignal", subscriptionQosOnChange);
                            callOperation("triggerBroadcasts", {
                                broadcastName: "weakSignal",
                                times: 1
                            });
                            expectPublication(spy, function(call) {
                               expect(call.args[0].radioStation).toEqual("radioStation");
                               expect(call.args[0].byteBuffer).toEqual([0,1,2,3,4,5,6,7,8,9,8,7,6,5,4,3,2,1,0]);
                            });
                        });

                        it("subscribe to broadcastWithEnum", function() {
                            var spy = setupSubscriptionAndReturnSpy("broadcastWithEnum", subscriptionQosOnChange);
                            callOperation("triggerBroadcasts", {
                                broadcastName: "broadcastWithEnum",
                                times: 1
                            });
                            expectPublication(spy, function(call) {
                               expect(call.args[0].enumOutput).toEqual(Country.CANADA);
                               expect(call.args[0].enumArrayOutput).toEqual([Country.GERMANY, Country.ITALY]);
                            });
                        });


                        it("subscribe to type def broadcast", function() {
                            var typeDefStructOutput = new RadioStation({
                                name: "TestEnd2EndCommProviderWorker.broadcastWithTypeDefs.RadioStation",
                                byteBuffer: []
                            });
                            var typeDefPrimitiveOutput = 123456;

                            var spy = setupSubscriptionAndReturnSpy("broadcastWithTypeDefs", subscriptionQosOnChange);
                            callOperation("triggerBroadcasts", {
                                broadcastName: "broadcastWithTypeDefs",
                                times: 1
                            });
                            expectPublication(spy, function(call) {
                               expect(call.args[0].typeDefStructOutput).toEqual(typeDefStructOutput);
                               expect(call.args[0].typeDefPrimitiveOutput).toEqual(typeDefPrimitiveOutput);
                            });
                        });

                        it("subscribe to broadcastWithEnum and get burst", function() {
                            subscriptionQosOnChange.minIntervalMs = 0;
                            var times = 100, spy = setupSubscriptionAndReturnSpy("broadcastWithEnum", subscriptionQosOnChange);
                            callOperation("triggerBroadcasts", {
                                broadcastName: "broadcastWithEnum",
                                times: times
                            });
                            expectMultiplePublications(spy, times, 5000, function(calls) {
                               var i;
                               for (i=0;i<times;i++) {
                                   expect(calls[i].args[0].enumOutput).toEqual(Country.CANADA);
                                   expect(calls[i].args[0].enumArrayOutput).toEqual([Country.GERMANY, Country.ITALY]);
                               }
                            });
                        });

                        it("subscribe to enumArrayAttribute", function() {
                            var attributeName = "enumArrayAttribute";
                            var value1 = [Country.AUSTRIA];
                            setAttribute(attributeName, value1);
                            var spy = setupSubscriptionAndReturnSpy(attributeName, subscriptionQosOnChange);
                            expectPublication(spy, function(call) {
                               expect(call.args[0]).toEqual(value1);
                            });
                            var value2 = [Country.AUSTRIA, Country.GERMANY];
                            setAttribute(attributeName, value2);
                            expectPublication(spy, function(call) {
                                expect(call.args[0]).toEqual(value2);
                            });

                            var value3 = [Country.AUSTRIA, Country.GERMANY, Country.ITALY];
                            setAttribute(attributeName, value3);
                            expectPublication(spy, function(call) {
                                expect(call.args[0]).toEqual(value3);
                            });
                        });

                        function setAndTestAttributeTester(attribute) {
                            var lastRecursionIndex = -1, recursions = 5, onFulfilledSpy =
                                    jasmine.createSpy("onFulfilledSpy");

                            runs(function() {
                                setAndTestAttribute(attribute, recursions - 1).then(
                                        onFulfilledSpy).catch(IntegrationUtils.outputPromiseError);
                            });

                            waitsFor(function() {
                                return onFulfilledSpy.callCount > 0;
                            }, "get/set test to finish", 2 * provisioning.ttl * recursions); // each
                            // repetition
                            // consists
                            // of a
                            // get
                            // and
                            // set
                            // => 2
                            // ttls per repetition

                            runs(function() {
                                // additional sanity check whether recursion level
                                // really went down
                                // to 0
                                expect(onFulfilledSpy).toHaveBeenCalled();
                                expect(onFulfilledSpy).toHaveBeenCalledWith(0);
                            });
                        }

                        // when provider is working this should also work
                        it("checks whether the provider stores the attribute value", function() {
                            setAndTestAttributeTester(radioProxy.isOn);
                        });

                        it("can call an operation successfully (Provider sync, String parameter)", function() {
                            callOperation("addFavoriteStation",
                                    {
                                radioStation : 'stringStation'
                                    }
                            );
                        });

                        it("can call an operation and get a ProviderRuntimeException (Provider sync, String parameter)", function() {
                            var onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");
                            var catchSpy = jasmine.createSpy("catchSpy");

                            runs(function() {
                                radioProxy.addFavoriteStation({
                                    radioStation : 'stringStationerror'
                                }).then(onFulfilledSpy).catch(catchSpy);
                            });

                            waitsFor(function() {
                                return catchSpy.callCount > 0;
                            }, "operation call to fail", provisioning.ttl);

                            runs(function() {
                                expect(onFulfilledSpy).not.toHaveBeenCalled();
                                expect(catchSpy).toHaveBeenCalled();
                                expect(catchSpy.calls[0].args[0]._typeName).toBeDefined();
                                expect(catchSpy.calls[0].args[0]._typeName).toEqual("joynr.exceptions.ProviderRuntimeException");
                                expect(catchSpy.calls[0].args[0].detailMessage).toBeDefined();
                                expect(catchSpy.calls[0].args[0].detailMessage).toEqual("example message sync");
                            });
                        });

                        it("can call an operation and get an ApplicationException (Provider sync, String parameter)", function() {
                            var onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");
                            var catchSpy = jasmine.createSpy("catchSpy");

                            runs(function() {
                                radioProxy.addFavoriteStation({
                                    radioStation : 'stringStationerrorApplicationException'
                                }).then(onFulfilledSpy).catch(catchSpy);
                            });

                            waitsFor(function() {
                                return catchSpy.callCount > 0;
                            }, "operation call to fail", provisioning.ttl);

                            runs(function() {
                                expect(onFulfilledSpy).not.toHaveBeenCalled();
                                expect(catchSpy).toHaveBeenCalled();
                                expect(catchSpy.calls[0].args[0]._typeName).toBeDefined();
                                expect(catchSpy.calls[0].args[0]._typeName).toEqual("joynr.exceptions.ApplicationException");
                                expect(catchSpy.calls[0].args[0].error).toBeDefined();
                                expect(catchSpy.calls[0].args[0].error).toEqual(ErrorList.EXAMPLE_ERROR_2);
                            });
                        });

                        it("can call an operation successfully (Provider async, String parameter)", function() {
                            var onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");

                            runs(function() {
                                radioProxy.addFavoriteStation({
                                    radioStation : 'stringStationasync'
                                }).then(onFulfilledSpy).catch(IntegrationUtils.outputPromiseError);
                            });

                            waitsFor(function() {
                                return onFulfilledSpy.callCount > 0;
                            }, "operation call to finish", provisioning.ttl);

                            runs(function() {
                                expect(onFulfilledSpy).toHaveBeenCalled();
                            });
                        });

                        it("can call an operation and get a ProviderRuntimeException (Provider async, String parameter)", function() {
                            var onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");
                            var catchSpy = jasmine.createSpy("catchSpy");

                            runs(function() {
                                radioProxy.addFavoriteStation({
                                    radioStation : 'stringStationasyncerror'
                                }).then(onFulfilledSpy).catch(catchSpy);
                            });

                            waitsFor(function() {
                                return catchSpy.callCount > 0;
                            }, "operation call to fail", provisioning.ttl);

                            runs(function() {
                                expect(onFulfilledSpy).not.toHaveBeenCalled();
                                expect(catchSpy).toHaveBeenCalled();
                                expect(catchSpy.calls[0].args[0]._typeName).toBeDefined();
                                expect(catchSpy.calls[0].args[0]._typeName).toEqual("joynr.exceptions.ProviderRuntimeException");
                                expect(catchSpy.calls[0].args[0].detailMessage).toBeDefined();
                                expect(catchSpy.calls[0].args[0].detailMessage).toEqual("example message async");
                            });
                        });

                        it("can call an operation and get an ApplicationException (Provider async, String parameter)", function() {
                            var onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");
                            var catchSpy = jasmine.createSpy("catchSpy");

                            runs(function() {
                                radioProxy.addFavoriteStation({
                                    radioStation : 'stringStationasyncerrorApplicationException'
                                }).then(onFulfilledSpy).catch(catchSpy);
                            });

                            waitsFor(function() {
                                return catchSpy.callCount > 0;
                            }, "operation call to fail", provisioning.ttl);

                            runs(function() {
                                expect(onFulfilledSpy).not.toHaveBeenCalled();
                                expect(catchSpy).toHaveBeenCalled();
                                expect(catchSpy.calls[0].args[0]._typeName).toBeDefined();
                                expect(catchSpy.calls[0].args[0]._typeName).toEqual("joynr.exceptions.ApplicationException");
                                expect(catchSpy.calls[0].args[0].error).toBeDefined();
                                expect(catchSpy.calls[0].args[0].error).toEqual(ErrorList.EXAMPLE_ERROR_1);
                            });
                        });

                        it("can call an operation (parameter of complex type)", function() {
                            callOperation("addFavoriteStation",
                                    {
                                        radioStation : 'stringStation'
                                    }
                            );
                        });

                        it(
                                "can call an operation (parameter of byteBuffer type",
                                function() {
                                    callOperation(
                                            "methodWithByteBuffer",
                                            {
                                                input : [0,1,2,3,4,5,6,7,8,9,8,7,6,5,4,3,2,1,0]
                                            },
                                            {
                                                result : [0,1,2,3,4,5,6,7,8,9,8,7,6,5,4,3,2,1,0]
                                            });
                               });

                        it(
                                "can call an operation with working parameters and return type",
                                function() {
                                    callOperation(
                                            "addFavoriteStation",
                                            {
                                                radioStation : "truelyContainingTheString\"True\""
                                            },
                                            {
                                                returnValue : true
                                            });
                                    callOperation(
                                            "addFavoriteStation",
                                            {
                                                radioStation : "This is false!"
                                            },
                                            {
                                                returnValue : false
                                            });
                                    callOperation(
                                            "addFavoriteStation",
                                            {
                                                radioStation : new RadioStation(
                                                        {
                                                            name : "truelyContainingTheRadioStationString\"True\""
                                                        })
                                            },
                                            {
                                                returnValue : true
                                            });
                                    callOperation(
                                            "addFavoriteStation",
                                            {
                                                radioStation : new RadioStation({
                                                    name : "This is a false RadioStation!"
                                                })
                                            },
                                            {
                                                returnValue : false
                                            });
                                });

                        it(
                                "can call an operation with typedef arguments",
                                function() {
                                    var typeDefStructInput = new RadioStation({
                                        name: "TestEnd2EndComm.methodWithTypeDef.RadioStation",
                                        byteBuffer: []
                                    });
                                    var typeDefPrimitiveInput = 1234543;
                                    callOperation(
                                            "methodWithTypeDef",
                                            {
                                                typeDefStructInput : typeDefStructInput,
                                                typeDefPrimitiveInput : typeDefPrimitiveInput
                                            },
                                            {
                                                typeDefStructOutput : typeDefStructInput,
                                                typeDefPrimitiveOutput : typeDefPrimitiveInput
                                            });
                               });

                        it(
                                "can call an operation with enum arguments and enum return type",
                                function() {
                                    callOperation(
                                            "operationWithEnumsAsInputAndOutput",
                                            {
                                                enumInput : Country.GERMANY,
                                                enumArrayInput : []
                                            },
                                            {
                                                enumOutput : Country.GERMANY
                                            });
                                    callOperation(
                                            "operationWithEnumsAsInputAndOutput",
                                            {
                                                enumInput : Country.GERMANY,
                                                enumArrayInput : [Country.AUSTRIA]
                                            },
                                            {
                                                enumOutput : Country.AUSTRIA
                                            });
                                    callOperation(
                                            "operationWithEnumsAsInputAndOutput",
                                            {
                                                enumInput : Country.GERMANY,
                                                enumArrayInput : [Country.AUSTRIA, Country.GERMANY, Country.AUSTRALIA]
                                            },
                                            {
                                                enumOutput : Country.AUSTRIA
                                            });
                                    callOperation(
                                            "operationWithEnumsAsInputAndOutput",
                                            {
                                                enumInput : Country.GERMANY,
                                                enumArrayInput : [Country.CANADA, Country.AUSTRIA, Country.ITALY]
                                            },
                                            {
                                                enumOutput : Country.CANADA
                                            });
                               });

                        it(
                                "can call an operation with multiple return values and async provider",
                                function() {
                                    var inputData = {
                                        enumInput : Country.GERMANY,
                                        enumArrayInput : [Country.GERMANY, Country.ITALY],
                                        stringInput : "StringTest",
                                        syncTest : false
                                    };
                                    callOperation(
                                            "operationWithMultipleOutputParameters",
                                            inputData,
                                            {
                                                enumArrayOutput : inputData.enumArrayInput,
                                                enumOutput : inputData.enumInput,
                                                stringOutput : inputData.stringInput,
                                                booleanOutput : inputData.syncTest
                                            });

                        });

                        it(
                                "can call an operation with multiple return values and sync provider",
                                function() {
                                    var inputData = {
                                        enumInput : Country.GERMANY,
                                        enumArrayInput : [Country.GERMANY, Country.ITALY],
                                        stringInput : "StringTest",
                                        syncTest : true
                                    };
                                    callOperation(
                                            "operationWithMultipleOutputParameters",
                                            inputData,
                                            {
                                                enumArrayOutput : inputData.enumArrayInput,
                                                enumOutput : inputData.enumInput,
                                                stringOutput : inputData.stringInput,
                                                booleanOutput : inputData.syncTest
                                            });
                        });

                        it(
                                "can call an operation with enum arguments and enum array as return type",
                                function() {
                                    callOperation(
                                            "operationWithEnumsAsInputAndEnumArrayAsOutput",
                                            {
                                                enumInput : Country.GERMANY,
                                                enumArrayInput : []
                                            },
                                            {
                                                enumOutput : [Country.GERMANY]
                                            });
                                    callOperation(
                                            "operationWithEnumsAsInputAndEnumArrayAsOutput",
                                            {
                                                enumInput : Country.GERMANY,
                                                enumArrayInput : [Country.AUSTRIA]
                                            },
                                            {
                                                enumOutput : [Country.AUSTRIA, Country.GERMANY]
                                            });
                                    callOperation(
                                            "operationWithEnumsAsInputAndEnumArrayAsOutput",
                                            {
                                                enumInput : Country.GERMANY,
                                                enumArrayInput : [Country.AUSTRIA, Country.GERMANY, Country.AUSTRALIA]
                                            },
                                            {
                                                enumOutput : [Country.AUSTRIA, Country.GERMANY, Country.AUSTRALIA, Country.GERMANY]
                                            });
                                    callOperation(
                                            "operationWithEnumsAsInputAndEnumArrayAsOutput",
                                            {
                                                enumInput : Country.GERMANY,
                                                enumArrayInput : [Country.CANADA, Country.AUSTRIA, Country.ITALY]
                                            },
                                            {
                                                enumOutput : [Country.CANADA, Country.AUSTRIA, Country.ITALY, Country.GERMANY]
                                            });
                               });

                        it(
                                "can call an operation with double array as argument and string array as return type",
                                function() {
                                    callOperation(
                                            "methodWithSingleArrayParameters",
                                            {
                                                doubleArrayArg : [0.01,1.1,2.2,3.3]
                                            },
                                            {
                                                stringArrayOut : ["0.01", "1.1", "2.2", "3.3"]
                                            });
                               });

                        it("can start a subscription and provides a subscription id", function() {
                            var spy;

                            runs(function() {
                                spy = jasmine.createSpyObj("spy", [
                                    "onFulfilled",
                                    "onError"
                                ]);
                                radioProxy.numberOfStations.subscribe({
                                    subscriptionQos : subscriptionQosOnChange
                                }).then(spy.onFulfilled).catch(function(error) {
                                    spy.onError(error);
                                    IntegrationUtils.outputPromiseError(error);
                                });
                            });

                            waitsFor(function() {
                                return spy.onFulfilled.callCount > 0;
                            }, "subscription to be registered", provisioning.ttl);

                            runs(function() {
                                expect(spy.onFulfilled).toHaveBeenCalled();
                                expect(typeof spy.onFulfilled.mostRecentCall.args[0] === "string")
                                        .toBeTruthy();
                                expect(spy.onError).not.toHaveBeenCalled();
                            });
                        });

                        it("publishes a value with an onChange subscription", function() {
                            publishesValue(subscriptionQosOnChange);
                        });

                        it("publishes a value with an interval subscription", function() {
                            publishesValue(subscriptionQosInterval);
                        });

                        it("publishes a value with a mixed subscription", function() {
                            publishesValue(subscriptionQosMixed);
                        });

                        it("publishes a value with an ending subscription", function() {
                            subscriptionQosMixed.expiryDateMs = subscriptionLength + Date.now();
                            publishesValue(subscriptionQosMixed);
                        });

                        it(
                                "initially publishes a value on subscription",
                                function() {
                                    var spy;

                                    runs(function() {
                                        spy = jasmine.createSpyObj("spy", [
                                            "onFulfilled",
                                            "onReceive",
                                            "onError"
                                        ]);
                                        radioProxy.numberOfStations.subscribe({
                                            subscriptionQos : subscriptionQosOnChange,
                                            onReceive : spy.onReceive,
                                            onError : spy.onError
                                        }).then(
                                                spy.onFulfilled).catch(IntegrationUtils.outputPromiseError);
                                    });

                                    waitsFor(
                                            function() {
                                                return spy.onFulfilled.callCount > 0
                                                    && (spy.onReceive.calls.length > 0 || spy.onError.calls.length > 0);
                                            },
                                            "initial onReceive to occur",
                                            provisioning.ttl); // timeout for
                                    // subscription request
                                    // round trip

                                    runs(function() {
                                        expect(spy.onFulfilled).toHaveBeenCalled();
                                        expect(spy.onReceive).toHaveBeenCalled();
                                        expect(spy.onReceive.calls[0].args[0]).toEqual(-1);
                                        expect(spy.onError).not.toHaveBeenCalled();
                                    });
                                });

                        var nrPubs = 3;
                        it(
                                "publishes correct values with onChange " + nrPubs + " times",
                                function() {
                                    var spy;

                                    runs(function() {
                                        spy = jasmine.createSpyObj("spy", [
                                            "onFulfilled",
                                            "onReceive",
                                            "onError"
                                        ]);
                                        radioProxy.numberOfStations.subscribe({
                                            subscriptionQos : subscriptionQosOnChange,
                                            onReceive : spy.onReceive,
                                            onError : spy.onError
                                        }).then(
                                                spy.onFulfilled).catch(IntegrationUtils.outputPromiseError);
                                    });

                                    waitsFor(function() {
                                        return spy.onFulfilled.callCount > 0;
                                    }, "subscription promise to resolve", provisioning.ttl); // timeout
                                    // for
                                    // subscription
                                    // request
                                    // round
                                    // trip

                                    runs(function() {
                                        expect(spy.onFulfilled).toHaveBeenCalled();
                                    });

                                    waitsFor(
                                            function() {
                                                return (spy.onReceive.calls.length > 0 || spy.onError.calls.length > 0);
                                            },
                                            "initial onReceive to occur",
                                            provisioning.ttl); // timeout for
                                    // subscription request
                                    // round trip

                                    runs(function() {
                                        expect(spy.onReceive).toHaveBeenCalled();
                                        expect(spy.onReceive.calls[0].args[0]).toEqual(-1);
                                        expect(spy.onError).not.toHaveBeenCalled();

                                        spy.onReceive.reset();
                                        spy.onError.reset();
                                    });

                                    waitsFor(
                                            function() {
                                                return (spy.onReceive.calls.length >= nrPubs || spy.onError.calls.length > 0);
                                            },
                                            "subscription to be registered and first publication to occur",
                                            provisioning.ttl + (nrPubs + 1) * 1000); // timeout
                                    // for subscription request round trip and nrPubs publications, safety (+1)

                                    runs(function() {
                                        expect(spy.onReceive).toHaveBeenCalled();
                                        expect(spy.onError).not.toHaveBeenCalled();

                                        expect(spy.onReceive.calls.length).toEqual(nrPubs);

                                        var i;
                                        for (i = 0; i < nrPubs; ++i) {
                                            expect(spy.onReceive.calls[i].args[0]).toEqual(i);
                                        }
                                    });
                                });

                        it(
                                "publishes correct values with a mixed subscription",
                                function() {
                                    var spy;

                                    // provider will fire an interval publication 1s after
                                    // initialization with the response "interval"
                                    // after another 0.5 ms it will fire an onChange publication
                                    // with the response "valueChanged1"
                                    // after another 10 ms it will try to fire an onChange
                                    // publication with the response "valueChanged2", should be
                                    // blocked by the
                                    // PublicationManager on the Provider side

                                    runs(function() {
                                        spy = jasmine.createSpyObj("spy", [
                                            "onFulfilled",
                                            "onReceive",
                                            "onError"
                                        ]);
                                        subscriptionQosMixed.expiryDateMs =
                                                subscriptionLength + Date.now();
                                        radioProxy.mixedSubscriptions.subscribe({
                                            subscriptionQos : subscriptionQosMixed,
                                            onReceive : spy.onReceive,
                                            onError : spy.onError
                                        }).then(spy.onFulfilled).catch(function(error) {
                                            spy.onError(error);
                                            IntegrationUtils.outputPromiseError(error);
                                        });
                                    });

                                    waitsFor(function() {
                                        return spy.onFulfilled.callCount > 0;
                                    }, "subscription to be registered", provisioning.ttl);

                                    runs(function() {
                                        expect(spy.onFulfilled).toHaveBeenCalled();
                                    });

                                    waitsFor(
                                            function() {
                                                return (spy.onReceive.calls.length > 0 || spy.onError.calls.length > 0);
                                            },
                                            "initial publication to occur",
                                            provisioning.ttl); // timeout for
                                    // subscription request
                                    // round trip

                                    runs(function() {
                                        expect(spy.onReceive).toHaveBeenCalled();
                                        expect(spy.onReceive.calls[0].args[0])
                                                .toEqual("interval");
                                        expect(spy.onError).not.toHaveBeenCalled();

                                        spy.onReceive.reset();
                                        spy.onError.reset();
                                    });

                                    waitsFor(
                                            function() {
                                                return (spy.onReceive.callCount > 0 || spy.onError.callCount > 0);
                                            },
                                            "the interval publication to occur",
                                            subscriptionQosMixed.maxIntervalMs + provisioning.ttl);

                                    runs(function() {
                                        expect(spy.onReceive).toHaveBeenCalled();
                                        expect(spy.onReceive).toHaveBeenCalledWith("interval");
                                        spy.onReceive.reset();
                                    });

                                    waitsFor(function() {
                                        return spy.onReceive.callCount > 0;
                                    }, "the onChange publication to occur", 500 + provisioning.ttl);

                                    var timeout;

                                    runs(function() {
                                        timeout = false;
                                        expect(spy.onReceive).toHaveBeenCalled();
                                        expect(spy.onReceive).toHaveBeenCalledWith(
                                                "valueChanged1");
                                        spy.onReceive.reset();
                                        joynr.util.LongTimer.setTimeout(function() {
                                            timeout = true;
                                        }, subscriptionQosMixed.minIntervalMs / 2);
                                    });

                                    waitsFor(
                                            function() {
                                                return timeout || spy.onReceive.callCount > 0;
                                            },
                                            "the second onChange publication to not occur",
                                            subscriptionQosMixed.minIntervalMs + safetyTimeout);

                                    runs(function() {
                                        expect(spy.onReceive).not.toHaveBeenCalled();
                                    });

                                    waitsFor(
                                            function() {
                                                return spy.onReceive.callCount > 0;
                                            },
                                            "the second onChange publication occur after minIntervalMs",
                                            provisioning.ttl + safetyTimeout);

                                    runs(function() {
                                        expect(spy.onReceive).toHaveBeenCalled();
                                        expect(spy.onReceive).toHaveBeenCalledWith("interval");
                                    });
                                });

                        it(
                                "terminates correctly according to the endDate ",
                                function() {
                                    var spy, timeout;

                                    // provider will fire an interval publication 1s
                                    // after
                                    // initialization with the response "interval"
                                    // after another 0.5 ms it will fire an onChange
                                    // publication
                                    // with the response "valueChanged1"
                                    // after another 10 ms it will try to fire an
                                    // onChange
                                    // publication with the response "valueChanged2",
                                    // should be
                                    // blocked by the
                                    // PublicationManager on the Provider side

                                    runs(function() {
                                        spy = jasmine.createSpyObj("spy", [
                                            "onFulfilled",
                                            "onReceive",
                                            "onError"
                                        ]);
                                        subscriptionQosMixed.expiryDateMs =
                                                subscriptionQosMixed.maxIntervalMs * 1.5 + Date.now();
                                        radioProxy.isOn.subscribe({
                                            subscriptionQos : subscriptionQosMixed,
                                            onReceive : spy.onReceive,
                                            onError : spy.onError
                                        }).then(spy.onFulfilled).catch(function(error) {
                                            spy.onError(error);
                                            IntegrationUtils.outputPromiseError(error);
                                        });
                                    });

                                    waitsFor(function() {
                                        return spy.onFulfilled.callCount > 0;
                                    }, "subscription to be registered", provisioning.ttl);

                                    runs(function() {
                                        expect(spy.onFulfilled).toHaveBeenCalled();
                                    });

                                    waitsFor(
                                            function() {
                                                return (spy.onReceive.calls.length > 0 || spy.onError.calls.length > 0);
                                            },
                                            "initial publication to occur",
                                            provisioning.ttl); // timeout for
                                    // subscription request
                                    // round trip

                                    runs(function() {
                                        expect(spy.onReceive).toHaveBeenCalled();
                                        expect(spy.onReceive.calls[0].args[0]).toEqual(true);
                                        expect(spy.onError).not.toHaveBeenCalled();

                                        spy.onReceive.reset();
                                        spy.onError.reset();
                                    });

                                    waitsFor(
                                            function() {
                                                return (spy.onReceive.callCount > 0 || spy.onError.callCount > 0);
                                            },
                                            "the interval onReceive to occur",
                                            subscriptionQosMixed.maxIntervalMs + provisioning.ttl);

                                    runs(function() {
                                        expect(spy.onReceive).toHaveBeenCalled();
                                        expect(spy.onReceive).toHaveBeenCalledWith(true);
                                        spy.onReceive.reset();

                                        joynr.util.LongTimer.setTimeout(function() {
                                            timeout = true;
                                        }, subscriptionQosMixed.maxIntervalMs + provisioning.ttl);
                                    });

                                    waitsFor(
                                            function() {
                                                return timeout || spy.onReceive.callCount > 0;
                                            },
                                            "the interval onReceive not to occur again",
                                            subscriptionQosMixed.maxIntervalMs
                                                + provisioning.ttl
                                                + safetyTimeout);

                                    runs(function() {
                                        expect(spy.onReceive).not.toHaveBeenCalled();
                                    });
                                });

                        it("unsubscribes onChange subscription successfully", function() {
                            checkUnsubscribe(1000, subscriptionQosOnChange);
                        });

                        it("unsubscribes interval subscription successfully", function() {
                            checkUnsubscribe(1000, subscriptionQosInterval);
                        });

                        it(
                                "attributes are working with predefined implementation on provider side",
                                function() {
                                    setAndTestAttributeRecursive(radioProxy.attrProvidedImpl);
                                });

                        it(
                                "operation is working with predefined implementation on provider side",
                                function() {
                                    var testArgument = "This is my test argument";
                                    callOperation(
                                            "methodProvidedImpl",
                                            {
                                                arg : testArgument
                                            },
                                            {
                                                returnValue : testArgument
                                            });
                                });

                        afterEach(function() {
                            var shutDownWW;
                            var shutDownLibJoynr;

                            runs(function() {
                                IntegrationUtils.shutdownWebWorker(workerId).then(function() {
                                    shutDownWW = true;
                                });
                                IntegrationUtils.shutdownLibjoynr().then(function() {
                                    shutDownLibJoynr = true;
                                });
                            });

                            waitsFor(function() {
                                return shutDownWW && shutDownLibJoynr;
                            }, "WebWorker and Libjoynr to be shut down", 5000);
                        });

                    });
        });
