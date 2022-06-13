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
import ProxyEvent from "../../../../main/js/joynr/proxy/ProxyEvent";

import DiscoveryQos from "../../../../main/js/joynr/proxy/DiscoveryQos";
import MessagingQos from "../../../../main/js/joynr/messaging/MessagingQos";
import MulticastSubscriptionQos from "../../../../main/js/joynr/proxy/MulticastSubscriptionQos";
import DiscoveryEntryWithMetaInfo from "../../../../main/js/generated/joynr/types/DiscoveryEntryWithMetaInfo";
import Version from "../../../../main/js/generated/joynr/types/Version";
import ProviderQos from "../../../../main/js/generated/joynr/types/ProviderQos";
import * as UtilInternal from "../../../../main/js/joynr/util/UtilInternal";

describe("libjoynr-js.joynr.proxy.ProxyEvent", () => {
    let weakSignal: any, broadcastWithoutFilterParameters: any;
    let subscriptionId: any;
    const subscriptionQos = new MulticastSubscriptionQos();
    let subscriptionManagerSpy: any;
    let proxyParticipantId: any;
    let providerDiscoveryEntry: any;
    let broadcastName: string;

    beforeEach(done => {
        proxyParticipantId = "proxyParticipantId";
        providerDiscoveryEntry = new DiscoveryEntryWithMetaInfo({
            providerVersion: new Version({ majorVersion: 0, minorVersion: 23 }),
            domain: "testProviderDomain",
            interfaceName: "interfaceName",
            participantId: "providerParticipantId",
            qos: new ProviderQos({} as any),
            lastSeenDateMs: Date.now(),
            expiryDateMs: Date.now() + 60000,
            publicKeyId: "publicKeyId",
            isLocal: true
        });
        const fakeProxy = {
            proxyParticipantId,
            providerDiscoveryEntry
        };

        subscriptionManagerSpy = {
            registerBroadcastSubscription: jest.fn(),
            unregisterSubscription: jest.fn()
        };
        subscriptionId = "subscriptionId";

        subscriptionManagerSpy.registerBroadcastSubscription.mockReturnValue(Promise.resolve(subscriptionId));
        subscriptionManagerSpy.unregisterSubscription.mockReturnValue(Promise.resolve());

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
            },
            broadcastParameter: []
        });

        broadcastWithoutFilterParameters = new ProxyEvent(fakeProxy, {
            broadcastName,
            discoveryQos: new DiscoveryQos(),
            messagingQos: new MessagingQos(),
            dependencies: {
                subscriptionManager: subscriptionManagerSpy
            },
            broadcastParameter: []
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

    it("has correct members", () => {
        expect(weakSignal.subscribe).toBeDefined();
        expect(weakSignal.unsubscribe).toBeDefined();
    });

    it("subscribe calls subscriptionManager", async () => {
        const partitions = ["1", "2", "3"];
        const onReceive = function() {
            // Do nothing
        };
        const onError = function() {
            // Do nothing
        };
        const onSubscribed = function() {
            // Do nothing
        };

        const expectedPartitions = UtilInternal.extend([], partitions);
        const expectedSubscriptionQos = new MulticastSubscriptionQos(UtilInternal.extendDeep({}, subscriptionQos));
        let expectedDiscoveryEntry: any = UtilInternal.extendDeep({}, providerDiscoveryEntry);
        expectedDiscoveryEntry.providerVersion = new Version(expectedDiscoveryEntry.providerVersion);
        expectedDiscoveryEntry.qos = new ProviderQos(expectedDiscoveryEntry.qos);
        expectedDiscoveryEntry = new DiscoveryEntryWithMetaInfo(expectedDiscoveryEntry);

        await weakSignal.subscribe({
            subscriptionQos,
            partitions,
            onReceive,
            onError,
            onSubscribed
        });

        expect(subscriptionManagerSpy.registerBroadcastSubscription).toHaveBeenCalled();
        expect(subscriptionManagerSpy.registerBroadcastSubscription.mock.calls[0][0].proxyId).toEqual(
            proxyParticipantId
        );
        expect(subscriptionManagerSpy.registerBroadcastSubscription.mock.calls[0][0].providerDiscoveryEntry).toEqual(
            expectedDiscoveryEntry
        );
        expect(subscriptionManagerSpy.registerBroadcastSubscription.mock.calls[0][0].broadcastName).toBe(broadcastName);
        expect(subscriptionManagerSpy.registerBroadcastSubscription.mock.calls[0][0].subscriptionQos).toEqual(
            expect.objectContaining({
                _typeName: expectedSubscriptionQos._typeName,
                expiryDateMs: expectedSubscriptionQos.expiryDateMs
            })
        );
        expect(subscriptionManagerSpy.registerBroadcastSubscription.mock.calls[0][0].subscriptionId).toBeUndefined();
        expect(subscriptionManagerSpy.registerBroadcastSubscription.mock.calls[0][0].selective).toEqual(true);
        expect(subscriptionManagerSpy.registerBroadcastSubscription.mock.calls[0][0].partitions).toEqual(
            expectedPartitions
        );
        expect(subscriptionManagerSpy.registerBroadcastSubscription.mock.calls[0][0].onError).toBe(onError);
        expect(subscriptionManagerSpy.registerBroadcastSubscription.mock.calls[0][0].onSubscribed).toBe(onSubscribed);
    });

    it("subscribe provides a subscriptionId", async () => {
        const passedId = await weakSignal.subscribe({
            subscriptionQos,
            onReceive() {
                // Do nothing
            }
        });
        expect(passedId).toBeDefined();
        expect(typeof passedId === "string").toBeTruthy();
    });

    it("subscribe and unsubscribe notify on success", async () => {
        // precondition: the broadcast object has already been created
        expect(weakSignal.subscribe).toBeDefined();
        expect(typeof weakSignal.subscribe === "function").toBeTruthy();
        expect(weakSignal.unsubscribe).toBeDefined();
        expect(typeof weakSignal.unsubscribe === "function").toBeTruthy();

        const passedSubscriptionId = await weakSignal.subscribe({
            subscriptionQos,
            receive() {
                // Do nothing
            }
        });
        return weakSignal.unsubscribe({
            subscriptionId: passedSubscriptionId
        });
    });

    it("subscribe rejects if filter parameters are only partially filled", done => {
        subscriptionManagerSpy.registerBroadcastSubscription.mockReturnValue(Promise.resolve());

        //subscribing without filter parameters should work
        weakSignal
            .subscribe({
                subscriptionQos,
                receive() {
                    // Do nothing
                }
            })
            .then(() => {
                //subscribing with empty filter parameters should work
                return weakSignal.subscribe({
                    subscriptionQos,
                    receive() {
                        // Do nothing
                    },
                    filterParameters: weakSignal.createFilterParameters()
                });
            })
            .then(() => {
                //subscribing with filter parameters having value null should work
                return weakSignal.subscribe({
                    subscriptionQos,
                    receive() {
                        // Do nothing
                    }
                });
            })
            .then(() => {
                //subscribing with filter parameters having value null should work
                return weakSignal.subscribe({
                    subscriptionQos,
                    receive() {
                        /// Do nothing
                    },
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
                        receive() {
                            // Do nothing
                        },
                        filterParameters
                    })
                    .then(() => done.fail())
                    .catch(() => done());
            })
            .catch(() => done.fail());
    });

    it("subscribe notifies on failure", done => {
        // precondition: the broadcast object has already been created
        expect(weakSignal.subscribe).toBeDefined();
        expect(typeof weakSignal.subscribe === "function").toBeTruthy();

        const expectedError = new Error("error registering broadcast");
        subscriptionManagerSpy.registerBroadcastSubscription.mockReturnValue(Promise.reject(expectedError));

        weakSignal
            .subscribe({
                subscriptionQos,
                receive() {
                    // Do nothing
                }
            })
            .then(() => done.fail())
            .catch((error: any) => {
                expect(error).toEqual(expectedError);
                done();
            })
            .catch(() => done.fail());
    });

    it("unsubscribe notifies on failure", async () => {
        // precondition: the broadcast object has already been created
        expect(weakSignal.subscribe).toBeDefined();
        expect(typeof weakSignal.subscribe === "function").toBeTruthy();
        expect(weakSignal.unsubscribe).toBeDefined();
        expect(typeof weakSignal.unsubscribe === "function").toBeTruthy();

        const expectedError = new Error("error unsubscribing from broadcast");
        subscriptionManagerSpy.unregisterSubscription.mockReturnValue(Promise.reject(expectedError));

        const passedSubscriptionId = await weakSignal.subscribe({
            subscriptionQos,
            receive() {
                // Do nothing
            }
        });
        await expect(
            weakSignal.unsubscribe({
                subscriptionId: passedSubscriptionId
            })
        ).rejects.toBeInstanceOf(Error);
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
