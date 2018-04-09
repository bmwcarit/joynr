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

const Promise = require("../../../main/js/global/Promise");
let joynr = require("joynr"),
    RadioStation = require("../../generated/joynr/vehicle/radiotypes/RadioStation"),
    Country = require("../../generated/joynr/datatypes/exampleTypes/Country"),
    ComplexTestType = require("../../generated/joynr/tests/testTypes/ComplexTestType"),
    SubscriptionException = require("../../../main/js/joynr/exceptions/SubscriptionException"),
    IntegrationUtils = require("./IntegrationUtils"),
    End2EndAbstractTest = require("./End2EndAbstractTest"),
    provisioning = require("../../resources/joynr/provisioning/provisioning_cc"),
    waitsFor = require("../global/WaitsFor");

describe("libjoynr-js.integration.end2end.subscription", () => {
    const subscriptionLength = 2000;
    const safetyTimeout = 200;
    let subscriptionQosOnChange;
    let subscriptionQosInterval;
    let subscriptionQosMulticast;
    let subscriptionQosMixed;
    let radioProxy;
    const abstractTest = new End2EndAbstractTest("End2EndSubscriptionTest");
    const setAttribute = abstractTest.setAttribute;
    const setupSubscriptionAndReturnSpy = abstractTest.setupSubscriptionAndReturnSpy;
    const unsubscribeSubscription = abstractTest.unsubscribeSubscription;
    const callOperation = abstractTest.callOperation;
    const expectPublication = abstractTest.expectPublication;

    beforeEach(done => {
        abstractTest.beforeEach().then(settings => {
            subscriptionQosOnChange = new joynr.proxy.OnChangeSubscriptionQos({
                minIntervalMs: 50
            });

            subscriptionQosInterval = new joynr.proxy.PeriodicSubscriptionQos({
                periodMs: 1000
            });

            subscriptionQosMixed = new joynr.proxy.OnChangeWithKeepAliveSubscriptionQos({
                minIntervalMs: 100,
                maxIntervalMs: 1000
            });

            subscriptionQosMulticast = new joynr.proxy.MulticastSubscriptionQos({
                validityMs: 100000
            });

            radioProxy = settings.radioProxy;
            done();
        });
    });

    function expectMultiplePublications(spy, expectedPublications, timeout, expectationFct) {
        return waitsFor(
            () => {
                return spy.onReceive.calls.count() + spy.onError.calls.count() >= expectedPublications;
            },
            expectedPublications + " publications to occur",
            timeout
        )
            .then(() => {
                const promise = new Promise((resolve, reject) => {
                    setTimeout(() => {
                        resolve();
                    }, 100);
                });
                return promise.then(() => {
                    expect(spy.onReceive.calls.count()).toBe(expectedPublications);
                    expectationFct(spy.onReceive.calls);
                    spy.onReceive.calls.reset();
                    return null;
                });
            })
            .catch(error => {
                throw new Error(
                    "only " +
                        spy.onReceive.calls.count() +
                        " successful publications arrived from expected " +
                        expectedPublications +
                        ":" +
                        error
                );
            });
    }

    function expectPublicationError(spy) {
        return waitsFor(
            () => {
                return spy.onReceive.calls.count() > 0 || spy.onError.calls.count() > 0;
            },
            "first error to occur",
            500 + provisioning.ttl
        ).then(() => {
            expect(spy.onReceive).not.toHaveBeenCalled();
            expect(spy.onError).toHaveBeenCalled();
            expect(spy.onError.calls.argsFor(0)[0]._typeName).toEqual("joynr.exceptions.ProviderRuntimeException");
            spy.onError.calls.reset();
        });
    }
    function publishesValue(subscriptionQos) {
        return setupSubscriptionAndReturnSpy("numberOfStations", subscriptionQos).then(spy => {
            return expectPublication(spy, publicationCallback => {
                expect(typeof publicationCallback.args[0] === "number").toBeTruthy();
            });
        });
    }

    function checkUnsubscribe(timeout, subscriptionQos) {
        let spy, subscriptionId;

        spy = jasmine.createSpyObj("spy", ["onFulfilled", "onReceive", "onError"]);
        radioProxy.numberOfStations
            .subscribe({
                subscriptionQos,
                onReceive: spy.onReceive,
                onError: spy.onError
            })
            .then(id => {
                subscriptionId = id;
                spy.onFulfilled(id);
            })
            .catch(error => {
                return IntegrationUtils.outputPromiseError(
                    new Error("End2EndSubscriptionTest.checkUnsubscribe. Error while subscribing: " + error.message)
                );
            });

        let nrOfPublications;
        return waitsFor(
            () => {
                return (
                    spy.onFulfilled.calls.count() > 0 &&
                    (spy.onReceive.calls.count() > 0 || spy.onError.calls.count() > 0)
                );
            },
            "subscription to be registered and first publication to occur",
            provisioning.ttl
        )
            .then(() => {
                expect(spy.onFulfilled).toHaveBeenCalled();
                spy.onFulfilled.calls.reset();
                nrOfPublications = spy.onReceive.calls.count();
                radioProxy.numberOfStations
                    .unsubscribe({
                        subscriptionId
                    })
                    .then(spy.onFulfilled)
                    .catch(error => {
                        return IntegrationUtils.outputPromiseError(
                            new Error(
                                "End2EndSubscriptionTest.checkUnsubscribe. Error while unsubscribing: " + error.message
                            )
                        );
                    });

                return waitsFor(
                    () => {
                        return spy.onFulfilled.calls.count() > 0;
                    },
                    "unsubscribe to complete successfully",
                    provisioning.ttl
                );
            })
            .then(() => {
                expect(spy.onFulfilled).toHaveBeenCalled();

                // wait 2 times the publication interval
                let waitForPublication = true;
                joynr.util.LongTimer.setTimeout(() => {
                    waitForPublication = false;
                }, 2 * timeout);

                return waitsFor(() => {
                    return !waitForPublication;
                }, 4 * timeout);
            })
            .then(() => {
                // check that no publications occured since the unsubscribe
                expect(spy.onReceive.calls.count()).toEqual(nrOfPublications);
            });
    }

    it("subscribe to failingSyncAttribute", done => {
        setupSubscriptionAndReturnSpy("failingSyncAttribute", subscriptionQosInterval)
            .then(spy => {
                return expectPublicationError(spy);
            })
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("subscribe to failingAsyncAttribute", done => {
        setupSubscriptionAndReturnSpy("failingAsyncAttribute", subscriptionQosInterval)
            .then(spy => {
                return expectPublicationError(spy);
            })
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("subscribe to enumAttribute", done => {
        let mySpy;
        setAttribute("enumAttribute", Country.AUSTRIA)
            .then(() => {
                return setupSubscriptionAndReturnSpy("enumAttribute", subscriptionQosOnChange);
            })
            .then(spy => {
                mySpy = spy;
                return expectPublication(mySpy, call => {
                    expect(call.args[0]).toEqual(Country.AUSTRIA);
                });
            })
            .then(() => {
                return setAttribute("enumAttribute", Country.AUSTRALIA);
            })
            .then(() => {
                return expectPublication(mySpy, call => {
                    expect(call.args[0]).toEqual(Country.AUSTRALIA);
                });
            })
            .then(() => {
                return setAttribute("enumAttribute", Country.ITALY);
            })
            .then(() => {
                return expectPublication(mySpy, call => {
                    expect(call.args[0]).toEqual(Country.ITALY);
                });
            })
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("subscribe to complex typedef attribute", done => {
        const value = new RadioStation({
            name: "TestEnd2EndComm.typeDefForStructAttribute.RadioStation",
            byteBuffer: []
        });
        setAttribute("typeDefForStruct", value)
            .then(done => {
                return setupSubscriptionAndReturnSpy("typeDefForStruct", subscriptionQosOnChange);
            })
            .then(spy => {
                return expectPublication(spy, call => {
                    expect(call.args[0]).toEqual(value);
                });
            })
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("subscribe to primitive typedef attribute", done => {
        const value = 1234543;
        let mySpy;
        setAttribute("typeDefForPrimitive", value)
            .then(() => {
                return setupSubscriptionAndReturnSpy("typeDefForPrimitive", subscriptionQosOnChange);
            })
            .then(spy => {
                mySpy = spy;
                return null;
            })
            .then(() => {
                return expectPublication(mySpy, call => {
                    expect(call.args[0]).toEqual(value);
                });
            })
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it(
        "subscribe to byteBufferAttribute",
        done => {
            //initialize attribute
            let mySpy;
            let testByteBufferAttribute;

            setAttribute("byteBufferAttribute", [])
                .then(() => {
                    return setupSubscriptionAndReturnSpy("byteBufferAttribute", subscriptionQosOnChange);
                })
                .then(spy => {
                    mySpy = spy;
                    return expectPublication(mySpy, call => {
                        expect(call.args[0]).toEqual([]);
                    });
                })
                .then(() => {
                    testByteBufferAttribute = function(expectedByteBuffer) {
                        return setAttribute("byteBufferAttribute", expectedByteBuffer).then(() => {
                            return expectPublication(mySpy, call => {
                                expect(call.args[0]).toEqual(expectedByteBuffer);
                            });
                        });
                    };
                    return testByteBufferAttribute([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0]);
                })
                .then(() => {
                    return testByteBufferAttribute([255]);
                })
                .then(() => {
                    return testByteBufferAttribute([2, 2, 2, 2]);
                })
                .then(() => {
                    let i,
                        byteBuffer10k = [];
                    for (i = 0; i < 10000; i++) {
                        byteBuffer10k.push(i % 256);
                    }
                    return testByteBufferAttribute(byteBuffer10k);
                })
                .then(() => {
                    done();
                    return null;
                })
                .catch(fail);
        },
        20000
    );

    it("subscribe to weakSignal broadcast having ByteBuffer as output parameter", done => {
        let mySpy;
        setupSubscriptionAndReturnSpy("weakSignal", subscriptionQosOnChange)
            .then(spy => {
                mySpy = spy;
                return callOperation("triggerBroadcasts", {
                    broadcastName: "weakSignal",
                    times: 1
                });
            })
            .then(() => {
                return expectPublication(mySpy, call => {
                    expect(call.args[0].radioStation).toEqual("radioStation");
                    expect(call.args[0].byteBuffer).toEqual([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0]);
                });
            })
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    //enable this test once the proxy side is ready for fire n' forget
    it("call methodFireAndForgetWithoutParams and expect to call the provider", done => {
        let mySpy;
        setupSubscriptionAndReturnSpy("fireAndForgetCallArrived", subscriptionQosOnChange)
            .then(spy => {
                mySpy = spy;
                return callOperation("methodFireAndForgetWithoutParams", {});
            })
            .then(() => {
                return expectPublication(mySpy, call => {
                    expect(call.args[0].methodName).toEqual("methodFireAndForgetWithoutParams");
                });
            })
            .then(() => {
                done();
            })
            .catch(fail);
    });

    //enable this test once the proxy side is ready for fire n' forget
    it("call methodFireAndForget and expect to call the provider", done => {
        let mySpy;
        setupSubscriptionAndReturnSpy("fireAndForgetCallArrived", subscriptionQosOnChange)
            .then(spy => {
                mySpy = spy;
                return callOperation("methodFireAndForget", {
                    intIn: 0,
                    stringIn: "methodFireAndForget",
                    complexTestTypeIn: new ComplexTestType({
                        a: 0,
                        b: 1
                    })
                });
            })
            .then(() => {
                return expectPublication(mySpy, call => {
                    expect(call.args[0].methodName).toEqual("methodFireAndForget");
                });
            })
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("subscribe to broadcastWithEnum", done => {
        let mySpy;
        setupSubscriptionAndReturnSpy("broadcastWithEnum", subscriptionQosMulticast)
            .then(spy => {
                mySpy = spy;
                return callOperation("triggerBroadcasts", {
                    broadcastName: "broadcastWithEnum",
                    times: 1
                });
            })
            .then(() => {
                return expectPublication(mySpy, call => {
                    expect(call.args[0].enumOutput).toEqual(Country.CANADA);
                    expect(call.args[0].enumArrayOutput).toEqual([Country.GERMANY, Country.ITALY]);
                });
            })
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("subscribe to emptyBroadcast", done => {
        let mySpy;
        setupSubscriptionAndReturnSpy("emptyBroadcast", subscriptionQosOnChange)
            .then(spy => {
                mySpy = spy;
                return callOperation("triggerBroadcasts", {
                    broadcastName: "emptyBroadcast",
                    times: 1
                });
            })
            .then(() => {
                return expectPublication(mySpy, call => {
                    //no expectation for call, as empty broadcast
                });
            })
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    describe("multicasts with partitions", () => {
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
                partitions,
                hierarchicBroadcast
            });
        }

        function expectNoMorePublication(spy, timeout) {
            return waitsFor(
                () => {
                    return spy.onReceive.calls.count() > 0 || spy.onError.calls.count() > 0;
                },
                "wait for interaction with spy",
                timeout || provisioning.ttl
            )
                .then(() => {
                    throw new Error("unexpected publication");
                })
                .catch(() => {
                    return spy;
                });
        }

        function testMulticastWithPartitionsExtended(subscribePartitions, publicationPartitions, times, timeout) {
            return setupMulticastSubscriptionWithPartitionAndReturnSpy(subscribePartitions)
                .then(spy => {
                    return Promise.all(
                        publicationPartitions.map(partitions => {
                            return triggerBroadcastWithPartitions(partitions, true);
                        })
                    ).then(() => {
                        return spy;
                    });
                })
                .then(spy => {
                    const noop = function() {};
                    return expectMultiplePublications(spy, times, timeout, noop).then(() => {
                        spy.onReceive.calls.reset();
                        return spy;
                    });
                })
                .then(spy => {
                    /* the provider sends broadcasts for the complete partition hierarchy.
                                 * However, expect only one publication here
                                 */
                    return expectNoMorePublication(spy, 500).then(unsubscribeMulticastSubscription);
                });
        }
        function testMulticastWithPartitions(partitions, done) {
            return testMulticastWithPartitionsExtended(partitions, [partitions], 1, 5000)
                .then(() => {
                    done();
                    return null;
                })
                .catch(fail);
        }

        it("with empty partitions", done => {
            testMulticastWithPartitions([], done);
        });

        it("with first-level partition", done => {
            testMulticastWithPartitions(["a"], done);
        });

        it("with multi-level partition", done => {
            testMulticastWithPartitions(["a", "b", "c", "d", "e", "f", "g"], done);
        });

        it("with multi-level partition including asterisk", done => {
            const timeout = 5000;
            testMulticastWithPartitionsExtended(
                ["a", "b", "c", "d", "e", "f", "*"],
                [["a", "b", "c", "d", "e", "f"]],
                1,
                timeout
            )
                .then(() => {
                    return testMulticastWithPartitionsExtended(
                        ["a", "b", "c", "d", "e", "f", "*"],
                        [["a", "b", "c", "d", "e", "f", "g", "h"]],
                        3,
                        timeout
                    );
                })
                .then(() => {
                    done();
                    return null;
                })
                .catch(fail);
        });

        it("with multi-level partition including plus sign", done => {
            const timeout = 5000;
            testMulticastWithPartitionsExtended(["a", "+", "c"], [["a", "b", "c"]], 1, timeout)
                .then(() => {
                    return testMulticastWithPartitionsExtended(
                        ["a", "+", "c"],
                        [["a", "b", "c"], ["a", "b", "d"], ["a", "xyz", "c", "d", "e", "f"]],
                        2,
                        timeout
                    );
                })
                .then(() => {
                    done();
                    return null;
                })
                .catch(fail);
        });

        it("subscribe to the same non-selective broadcast with different partitions", done => {
            const partitions0 = ["a", "b"];
            const partitions1 = ["a"];
            return setupMulticastSubscriptionWithPartitionAndReturnSpy(partitions0)
                .then(spy0 => {
                    return setupMulticastSubscriptionWithPartitionAndReturnSpy(partitions1).then(spy1 => {
                        return [spy0, spy1];
                    });
                })
                .then(spies => {
                    return triggerBroadcastWithPartitions(partitions0, false).then(() => {
                        return spies;
                    });
                })
                .then(spies => {
                    return expectPublication(spies[0]).then(() => {
                        spies[0].onReceive.calls.reset();
                        return spies;
                    });
                })
                .then(spies => {
                    /* the provider sends broadcasts for partition0 only.
                                 * So, expect no publication for subscription with partition1
                                 */
                    return expectNoMorePublication(spies[1], 500).then(() => {
                        return spies;
                    });
                })
                .then(spies => {
                    return triggerBroadcastWithPartitions(partitions1, false).then(() => {
                        return spies;
                    });
                })
                .then(spies => {
                    return expectPublication(spies[1]).then(() => {
                        return spies;
                    });
                })
                .then(spies => {
                    /* the provider sends broadcasts for partition1 only.
                                 * So, expect no publication for subscription with partition0
                                 */
                    return expectNoMorePublication(spies[0], 500).then(() => {
                        return spies;
                    });
                })
                .then(() => {
                    done();
                    return null;
                })
                .catch(fail);
        });
    });

    it("subscribe to type def broadcast", done => {
        const typeDefStructOutput = new RadioStation({
            name: "TestEnd2EndCommProviderProcess.broadcastWithTypeDefs.RadioStation",
            byteBuffer: []
        });
        const typeDefPrimitiveOutput = 123456;
        let mySpy;

        setupSubscriptionAndReturnSpy("broadcastWithTypeDefs", subscriptionQosOnChange)
            .then(spy => {
                mySpy = spy;
                return callOperation("triggerBroadcasts", {
                    broadcastName: "broadcastWithTypeDefs",
                    times: 1
                });
            })
            .then(() => {
                return expectPublication(mySpy, call => {
                    expect(call.args[0].typeDefStructOutput).toEqual(typeDefStructOutput);
                    expect(call.args[0].typeDefPrimitiveOutput).toEqual(typeDefPrimitiveOutput);
                });
            })
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it(
        "subscribe to broadcastWithEnum and get burst",
        done => {
            subscriptionQosOnChange.minIntervalMs = 0;
            const times = 100;
            let mySpy;
            setupSubscriptionAndReturnSpy("broadcastWithEnum", subscriptionQosOnChange)
                .then(spy => {
                    mySpy = spy;
                    return callOperation("triggerBroadcasts", {
                        broadcastName: "broadcastWithEnum",
                        times
                    });
                })
                .then(() => {
                    return expectMultiplePublications(mySpy, times, 5000, calls => {
                        let i;
                        for (i = 0; i < times; i++) {
                            expect(calls.argsFor(i)[0].enumOutput).toEqual(Country.CANADA);
                            expect(calls.argsFor(i)[0].enumArrayOutput).toEqual([Country.GERMANY, Country.ITALY]);
                        }
                    });
                })
                .then(() => {
                    done();
                    return null;
                })
                .catch(fail);
        },
        60000
    );

    it("subscribe to enumArrayAttribute", done => {
        const attributeName = "enumArrayAttribute";
        const value1 = [Country.AUSTRIA];
        const value2 = [Country.AUSTRIA, Country.GERMANY];
        const value3 = [Country.AUSTRIA, Country.GERMANY, Country.ITALY];
        let mySpy;
        setAttribute(attributeName, value1)
            .then(() => {
                return setupSubscriptionAndReturnSpy(attributeName, subscriptionQosOnChange);
            })
            .then(spy => {
                mySpy = spy;
                return expectPublication(mySpy, call => {
                    expect(call.args[0]).toEqual(value1);
                });
            })
            .then(() => {
                return setAttribute(attributeName, value2);
            })
            .then(() => {
                return expectPublication(mySpy, call => {
                    expect(call.args[0]).toEqual(value2);
                });
            })
            .then(() => {
                return setAttribute(attributeName, value3);
            })
            .then(() => {
                return expectPublication(mySpy, call => {
                    expect(call.args[0]).toEqual(value3);
                });
            })
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("resolves attribute subscription and calls onSubscribed", done => {
        const spy = jasmine.createSpyObj("spy", ["onReceive", "onError", "onSubscribed"]);
        let subscriptionId, detailMessage;
        const qosSettings = new joynr.proxy.PeriodicSubscriptionQos();
        let storedSubscriptionId;

        radioProxy.numberOfStations
            .subscribe({
                subscriptionQos: qosSettings,
                onReceive: spy.onReceive,
                onError: spy.onError,
                onSubscribed: spy.onSubscribed
            })
            .then(subscriptionId => {
                storedSubscriptionId = subscriptionId;
                return waitsFor(
                    () => {
                        return spy.onSubscribed.calls.count() === 1;
                    },
                    "onSubscribed to get called",
                    1000
                );
            })
            .then(() => {
                expect(spy.onError).not.toHaveBeenCalled();
                expect(spy.onSubscribed).toHaveBeenCalled();
                expect(spy.onSubscribed).toHaveBeenCalledWith(storedSubscriptionId);
                done();
                return null;
            })
            .catch(fail);
    });

    it("rejects attribute subscription if periodMs is too small", done => {
        const spy = jasmine.createSpyObj("spy", ["onReceive", "onError", "onSubscribed"]);
        let subscriptionId, detailMessage;
        const qosSettings = new joynr.proxy.PeriodicSubscriptionQos({
            expiryDateMs: 0,
            alertAfterIntervalMs: 0,
            publicationTtlMs: 1000
        });
        // forcibly fake it! The constructor throws, if using this directly
        qosSettings.periodMs = joynr.proxy.PeriodicSubscriptionQos.MIN_PERIOD_MS - 1;

        radioProxy.numberOfStations
            .subscribe({
                subscriptionQos: qosSettings,
                onReceive: spy.onReceive,
                onError: spy.onError,
                onSubscribed: spy.onSubscribed
            })
            .then(subscriptionId => {
                fail("unexpected success");
            })
            .catch(error => {
                expect(error).toBeDefined();
                expect(error instanceof SubscriptionException);
                expect(error.subscriptionId).toBeDefined();
                expect(error.detailMessage).toMatch(/is smaller than PeriodicSubscriptionQos/);
                subscriptionId = error.subscriptionId;
                detailMessage = error.detailMessage;
                return waitsFor(
                    () => {
                        return spy.onError.calls.count() === 1;
                    },
                    "onError to get called",
                    1000
                );
            })
            .then(() => {
                expect(spy.onReceive).not.toHaveBeenCalled();
                expect(spy.onSubscribed).not.toHaveBeenCalled();
                expect(spy.onError).toHaveBeenCalled();
                expect(spy.onError.calls.mostRecent().args[0]).toBeDefined();
                const error = spy.onError.calls.mostRecent().args[0];
                expect(error instanceof SubscriptionException);
                expect(error.subscriptionId).toEqual(subscriptionId);
                expect(error.detailMessage).toEqual(detailMessage);
                done();
                return null;
            })
            .catch(fail);
    });

    it("rejects subscription to non-existing attribute ", done => {
        const spy = jasmine.createSpyObj("spy", ["onReceive", "onError", "onSubscribed"]);
        let subscriptionId, detailMessage;

        radioProxy.nonExistingAttributeOnProviderSide
            .subscribe({
                subscriptionQos: new joynr.proxy.OnChangeSubscriptionQos(),
                onReceive: spy.onReceive,
                onError: spy.onError,
                onSubscribed: spy.onSubscribed
            })
            .then(subscriptionId => {
                fail("unexpected success");
            })
            .catch(error => {
                expect(error).toBeDefined();
                expect(error instanceof SubscriptionException);
                expect(error.subscriptionId).toBeDefined();
                expect(error.detailMessage).toMatch(/misses attribute/);
                subscriptionId = error.subscriptionId;
                detailMessage = error.detailMessage;
                return waitsFor(
                    () => {
                        return spy.onError.calls.count() === 1;
                    },
                    "onError to get called",
                    1000
                );
            })
            .then(() => {
                expect(spy.onReceive).not.toHaveBeenCalled();
                expect(spy.onSubscribed).not.toHaveBeenCalled();
                expect(spy.onError).toHaveBeenCalled();
                expect(spy.onError.calls.mostRecent().args[0]).toBeDefined();
                const error = spy.onError.calls.mostRecent().args[0];
                expect(error instanceof SubscriptionException);
                expect(error.subscriptionId).toEqual(subscriptionId);
                expect(error.detailMessage).toEqual(detailMessage);
                done();
                return null;
            })
            .catch(fail);
    });

    // The following test does unfortunately not work, because the expiryDateMs
    // is used to shorten the ttl of the message in SubscriptionManager.
    // This leads to a negative ttl, which has ChannelMessagingSender
    // throwing an Error at us (where it should throw an exception).
    // The message is thus not getting send to provider.
    // Leaving the test in at the moment so it can be used later
    // once exception handling gets fixed.
    xit("rejects on subscribe to attribute with bad expiryDateMs", done => {
        const spy = jasmine.createSpyObj("spy", ["onReceive", "onError"]);
        let subscriptionId, detailMessage;

        radioProxy.numberOfStations
            .subscribe({
                subscriptionQos: new joynr.proxy.OnChangeSubscriptionQos({
                    expiryDateMs: Date.now() - 10000
                }),
                onReceive: spy.onReceive,
                onError: spy.onError
            })
            .then(subscriptionId => {
                fail("unexpected success");
            })
            .catch(error => {
                expect(error).toBeDefined();
                expect(error instanceof SubscriptionException);
                expect(error.subscriptionId).toBeDefined();
                expect(error.detailMessage).toMatch(/lies in the past/);
                subscriptionId = error.subscriptionId;
                detailMessage = error.detailMessage;
                return waitsFor(
                    () => {
                        return spy.onError.calls.count() === 1;
                    },
                    "onError to get called",
                    1000
                );
            })
            .then(() => {
                expect(spy.onReceive).not.toHaveBeenCalled();
                expect(spy.onError).toHaveBeenCalled();
                expect(spy.onError.calls.mostRecent().args[0]).toBeDefined();
                const error = spy.onError.calls.mostRecent().args[0];
                expect(error instanceof SubscriptionException);
                expect(error.subscriptionId).toEqual(subscriptionId);
                expect(error.detailMessage).toEqual(detailMessage);
                done();
                return null;
            })
            .catch(fail);
    });

    it("can start a subscription and provides a subscription id", done => {
        let spy;

        spy = jasmine.createSpyObj("spy", ["onFulfilled", "onReceive", "onError"]);
        radioProxy.numberOfStations
            .subscribe({
                subscriptionQos: subscriptionQosOnChange,
                onReceive: spy.onReceive,
                onError: spy.onError
            })
            .then(spy.onFulfilled)
            .catch(error => {
                spy.onError(error);
                IntegrationUtils.outputPromiseError(error);
            });

        waitsFor(
            () => {
                return spy.onFulfilled.calls.count() > 0;
            },
            "subscription to be registered",
            provisioning.ttl
        )
            .then(() => {
                expect(spy.onFulfilled).toHaveBeenCalled();
                expect(typeof spy.onFulfilled.calls.mostRecent().args[0] === "string").toBeTruthy();
                expect(spy.onError).not.toHaveBeenCalled();
                done();
                return null;
            })
            .catch(fail);
    });

    it("publishes a value with an onChange subscription", done => {
        publishesValue(subscriptionQosOnChange)
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("publishes a value with an interval subscription", done => {
        publishesValue(subscriptionQosInterval)
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("publishes a value with a mixed subscription", done => {
        publishesValue(subscriptionQosMixed)
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("publishes a value with an ending subscription", done => {
        subscriptionQosMixed.expiryDateMs = subscriptionLength + Date.now();
        publishesValue(subscriptionQosMixed)
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("initially publishes a value on subscription", done => {
        let spy;

        spy = jasmine.createSpyObj("spy", ["onFulfilled", "onReceive", "onError"]);
        radioProxy.numberOfStations
            .subscribe({
                subscriptionQos: subscriptionQosOnChange,
                onReceive: spy.onReceive,
                onError: spy.onError
            })
            .then(spy.onFulfilled)
            .catch(error => {
                return IntegrationUtils.outputPromiseError(
                    new Error(
                        "End2EndSubscriptionTest.initially publishes a value on subscription. Subscribe to number of stations: " +
                            error.message
                    )
                );
            });

        // timeout for
        // subscription request
        // round trip
        waitsFor(
            () => {
                return (
                    spy.onFulfilled.calls.count() > 0 &&
                    (spy.onReceive.calls.count() > 0 || spy.onError.calls.count() > 0)
                );
            },
            "initial onReceive to occur",
            provisioning.ttl
        )
            .then(() => {
                expect(spy.onFulfilled).toHaveBeenCalled();
                expect(spy.onReceive).toHaveBeenCalled();
                expect(spy.onReceive.calls.argsFor(0)[0]).toEqual(-1);
                expect(spy.onError).not.toHaveBeenCalled();
                done();
                return null;
            })
            .catch(fail);
    });

    const nrPubs = 3;
    it("publishes correct values with onChange " + nrPubs + " times", done => {
        let spy;

        spy = jasmine.createSpyObj("spy", ["onFulfilled", "onReceive", "onError"]);
        radioProxy.numberOfStations
            .subscribe({
                subscriptionQos: subscriptionQosOnChange,
                onReceive: spy.onReceive,
                onError: spy.onError
            })
            .then(spy.onFulfilled)
            .catch(error => {
                return IntegrationUtils.outputPromiseError(
                    new Error("publishes correct values with onChange. Subscribe to numberOfStations: " + error.message)
                );
            });

        // timeout
        // for
        // subscription
        // request
        // round
        // trip
        waitsFor(
            () => {
                return spy.onFulfilled.calls.count() > 0;
            },
            "subscription promise to resolve",
            provisioning.ttl
        )
            .then(() => {
                expect(spy.onFulfilled).toHaveBeenCalled();

                // timeout for
                // subscription request
                // round trip
                return waitsFor(
                    () => {
                        return spy.onReceive.calls.count() > 0 || spy.onError.calls.count() > 0;
                    },
                    "initial onReceive to occur",
                    provisioning.ttl
                );
            })
            .then(() => {
                expect(spy.onReceive).toHaveBeenCalled();
                expect(spy.onReceive.calls.argsFor(0)[0]).toEqual(-1);
                expect(spy.onError).not.toHaveBeenCalled();

                spy.onReceive.calls.reset();
                spy.onError.calls.reset();

                // for subscription request round trip and nrPubs publications, safety (+1)
                return waitsFor(
                    () => {
                        return spy.onReceive.calls.count() >= nrPubs || spy.onError.calls.count() > 0;
                    },
                    "subscription to be registered and first publication to occur",
                    provisioning.ttl + (nrPubs + 1) * 1000
                ); // timeout
            })
            .then(() => {
                expect(spy.onReceive).toHaveBeenCalled();
                expect(spy.onError).not.toHaveBeenCalled();

                expect(spy.onReceive.calls.count()).toEqual(nrPubs);

                let i;
                for (i = 0; i < nrPubs; ++i) {
                    expect(spy.onReceive.calls.argsFor(i)[0]).toEqual(i);
                }
            })
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("publishes correct values with a mixed subscription", done => {
        let spy;

        // provider will fire an interval publication 1s after
        // initialization with the response "interval"
        // after another 0.5 ms it will fire an onChange publication
        // with the response "valueChanged1"
        // after another 10 ms it will try to fire an onChange
        // publication with the response "valueChanged2", should be
        // blocked by the
        // PublicationManager on the Provider side

        spy = jasmine.createSpyObj("spy", ["onFulfilled", "onReceive", "onError"]);
        subscriptionQosMixed.expiryDateMs = subscriptionLength + Date.now();
        radioProxy.mixedSubscriptions
            .subscribe({
                subscriptionQos: subscriptionQosMixed,
                onReceive: spy.onReceive,
                onError: spy.onError
            })
            .then(spy.onFulfilled)
            .catch(error => {
                spy.onError(error);
                IntegrationUtils.outputPromiseError(error);
            });

        waitsFor(
            () => {
                return spy.onFulfilled.calls.count() > 0;
            },
            "subscription to be registered",
            provisioning.ttl
        )
            .then(() => {
                expect(spy.onFulfilled).toHaveBeenCalled();

                return waitsFor(
                    () => {
                        return spy.onReceive.calls.count() + spy.onError.calls.count() > 0;
                    },
                    "initial publication to occur",
                    provisioning.ttl
                ); // timeout for
                // subscription request
                // round trip
            })
            .then(() => {
                expect(spy.onReceive).toHaveBeenCalled();
                expect(spy.onReceive.calls.argsFor(0)[0]).toEqual("interval");
                expect(spy.onError).not.toHaveBeenCalled();

                return waitsFor(
                    () => {
                        return spy.onReceive.calls.count() + spy.onError.calls.count() > 1;
                    },
                    "the interval subscription update due to maxIntervalMs to occur",
                    subscriptionQosMixed.maxIntervalMs + provisioning.ttl
                );
            })
            .then(() => {
                expect(spy.onReceive).toHaveBeenCalled();
                expect(spy.onReceive.calls.argsFor(1)[0]).toEqual("interval");

                return waitsFor(
                    () => {
                        return spy.onReceive.calls.count() > 2;
                    },
                    "the second (onChange) publication to occur",
                    subscriptionQosMixed.maxIntervalMs + safetyTimeout
                );
            })
            .then(() => {
                expect(spy.onReceive.calls.argsFor(2)[0]).toEqual("valueChanged1");

                return waitsFor(
                    () => {
                        return spy.onReceive.calls.count() > 3;
                    },
                    "the second onChange publication occur after minIntervalMs",
                    subscriptionQosMixed.maxIntervalMs + safetyTimeout
                );
            })
            .then(() => {
                expect(spy.onReceive.calls.argsFor(3)[0]).toEqual("valueChanged2");
                expect(spy.onReceive.calls.count()).toBeLessThan(5);
                done();
                return null;
            })
            .catch(fail);
    });

    it(
        "terminates correctly according to the endDate ",
        done => {
            let spy, timeout;

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

            spy = jasmine.createSpyObj("spy", ["onFulfilled", "onReceive", "onError"]);
            subscriptionQosMixed.expiryDateMs = subscriptionQosMixed.maxIntervalMs * 1.5 + Date.now();
            radioProxy.isOn
                .subscribe({
                    subscriptionQos: subscriptionQosMixed,
                    onReceive: spy.onReceive,
                    onError: spy.onError
                })
                .then(spy.onFulfilled)
                .catch(error => {
                    spy.onError(error);
                    IntegrationUtils.outputPromiseError(error);
                });

            waitsFor(
                () => {
                    return spy.onFulfilled.calls.count() > 0;
                },
                "subscription to be registered",
                provisioning.ttl
            )
                .then(() => {
                    expect(spy.onFulfilled).toHaveBeenCalled();
                    return waitsFor(
                        () => {
                            return spy.onReceive.calls.count() > 0 || spy.onError.calls.count() > 0;
                        },
                        "initial publication to occur",
                        provisioning.ttl
                    ); // timeout for
                    // subscription request
                    // round trip
                })
                .then(() => {
                    expect(spy.onReceive).toHaveBeenCalled();
                    expect(spy.onReceive.calls.argsFor(0)[0]).toEqual(true);
                    expect(spy.onError).not.toHaveBeenCalled();

                    spy.onReceive.calls.reset();
                    spy.onError.calls.reset();

                    return waitsFor(
                        () => {
                            return spy.onReceive.calls.count() > 0 || spy.onError.calls.count() > 0;
                        },
                        "the interval onReceive to occur",
                        2 * subscriptionQosMixed.maxIntervalMs
                    );
                })
                .then(() => {
                    expect(spy.onReceive).toHaveBeenCalled();
                    expect(spy.onReceive).toHaveBeenCalledWith(true);
                    spy.onReceive.calls.reset();

                    joynr.util.LongTimer.setTimeout(() => {
                        timeout = true;
                    }, subscriptionQosMixed.expiryDateMs - Date.now() + safetyTimeout);
                    return waitsFor(
                        () => {
                            return timeout || spy.onReceive.calls.count() > 0;
                        },
                        "the interval onReceive not to occur again",
                        subscriptionQosMixed.expiryDateMs - Date.now() + safetyTimeout
                    );
                })
                .then(() => {
                    expect(spy.onReceive).not.toHaveBeenCalled();
                    done();
                    return null;
                })
                .catch(fail);
        },
        10000
    );

    it("unsubscribes onChange subscription successfully", done => {
        checkUnsubscribe(1000, subscriptionQosOnChange)
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("unsubscribes interval subscription successfully", done => {
        checkUnsubscribe(1000, subscriptionQosInterval)
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    afterEach(abstractTest.afterEach);
});
