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
require("../../node-unit-test-helper");
//TODO: some of this relies on the dummy implementation, change accordingly when implementing
const ProxyEvent = require("../../../../main/js/joynr/proxy/ProxyEvent");
const DiscoveryQos = require("../../../../main/js/joynr/proxy/DiscoveryQos");
const MessagingQos = require("../../../../main/js/joynr/messaging/MessagingQos");
const MulticastSubscriptionQos = require("../../../../main/js/joynr/proxy/MulticastSubscriptionQos");
const DiscoveryEntryWithMetaInfo = require("../../../../main/js/generated/joynr/types/DiscoveryEntryWithMetaInfo");
const Version = require("../../../../main/js/generated/joynr/types/Version");
const ProviderQos = require("../../../../main/js/generated/joynr/types/ProviderQos");
const UtilInternal = require("../../../../main/js/joynr/util/UtilInternal");
const waitsFor = require("../../../../test/js/global/WaitsFor");

const asyncTimeout = 5000;

describe("libjoynr-js.joynr.proxy.ProxyEvent", () => {
    let weakSignal, broadcastWithoutFilterParameters;
    let subscriptionId;
    const subscriptionQos = new MulticastSubscriptionQos();
    let subscriptionManagerSpy;
    let proxyParticipantId;
    let providerDiscoveryEntry;
    let broadcastName;

    function checkSpy(spy, errorExpected) {
        if (errorExpected) {
            expect(spy.onFulfilled).not.toHaveBeenCalled();
            expect(spy.onRejected).toHaveBeenCalled();
        } else {
            expect(spy.onFulfilled).toHaveBeenCalled();
            expect(spy.onRejected).not.toHaveBeenCalled();
        }
    }

    beforeEach(done => {
        proxyParticipantId = "proxyParticipantId";
        providerDiscoveryEntry = new DiscoveryEntryWithMetaInfo({
            providerVersion: new Version({ majorVersion: 0, minorVersion: 23 }),
            domain: "testProviderDomain",
            interfaceName: "interfaceName",
            participantId: "providerParticipantId",
            qos: new ProviderQos({}),
            lastSeenDateMs: Date.now(),
            expiryDateMs: Date.now() + 60000,
            publicKeyId: "publicKeyId",
            isLocal: true
        });
        const fakeProxy = {
            proxyParticipantId,
            providerDiscoveryEntry
        };

        subscriptionManagerSpy = jasmine.createSpyObj("subscriptionManager", [
            "registerBroadcastSubscription",
            "unregisterSubscription"
        ]);
        subscriptionId = "subscriptionId";

        subscriptionManagerSpy.registerBroadcastSubscription.and.returnValue(Promise.resolve(subscriptionId));
        subscriptionManagerSpy.unregisterSubscription.and.returnValue(Promise.resolve());

        broadcastName = "weakSignal";
        weakSignal = new ProxyEvent(fakeProxy, {
            broadcastName,
            discoveryQos: new DiscoveryQos(),
            messagingQos: new MessagingQos(),
            dependencies: {
                subscriptionManager: subscriptionManagerSpy
            },
            selective: true,
            filterParameters: {
                a: "reservedForTypeInfo",
                b: "reservedForTypeInfo",
                c: "reservedForTypeInfo"
            }
        });

        broadcastWithoutFilterParameters = new ProxyEvent(fakeProxy, {
            broadcastName,
            discoveryQos: new DiscoveryQos(),
            messagingQos: new MessagingQos(),
            dependencies: {
                subscriptionManager: subscriptionManagerSpy
            }
        });

        done();
    });

    it("selective broadcast without filter parameters works", done => {
        const filterParameterValues = { a: "valueForA" };

        const broadcastFilterParameters = broadcastWithoutFilterParameters.createFilterParameters();
        broadcastFilterParameters.filterParameters = filterParameterValues;
        expect(broadcastFilterParameters.filterParameters).toEqual(filterParameterValues);
        done();
    });

    it("is of correct type", done => {
        expect(weakSignal).toBeDefined();
        expect(weakSignal).not.toBeNull();
        expect(typeof weakSignal === "object").toBeTruthy();
        expect(weakSignal instanceof ProxyEvent).toBeTruthy();
        done();
    });

    it("has correct members", done => {
        expect(weakSignal.subscribe).toBeDefined();
        expect(weakSignal.unsubscribe).toBeDefined();
        done();
    });

    it("subscribe calls subscriptionManager", done => {
        const partitions = ["1", "2", "3"];
        const onReceive = function() {};
        const onError = function() {};
        const onSubscribed = function() {};

        const expectedPartitions = UtilInternal.extend([], partitions);
        const expectedSubscriptionQos = new MulticastSubscriptionQos(UtilInternal.extendDeep({}, subscriptionQos));
        let expectedDiscoveryEntry = UtilInternal.extendDeep({}, providerDiscoveryEntry);
        expectedDiscoveryEntry.providerVersion = new Version(expectedDiscoveryEntry.providerVersion);
        expectedDiscoveryEntry.qos = new ProviderQos(expectedDiscoveryEntry.qos);
        expectedDiscoveryEntry = new DiscoveryEntryWithMetaInfo(expectedDiscoveryEntry);

        weakSignal
            .subscribe({
                subscriptionQos,
                partitions,
                onReceive,
                onError,
                onSubscribed
            })
            .catch(fail);

        expect(subscriptionManagerSpy.registerBroadcastSubscription).toHaveBeenCalled();
        expect(subscriptionManagerSpy.registerBroadcastSubscription.calls.argsFor(0)[0].proxyId).toEqual(
            proxyParticipantId
        );
        expect(subscriptionManagerSpy.registerBroadcastSubscription.calls.argsFor(0)[0].providerDiscoveryEntry).toEqual(
            expectedDiscoveryEntry
        );
        expect(subscriptionManagerSpy.registerBroadcastSubscription.calls.argsFor(0)[0].broadcastName).toBe(
            broadcastName
        );
        expect(subscriptionManagerSpy.registerBroadcastSubscription.calls.argsFor(0)[0].subscriptionQos).toEqual(
            jasmine.objectContaining({
                _typeName: expectedSubscriptionQos._typeName,
                expiryDateMs: expectedSubscriptionQos.expiryDateMs
            })
        );
        expect(subscriptionManagerSpy.registerBroadcastSubscription.calls.argsFor(0)[0].subscriptionId).toBeUndefined();
        expect(subscriptionManagerSpy.registerBroadcastSubscription.calls.argsFor(0)[0].selective).toEqual(true);
        expect(subscriptionManagerSpy.registerBroadcastSubscription.calls.argsFor(0)[0].partitions).toEqual(
            expectedPartitions
        );
        expect(subscriptionManagerSpy.registerBroadcastSubscription.calls.argsFor(0)[0].onError).toBe(onError);
        expect(subscriptionManagerSpy.registerBroadcastSubscription.calls.argsFor(0)[0].onSubscribed).toBe(
            onSubscribed
        );
        done();
    });

    it("subscribe provides a subscriptionId", done => {
        const spy = jasmine.createSpyObj("spy", ["onFulfilled", "onRejected"]);

        spy.onFulfilled.and.callFake(subscriptionId => {
            return subscriptionId;
        });

        weakSignal
            .subscribe({
                subscriptionQos,
                onReceive() {}
            })
            .then(spy.onFulfilled)
            .catch(spy.onRejected)
            .then(passedId => {
                expect(passedId).toBeDefined();
                expect(typeof passedId === "string").toBeTruthy();
            });

        waitsFor(
            () => {
                return spy.onFulfilled.calls.count() > 0;
            },
            "The promise is not pending any more",
            asyncTimeout
        )
            .then(() => {
                checkSpy(spy);
                done();
                return null;
            })
            .catch(fail);
    });

    it("subscribe and unsubscribe notify on success", done => {
        // precondition: the broadcast object has already been created
        expect(weakSignal.subscribe).toBeDefined();
        expect(typeof weakSignal.subscribe === "function").toBeTruthy();
        expect(weakSignal.unsubscribe).toBeDefined();
        expect(typeof weakSignal.unsubscribe === "function").toBeTruthy();

        const spySubscribePromise = jasmine.createSpyObj("spySubscribePromise", ["onFulfilled", "onRejected"]);
        // the spy returns the subscriptionId that it was called with,
        // so that the next then in the promise chain also can see it
        spySubscribePromise.onFulfilled.and.callFake(passedSubscriptionId => {
            return passedSubscriptionId;
        });

        const spyUnsubscribePromise = jasmine.createSpyObj("spyUnsubscribePromise", ["onFulfilled", "onRejected"]);

        weakSignal
            .subscribe({
                subscriptionQos,
                receive() {}
            })
            .then(spySubscribePromise.onFulfilled)
            .catch(spySubscribePromise.onRejected)
            .then(passedSubscriptionId => {
                return weakSignal.unsubscribe({
                    subscriptionId: passedSubscriptionId
                });
            })
            .then(() => {
                spyUnsubscribePromise.onFulfilled();
                return null;
            })
            .catch(() => {
                spyUnsubscribePromise.onRejected();
                return null;
            });

        waitsFor(
            () => {
                return spyUnsubscribePromise.onFulfilled.calls.count() > 0;
            },
            "The promise is not pending any more",
            asyncTimeout
        )
            .then(() => {
                expect(spySubscribePromise.onFulfilled).toHaveBeenCalled();
                expect(spySubscribePromise.onRejected).not.toHaveBeenCalled();

                expect(spyUnsubscribePromise.onFulfilled).toHaveBeenCalled();
                expect(spyUnsubscribePromise.onRejected).not.toHaveBeenCalled();
                done();
                return null;
            })
            .catch(fail);
    });

    it("subscribe rejects if filter parameters are only partially filled", done => {
        subscriptionManagerSpy.registerBroadcastSubscription.and.returnValue(Promise.resolve());

        //subscribing without filter parameters should work
        weakSignal
            .subscribe({
                subscriptionQos,
                receive() {}
            })
            .then(() => {
                //subscribing with empty filter parameters should work
                return weakSignal.subscribe({
                    subscriptionQos,
                    receive() {},
                    filterParameters: weakSignal.createFilterParameters()
                });
            })
            .then(() => {
                //subscribing with filter parameters having value null should work
                return weakSignal.subscribe({
                    subscriptionQos,
                    receive() {}
                });
            })
            .then(() => {
                //subscribing with filter parameters having value null should work
                return weakSignal.subscribe({
                    subscriptionQos,
                    receive() {},
                    filterParameters: null
                });
            })
            .then(() => {
                //subscribing with partially defined filter parameters should fail
                const filterParameters = weakSignal.createFilterParameters();
                filterParameters.setA("a");
                filterParameters.setB("b");
                //do not set filter paramter "c", so assuming an error during subscribe
                return weakSignal
                    .subscribe({
                        subscriptionQos,
                        receive() {},
                        filterParameters
                    })
                    .then(fail)
                    .catch(() => done());
            })
            .catch(fail);
    });

    it("subscribe notifies on failure", done => {
        // precondition: the broadcast object has already been created
        expect(weakSignal.subscribe).toBeDefined();
        expect(typeof weakSignal.subscribe === "function").toBeTruthy();

        const expectedError = new Error("error registering broadcast");
        subscriptionManagerSpy.registerBroadcastSubscription.and.returnValue(Promise.reject(expectedError));

        weakSignal
            .subscribe({
                subscriptionQos,
                receive() {}
            })
            .then(fail)
            .catch(error => {
                expect(error).toEqual(expectedError);
                done();
                return null;
            })
            .catch(fail);
    });

    it("unsubscribe notifies on failure", done => {
        // precondition: the broadcast object has already been created
        expect(weakSignal.subscribe).toBeDefined();
        expect(typeof weakSignal.subscribe === "function").toBeTruthy();
        expect(weakSignal.unsubscribe).toBeDefined();
        expect(typeof weakSignal.unsubscribe === "function").toBeTruthy();

        const spyUnsubscribePromise = jasmine.createSpyObj("spyUnsubscribePromise", ["onFulfilled", "onRejected"]);

        const expectedError = new Error("error unsubscribing from broadcast");
        subscriptionManagerSpy.unregisterSubscription.and.returnValue(Promise.reject(expectedError));

        weakSignal
            .subscribe({
                subscriptionQos,
                receive() {}
            })
            .then(passedSubscriptionId => {
                return weakSignal.unsubscribe({
                    subscriptionId: passedSubscriptionId
                });
            })
            .then(spyUnsubscribePromise.onFulfilled)
            .catch(spyUnsubscribePromise.onRejected);

        waitsFor(
            () => {
                return spyUnsubscribePromise.onRejected.calls.count() > 0;
            },
            "The promise is not pending any more",
            asyncTimeout
        )
            .then(() => {
                expect(spyUnsubscribePromise.onFulfilled).not.toHaveBeenCalled();
                expect(spyUnsubscribePromise.onRejected).toHaveBeenCalledWith(expectedError);
                done();
                return null;
            })
            .catch(fail);
    });
    it("throws error if subscribe is invoked with invalid partitions", () => {
        expect(() => {
            weakSignal.subscribe({
                subscriptionQos,
                partitions: ["_"]
            });
        }).toThrow();
        expect(() => {
            weakSignal.subscribe({
                subscriptionQos,
                partitions: ["./$"]
            });
        }).toThrow();
    });
});
