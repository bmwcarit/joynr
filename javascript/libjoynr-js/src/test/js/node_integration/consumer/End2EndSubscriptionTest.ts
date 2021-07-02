/*eslint no-unused-vars: "off" */
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

import joynr from "../../../../main/js/joynr";
import { SubscribeSettings } from "../../../../main/js/joynr/proxy/ProxyAttribute";
import * as RadioProxy from "../../../generated/joynr/vehicle/RadioProxy";
import RadioStation from "../../../generated/joynr/vehicle/radiotypes/RadioStation";
import Country from "../../../generated/joynr/datatypes/exampleTypes/Country";
import ComplexTestType from "../../../generated/joynr/tests/testTypes/ComplexTestType";
import SubscriptionException from "../../../../main/js/joynr/exceptions/SubscriptionException";
import * as IntegrationUtils from "../IntegrationUtils";
import End2EndAbstractTest from "../End2EndAbstractTest";
import * as DiagnosticTags from "joynr/joynr/system/DiagnosticTags";
import OnChangeSubscriptionQos = require("../../../../main/js/joynr/proxy/OnChangeSubscriptionQos");
import PeriodicSubscriptionQos = require("../../../../main/js/joynr/proxy/PeriodicSubscriptionQos");
import MulticastSubscriptionQos = require("../../../../main/js/joynr/proxy/MulticastSubscriptionQos");
import OnChangeWithKeepAliveSubscriptionQos = require("../../../../main/js/joynr/proxy/OnChangeWithKeepAliveSubscriptionQos");
import { reversePromise } from "../../testUtil";
import SubscriptionQos = require("../../../../main/js/joynr/proxy/SubscriptionQos");

interface Spies {
    onReceive: jest.Mock;
    onError: jest.Mock;
}

