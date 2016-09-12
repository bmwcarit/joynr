/*global fail: true, xit: true */
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

define([
            "global/Promise",
            "joynr",
            "joynr/vehicle/RadioProxy",
            "joynr/vehicle/radiotypes/RadioStation",
            "joynr/vehicle/radiotypes/ErrorList",
            "joynr/datatypes/exampleTypes/Country",
            "joynr/datatypes/exampleTypes/StringMap",
            "joynr/tests/testTypes/ComplexTestType",
            "joynr/exceptions/SubscriptionException",
            "integration/IntegrationUtils",
            "joynr/provisioning/provisioning_cc",
            "integration/provisioning_end2end_common",
            "global/WaitsFor"
        ],
        function(
                Promise,
                joynr,
                RadioProxy,
                RadioStation,
                ErrorList,
                Country,
                StringMap,
                ComplexTestType,
                SubscriptionException,
                IntegrationUtils,
                provisioning,
                provisioning_end2end,
                waitsFor) {

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

                        beforeEach(function(done) {

                            var testProvisioning = null;
                            radioProxy = undefined;

                            provisioningSuffix = "End2EndCommTest" + "-" + Date.now();
                            testProvisioning =
                                    IntegrationUtils.getProvisioning(
                                            provisioning,
                                            provisioningSuffix);


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
                                    // Prevent freezing of object through proxy build
                                    // since we need to add faked attribute below
                                    spyOn(Object, 'freeze').and.callFake(function(obj) {
                                        return obj;
                                    });
                                    return IntegrationUtils.buildProxy(RadioProxy);
                                }).then(function(newRadioProxy) {
                                    radioProxy = newRadioProxy;

                                    // Add an attribute that does not exist on provider side
                                    // for special subscription test
                                    radioProxy.nonExistingAttributeOnProviderSide =
                                        new radioProxy.settings.proxyElementTypes.ProxyAttributeNotifyReadWrite(radioProxy, radioProxy.settings, "nonExistingAttributeOnProviderSide", "Integer");

                                    // restore freeze behavior
                                    Object.freeze.and.callThrough();
                                    radioProxy = Object.freeze(radioProxy);
                                    done();
                                    return null;
                                });
                            });
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
                                                    return null;
                                                }).catch(function(error) {
                                                    reject(new Error(
                                                            "Failed to retrieve attribute value (recursion index "
                                                                + index
                                                                + "): "
                                                                + error));
                                                });
                                        return null;
                                    }).catch(function(error) {
                                        reject(new Error(
                                                "Failed to set attribute value (recursion index "
                                                    + index
                                                    + "): "
                                                    + error));
                                    });
                                    return null;
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

                            radioProxy[operationName](opArgs).then(
                                    onFulfilledSpy).catch(IntegrationUtils.outputPromiseError);

                            return waitsFor(function() {
                                return onFulfilledSpy.calls.count() > 0;
                            }, "operation call to finish", provisioning.ttl).then(function() {
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

                            promise = radioProxy[subscribingEntity].subscribe({
                                subscriptionQos : subscriptionQos,
                                onReceive : spy.onReceive,
                                onError : spy.onError
                            }).then(spy.onFulfilled).catch(IntegrationUtils.outputPromiseError);

                            return waitsFor(function() {
                                return spy.onFulfilled.calls.count() > 0;
                            }, "subscription to be registered", provisioning.ttl).then(function() {
                                return Promise.resolve(spy);
                            });
                        }

                        function expectPublication(spy, expectationFct){
                            return waitsFor(
                                    function() {
                                        return (spy.onReceive.calls.count() > 0 || spy.onError.calls.count() > 0);
                                    },
                                    "first publication to occur",
                                    500 + provisioning.ttl).then(function() {
                                expect(spy.onReceive).toHaveBeenCalled();
                                expect(spy.onError).not.toHaveBeenCalled();
                                expectationFct(spy.onReceive.calls.mostRecent());
                                spy.onReceive.calls.reset();
                            });
                        }

                        function expectMultiplePublications(spy, expectedPublications, timeout, expectationFct){
                            return waitsFor(
                                    function() {
                                        return (spy.onReceive.calls.count() + spy.onError.calls.count() >= expectedPublications);
                                    },
                                    expectedPublications + "publications to occur",
                                    timeout).then(function() {
                                var promise = new Promise(function(resolve, reject) {
                                    setTimeout(function() {
                                        resolve();
                                    }, 100);
                                });
                                return promise.then(function() {
                                    expect(spy.onReceive.calls.count()).toBe(expectedPublications);
                                    expectationFct(spy.onReceive.calls);
                                    spy.onReceive.calls.reset();
                                });
                            });
                        }

                        function expectPublicationError(spy){
                            return waitsFor(
                                    function() {
                                        return (spy.onReceive.calls.count() > 0 || spy.onError.calls.count() > 0);
                                    },
                                    "first error to occur",
                                    500 + provisioning.ttl).then(function() {
                                expect(spy.onReceive).not.toHaveBeenCalled();
                                expect(spy.onError).toHaveBeenCalled();
                                expect(spy.onError.calls.argsFor(0)[0]._typeName).toEqual("joynr.exceptions.ProviderRuntimeException");
                                spy.onError.calls.reset();
                            });
                        }
                        function publishesValue(subscriptionQos) {
                            return setupSubscriptionAndReturnSpy("numberOfStations", subscriptionQos).then(function(spy) {
                                return expectPublication(spy, function(publicationCallback) {
                                    expect(typeof publicationCallback.args[0] === "number").toBeTruthy();
                                });
                            });
                        }

                        function checkUnsubscribe(timeout, subscriptionQos) {
                            var spy, subscriptionId;

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

                            var nrOfPublications;
                            return waitsFor(
                                    function() {
                                        return spy.onFulfilled.calls.count() > 0
                                            && (spy.onReceive.calls.count() > 0 || spy.onError.calls.count() > 0);
                                    },
                                    "subscription to be registered and first publication to occur",
                                    provisioning.ttl).then(function() {
                                expect(spy.onFulfilled).toHaveBeenCalled();
                                spy.onFulfilled.calls.reset();
                                nrOfPublications = spy.onReceive.calls.count();
                                radioProxy.numberOfStations.unsubscribe({
                                    subscriptionId : subscriptionId
                                }).then(spy.onFulfilled).catch(IntegrationUtils.outputPromiseError);

                                return waitsFor(function() {
                                    return spy.onFulfilled.calls.count() > 0;
                                }, "unsubscribe to complete successfully", provisioning.ttl);
                            }).then(function() {
                                expect(spy.onFulfilled).toHaveBeenCalled();

                                // wait 2 times the publication interval
                                var waitForPublication = true;
                                joynr.util.LongTimer.setTimeout(function() {
                                    waitForPublication = false;
                                }, 2 * timeout);

                                return waitsFor(function() {
                                    return !waitForPublication;
                                }, 4 * timeout);
                            }).then(function() {
                                // check that no publications occured since the unsubscribe
                                expect(spy.onReceive.calls.count()).toEqual(nrOfPublications);
                            });
                        }

                        var getAttribute = function(attributeName, expectedValue) {
                            var onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");

                            return waitsFor(function() {
                                return radioProxy !== undefined;
                            }, "radioProxy is defined", provisioning.ttl).then(function() {
                                radioProxy[attributeName].get().then(function(value) {
                                    expect(value).toEqual(expectedValue);
                                    onFulfilledSpy(value);
                                }).catch(IntegrationUtils.outputPromiseError);

                                return waitsFor(function() {
                                    return onFulfilledSpy.calls.count() > 0;
                                }, "attribute " + attributeName + " is received", provisioning.ttl);
                            }).then(function() {
                                expect(onFulfilledSpy).toHaveBeenCalled();
                            });
                        };

                        var getFailingAttribute = function(attributeName) {
                            var onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");
                            var catchSpy = jasmine.createSpy("catchSpy");

                            return waitsFor(function() {
                                return radioProxy !== undefined;
                            }, "radioProxy is defined", provisioning.ttl).then(function() {
                                radioProxy[attributeName].get().then(function(value) {
                                    onFulfilledSpy(value);
                                }).catch(function(exception) {
                                    catchSpy(exception);
                                });

                                return waitsFor(function() {
                                    return catchSpy.calls.count() > 0;
                                }, "getter for attribute " + attributeName + " returns exception", provisioning.ttl);
                            }).then(function() {
                                expect(catchSpy).toHaveBeenCalled();
                                expect(catchSpy.calls.argsFor(0)[0]._typeName).toBeDefined();
                                expect(catchSpy.calls.argsFor(0)[0]._typeName).toEqual("joynr.exceptions.ProviderRuntimeException");
                            });
                        };

                        var setAttribute = function(attributeName, value) {
                            var onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");

                            radioProxy[attributeName].set({
                                value : value
                            }).then(onFulfilledSpy).catch(IntegrationUtils.outputPromiseError);

                            return waitsFor(function() {
                                return onFulfilledSpy.calls.count() > 0;
                            }, "attribute is set", provisioning.ttl).then(function() {
                                expect(onFulfilledSpy).toHaveBeenCalled();
                            });
                        };

                        it("gets the attribute", function(done) {
                            getAttribute("isOn", true).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("gets the enumAttribute", function(done) {
                            getAttribute("enumAttribute", Country.GERMANY).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("gets the enumArrayAttribute", function(done) {
                            getAttribute("enumArrayAttribute", [Country.GERMANY]).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("gets an exception for failingSyncAttribute", function(done) {
                            getFailingAttribute("failingSyncAttribute").then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("gets an exception for failingAsyncAttribute", function(done) {
                            getFailingAttribute("failingAsyncAttribute").then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("sets the enumArrayAttribute", function(done) {
                            var value = [];
                            setAttribute("enumArrayAttribute", value).then(function() {
                                return getAttribute("enumArrayAttribute", value);
                            }).then(function() {
                                value = [Country.GERMANY, Country.AUSTRIA, Country.AUSTRALIA, Country.CANADA, Country.ITALY];
                                return setAttribute("enumArrayAttribute", value);
                            }).then(function() {
                                return getAttribute("enumArrayAttribute", value);
                            }).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("sets the typeDef attributes", function(done) {
                            var value = new RadioStation({
                                name: "TestEnd2EndComm.typeDefForStructAttribute.RadioStation",
                                byteBuffer: []
                            });
                            setAttribute("typeDefForStruct", value).then(function() {
                                return getAttribute("typeDefForStruct", value);
                            }).then(function() {
                                value = 1234543;
                                return setAttribute("typeDefForPrimitive", value);
                            }).then(function() {
                                return getAttribute("typeDefForPrimitive", value);
                            }).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("get/sets the attribute with starting capital letter", function(done) {
                            setAttribute("StartWithCapitalLetter", true).then(function() {
                                return getAttribute("StartWithCapitalLetter", true);
                            }).then(function() {
                                return setAttribute("StartWithCapitalLetter", false);
                            }).then(function() {
                                return getAttribute("StartWithCapitalLetter", false);
                            }).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("sets the attribute", function(done) {
                            setAttribute("isOn", true).then(function() {
                                return getAttribute("isOn", true);
                            }).then(function() {
                                return setAttribute("isOn", false);
                            }).then(function() {
                                return getAttribute("isOn", false);
                            }).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("sets the enumAttribute", function(done) {
                            setAttribute("enumAttribute", Country.AUSTRIA).then(function() {
                                return getAttribute("enumAttribute", Country.AUSTRIA);
                            }).then(function() {
                                return setAttribute("enumAttribute", Country.AUSTRALIA);
                            }).then(function() {
                                return getAttribute("enumAttribute", Country.AUSTRALIA);
                            }).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("sets the byteBufferAttribute", function(done) {
                            var testByteBuffer = [1,2,3,4];
                            setAttribute("byteBufferAttribute", testByteBuffer).then(function() {
                                return getAttribute("byteBufferAttribute", testByteBuffer);
                            }).then(function() {
                                return setAttribute("byteBufferAttribute", testByteBuffer);
                            }).then(function() {
                                return getAttribute("byteBufferAttribute", testByteBuffer);
                            }).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("sets the stringMapAttribute", function(done) {
                            var stringMap = new StringMap({key1: "value1"});
                            setAttribute("stringMapAttribute", stringMap).then(function() {
                                return getAttribute("stringMapAttribute", stringMap);
                            }).then(function() {
                                return setAttribute("stringMapAttribute", stringMap);
                            }).then(function() {
                                return getAttribute("stringMapAttribute", stringMap);
                            }).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("subscribe to failingSyncAttribute", function(done) {
                            setupSubscriptionAndReturnSpy("failingSyncAttribute", subscriptionQosInterval).then(function(spy) {
                                return expectPublicationError(spy);
                            }).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("subscribe to failingAsyncAttribute", function(done) {
                            setupSubscriptionAndReturnSpy("failingAsyncAttribute", subscriptionQosInterval).then(function(spy) {
                                return expectPublicationError(spy);
                            }).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("subscribe to enumAttribute", function(done) {
                            var mySpy;
                            setAttribute("enumAttribute", Country.AUSTRIA).then(function() {
                                return setupSubscriptionAndReturnSpy("enumAttribute", subscriptionQosOnChange);
                            }).then(function(spy) {
                                mySpy = spy;
                                return expectPublication(mySpy, function(call) {
                                   expect(call.args[0]).toEqual(Country.AUSTRIA);
                                });
                            }).then(function() {
                                return setAttribute("enumAttribute", Country.AUSTRALIA);
                            }).then(function() {
                                return expectPublication(mySpy, function(call) {
                                    expect(call.args[0]).toEqual(Country.AUSTRALIA);
                                });
                            }).then(function() {
                                return setAttribute("enumAttribute", Country.ITALY);
                            }).then(function() {
                                return expectPublication(mySpy, function(call) {
                                    expect(call.args[0]).toEqual(Country.ITALY);
                                });
                            }).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("subscribe to complex typedef attribute", function(done) {
                            var value = new RadioStation({
                                name: "TestEnd2EndComm.typeDefForStructAttribute.RadioStation",
                                byteBuffer: []
                            });
                            setAttribute("typeDefForStruct", value).then(function(done) {
                                return setupSubscriptionAndReturnSpy("typeDefForStruct", subscriptionQosOnChange);
                            }).then(function(spy) {
                                return expectPublication(spy, function(call) {
                                   expect(call.args[0]).toEqual(value);
                                });
                            }).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("subscribe to primitive typedef attribute", function(done) {
                            var value = 1234543;
                            var mySpy;
                            setAttribute("typeDefForPrimitive", value).then(function() {
                                return setupSubscriptionAndReturnSpy("typeDefForPrimitive", subscriptionQosOnChange);
                            }).then(function(spy) {
                                mySpy = spy;
                                return getAttribute("typeDefForPrimitive", value);
                            }).then(function() {
                                return expectPublication(mySpy, function(call) {
                                   expect(call.args[0]).toEqual(value);
                                });
                            }).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("subscribe to byteBufferAttribute", function(done) {
                            //initialize attribute
                            var mySpy;
                            var testByteBufferAttribute;

                            setAttribute("byteBufferAttribute", []).then(function() {
                                return setupSubscriptionAndReturnSpy("byteBufferAttribute", subscriptionQosOnChange);
                            }).then(function(spy) {
                                mySpy = spy;
                                return expectPublication(mySpy, function(call) {
                                    expect(call.args[0]).toEqual([]);
                                });
                            }).then(function() {
                                testByteBufferAttribute = function(expectedByteBuffer) {
                                    return setAttribute("byteBufferAttribute", expectedByteBuffer).then(function() {
                                        return expectPublication(mySpy, function(call) {
                                           expect(call.args[0]).toEqual(expectedByteBuffer);
                                        });
                                    });
                                };
                                return testByteBufferAttribute([0,1,2,3,4,5,6,7,8,9,8,7,6,5,4,3,2,1,0]);
                            }).then(function() {
                                return testByteBufferAttribute([255]);
                            }).then(function() {
                                return testByteBufferAttribute([2,2,2,2]);
                            }).then(function() {
                                var i, byteBuffer10k = [];
                                for (i = 0; i < 10000; i++) {
                                    byteBuffer10k.push(i % 256);
                                }
                                return testByteBufferAttribute(byteBuffer10k);
                            }).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("subscribe to weakSignal broadcast having ByteBuffer as output parameter", function(done) {
                            var mySpy;
                            setupSubscriptionAndReturnSpy("weakSignal", subscriptionQosOnChange).then(function(spy) {
                                mySpy = spy;
                                return callOperation("triggerBroadcasts", {
                                    broadcastName: "weakSignal",
                                    times: 1
                                });
                            }).then(function() {
                                return expectPublication(mySpy, function(call) {
                                   expect(call.args[0].radioStation).toEqual("radioStation");
                                   expect(call.args[0].byteBuffer).toEqual([0,1,2,3,4,5,6,7,8,9,8,7,6,5,4,3,2,1,0]);
                                });
                            }).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        //enable this test once the proxy side is ready for fire n' forget
                        it("call methodFireAndForgetWithoutParams and expect to call the provider", function(done) {
                            var mySpy;
                            setupSubscriptionAndReturnSpy("fireAndForgetCallArrived", subscriptionQosOnChange).then(function(spy) {
                                mySpy = spy;
                                return callOperation("methodFireAndForgetWithoutParams", {});
                            }).then(function() {
                                return expectPublication(mySpy, function(call) {
                                   expect(call.args[0].methodName).toEqual("methodFireAndForgetWithoutParams");
                                });
                            }).then(function() {
                                done();
                            }).catch(fail);
                        });

                        //enable this test once the proxy side is ready for fire n' forget
                        it("call methodFireAndForget and expect to call the provider", function(done) {
                            var mySpy;
                            setupSubscriptionAndReturnSpy("fireAndForgetCallArrived", subscriptionQosOnChange).then(function(spy) {
                                mySpy = spy;
                                return callOperation("methodFireAndForget", {
                                    intIn: 0,
                                    stringIn : "methodFireAndForget",
                                    complexTestTypeIn : new ComplexTestType({
                                        a : 0,
                                        b : 1
                                    })
                                });
                            }).then(function() {
                                return expectPublication(mySpy, function(call) {
                                   expect(call.args[0].methodName).toEqual("methodFireAndForget");
                                });
                            }).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("subscribe to broadcastWithEnum", function(done) {
                            var mySpy;
                            setupSubscriptionAndReturnSpy("broadcastWithEnum", subscriptionQosOnChange).then(function(spy) {
                                mySpy = spy;
                                return callOperation("triggerBroadcasts", {
                                    broadcastName: "broadcastWithEnum",
                                    times: 1 });
                            }).then(function() {
                                return expectPublication(mySpy, function(call) {
                                   expect(call.args[0].enumOutput).toEqual(Country.CANADA);
                                   expect(call.args[0].enumArrayOutput).toEqual([Country.GERMANY, Country.ITALY]);
                                });
                            }).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("subscribe to emptyBroadcast", function(done) {
                            var mySpy;
                            setupSubscriptionAndReturnSpy("emptyBroadcast", subscriptionQosOnChange).then(function(spy) {
                                mySpy = spy;
                                return callOperation("triggerBroadcasts", {
                                    broadcastName: "emptyBroadcast",
                                    times: 1 });
                            }).then(function() {
                                return expectPublication(mySpy, function(call) {
                                    //no expectation for call, as empty broadcast
                                });
                            }).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("subscribe to type def broadcast", function(done) {
                            var typeDefStructOutput = new RadioStation({
                                name: "TestEnd2EndCommProviderWorker.broadcastWithTypeDefs.RadioStation",
                                byteBuffer: []
                            });
                            var typeDefPrimitiveOutput = 123456;
                            var mySpy;

                            setupSubscriptionAndReturnSpy("broadcastWithTypeDefs", subscriptionQosOnChange).then(function(spy) {
                                mySpy = spy;
                                return callOperation("triggerBroadcasts", {
                                    broadcastName: "broadcastWithTypeDefs",
                                    times: 1 });
                            }).then(function() {
                                return expectPublication(mySpy, function(call) {
                                   expect(call.args[0].typeDefStructOutput).toEqual(typeDefStructOutput);
                                   expect(call.args[0].typeDefPrimitiveOutput).toEqual(typeDefPrimitiveOutput);
                                });
                            }).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("subscribe to broadcastWithEnum and get burst", function(done) {
                            subscriptionQosOnChange.minIntervalMs = 0;
                            var times = 100;
                            var mySpy;
                            setupSubscriptionAndReturnSpy("broadcastWithEnum", subscriptionQosOnChange).then(function(spy) {
                                mySpy = spy;
                                return callOperation("triggerBroadcasts", {
                                    broadcastName: "broadcastWithEnum",
                                    times: times });
                            }).then(function() {
                                return expectMultiplePublications(mySpy, times, 5000, function(calls) {
                                   var i;
                                   for (i=0;i<times;i++) {
                                       expect(calls.argsFor(i)[0].enumOutput).toEqual(Country.CANADA);
                                       expect(calls.argsFor(i)[0].enumArrayOutput).toEqual([Country.GERMANY, Country.ITALY]);
                                   }
                                });
                            }).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("subscribe to enumArrayAttribute", function(done) {
                            var attributeName = "enumArrayAttribute";
                            var value1 = [Country.AUSTRIA];
                            var value2 = [Country.AUSTRIA, Country.GERMANY];
                            var value3 = [Country.AUSTRIA, Country.GERMANY, Country.ITALY];
                            var mySpy;
                            setAttribute(attributeName, value1).then(function() {
                                return setupSubscriptionAndReturnSpy(attributeName, subscriptionQosOnChange);
                            }).then(function(spy) {
                                mySpy = spy;
                                return expectPublication(mySpy, function(call) {
                                   expect(call.args[0]).toEqual(value1);
                                });
                            }).then(function() {
                                return setAttribute(attributeName, value2);
                            }).then(function() {
                                return expectPublication(mySpy, function(call) {
                                    expect(call.args[0]).toEqual(value2);
                                });
                            }).then(function() {
                                return setAttribute(attributeName, value3);
                            }).then(function() {
                                return expectPublication(mySpy, function(call) {
                                    expect(call.args[0]).toEqual(value3);
                                });
                            }).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("resolves attribute subscription and calls onSubscribed", function(done) {
                            var spy = jasmine.createSpyObj("spy", [
                                "onReceive",
                                "onError",
                                "onSubscribed"
                            ]);
                            var subscriptionId, detailMessage;
                            var qosSettings = new joynr.proxy.PeriodicSubscriptionQos();
                            var storedSubscriptionId;

                            radioProxy.numberOfStations.subscribe({
                                subscriptionQos : qosSettings,
                                onReceive : spy.onReceive,
                                onError : spy.onError,
                                onSubscribed: spy.onSubscribed
                            }).then(function(subscriptionId) {
                                storedSubscriptionId = subscriptionId;
                                return waitsFor(function() {
                                    return spy.onSubscribed.calls.count() === 1;
                                }, "onSubscribed to get called", 1000);
                            }).then(function() {
                                expect(spy.onError).not.toHaveBeenCalled();
                                expect(spy.onSubscribed).toHaveBeenCalled();
                                expect(spy.onSubscribed).toHaveBeenCalledWith(storedSubscriptionId);
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("rejects attribute subscription if periodMs is too small", function(done) {
                            var spy = jasmine.createSpyObj("spy", [
                                "onReceive",
                                "onError",
                                "onSubscribed"
                            ]);
                            var subscriptionId, detailMessage;
                            var qosSettings = new joynr.proxy.PeriodicSubscriptionQos({
                                expiryDateMs : 0,
                                alertAfterIntervalMs : 0,
                                publicationTtlMs : 1000
                            });
                            // forcibly fake it! The constructor throws, if using this directly
                            qosSettings.periodMs = joynr.proxy.PeriodicSubscriptionQos.MIN_PERIOD_MS - 1;

                            radioProxy.numberOfStations.subscribe({
                                subscriptionQos : qosSettings,
                                onReceive : spy.onReceive,
                                onError : spy.onError,
                                onSubscribed : spy.onSubscribed
                            }).then(function(subscriptionId) {
                                fail("unexpected success");
                            }).catch(function(error) {
                                expect(error).toBeDefined();
                                expect(error instanceof SubscriptionException);
                                expect(error.subscriptionId).toBeDefined();
                                expect(error.detailMessage).toMatch(/is smaller than PeriodicSubscriptionQos/);
                                subscriptionId = error.subscriptionId;
                                detailMessage = error.detailMessage;
                                return waitsFor(function() {
                                    return spy.onError.calls.count() === 1;
                                }, "onError to get called", 1000);
                            }).then(function() {
                                expect(spy.onReceive).not.toHaveBeenCalled();
                                expect(spy.onSubscribed).not.toHaveBeenCalled();
                                expect(spy.onError).toHaveBeenCalled();
                                expect(spy.onError.calls.mostRecent().args[0]).toBeDefined();
                                var error = spy.onError.calls.mostRecent().args[0];
                                expect(error instanceof SubscriptionException);
                                expect(error.subscriptionId).toEqual(subscriptionId);
                                expect(error.detailMessage).toEqual(detailMessage);
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("rejects subscription to non-existing attribute ", function(done) {
                            var spy = jasmine.createSpyObj("spy", [
                                "onReceive",
                                "onError",
                                "onSubscribed"
                            ]);
                            var subscriptionId, detailMessage;

                            radioProxy.nonExistingAttributeOnProviderSide.subscribe({
                                subscriptionQos : new joynr.proxy.OnChangeSubscriptionQos(),
                                onReceive : spy.onReceive,
                                onError : spy.onError,
                                onSubscribed : spy.onSubscribed
                            }).then(function(subscriptionId) {
                                fail("unexpected success");
                            }).catch(function(error) {
                                expect(error).toBeDefined();
                                expect(error instanceof SubscriptionException);
                                expect(error.subscriptionId).toBeDefined();
                                expect(error.detailMessage).toMatch(/misses attribute/);
                                subscriptionId = error.subscriptionId;
                                detailMessage = error.detailMessage;
                                return waitsFor(function() {
                                    return spy.onError.calls.count() === 1;
                                }, "onError to get called", 1000);
                            }).then(function() {
                                expect(spy.onReceive).not.toHaveBeenCalled();
                                expect(spy.onSubscribed).not.toHaveBeenCalled();
                                expect(spy.onError).toHaveBeenCalled();
                                expect(spy.onError.calls.mostRecent().args[0]).toBeDefined();
                                var error = spy.onError.calls.mostRecent().args[0];
                                expect(error instanceof SubscriptionException);
                                expect(error.subscriptionId).toEqual(subscriptionId);
                                expect(error.detailMessage).toEqual(detailMessage);
                                done();
                                return null;
                            }).catch(fail);
                        });

                        // The following test does unfortunately not work, because the expiryDateMs
                        // is used to shorten the ttl of the message in SubscriptionManager.
                        // This leads to a negative ttl, which has ChannelMessagingSender
                        // throwing an Error at us (where it should throw an exception).
                        // The message is thus not getting send to provider.
                        // Leaving the test in at the moment so it can be used later
                        // once exception handling gets fixed.
                        xit("rejects on subscribe to attribute with bad expiryDateMs", function(done) {
                            var spy = jasmine.createSpyObj("spy", [
                                "onReceive",
                                "onError"
                            ]);
                            var subscriptionId, detailMessage;

                            radioProxy.numberOfStations.subscribe({
                                subscriptionQos : new joynr.proxy.OnChangeSubscriptionQos({
                                    expiryDateMs : Date.now() - 10000
                                }),
                                onReceive : spy.onReceive,
                                onError : spy.onError
                            }).then(function(subscriptionId) {
                                fail("unexpected success");
                            }).catch(function(error) {
                                expect(error).toBeDefined();
                                expect(error instanceof SubscriptionException);
                                expect(error.subscriptionId).toBeDefined();
                                expect(error.detailMessage).toMatch(/lies in the past/);
                                subscriptionId = error.subscriptionId;
                                detailMessage = error.detailMessage;
                                return waitsFor(function() {
                                    return spy.onError.calls.count() === 1;
                                }, "onError to get called", 1000);
                            }).then(function() {
                                expect(spy.onReceive).not.toHaveBeenCalled();
                                expect(spy.onError).toHaveBeenCalled();
                                expect(spy.onError.calls.mostRecent().args[0]).toBeDefined();
                                var error = spy.onError.calls.mostRecent().args[0];
                                expect(error instanceof SubscriptionException);
                                expect(error.subscriptionId).toEqual(subscriptionId);
                                expect(error.detailMessage).toEqual(detailMessage);
                                done();
                                return null;
                            }).catch(fail);
                        });

                        function setAndTestAttributeTester(attribute) {
                            var lastRecursionIndex = -1, recursions = 5, onFulfilledSpy =
                                    jasmine.createSpy("onFulfilledSpy");

                            setAndTestAttribute(attribute, recursions - 1).then(
                                    onFulfilledSpy).catch(IntegrationUtils.outputPromiseError);

                            return waitsFor(function() {
                                return onFulfilledSpy.calls.count() > 0;
                            }, "get/set test to finish", 2 * provisioning.ttl * recursions).then(function() {
                                // each
                                // repetition
                                // consists
                                // of a
                                // get
                                // and
                                // set
                                // => 2
                                // ttls per repetition

                                // additional sanity check whether recursion level
                                // really went down
                                // to 0
                                expect(onFulfilledSpy).toHaveBeenCalled();
                                expect(onFulfilledSpy).toHaveBeenCalledWith(0);
                            });
                        }

                        // when provider is working this should also work
                        it("checks whether the provider stores the attribute value", function(done) {
                            setAndTestAttributeTester(radioProxy.isOn).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("can call an operation successfully (Provider sync, String parameter)", function(done) {
                            callOperation("addFavoriteStation",
                                    {
                                radioStation : 'stringStation'
                                    }
                            ).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("can call an operation and get a ProviderRuntimeException (Provider sync, String parameter)", function(done) {
                            var onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");
                            var catchSpy = jasmine.createSpy("catchSpy");

                            radioProxy.addFavoriteStation({
                                radioStation : 'stringStationerror'
                            }).then(onFulfilledSpy).catch(catchSpy);

                            waitsFor(function() {
                                return catchSpy.calls.count() > 0;
                            }, "operation call to fail", provisioning.ttl).then(function() {
                                expect(onFulfilledSpy).not.toHaveBeenCalled();
                                expect(catchSpy).toHaveBeenCalled();
                                expect(catchSpy.calls.argsFor(0)[0]._typeName).toBeDefined();
                                expect(catchSpy.calls.argsFor(0)[0]._typeName).toEqual("joynr.exceptions.ProviderRuntimeException");
                                expect(catchSpy.calls.argsFor(0)[0].detailMessage).toBeDefined();
                                expect(catchSpy.calls.argsFor(0)[0].detailMessage).toEqual("example message sync");
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("can call an operation and get an ApplicationException (Provider sync, String parameter)", function(done) {
                            var onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");
                            var catchSpy = jasmine.createSpy("catchSpy");

                            radioProxy.addFavoriteStation({
                                radioStation : 'stringStationerrorApplicationException'
                            }).then(onFulfilledSpy).catch(catchSpy);

                            waitsFor(function() {
                                return catchSpy.calls.count() > 0;
                            }, "operation call to fail", provisioning.ttl).then(function() {
                                expect(onFulfilledSpy).not.toHaveBeenCalled();
                                expect(catchSpy).toHaveBeenCalled();
                                expect(catchSpy.calls.argsFor(0)[0]._typeName).toBeDefined();
                                expect(catchSpy.calls.argsFor(0)[0]._typeName).toEqual("joynr.exceptions.ApplicationException");
                                expect(catchSpy.calls.argsFor(0)[0].error).toBeDefined();
                                //expect(catchSpy.calls.argsFor(0)[0].error).toEqual(ErrorList.EXAMPLE_ERROR_2);
                                expect(catchSpy.calls.argsFor(0)[0].error).toEqual(jasmine.objectContaining(ErrorList.EXAMPLE_ERROR_2));
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("can call an operation successfully (Provider async, String parameter)", function(done) {
                            var onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");

                            radioProxy.addFavoriteStation({
                                radioStation : 'stringStationasync'
                            }).then(onFulfilledSpy).catch(IntegrationUtils.outputPromiseError);

                            waitsFor(function() {
                                return onFulfilledSpy.calls.count() > 0;
                            }, "operation call to finish", provisioning.ttl).then(function() {
                                expect(onFulfilledSpy).toHaveBeenCalled();
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("can call an operation and get a ProviderRuntimeException (Provider async, String parameter)", function(done) {
                            var onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");
                            var catchSpy = jasmine.createSpy("catchSpy");

                            radioProxy.addFavoriteStation({
                                radioStation : 'stringStationasyncerror'
                            }).then(onFulfilledSpy).catch(catchSpy);

                            waitsFor(function() {
                                return catchSpy.calls.count() > 0;
                            }, "operation call to fail", provisioning.ttl).then(function() {
                                expect(onFulfilledSpy).not.toHaveBeenCalled();
                                expect(catchSpy).toHaveBeenCalled();
                                expect(catchSpy.calls.argsFor(0)[0]._typeName).toBeDefined();
                                expect(catchSpy.calls.argsFor(0)[0]._typeName).toEqual("joynr.exceptions.ProviderRuntimeException");
                                expect(catchSpy.calls.argsFor(0)[0].detailMessage).toBeDefined();
                                expect(catchSpy.calls.argsFor(0)[0].detailMessage).toEqual("example message async");
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("can call an operation and get an ApplicationException (Provider async, String parameter)", function(done) {
                            var onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");
                            var catchSpy = jasmine.createSpy("catchSpy");

                            radioProxy.addFavoriteStation({
                                radioStation : 'stringStationasyncerrorApplicationException'
                            }).then(onFulfilledSpy).catch(catchSpy);

                            waitsFor(function() {
                                return catchSpy.calls.count() > 0;
                            }, "operation call to fail", provisioning.ttl).then(function() {
                                expect(onFulfilledSpy).not.toHaveBeenCalled();
                                expect(catchSpy).toHaveBeenCalled();
                                expect(catchSpy.calls.argsFor(0)[0]._typeName).toBeDefined();
                                expect(catchSpy.calls.argsFor(0)[0]._typeName).toEqual("joynr.exceptions.ApplicationException");
                                expect(catchSpy.calls.argsFor(0)[0].error).toBeDefined();
                                //expect(catchSpy.calls.argsFor(0)[0].error).toEqual(ErrorList.EXAMPLE_ERROR_1);
                                expect(catchSpy.calls.argsFor(0)[0].error).toEqual(jasmine.objectContaining(ErrorList.EXAMPLE_ERROR_1));
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("can call an operation (parameter of complex type)", function(done) {
                            callOperation("addFavoriteStation",
                                    {
                                        radioStation : 'stringStation'
                                    }
                            ).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it(
                                "can call an operation (parameter of byteBuffer type",
                                function(done) {
                                    callOperation(
                                            "methodWithByteBuffer",
                                            {
                                                input : [0,1,2,3,4,5,6,7,8,9,8,7,6,5,4,3,2,1,0]
                                            },
                                            {
                                                result : [0,1,2,3,4,5,6,7,8,9,8,7,6,5,4,3,2,1,0]
                                            }).then(function() {
                                        done();
                                        return null;
                                    }).catch(fail);
                               });

                        it(
                                "can call an operation with working parameters and return type",
                                function(done) {
                                    callOperation(
                                            "addFavoriteStation",
                                            {
                                                radioStation : "truelyContainingTheString\"True\""
                                            },
                                            {
                                                returnValue : true
                                    }).then(function() {
                                        return callOperation(
                                                "addFavoriteStation",
                                                {
                                                    radioStation : "This is false!"
                                                },
                                                {
                                                    returnValue : false
                                                });
                                    }).then(function() {
                                        return callOperation(
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
                                    }).then(function() {
                                        return callOperation(
                                                "addFavoriteStation",
                                                {
                                                    radioStation : new RadioStation({
                                                        name : "This is a false RadioStation!"
                                                    })
                                                },
                                                {
                                                    returnValue : false
                                                });
                                    }).then(function() {
                                        done();
                                        return null;
                                    }).catch(fail);
                                });

                        it(
                                "can call an operation with typedef arguments",
                                function(done) {
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
                                            }).then(function() {
                                        done();
                                        return null;
                                    }).catch(fail);
                               });

                        it(
                                "can call an operation with enum arguments and enum return type",
                                function(done) {
                                    callOperation(
                                            "operationWithEnumsAsInputAndOutput",
                                            {
                                                enumInput : Country.GERMANY,
                                                enumArrayInput : []
                                            },
                                            {
                                                enumOutput : Country.GERMANY
                                            }).then(function() {
                                        return callOperation(
                                                "operationWithEnumsAsInputAndOutput",
                                                {
                                                    enumInput : Country.GERMANY,
                                                    enumArrayInput : [Country.AUSTRIA]
                                                },
                                                {
                                                    enumOutput : Country.AUSTRIA
                                                });
                                    }).then(function() {
                                        return callOperation(
                                                "operationWithEnumsAsInputAndOutput",
                                                {
                                                    enumInput : Country.GERMANY,
                                                    enumArrayInput : [Country.AUSTRIA, Country.GERMANY, Country.AUSTRALIA]
                                                },
                                                {
                                                    enumOutput : Country.AUSTRIA
                                                });
                                    }).then(function() {
                                        return callOperation(
                                                "operationWithEnumsAsInputAndOutput",
                                                {
                                                    enumInput : Country.GERMANY,
                                                    enumArrayInput : [Country.CANADA, Country.AUSTRIA, Country.ITALY]
                                                },
                                                {
                                                    enumOutput : Country.CANADA
                                                });
                                    }).then(function() {
                                        done();
                                        return null;
                                    }).catch(fail);
                               });

                        it(
                                "can call an operation with multiple return values and async provider",
                                function(done) {
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
                                            }).then(function() {
                                        done();
                                        return null;
                                    }).catch(fail);

                        });

                        it(
                                "can call an operation with multiple return values and sync provider",
                                function(done) {
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
                                            }).then(function() {
                                        done();
                                        return null;
                                    }).catch(fail);
                        });

                        it(
                                "can call an operation with enum arguments and enum array as return type",
                                function(done) {
                                    callOperation(
                                            "operationWithEnumsAsInputAndEnumArrayAsOutput",
                                            {
                                                enumInput : Country.GERMANY,
                                                enumArrayInput : []
                                            },
                                            {
                                                enumOutput : [Country.GERMANY]
                                    }).then(function() {
                                        return callOperation(
                                                "operationWithEnumsAsInputAndEnumArrayAsOutput",
                                                {
                                                    enumInput : Country.GERMANY,
                                                    enumArrayInput : [Country.AUSTRIA]
                                                },
                                                {
                                                    enumOutput : [Country.AUSTRIA, Country.GERMANY]
                                                });
                                    }).then(function() {
                                        return callOperation(
                                                "operationWithEnumsAsInputAndEnumArrayAsOutput",
                                                {
                                                    enumInput : Country.GERMANY,
                                                    enumArrayInput : [Country.AUSTRIA, Country.GERMANY, Country.AUSTRALIA]
                                                },
                                                {
                                                    enumOutput : [Country.AUSTRIA, Country.GERMANY, Country.AUSTRALIA, Country.GERMANY]
                                                });
                                    }).then(function() {
                                        return callOperation(
                                                "operationWithEnumsAsInputAndEnumArrayAsOutput",
                                                {
                                                    enumInput : Country.GERMANY,
                                                    enumArrayInput : [Country.CANADA, Country.AUSTRIA, Country.ITALY]
                                                },
                                                {
                                                    enumOutput : [Country.CANADA, Country.AUSTRIA, Country.ITALY, Country.GERMANY]
                                                });
                                    }).then(function() {
                                        done();
                                        return null;
                                    }).catch(fail);
                               });

                        it(
                                "can call an operation with double array as argument and string array as return type",
                                function(done) {
                                    callOperation(
                                            "methodWithSingleArrayParameters",
                                            {
                                                doubleArrayArg : [0.01,1.1,2.2,3.3]
                                            },
                                            {
                                                stringArrayOut : ["0.01", "1.1", "2.2", "3.3"]
                                            }).then(function() {
                                        done();
                                        return null;
                                    }).catch(fail);
                               });

                        it("can start a subscription and provides a subscription id", function(done) {
                            var spy;

                            spy = jasmine.createSpyObj("spy", [
                                "onFulfilled",
                                "onReceive",
                                "onError"
                            ]);
                            radioProxy.numberOfStations.subscribe({
                                subscriptionQos : subscriptionQosOnChange,
                                onReceive : spy.onReceive,
                                onError : spy.onError
                            }).then(spy.onFulfilled).catch(function(error) {
                                spy.onError(error);
                                IntegrationUtils.outputPromiseError(error);
                            });

                            waitsFor(function() {
                                return spy.onFulfilled.calls.count() > 0;
                            }, "subscription to be registered", provisioning.ttl).then(function() {
                                expect(spy.onFulfilled).toHaveBeenCalled();
                                expect(typeof spy.onFulfilled.calls.mostRecent().args[0] === "string")
                                        .toBeTruthy();
                                expect(spy.onError).not.toHaveBeenCalled();
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("publishes a value with an onChange subscription", function(done) {
                            publishesValue(subscriptionQosOnChange).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("publishes a value with an interval subscription", function(done) {
                            publishesValue(subscriptionQosInterval).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("publishes a value with a mixed subscription", function(done) {
                            publishesValue(subscriptionQosMixed).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("publishes a value with an ending subscription", function(done) {
                            subscriptionQosMixed.expiryDateMs = subscriptionLength + Date.now();
                            publishesValue(subscriptionQosMixed).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it(
                                "initially publishes a value on subscription",
                                function(done) {
                                    var spy;

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

                                    // timeout for
                                    // subscription request
                                    // round trip
                                    waitsFor(
                                            function() {
                                                return spy.onFulfilled.calls.count() > 0
                                                    && (spy.onReceive.calls.count() > 0 || spy.onError.calls.count() > 0);
                                            },
                                            "initial onReceive to occur",
                                            provisioning.ttl).then(function() {
                                        expect(spy.onFulfilled).toHaveBeenCalled();
                                        expect(spy.onReceive).toHaveBeenCalled();
                                        expect(spy.onReceive.calls.argsFor(0)[0]).toEqual(-1);
                                        expect(spy.onError).not.toHaveBeenCalled();
                                        done();
                                        return null;
                                    }).catch(fail);
                                });

                        var nrPubs = 3;
                        it(
                                "publishes correct values with onChange " + nrPubs + " times",
                                function(done) {
                                    var spy;

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

                                    // timeout
                                    // for
                                    // subscription
                                    // request
                                    // round
                                    // trip
                                    waitsFor(function() {
                                        return spy.onFulfilled.calls.count() > 0;
                                    }, "subscription promise to resolve", provisioning.ttl).then(function() {
                                        expect(spy.onFulfilled).toHaveBeenCalled();

                                        // timeout for
                                        // subscription request
                                        // round trip
                                        return waitsFor(
                                            function() {
                                                return (spy.onReceive.calls.count() > 0 || spy.onError.calls.count() > 0);
                                            },
                                            "initial onReceive to occur",
                                            provisioning.ttl);
                                    }).then(function() {
                                        expect(spy.onReceive).toHaveBeenCalled();
                                        expect(spy.onReceive.calls.argsFor(0)[0]).toEqual(-1);
                                        expect(spy.onError).not.toHaveBeenCalled();

                                        spy.onReceive.calls.reset();
                                        spy.onError.calls.reset();

                                        // for subscription request round trip and nrPubs publications, safety (+1)
                                        return waitsFor(
                                            function() {
                                                return (spy.onReceive.calls.count() >= nrPubs || spy.onError.calls.count() > 0);
                                            },
                                            "subscription to be registered and first publication to occur",
                                            provisioning.ttl + (nrPubs + 1) * 1000); // timeout
                                    }).then(function() {
                                        expect(spy.onReceive).toHaveBeenCalled();
                                        expect(spy.onError).not.toHaveBeenCalled();

                                        expect(spy.onReceive.calls.count()).toEqual(nrPubs);

                                        var i;
                                        for (i = 0; i < nrPubs; ++i) {
                                            expect(spy.onReceive.calls.argsFor(i)[0]).toEqual(i);
                                        }
                                    }).then(function() {
                                        done();
                                        return null;
                                    }).catch(fail);
                                });

                        it(
                                "publishes correct values with a mixed subscription",
                                function(done) {
                                    var spy;

                                    // provider will fire an interval publication 1s after
                                    // initialization with the response "interval"
                                    // after another 0.5 ms it will fire an onChange publication
                                    // with the response "valueChanged1"
                                    // after another 10 ms it will try to fire an onChange
                                    // publication with the response "valueChanged2", should be
                                    // blocked by the
                                    // PublicationManager on the Provider side

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

                                    waitsFor(function() {
                                        return spy.onFulfilled.calls.count() > 0;
                                    }, "subscription to be registered", provisioning.ttl).then(function() {
                                        expect(spy.onFulfilled).toHaveBeenCalled();

                                        return waitsFor(
                                                function() {
                                                    return (spy.onReceive.calls.count() > 0 || spy.onError.calls.count() > 0);
                                                },
                                                "initial publication to occur",
                                                provisioning.ttl); // timeout for
                                        // subscription request
                                        // round trip
                                    }).then(function() {
                                        expect(spy.onReceive).toHaveBeenCalled();
                                        expect(spy.onReceive.calls.argsFor(0)[0])
                                                .toEqual("interval");
                                        expect(spy.onError).not.toHaveBeenCalled();

                                        spy.onReceive.calls.reset();
                                        spy.onError.calls.reset();

                                        return waitsFor(
                                                function() {
                                                        return (spy.onReceive.calls.count() > 0 || spy.onError.calls.count() > 0);
                                                },
                                                "the interval publication to occur",
                                                subscriptionQosMixed.maxIntervalMs + provisioning.ttl);
                                    }).then(function() {
                                        expect(spy.onReceive).toHaveBeenCalled();
                                        expect(spy.onReceive).toHaveBeenCalledWith("interval");
                                        spy.onReceive.calls.reset();

                                        return waitsFor(function() {
                                            return spy.onReceive.calls.count() > 0;
                                        }, "the onChange publication to occur", 500 + provisioning.ttl);
                                    }).then(function() {
                                        var timeout;

                                        timeout = false;
                                        expect(spy.onReceive).toHaveBeenCalled();
                                        expect(spy.onReceive).toHaveBeenCalledWith(
                                                "valueChanged1");
                                        spy.onReceive.calls.reset();
                                        joynr.util.LongTimer.setTimeout(function() {
                                            timeout = true;
                                        }, subscriptionQosMixed.minIntervalMs / 2);

                                        return waitsFor(
                                            function() {
                                                return timeout || spy.onReceive.calls.count() > 0;
                                            },
                                            "the second onChange publication to not occur",
                                            subscriptionQosMixed.minIntervalMs + safetyTimeout);
                                    }).then(function() {
                                        expect(spy.onReceive).not.toHaveBeenCalled();

                                        return waitsFor(
                                            function() {
                                                return spy.onReceive.calls.count() > 0;
                                            },
                                            "the second onChange publication occur after minIntervalMs",
                                            provisioning.ttl + safetyTimeout);
                                    }).then(function() {
                                        expect(spy.onReceive).toHaveBeenCalled();
                                        expect(spy.onReceive).toHaveBeenCalledWith("interval");
                                        done();
                                        return null;
                                    }).catch(fail);
                                });

                        it(
                                "terminates correctly according to the endDate ",
                                function(done) {
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

                                    waitsFor(function() {
                                        return spy.onFulfilled.calls.count() > 0;
                                    }, "subscription to be registered", provisioning.ttl).then(function() {
                                        expect(spy.onFulfilled).toHaveBeenCalled();
                                        return waitsFor(
                                            function() {
                                                return (spy.onReceive.calls.count() > 0 || spy.onError.calls.count() > 0);
                                            },
                                            "initial publication to occur",
                                            provisioning.ttl); // timeout for
                                        // subscription request
                                        // round trip
                                    }).then(function() {
                                        expect(spy.onReceive).toHaveBeenCalled();
                                        expect(spy.onReceive.calls.argsFor(0)[0]).toEqual(true);
                                        expect(spy.onError).not.toHaveBeenCalled();

                                        spy.onReceive.calls.reset();
                                        spy.onError.calls.reset();

                                        return waitsFor(
                                            function() {
                                                    return (spy.onReceive.calls.count() > 0 || spy.onError.calls.count() > 0);
                                            },
                                            "the interval onReceive to occur",
                                            subscriptionQosMixed.maxIntervalMs + provisioning.ttl);
                                    }).then(function() {
                                        expect(spy.onReceive).toHaveBeenCalled();
                                        expect(spy.onReceive).toHaveBeenCalledWith(true);
                                        spy.onReceive.calls.reset();

                                        joynr.util.LongTimer.setTimeout(function() {
                                            timeout = true;
                                        }, subscriptionQosMixed.maxIntervalMs + provisioning.ttl);
                                        return waitsFor(
                                            function() {
                                                return timeout || spy.onReceive.calls.count() > 0;
                                            },
                                            "the interval onReceive not to occur again",
                                            subscriptionQosMixed.maxIntervalMs
                                                + provisioning.ttl
                                                + safetyTimeout);
                                    }).then(function() {
                                        expect(spy.onReceive).not.toHaveBeenCalled();
                                        done();
                                        return null;
                                    }).catch(fail);
                        }, 10000);

                        it("unsubscribes onChange subscription successfully", function(done) {
                            checkUnsubscribe(1000, subscriptionQosOnChange).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("unsubscribes interval subscription successfully", function(done) {
                            checkUnsubscribe(1000, subscriptionQosInterval).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it(
                                "attributes are working with predefined implementation on provider side",
                                function(done) {
                                    //setAndTestAttributeRecursive(radioProxy.attrProvidedImpl).then(function() {
                                    setAndTestAttribute(radioProxy.attrProvidedImpl, 0).then(function() {
                                        done();
                                        return null;
                                    }).catch(fail);
                                });

                        it(
                                "operation is working with predefined implementation on provider side",
                                function(done) {
                                    var testArgument = "This is my test argument";
                                    callOperation(
                                            "methodProvidedImpl",
                                            {
                                                arg : testArgument
                                            },
                                            {
                                                returnValue : testArgument
                                    }).then(function() {
                                        done();
                                        return null;
                                    }).catch(fail);
                                });

                        afterEach(function(done) {
                            IntegrationUtils.shutdownWebWorker(workerId).then(function() {
                                return IntegrationUtils.shutdownLibjoynr();
                            }).then(function() {
                                done();
                                return null;
                            }).catch(function() {
                                throw new Error("shutdown Webworker and Libjoynr failed");
                            });
                        });

                    });
        });
