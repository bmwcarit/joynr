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
            "integration/End2EndAbstractTest",
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
                End2EndAbstractTest,
                provisioning,
                provisioning_end2end,
                waitsFor) {

            describe(
                    "libjoynr-js.integration.end2end.subscription",
                    function() {

                        var subscriptionLength = 2000;
                        var safetyTimeout = 200;
                        var subscriptionQosOnChange;
                        var subscriptionQosInterval;
                        var subscriptionQosMulticast;
                        var subscriptionQosMixed;
                        var radioProxy;
                        var abstractTest = new End2EndAbstractTest("End2EndSubscriptionTest");
                        var setAttribute = abstractTest.setAttribute;
                        var setupSubscriptionAndReturnSpy = abstractTest.setupSubscriptionAndReturnSpy;
                        var unsubscribeSubscription = abstractTest.unsubscribeSubscription;
                        var callOperation = abstractTest.callOperation;
                        var expectPublication = abstractTest.expectPublication;

                        beforeEach(function(done) {
                            abstractTest.beforeEach().then(function(settings) {
                                subscriptionQosOnChange = new joynr.proxy.OnChangeSubscriptionQos({
                                    minIntervalMs : 50
                                });

                                subscriptionQosInterval = new joynr.proxy.PeriodicSubscriptionQos({
                                    periodMs : 1000
                                });

                                subscriptionQosMixed = new joynr.proxy.OnChangeWithKeepAliveSubscriptionQos({
                                    minIntervalMs : 100,
                                    maxIntervalMs : 1000
                                });

                                subscriptionQosMulticast = new joynr.proxy.MulticastSubscriptionQos({
                                    validityMs : 100000
                                });

                                radioProxy = settings.radioProxy;
                                done();
                            });
                        });

                        function expectMultiplePublications(spy, expectedPublications, timeout, expectationFct){
                            return waitsFor(function() {
                                        return (spy.onReceive.calls.count() + spy.onError.calls.count() >= expectedPublications);
                                    },
                                    expectedPublications + " publications to occur",
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
                                    return null;
                                });
                            }).catch(function(error) {
                                throw new Error("only " + spy.onReceive.calls.count() + " successful publications arrived from expected " + expectedPublications + ":" + error);
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
                            }).catch(function(error) {
                                return IntegrationUtils.outputPromiseError(new Error("End2EndSubscriptionTest.checkUnsubscribe. Error while subscribing: " + error.message));
                            });

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
                                }).then(spy.onFulfilled).catch(function(error) {
                                    return IntegrationUtils.outputPromiseError(new Error("End2EndSubscriptionTest.checkUnsubscribe. Error while unsubscribing: " + error.message));
                                });

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
                                return null;
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
                        }, 20000);

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
                            setupSubscriptionAndReturnSpy("broadcastWithEnum", subscriptionQosMulticast).then(function(spy) {
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

                        describe("multicasts with partitions", function() {
                            function unsubscribeMulticastSubscription(spy) {
                                unsubscribeSubscription("emptyBroadcast", spy.onFulfilled.calls.mostRecent().args[0]);
                            }
                            function setupMulticastSubscriptionWithPartitionAndReturnSpy(partitions) {
                                return setupSubscriptionAndReturnSpy("emptyBroadcast", subscriptionQosOnChange, partitions);
                            }

                            function triggerBroadcastWithPartitions(partitions, hierarchicBroadcast) {
                                return callOperation("triggerBroadcastsWithPartitions", {
                                    broadcastName: "emptyBroadcast",
                                    times: 1,
                                    partitions: partitions,
                                    hierarchicBroadcast: hierarchicBroadcast
                                });
                            }

                            function expectNoMorePublication (spy, timeout) {
                                return waitsFor(
                                        function() {
                                            return (spy.onReceive.calls.count() > 0 || spy.onError.calls.count() > 0);
                                        },
                                        "wait for interaction with spy",
                                        timeout || provisioning.ttl)
                                        .then(function() { throw new Error("unexpected publication"); })
                                        .catch(function() { return spy; });
                            }

                            function testMulticastWithPartitionsExtended(subscribePartitions, publicationPartitions, times, timeout) {
                                return setupMulticastSubscriptionWithPartitionAndReturnSpy(subscribePartitions).then(function(spy) {
                                    return Promise.all(publicationPartitions.map(function(partitions) {
                                        return triggerBroadcastWithPartitions(partitions, true);
                                    })).then(function() { return spy; });
                                }).then(function(spy) {
                                    var noop = function() {};
                                    return expectMultiplePublications(spy, times, timeout, noop).then(function() {
                                        spy.onReceive.calls.reset();
                                        return spy;
                                    });
                                }).then(function(spy) {
                                    /* the provider sends broadcasts for the complete partition hierarchy.
                                     * However, expect only one publication here
                                     */
                                    return expectNoMorePublication(spy, 500).then(unsubscribeMulticastSubscription);
                                });
                            }
                            function testMulticastWithPartitions(partitions, done) {
                                return testMulticastWithPartitionsExtended(partitions, [partitions], 1, 5000).then(function() {
                                    done();
                                    return null;
                                }).catch(fail);
                            }

                            it("with empty partitions", function(done) {
                                testMulticastWithPartitions([], done);
                            });

                            it("with first-level partition", function(done) {
                                testMulticastWithPartitions([ "a" ], done);
                            });

                            it("with multi-level partition", function(done) {
                                testMulticastWithPartitions([ "a" , "b", "c", "d", "e", "f", "g"], done);
                            });

                            it("with multi-level partition including asterisk", function(done) {
                                var timeout = 5000;
                                testMulticastWithPartitionsExtended([ "a" , "b", "c", "d", "e", "f", "*"], [[ "a" , "b", "c", "d", "e", "f"]], 1, timeout).then(function() {
                                    return testMulticastWithPartitionsExtended([ "a" , "b", "c", "d", "e", "f", "*"], [[ "a" , "b", "c", "d", "e", "f", "g", "h"]], 3, timeout);
                                }).then(function() {
                                    done();
                                    return null;
                                }).catch(fail);
                            });

                            it("with multi-level partition including plus sign", function(done) {
                                var timeout = 5000;
                                testMulticastWithPartitionsExtended([ "a" , "+", "c"], [[ "a" , "b", "c"]], 1, timeout).then(function() {
                                    return testMulticastWithPartitionsExtended([ "a" , "+", "c"], [[ "a" , "b", "c"], [ "a" , "b", "d"], [ "a" , "xyz", "c" , "d", "e", "f"]], 2, timeout);
                                }).then(function() {
                                    done();
                                    return null;
                                }).catch(fail);
                            });

                            it("subscribe to the same non-selective broadcast with different partitions", function(done) {
                                var partitions0 = ["a", "b"];
                                var partitions1 = ["a"];
                                return setupMulticastSubscriptionWithPartitionAndReturnSpy(partitions0).then(function(spy0) {
                                    return setupMulticastSubscriptionWithPartitionAndReturnSpy(partitions1).then(function(spy1) {
                                        return [ spy0, spy1 ];
                                    });
                                }).then(function(spies) {
                                    return triggerBroadcastWithPartitions(partitions0, false).then(function() { return spies; });
                                }).then(function(spies) {
                                    return expectPublication(spies[0]).then(function() {
                                        spies[0].onReceive.calls.reset();
                                        return spies;
                                    });
                                }).then(function(spies) {
                                    /* the provider sends broadcasts for partition0 only.
                                     * So, expect no publication for subscription with partition1
                                     */
                                    return expectNoMorePublication(spies[1], 500).then(function() { return spies; } );
                                }).then(function(spies) {
                                    return triggerBroadcastWithPartitions(partitions1, false).then(function() { return spies; });
                                }).then(function(spies) {
                                    return expectPublication(spies[1]).then(function() {
                                        return spies;
                                    });
                                }).then(function(spies) {
                                    /* the provider sends broadcasts for partition1 only.
                                     * So, expect no publication for subscription with partition0
                                     */
                                    return expectNoMorePublication(spies[0], 500).then(function() { return spies; } );
                                }).then(function() {
                                    done();
                                    return null;
                                }).catch(fail);
                            });

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
                        }, 60000);

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
                                            spy.onFulfilled).catch(function(error) {
                                                return IntegrationUtils.outputPromiseError(new Error("End2EndSubscriptionTest.initially publishes a value on subscription. Subscribe to number of stations: " + error.message));
                                            });

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
                                            spy.onFulfilled).catch(function(error) {
                                                return IntegrationUtils.outputPromiseError(new Error("publishes correct values with onChange. Subscribe to numberOfStations: " + error.message));
                                            });

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

                                        return waitsFor(function() {
                                                return (spy.onReceive.calls.count() + spy.onError.calls.count() > 0);
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

                                        return waitsFor(function() {
                                                return (spy.onReceive.calls.count() + spy.onError.calls.count()) > 1;
                                            },
                                            "the interval subscription update due to maxIntervalMs to occur",
                                            subscriptionQosMixed.maxIntervalMs + provisioning.ttl);
                                    }).then(function() {
                                        expect(spy.onReceive).toHaveBeenCalled();
                                        expect(spy.onReceive.calls.argsFor(1)[0]).toEqual("interval");

                                        return waitsFor(function() {
                                            return spy.onReceive.calls.count() > 2;
                                        }, "the second (onChange) publication to occur", subscriptionQosMixed.maxIntervalMs + safetyTimeout);
                                    }).then(function() {
                                        expect(spy.onReceive.calls.argsFor(2)[0]).toEqual("valueChanged1");

                                        return waitsFor(function() {
                                            return spy.onReceive.calls.count() > 3;
                                        },
                                        "the second onChange publication occur after minIntervalMs",
                                        subscriptionQosMixed.maxIntervalMs + safetyTimeout);
                                    }).then(function() {
                                        expect(spy.onReceive.calls.argsFor(3)[0]).toEqual("valueChanged2");
                                        expect(spy.onReceive.calls.count()).toBeLessThan(5);
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
                                            2 * subscriptionQosMixed.maxIntervalMs);
                                    }).then(function() {
                                        expect(spy.onReceive).toHaveBeenCalled();
                                        expect(spy.onReceive).toHaveBeenCalledWith(true);
                                        spy.onReceive.calls.reset();

                                        joynr.util.LongTimer.setTimeout(function() {
                                            timeout = true;
                                        }, subscriptionQosMixed.expiryDateMs
                                               - Date.now()
                                               + safetyTimeout);
                                        return waitsFor(
                                            function() {
                                                return timeout || spy.onReceive.calls.count() > 0;
                                            },
                                            "the interval onReceive not to occur again",
                                            subscriptionQosMixed.expiryDateMs
                                                - Date.now()
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

                        afterEach(abstractTest.afterEach);

                    });
        });