describe("libjoynr-js.integration.end2end.subscription", () => {
    const subscriptionLength = 2000;
    const safetyTimeout = 500;
    let subscriptionQosOnChange: OnChangeSubscriptionQos;
    let subscriptionQosInterval: PeriodicSubscriptionQos;
    let subscriptionQosMulticast: MulticastSubscriptionQos;
    let subscriptionQosMixed: OnChangeWithKeepAliveSubscriptionQos;
    let radioProxy: RadioProxy;
    const abstractTest = new End2EndAbstractTest("End2EndSubscriptionTest", "TestEnd2EndCommProviderProcess");
    const setAttribute = abstractTest.setAttribute;
    const setupSubscriptionAndReturnSpy = abstractTest.setupSubscriptionAndReturnSpy;
    const unsubscribeSubscription = abstractTest.unsubscribeSubscription;
    const callOperation = abstractTest.callOperation;
    const expectPublication = abstractTest.expectPublication;
    const waitFor = abstractTest.waitFor;
    const terminateAllSubscriptions = abstractTest.terminateAllSubscriptions;
    let subscriptionSettings: SubscribeSettings<any> &
        Spies & {
            partitions?: string[];
        };
    let subScriptionDeferred: {
        onReceive: IntegrationUtils.Deferred;
        onError: IntegrationUtils.Deferred;
    };

    let spyOnReceiveCounter: number;
    let spyOnReceiveCallsNumber: number;

    beforeEach(async () => {
        const settings = await abstractTest.beforeEach();

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

        subScriptionDeferred = {
            onReceive: IntegrationUtils.createPromise(),
            onError: IntegrationUtils.createPromise()
        };
        subscriptionSettings = {
            subscriptionQos: subscriptionQosInterval,
            onReceive: jest.fn().mockImplementation((...args) => subScriptionDeferred.onReceive.resolve(...args)),
            onError: jest.fn().mockImplementation((...args) => subScriptionDeferred.onError.resolve(...args)),
            onSubscribed: jest.fn()
        };

        // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
        radioProxy = settings.radioProxy!;

        spyOnReceiveCallsNumber = 0;
        spyOnReceiveCounter = 0;
    });

    async function expectMultiplePublications(spy: Spies, expectedPublications: number): Promise<any[]> {
        await subScriptionDeferred.onReceive.promise;
        const condition = (): boolean => {
            return spy.onReceive.mock.calls.length === expectedPublications;
        };
        await waitFor(condition, 1000);
        expect(spy.onReceive.mock.calls.length).toBe(expectedPublications);
        const calls = spy.onReceive.mock.calls;
        spy.onReceive.mockClear();
        return calls;
    }

    async function publishesValue(subscriptionQos: SubscriptionQos) {
        const spy = await setupSubscriptionAndReturnSpy("numberOfStations", subscriptionQos);
        await expectPublication(spy, (publicationCallback: any) => {
            expect(typeof publicationCallback[0] === "number").toBeTruthy();
        });
    }

    async function checkUnsubscribe(timeout: number, subscriptionQos: SubscriptionQos) {
        subscriptionSettings.subscriptionQos = subscriptionQos;
        const subscriptionId = await radioProxy.numberOfStations.subscribe(subscriptionSettings);
        await subScriptionDeferred.onReceive.promise;

        const nrOfPublications = subscriptionSettings.onReceive.mock.calls.length;

        await radioProxy.numberOfStations.unsubscribe({
            subscriptionId
        });

        await IntegrationUtils.waitALittle(2 * timeout);
        expect(subscriptionSettings.onReceive.mock.calls.length).toEqual(nrOfPublications);
    }

    it("subscribe to failingSyncAttribute", async () => {
        await radioProxy.failingSyncAttribute.subscribe(subscriptionSettings);
        const exception = await subScriptionDeferred.onError.promise;
        // TODO: change SubscriptionManager.handlePublication to use Typing.augmentTypes.
        // expect(exception).toBeInstanceOf(ProviderRuntimeException);
        expect(exception._typeName).toEqual("joynr.exceptions.ProviderRuntimeException");
        expect(exception.detailMessage).toEqual("failure in failingSyncAttribute getter");
    });

    it("subscribe to failingAsyncAttribute", async () => {
        await radioProxy.failingAsyncAttribute.subscribe(subscriptionSettings);
        const exception = await subScriptionDeferred.onError.promise;
        // TODO: change SubscriptionManager.handlePublication to use Typing.augmentTypes.
        // expect(exception).toBeInstanceOf(ProviderRuntimeException);
        expect(exception._typeName).toEqual("joynr.exceptions.ProviderRuntimeException");
        expect(exception.detailMessage).toEqual("failure in failingSyncAttribute getter");
    });

    it("subscribe to enumAttribute", async () => {
        await radioProxy.enumAttribute.set({ value: Country.AUSTRIA });
        const mySpy = await setupSubscriptionAndReturnSpy("enumAttribute", subscriptionQosOnChange);

        await expectPublication(mySpy, (call: any) => {
            expect(call[0]).toEqual(Country.AUSTRIA);
        });

        await radioProxy.enumAttribute.set({ value: Country.AUSTRALIA });

        await expectPublication(mySpy, (call: any) => {
            expect(call[0]).toEqual(Country.AUSTRALIA);
        });

        await radioProxy.enumAttribute.set({ value: Country.ITALY });

        await expectPublication(mySpy, (call: any) => {
            expect(call[0]).toEqual(Country.ITALY);
        });
    });

    it("subscribe to complex typedef attribute", () => {
        const value = new RadioStation({
            name: "TestEnd2EndComm.typeDefForStructAttribute.RadioStation",
            byteBuffer: []
        });
        return setAttribute("typeDefForStruct", value)
            .then(() => {
                return setupSubscriptionAndReturnSpy("typeDefForStruct", subscriptionQosOnChange);
            })
            .then(spy => {
                return expectPublication(spy, (call: any) => {
                    expect(call[0]).toEqual(value);
                });
            });
    });

    it("subscribe to primitive typedef attribute", async () => {
        const value = 1234543;
        await setAttribute("typeDefForPrimitive", value);
        const mySpy = await setupSubscriptionAndReturnSpy("typeDefForPrimitive", subscriptionQosOnChange);

        await expectPublication(mySpy, (call: any) => {
            expect(call[0]).toEqual(value);
        });
    });

    it("subscribe to byteBufferAttribute", () => {
        //initialize attribute
        let mySpy: any;
        let testByteBufferAttribute: any;

        return setAttribute("byteBufferAttribute", [])
            .then(() => {
                return setupSubscriptionAndReturnSpy("byteBufferAttribute", subscriptionQosOnChange);
            })
            .then(spy => {
                mySpy = spy;
                return expectPublication(mySpy, (call: any) => {
                    expect(call[0]).toEqual([]);
                });
            })
            .then(() => {
                testByteBufferAttribute = function(expectedByteBuffer: any) {
                    return setAttribute("byteBufferAttribute", expectedByteBuffer).then(() => {
                        return expectPublication(mySpy, (call: any) => {
                            expect(call[0]).toEqual(expectedByteBuffer);
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
                const byteBuffer10k = [];
                for (let i = 0; i < 10000; i++) {
                    byteBuffer10k.push(i % 256);
                }
                return testByteBufferAttribute(byteBuffer10k);
            });
    });

    it("subscribe to weakSignal broadcast having ByteBuffer as output parameter", async () => {
        const mySpy = await setupSubscriptionAndReturnSpy("weakSignal", subscriptionQosOnChange);

        await callOperation("triggerBroadcasts", {
            broadcastName: "weakSignal",
            times: 1
        });

        await expectPublication(mySpy, (call: any) => {
            expect(call[0].radioStation).toEqual("radioStation");
            expect(call[0].byteBuffer).toEqual([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0]);
        });
    });

    it("call methodFireAndForgetWithoutParams and expect to call the provider", async () => {
        const mySpy = await setupSubscriptionAndReturnSpy("fireAndForgetCallArrived", subscriptionQosOnChange);
        await callOperation("methodFireAndForgetWithoutParams", {});

        await expectPublication(mySpy, (call: any) => {
            expect(call[0].methodName).toEqual("methodFireAndForgetWithoutParams");
        });
    });

    it("call methodFireAndForget and expect to call the provider", async () => {
        const mySpy = await setupSubscriptionAndReturnSpy("fireAndForgetCallArrived", subscriptionQosOnChange);

        await callOperation("methodFireAndForget", {
            intIn: 0,
            stringIn: "methodFireAndForget",
            complexTestTypeIn: new ComplexTestType({
                a: 0,
                b: 1
            })
        });

        await expectPublication(mySpy, (call: any) => {
            expect(call[0].methodName).toEqual("methodFireAndForget");
        });
    });

    it("subscribe to broadcastWithEnum", async () => {
        const mySpy = await setupSubscriptionAndReturnSpy("broadcastWithEnum", subscriptionQosMulticast);

        await callOperation("triggerBroadcasts", {
            broadcastName: "broadcastWithEnum",
            times: 1
        });

        await expectPublication(mySpy, (call: any) => {
            expect(call[0].enumOutput).toEqual(Country.CANADA);
            expect(call[0].enumArrayOutput).toEqual([Country.GERMANY, Country.ITALY]);
        });
    });

    it("subscribe to emptyBroadcast", async () => {
        const mySpy = await setupSubscriptionAndReturnSpy("emptyBroadcast", subscriptionQosOnChange);

        await callOperation("triggerBroadcasts", {
            broadcastName: "emptyBroadcast",
            times: 1
        });

        await expectPublication(mySpy);
    });

    describe("multicasts with partitions", () => {
        function unsubscribeMulticastSubscription(spy: any) {
            return unsubscribeSubscription("emptyBroadcast", spy.onFulfilled.mock.calls.slice(-1)[0][0]);
        }
        function setupMulticastSubscriptionWithPartitionAndReturnSpy(partitions: any) {
            return setupSubscriptionAndReturnSpy("emptyBroadcast", subscriptionQosOnChange, partitions);
        }

        function triggerBroadcastWithPartitions(partitions: any, hierarchicBroadcast: any) {
            return callOperation("triggerBroadcastsWithPartitions", {
                broadcastName: "emptyBroadcast",
                times: 1,
                partitions,
                hierarchicBroadcast
            });
        }

        async function expectNoMorePublication(spy: any, timeout: any): Promise<any> {
            await IntegrationUtils.waitALittle(timeout);
            expect(spy.onError).not.toHaveBeenCalled();
            expect(spy.onReceive).not.toHaveBeenCalled();
            return spy;
        }

        function testMulticastWithPartitionsExtended(
            subscribePartitions: any,
            publicationPartitions: any,
            times: any,
            timeout: number
        ) {
            spyOnReceiveCallsNumber = times;
            return setupMulticastSubscriptionWithPartitionAndReturnSpy(subscribePartitions)
                .then(spy => {
                    // Mock implementation of onReceive function
                    spy.onReceive.mockImplementation(() => {
                        spyOnReceiveCounter++;
                        if (spyOnReceiveCounter === spyOnReceiveCallsNumber) {
                            subScriptionDeferred.onReceive.resolve();
                        }
                    });
                    // Wait a little bit to be sure that subscribing is finished
                    return IntegrationUtils.waitALittle(timeout).then(() => {
                        return spy;
                    });
                })
                .then(spy => {
                    return Promise.all(
                        publicationPartitions.map((partitions: any) => {
                            return triggerBroadcastWithPartitions(partitions, true);
                        })
                    ).then(() => {
                        return spy;
                    });
                })
                .then(spy => {
                    return expectMultiplePublications(spy, times).then(() => {
                        spy.onReceive.mockClear();
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
        function testMulticastWithPartitions(partitions: string[]) {
            return testMulticastWithPartitionsExtended(partitions, [partitions], 1, 100);
        }

        it("with empty partitions", async () => {
            await testMulticastWithPartitions([]);
        }, 20000);

        it("with first-level partition", async () => {
            await testMulticastWithPartitions(["a"]);
        }, 20000);

        it("with multi-level partition", async () => {
            await testMulticastWithPartitions(["a", "b", "c", "d", "e", "f", "g"]);
        }, 20000);

        it("with multi-level publication partition including asterisk", async () => {
            await testMulticastWithPartitionsExtended(
                ["a", "b", "c", "d", "e", "f", "*"],
                [["a", "b", "c", "d", "e", "f"]],
                1,
                100
            );
        }, 20000);

        it("with multi-level subscribe partition including asterisk", async () => {
            await testMulticastWithPartitionsExtended(
                ["a", "b", "c", "d", "e", "f", "*"],
                [["a", "b", "c", "d", "e", "f", "g", "h"]],
                3,
                1000
            );
        }, 20000);

        it("with publication partition including plus sign", async () => {
            await testMulticastWithPartitionsExtended(["a", "+", "c"], [["a", "b", "c"]], 1, 1000);
        }, 20000);

        it("with multi-level publication partition including plus sign", async () => {
            await testMulticastWithPartitionsExtended(
                ["a", "+", "c"],
                [["a", "b", "c"], ["a", "b", "d"], ["a", "xyz", "c", "d", "e", "f"]],
                2,
                1000
            );
        }, 20000);

        it("subscribe to the same non-selective broadcast with different partitions", async () => {
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
                        spies[0].onReceive.mockClear();
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
                });
        });
    });

    it("subscribe to type def broadcast", async () => {
        const typeDefStructOutput = new RadioStation({
            name: "TestEnd2EndCommProviderProcess.broadcastWithTypeDefs.RadioStation",
            byteBuffer: []
        });
        const typeDefPrimitiveOutput = 123456;
        const mySpy = await setupSubscriptionAndReturnSpy("broadcastWithTypeDefs", subscriptionQosOnChange);

        await callOperation("triggerBroadcasts", {
            broadcastName: "broadcastWithTypeDefs",
            times: 1
        });

        await expectPublication(mySpy, (call: any) => {
            expect(call[0].typeDefStructOutput).toEqual(typeDefStructOutput);
            expect(call[0].typeDefPrimitiveOutput).toEqual(typeDefPrimitiveOutput);
        });
    });

    it("subscribe to broadcastWithEnum and get burst", async () => {
        subscriptionQosOnChange.minIntervalMs = 0;
        const times = 100;
        subscriptionSettings.subscriptionQos = subscriptionQosOnChange;
        await radioProxy.broadcastWithEnum.subscribe(subscriptionSettings);
        await callOperation("triggerBroadcasts", {
            broadcastName: "broadcastWithEnum",
            times
        });
        const calls = await expectMultiplePublications(subscriptionSettings, times);
        for (let i = 0; i < times; i++) {
            expect(calls[i][0].enumOutput).toEqual(Country.CANADA);
            expect(calls[i][0].enumArrayOutput).toEqual([Country.GERMANY, Country.ITALY]);
        }
    });

    it("subscribe to enumArrayAttribute", async () => {
        const attributeName = "enumArrayAttribute";
        const value1 = [Country.AUSTRIA];
        const value2 = [Country.AUSTRIA, Country.GERMANY];
        const value3 = [Country.AUSTRIA, Country.GERMANY, Country.ITALY];
        await setAttribute(attributeName, value1);
        const mySpy = await setupSubscriptionAndReturnSpy(attributeName, subscriptionQosOnChange);

        await expectPublication(mySpy, (call: any) => {
            expect(call[0]).toEqual(value1);
        });

        await setAttribute(attributeName, value2);

        await expectPublication(mySpy, (call: any) => {
            expect(call[0]).toEqual(value2);
        });

        await setAttribute(attributeName, value3);

        await expectPublication(mySpy, (call: any) => {
            expect(call[0]).toEqual(value3);
        });
    });

    it("resolves attribute subscription and calls onSubscribed", async () => {
        const spy = {
            onReceive: jest.fn(),
            onError: jest.fn(),
            onSubscribed: jest.fn()
        };
        const qosSettings = new joynr.proxy.PeriodicSubscriptionQos();
        const storedSubscriptionId = await radioProxy.numberOfStations.subscribe({
            subscriptionQos: qosSettings,
            onReceive: spy.onReceive,
            onError: spy.onError,
            onSubscribed: spy.onSubscribed
        });

        expect(spy.onError).not.toHaveBeenCalled();
        expect(spy.onSubscribed).toHaveBeenCalled();
        expect(spy.onSubscribed).toHaveBeenCalledWith(storedSubscriptionId);
    });

    it("rejects attribute subscription if periodMs is too small", async () => {
        const qosSettings = new joynr.proxy.PeriodicSubscriptionQos({
            expiryDateMs: 0,
            alertAfterIntervalMs: 0,
            publicationTtlMs: 1000
        });
        // forcibly fake it! The constructor throws, if using this directly
        qosSettings.periodMs = joynr.proxy.PeriodicSubscriptionQos.MIN_PERIOD_MS - 1;
        subscriptionSettings.subscriptionQos = qosSettings;
        const error = await reversePromise(radioProxy.numberOfStations.subscribe(subscriptionSettings));

        expect(error).toBeInstanceOf(SubscriptionException);
        expect(error.subscriptionId).toBeDefined();
        expect(error.detailMessage).toMatch(/is smaller than PeriodicSubscriptionQos/);
        const subscriptionId = error.subscriptionId;
        const detailMessage = error.detailMessage;
        expect(subscriptionSettings.onReceive).not.toHaveBeenCalled();

        await subScriptionDeferred.onError.promise;
        const callbackError = subscriptionSettings.onError.mock.calls.slice(-1)[0][0];
        expect(callbackError).toBeInstanceOf(SubscriptionException);
        expect(callbackError.subscriptionId).toEqual(subscriptionId);
        expect(callbackError.detailMessage).toEqual(detailMessage);
    });

    it("rejects subscription to non-existing attribute ", async () => {
        subscriptionSettings.subscriptionQos = new OnChangeSubscriptionQos();

        const error = await reversePromise(
            (radioProxy as any).nonExistingAttributeOnProviderSide.subscribe(subscriptionSettings)
        );
        expect(error).toBeInstanceOf(SubscriptionException);
        expect(error.subscriptionId).toBeDefined();
        expect(error.detailMessage).toMatch(/misses attribute/);
        const subscriptionId = error.subscriptionId;
        const detailMessage = error.detailMessage;

        await subScriptionDeferred.onError.promise;
        expect(subscriptionSettings.onReceive).not.toHaveBeenCalled();
        expect(subscriptionSettings.onSubscribed).not.toHaveBeenCalled();

        const callbackError = subscriptionSettings.onError.mock.calls.slice(-1)[0][0];
        expect(callbackError).toBeInstanceOf(SubscriptionException);
        expect(callbackError.subscriptionId).toEqual(subscriptionId);
        expect(callbackError.detailMessage).toEqual(detailMessage);
    });

    it("can start a subscription and provides a subscription id", async () => {
        const subscriptionID = await radioProxy.numberOfStations.subscribe(subscriptionSettings);
        expect(subscriptionID).toEqual(expect.any(String));
    });

    it("publishes a value with an onChange subscription", async () => {
        await publishesValue(subscriptionQosOnChange);
    });

    it("publishes a value with an interval subscription", async () => {
        await publishesValue(subscriptionQosInterval);
    });

    it("publishes a value with a mixed subscription", async () => {
        await publishesValue(subscriptionQosMixed);
    });

    it("publishes a value with an ending subscription", async () => {
        subscriptionQosMixed.expiryDateMs = subscriptionLength + Date.now();
        await publishesValue(subscriptionQosMixed);
    });

    it("initially publishes a value on subscription", async () => {
        subscriptionSettings.subscriptionQos = subscriptionQosOnChange;
        await radioProxy.numberOfStations.subscribe(subscriptionSettings);
        const value = await subScriptionDeferred.onReceive.promise;
        expect(value).toEqual(-1);
    });

    const nrPubs = 3;
    it(`publishes correct values with onChange ${nrPubs} times`, async () => {
        subscriptionSettings.subscriptionQos = subscriptionQosOnChange;
        await radioProxy.numberOfStations.subscribe(subscriptionSettings);
        let value = await subScriptionDeferred.onReceive.promise;
        expect(value).toEqual(-1);

        for (let i = 0; i < nrPubs; i++) {
            subScriptionDeferred.onReceive = IntegrationUtils.createPromise();
            value = await subScriptionDeferred.onReceive.promise;
            expect(value).toEqual(i);
        }
    });

    it("publishes correct values with a mixed subscription", async () => {
        // provider will fire an interval publication 1s after
        // initialization with the response "interval"
        // after another 1500 ms it will fire an onChange publication
        // with the response "valueChanged1"
        // after another 10 ms it will try to fire an onChange
        // publication with the response "valueChanged2", should be
        // blocked by the
        // PublicationManager on the Provider side

        subscriptionQosMixed.expiryDateMs = subscriptionLength + Date.now();
        subscriptionSettings.subscriptionQos = subscriptionQosMixed;
        await radioProxy.mixedSubscriptions.subscribe(subscriptionSettings);

        let value = await subScriptionDeferred.onReceive.promise;
        expect(value).toEqual("interval");

        subScriptionDeferred.onReceive = IntegrationUtils.createPromise();
        value = await subScriptionDeferred.onReceive.promise;
        expect(value).toEqual("interval");

        subScriptionDeferred.onReceive = IntegrationUtils.createPromise();
        value = await subScriptionDeferred.onReceive.promise;
        expect(value).toEqual("valueChanged1");

        subScriptionDeferred.onReceive = IntegrationUtils.createPromise();
        value = await subScriptionDeferred.onReceive.promise;
        expect(value).toEqual("valueChanged2");
    });

    it("terminates correctly according to the endDate ", async () => {
        // provider will fire an interval publication 1s after initialization with the response "interval"
        // after another 500 ms it will fire an onChange publication with the response "valueChanged1"
        // which should be blocked on the Provider side since the subscription is expired (1400ms)

        subscriptionQosMixed.expiryDateMs = 1400 + Date.now();
        subscriptionSettings.subscriptionQos = subscriptionQosMixed;
        await radioProxy.mixedSubscriptions.subscribe(subscriptionSettings);
        let value = await subScriptionDeferred.onReceive.promise;
        expect(value).toBe("interval");

        subScriptionDeferred.onReceive = IntegrationUtils.createPromise();
        value = await subScriptionDeferred.onReceive.promise;
        expect(value).toEqual("interval");

        (subscriptionSettings.onReceive as jest.Mock).mockClear();
        await IntegrationUtils.waitALittle(subscriptionQosMixed.expiryDateMs - Date.now() + safetyTimeout);
        expect(subscriptionSettings.onReceive).not.toHaveBeenCalled();
    });

    it("unsubscribes onChange subscription successfully", () => {
        return checkUnsubscribe(1000, subscriptionQosOnChange);
    });

    it("unsubscribes interval subscription successfully", () => {
        return checkUnsubscribe(1000, subscriptionQosInterval);
    });

    it("unsubscribes all active subscriptions", async () => {
        let value = 1234543;
        const toleranceMs = 200;
        await setAttribute("typeDefForPrimitive", value);
        const mySpy = await setupSubscriptionAndReturnSpy("typeDefForPrimitive", subscriptionQosOnChange);

        await expectPublication(mySpy, (call: any) => {
            expect(call[0]).toEqual(value);
        });

        await setAttribute("typeDefForPrimitive", ++value);
        await IntegrationUtils.waitALittle(toleranceMs);
        expect(mySpy.onReceive).toHaveBeenCalled();
        mySpy.onReceive.mockClear();

        jest.spyOn(DiagnosticTags, "forSubscriptionStop");
        await terminateAllSubscriptions();
        expect(DiagnosticTags.forSubscriptionStop).toHaveBeenCalled();

        await setAttribute("typeDefForPrimitive", ++value);
        await IntegrationUtils.waitALittle(toleranceMs);
        expect(mySpy.onReceive).not.toHaveBeenCalled();
    });

    afterEach(abstractTest.afterEach);
});
