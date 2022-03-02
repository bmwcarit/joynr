/*eslint no-use-before-define: "off"*/
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

import PublicationManager from "../../../../../main/js/joynr/dispatching/subscription/PublicationManager";
import SubscriptionReply from "../../../../../main/js/joynr/dispatching/types/SubscriptionReply";
import SubscriptionRequest from "../../../../../main/js/joynr/dispatching/types/SubscriptionRequest";
import BroadcastSubscriptionRequest from "../../../../../main/js/joynr/dispatching/types/BroadcastSubscriptionRequest";
import MulticastSubscriptionRequest from "../../../../../main/js/joynr/dispatching/types/MulticastSubscriptionRequest";
import SubscriptionStop from "../../../../../main/js/joynr/dispatching/types/SubscriptionStop";
import * as ProviderAttribute from "../../../../../main/js/joynr/provider/ProviderAttribute";
import ProviderEvent from "../../../../../main/js/joynr/provider/ProviderEvent";
import PeriodicSubscriptionQos from "../../../../../main/js/joynr/proxy/PeriodicSubscriptionQos";
import SubscriptionQos from "../../../../../main/js/joynr/proxy/SubscriptionQos";
import OnChangeSubscriptionQos from "../../../../../main/js/joynr/proxy/OnChangeSubscriptionQos";
import OnChangeWithKeepAliveSubscriptionQos from "../../../../../main/js/joynr/proxy/OnChangeWithKeepAliveSubscriptionQos";
import ProviderQos from "../../../../../main/js/generated/joynr/types/ProviderQos";
import ProviderScope from "../../../../../main/js/generated/joynr/types/ProviderScope";
import * as SubscriptionPublication from "../../../../../main/js/joynr/dispatching/types/SubscriptionPublication";
import * as SubscriptionUtil from "../../../../../main/js/joynr/dispatching/subscription/util/SubscriptionUtil";
import SubscriptionException from "../../../../../main/js/joynr/exceptions/SubscriptionException";
import LongTimer from "../../../../../main/js/joynr/util/LongTimer";
import { nanoid } from "nanoid";
import testUtil = require("../../../testUtil");

describe("libjoynr-js.joynr.dispatching.subscription.PublicationManager", () => {
    let callbackDispatcher: any;
    let proxyId: any, providerId: any, publicationManager: PublicationManager, dispatcherSpy: any;
    let provider: any, asyncGetterCallDelay: number, fakeTime: any, intervalSubscriptionRequest: any;
    let onChangeSubscriptionRequest: any, mixedSubscriptionRequest: any, onChangeBroadcastSubscriptionRequest: any;
    let mixedSubscriptionRequestWithAsyncAttribute: any, testAttributeName: string;
    let asyncTestAttributeName: string, value: any, minIntervalMs: number, maxIntervalMs: number, maxNrOfTimes: any;
    let subscriptionLength: any, asyncTestAttribute: any, testAttribute: any, providerSettings: any;
    let testAttributeNotNotifiable: any, testAttributeNotNotifiableName: string;
    let testBroadcastName: string, testBroadcast: any;
    let testNonSelectiveBroadcastName: string, testNonSelectiveBroadcast: any;

    function createSubscriptionRequest(
        isAttribute: any,
        subscribeToName: string,
        periodMs: number | undefined,
        subscriptionLength: number,
        onChange?: boolean,
        minIntervalMs?: number
    ) {
        let qosSettings: any, request: any;
        const expiryDateMs =
            subscriptionLength === SubscriptionQos.NO_EXPIRY_DATE
                ? SubscriptionQos.NO_EXPIRY_DATE
                : Date.now() + subscriptionLength;
        if (onChange) {
            if (periodMs !== undefined) {
                qosSettings = new OnChangeWithKeepAliveSubscriptionQos({
                    minIntervalMs: minIntervalMs || 50,
                    maxIntervalMs: periodMs,
                    expiryDateMs,
                    alertAfterIntervalMs: 0,
                    publicationTtlMs: 1000
                });
            } else {
                qosSettings = new OnChangeSubscriptionQos({
                    minIntervalMs: minIntervalMs || 50,
                    expiryDateMs,
                    publicationTtlMs: 1000
                });
            }
        } else {
            qosSettings = new PeriodicSubscriptionQos({
                periodMs,
                expiryDateMs,
                alertAfterIntervalMs: 0,
                publicationTtlMs: 1000
            });
        }

        if (isAttribute) {
            request = new SubscriptionRequest({
                subscriptionId: `subscriptionId${nanoid()}`,
                subscribedToName: subscribeToName,
                qos: qosSettings
            });
        } else {
            request = new BroadcastSubscriptionRequest({
                subscriptionId: `subscriptionId${nanoid()}`,
                subscribedToName: subscribeToName,
                qos: qosSettings,
                filterParameters: {} as any
            });
        }
        return request;
    }

    async function increaseFakeTime(timeMs: number): Promise<void> {
        fakeTime += timeMs;
        jest.advanceTimersByTime(timeMs);
        await testUtil.multipleSetImmediate();
    }

    function stopSubscription(subscriptionInfo: any) {
        publicationManager.handleSubscriptionStop(
            new SubscriptionStop({
                subscriptionId: subscriptionInfo.subscriptionId
            })
        );
    }

    function handleMulticastSubscriptionRequest() {
        const request = new MulticastSubscriptionRequest({
            subscriptionId: `subscriptionId${nanoid()}`,
            multicastId: SubscriptionUtil.createMulticastId(providerId, testNonSelectiveBroadcastName, []),
            subscribedToName: testNonSelectiveBroadcastName,
            qos: new OnChangeSubscriptionQos()
        });
        publicationManager.handleMulticastSubscriptionRequest(proxyId, providerId, request, callbackDispatcher);
        return request;
    }

    async function prepareTests() {
        callbackDispatcher = jest.fn();
        proxyId = `proxy${nanoid()}`;
        providerId = `provider${nanoid()}`;
        fakeTime = 123456789;
        testAttributeName = "testAttribute";
        testBroadcastName = "testBroadcast";
        testNonSelectiveBroadcastName = "testNonSelectiveBroadcastName";
        testAttributeNotNotifiableName = "testAttributeNotNotifiable";
        asyncTestAttributeName = "asyncTestAttribute";
        value = "the value";
        minIntervalMs = 100;
        maxIntervalMs = 1000;
        maxNrOfTimes = 5;
        asyncGetterCallDelay = 10;
        subscriptionLength = (maxNrOfTimes + 1) * maxIntervalMs;

        dispatcherSpy = {
            sendPublication: jest.fn(),
            sendMulticastPublication: jest.fn()
        };
        publicationManager = new PublicationManager(dispatcherSpy);

        provider = {
            registerOnChangeListener: jest.fn()
        };

        providerSettings = {
            providerQos: new ProviderQos({
                priority: 1234,
                scope: ProviderScope.GLOBAL
            } as any),
            dependencies: {
                publicationManager
            }
        };

        testBroadcast = new ProviderEvent({
            eventName: testBroadcastName,
            outputParameterProperties: [
                {
                    name: "param1",
                    type: "String"
                }
            ],
            selective: true,
            filterSettings: {
                positionOfInterest: "reservedForTypeInfo"
            }
        });

        testNonSelectiveBroadcast = new ProviderEvent({
            eventName: testNonSelectiveBroadcastName,
            outputParameterProperties: [
                {
                    name: "param1",
                    type: "String"
                }
            ],
            selective: false,
            filterSettings: undefined as any
        });

        testAttribute = new ProviderAttribute.ProviderReadWriteNotifyAttribute(
            provider,
            providerSettings,
            testAttributeName,
            "Boolean"
        );

        asyncTestAttribute = new ProviderAttribute.ProviderReadWriteNotifyAttribute(
            provider,
            providerSettings,
            asyncTestAttributeName,
            "Boolean"
        );

        testAttributeNotNotifiable = new ProviderAttribute.ProviderReadWriteAttribute(
            provider,
            providerSettings,
            testAttributeNotNotifiableName,
            "Boolean"
        );

        provider[testAttributeName] = testAttribute;
        provider[testBroadcastName] = testBroadcast;
        provider[testNonSelectiveBroadcastName] = testNonSelectiveBroadcast;
        provider[testAttributeNotNotifiableName] = testAttributeNotNotifiable;
        jest.spyOn(testAttribute, "get").mockImplementation(() => {
            return Promise.resolve("attributeValue");
        });
        jest.spyOn(testAttributeNotNotifiable, "get").mockImplementation(() => {
            return Promise.resolve("attributeValue");
        });

        provider[asyncTestAttributeName] = asyncTestAttribute;
        jest.spyOn(asyncTestAttribute, "get").mockImplementation(() => {
            /* eslint-disable no-unused-vars*/
            return new Promise(resolve => {
                LongTimer.setTimeout(() => {
                    resolve("asyncAttributeValue");
                }, asyncGetterCallDelay);
            });
            /* eslint-enable no-unused-vars*/
        });

        jest.useFakeTimers();
        jest.spyOn(Date, "now").mockImplementation(() => {
            return fakeTime;
        });

        intervalSubscriptionRequest = createSubscriptionRequest(
            true,
            testAttributeName,
            maxIntervalMs,
            subscriptionLength
        );
        onChangeSubscriptionRequest = createSubscriptionRequest(
            true,
            testAttributeName,
            undefined,
            subscriptionLength,
            true
        );
        mixedSubscriptionRequest = createSubscriptionRequest(
            true,
            testAttributeName,
            maxIntervalMs,
            subscriptionLength,
            true,
            minIntervalMs
        );
        onChangeBroadcastSubscriptionRequest = createSubscriptionRequest(
            false,
            testBroadcastName,
            undefined,
            subscriptionLength,
            true
        );
        mixedSubscriptionRequestWithAsyncAttribute = createSubscriptionRequest(
            true,
            asyncTestAttributeName,
            maxIntervalMs,
            subscriptionLength,
            true,
            minIntervalMs
        );
        expect(publicationManager.hasSubscriptions()).toBe(false);
    }

    beforeEach(async () => {
        await prepareTests();
    });

    afterEach(() => {
        jest.useRealTimers();
    });

    it("is instantiable", () => {
        expect(publicationManager).toBeDefined();
        expect(() => {
            // eslint-disable-next-line no-new
            new PublicationManager(dispatcherSpy);
        }).not.toThrow();
    });

    it("calls dispatcher with correct arguments", async () => {
        publicationManager.addPublicationProvider(providerId, provider);
        publicationManager.handleSubscriptionRequest(
            proxyId,
            providerId,
            onChangeSubscriptionRequest,
            callbackDispatcher
        );
        expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeTruthy();
        await increaseFakeTime(1);

        // reset first publication
        testAttribute.get.mockClear();
        dispatcherSpy.sendPublication.mockClear();

        await increaseFakeTime(50);

        testAttribute.valueChanged(value);
        await testUtil.multipleSetImmediate();

        expect(dispatcherSpy.sendPublication).toHaveBeenCalledWith(
            {
                from: providerId,
                to: proxyId,
                expiryDate: Date.now() + onChangeSubscriptionRequest.qos.publicationTtlMs
            },
            SubscriptionPublication.create({
                response: [value],
                subscriptionId: onChangeSubscriptionRequest.subscriptionId
            })
        );
        stopSubscription(onChangeSubscriptionRequest);
        expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeFalsy();
    });

    it("publishes first value once immediately after subscription", async () => {
        publicationManager.addPublicationProvider(providerId, provider);
        expect(testAttribute.get).not.toHaveBeenCalled();
        expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();
        publicationManager.handleSubscriptionRequest(
            proxyId,
            providerId,
            onChangeSubscriptionRequest,
            callbackDispatcher
        );
        expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeTruthy();
        await increaseFakeTime(1);

        expect(testAttribute.get.mock.calls.length).toEqual(1);
        expect(dispatcherSpy.sendPublication.mock.calls.length).toEqual(1);
        // cleanup
        stopSubscription(onChangeSubscriptionRequest);
        expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeFalsy();
    });

    it("creates a working interval subscription", async () => {
        publicationManager.addPublicationProvider(providerId, provider);
        publicationManager.handleSubscriptionRequest(
            proxyId,
            providerId,
            intervalSubscriptionRequest,
            callbackDispatcher
        );
        expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeTruthy();
        await increaseFakeTime(1);

        testAttribute.get.mockClear();
        dispatcherSpy.sendPublication.mockClear();

        for (let times = 1; times < maxNrOfTimes + 1; ++times) {
            // step the clock forward to 1 ms before the interval
            await increaseFakeTime(maxIntervalMs - 1);
            // 1 ms later the poll should happen
            await increaseFakeTime(1);
            expect(testAttribute.get.mock.calls.length).toEqual(times);
            expect(dispatcherSpy.sendPublication.mock.calls.length).toEqual(times);
            expect(dispatcherSpy.sendPublication.mock.calls.length).toEqual(times);
        }

        // cleanup
        stopSubscription(intervalSubscriptionRequest);
        expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeFalsy();
    });

    it("does not publish when interval subscription has an endDate in the past", async () => {
        intervalSubscriptionRequest.qos.expiryDateMs = Date.now() - 1;
        publicationManager.addPublicationProvider(providerId, provider);
        publicationManager.handleSubscriptionRequest(
            proxyId,
            providerId,
            intervalSubscriptionRequest,
            callbackDispatcher
        );

        expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeFalsy();
        // reset first publication
        testAttribute.get.mockClear();
        dispatcherSpy.sendPublication.mockClear();

        expect(testAttribute.get).not.toHaveBeenCalled();
        expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();
        await increaseFakeTime(subscriptionLength);

        expect(testAttribute.get).not.toHaveBeenCalled();
        expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();
        // cleanup
        stopSubscription(intervalSubscriptionRequest);
    });

    it("removes an interval attribute subscription after endDate", async () => {
        publicationManager.addPublicationProvider(providerId, provider);
        publicationManager.handleSubscriptionRequest(
            proxyId,
            providerId,
            intervalSubscriptionRequest,
            callbackDispatcher
        );
        expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeTruthy();
        await increaseFakeTime(1);

        // reset first publication
        testAttribute.get.mockClear();
        dispatcherSpy.sendPublication.mockClear();
        //increaseFakeTime(subscriptionLength);
        //
        // Do not increase by full subscriptionLength, because then
        // both the publication end timer and the timer for the current
        // interval period will become runnable at the same time.
        // Jasmine seems to call the end timer first (which should
        // cancel the other one), however the interval period timer
        // will still be run. Unfortunately it creates a new interval
        // period timer, which will run another publication after the
        // subscription has already expired which lets the test fail.
        // The following shorter increase of time will allow the
        // interval period timer to run first. Also note, that despite
        // the large time warp, only one publication will be
        // visible since Joynr has implemented interval timers by
        // a simple timer that retriggers itself after each interval
        // instead of using the Javascript setInterval().
        await increaseFakeTime(subscriptionLength - 2);

        // The following increase of time will only trigger the end timer
        // while the interval period timer remains waiting. So it
        // can be cleared ok.
        await increaseFakeTime(2);

        expect(testAttribute.get).toHaveBeenCalled();
        expect(dispatcherSpy.sendPublication).toHaveBeenCalled();
        // after another interval, the methods should not have been called again
        // (ie subscription terminated)
        testAttribute.get.mockClear();
        dispatcherSpy.sendPublication.mockClear();
        await increaseFakeTime(maxIntervalMs);
        expect(testAttribute.get).not.toHaveBeenCalled();
        expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();
        stopSubscription(intervalSubscriptionRequest);
        expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeFalsy();
    });

    it("removes an interval attribute subscription after subscription stop", async () => {
        publicationManager.addPublicationProvider(providerId, provider);
        publicationManager.handleSubscriptionRequest(
            proxyId,
            providerId,
            intervalSubscriptionRequest,
            callbackDispatcher
        );

        expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeTruthy();
        // reset first publication
        testAttribute.get.mockClear();
        dispatcherSpy.sendPublication.mockClear();

        await increaseFakeTime(maxIntervalMs + asyncGetterCallDelay);

        expect(testAttribute.get).toHaveBeenCalled();
        expect(dispatcherSpy.sendPublication).toHaveBeenCalled();
        // after subscription stop, the methods should not have been called
        // again (ie subscription terminated)
        publicationManager.handleSubscriptionStop(
            new SubscriptionStop({
                subscriptionId: intervalSubscriptionRequest.subscriptionId
            })
        );
        testAttribute.get.mockClear();
        dispatcherSpy.sendPublication.mockClear();

        await increaseFakeTime(maxIntervalMs + 1);
        expect(testAttribute.get).not.toHaveBeenCalled();
        expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();
    });

    it("creates a working onChange subscription", () => {
        publicationManager.addPublicationProvider(providerId, provider);
        publicationManager.handleSubscriptionRequest(
            proxyId,
            providerId,
            onChangeSubscriptionRequest,
            callbackDispatcher
        );

        expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeTruthy();

        // reset first publication
        testAttribute.get.mockClear();
        dispatcherSpy.sendPublication.mockClear();

        for (let times = 0; times < maxNrOfTimes; times++) {
            increaseFakeTime(50);
            testAttribute.valueChanged(value + times);
            expect(dispatcherSpy.sendPublication.mock.calls.length).toEqual(times + 1);
        }

        // cleanup
        stopSubscription(onChangeSubscriptionRequest);
        expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeFalsy();
    });

    it("does not publish when an onChange subscription has an endDate in the past", async () => {
        onChangeSubscriptionRequest.qos.expiryDateMs = Date.now() - 1;

        publicationManager.addPublicationProvider(providerId, provider);
        publicationManager.handleSubscriptionRequest(
            proxyId,
            providerId,
            onChangeSubscriptionRequest,
            callbackDispatcher
        );
        expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeFalsy();
        // reset first publication
        testAttribute.get.mockClear();
        dispatcherSpy.sendPublication.mockClear();

        testAttribute.valueChanged(`${value}someChange`);

        await increaseFakeTime(asyncGetterCallDelay);

        expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();

        // cleanup
        stopSubscription(onChangeSubscriptionRequest);
        expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeFalsy();
    });

    it("removes an onChange attribute subscription after endDate", async () => {
        publicationManager.addPublicationProvider(providerId, provider);
        publicationManager.handleSubscriptionRequest(
            proxyId,
            providerId,
            onChangeSubscriptionRequest,
            callbackDispatcher
        );
        expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeTruthy();
        await increaseFakeTime(1);

        // reset first publication
        testAttribute.get.mockClear();
        dispatcherSpy.sendPublication.mockClear();

        await increaseFakeTime(50);
        testAttribute.valueChanged(value);

        expect(dispatcherSpy.sendPublication).toHaveBeenCalled();

        // after another attribute change, the methods should not have been
        // called again (ie subscription terminated)
        await increaseFakeTime(subscriptionLength);
        dispatcherSpy.sendPublication.mockClear();

        testAttribute.valueChanged(value);

        expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();
        expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeFalsy();
    });

    it("removes an onChange attribute subscription after subscription stop", async () => {
        publicationManager.addPublicationProvider(providerId, provider);
        publicationManager.handleSubscriptionRequest(
            proxyId,
            providerId,
            onChangeSubscriptionRequest,
            callbackDispatcher
        );
        expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeTruthy();
        await increaseFakeTime(1);

        // reset first publication
        testAttribute.get.mockClear();
        dispatcherSpy.sendPublication.mockClear();

        await increaseFakeTime(50);
        testAttribute.valueChanged(value);

        expect(dispatcherSpy.sendPublication).toHaveBeenCalled();

        // after subscription stop, the methods should not have been called
        // again (ie subscription terminated)
        publicationManager.handleSubscriptionStop(
            new SubscriptionStop({
                subscriptionId: onChangeSubscriptionRequest.subscriptionId
            })
        );
        expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeFalsy();
        dispatcherSpy.sendPublication.mockClear();

        testAttribute.valueChanged(value);
        await increaseFakeTime(1);
        expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();
    });

    it("creates a mixed subscription and does not send two publications within minintervalMs in case async getter calls and valueChanged occur at the same time", async () => {
        publicationManager.addPublicationProvider(providerId, provider);
        publicationManager.handleSubscriptionRequest(
            proxyId,
            providerId,
            mixedSubscriptionRequestWithAsyncAttribute,
            callbackDispatcher
        );
        expect(
            publicationManager.hasSubscriptionsForProviderAttribute(providerId, asyncTestAttributeName)
        ).toBeTruthy();
        // wait until the first publication occurs
        await increaseFakeTime(asyncGetterCallDelay);

        // reset first publication
        asyncTestAttribute.get.mockClear();
        dispatcherSpy.sendPublication.mockClear();

        // let the minIntervalMs exceed, so that new value changes
        // immediately lead to publications
        increaseFakeTime(minIntervalMs);
        expect(asyncTestAttribute.get).not.toHaveBeenCalled();
        expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();

        asyncTestAttribute.valueChanged(value);
        increaseFakeTime(5);

        // this should cause an async timer, which sends a publication
        // after minIntervalMs-5
        asyncTestAttribute.valueChanged(value);

        // the getter has not been invoked so far
        expect(asyncTestAttribute.get).not.toHaveBeenCalled();
        await testUtil.multipleSetImmediate();

        expect(dispatcherSpy.sendPublication.mock.calls.length).toEqual(1);
        // now, lets increas the time until mininterval
        await increaseFakeTime(minIntervalMs - 5);

        // now, the async timer has exceeded, and the PublicationManager
        // invokes the get
        expect(asyncTestAttribute.get.mock.calls.length).toEqual(1);

        // now change the attribute value
        asyncTestAttribute.valueChanged(value);
        await testUtil.multipleSetImmediate();

        expect(dispatcherSpy.sendPublication.mock.calls.length).toEqual(2);
        // lets exceed the async getter delay, causing the async getter
        // to resolve the promise object
        increaseFakeTime(asyncGetterCallDelay);

        // now change the attribute value
        asyncTestAttribute.valueChanged(value);

        // this shall not result in a new sendPublication, as
        // asyncGetterCallDelay<minIntervalMs and the time
        // delay between two publications must be at least minIntervalMs
        expect(dispatcherSpy.sendPublication.mock.calls.length).toEqual(2);
        await increaseFakeTime(minIntervalMs - asyncGetterCallDelay);

        expect(asyncTestAttribute.get.mock.calls.length).toEqual(2);
        await increaseFakeTime(asyncGetterCallDelay);

        expect(asyncTestAttribute.get.mock.calls.length).toEqual(2);
        expect(dispatcherSpy.sendPublication.mock.calls.length).toEqual(3);

        stopSubscription(mixedSubscriptionRequestWithAsyncAttribute);
        expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, asyncTestAttributeName)).toBeFalsy();
    });

    it("creates a mixed subscription that publishes every maxIntervalMs", async () => {
        publicationManager.addPublicationProvider(providerId, provider);
        publicationManager.handleSubscriptionRequest(proxyId, providerId, mixedSubscriptionRequest, callbackDispatcher);
        expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeTruthy();
        await increaseFakeTime(1);

        // reset first publication
        testAttribute.get.mockClear();
        dispatcherSpy.sendPublication.mockClear();
        expect(testAttribute.get).not.toHaveBeenCalled();
        expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();

        for (let times = 0; times < maxNrOfTimes; ++times) {
            // step the clock forward to 1 ms before the interval
            await increaseFakeTime(maxIntervalMs - 1);
            // 1 ms later the poll should happen
            await increaseFakeTime(1);

            expect(testAttribute.get.mock.calls.length).toEqual(times + 1);
            expect(dispatcherSpy.sendPublication.mock.calls.length).toEqual(times + 1);
        }

        // cleanup
        stopSubscription(mixedSubscriptionRequest);
        expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeFalsy();
    });

    it("creates a mixed subscription that publishes valueChanges that occur each minIntervalMs", async () => {
        publicationManager.addPublicationProvider(providerId, provider);
        publicationManager.handleSubscriptionRequest(proxyId, providerId, mixedSubscriptionRequest, callbackDispatcher);
        expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeTruthy();
        await increaseFakeTime(1);

        testAttribute.get.mockClear();
        dispatcherSpy.sendPublication.mockClear();

        for (let times = 0; times < maxNrOfTimes; times++) {
            await increaseFakeTime(mixedSubscriptionRequest.qos.minIntervalMs);
            testAttribute.valueChanged(value + times);

            await testUtil.multipleSetImmediate();
            expect(testAttribute.get).not.toHaveBeenCalled();
            expect(dispatcherSpy.sendPublication.mock.calls.length).toEqual(times + 1);
        }

        // cleanup
        stopSubscription(mixedSubscriptionRequest);
        expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeFalsy();
    });

    it("creates a mixed subscription that publishes valueChanges that occur each maxIntervalMs-1", async () => {
        dispatcherSpy.sendPublication.mockClear();
        publicationManager.addPublicationProvider(providerId, provider);
        publicationManager.handleSubscriptionRequest(proxyId, providerId, mixedSubscriptionRequest, callbackDispatcher);
        expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeTruthy();
        await increaseFakeTime(1);

        testAttribute.get.mockClear();
        dispatcherSpy.sendPublication.mockClear();

        for (let times = 0; times < maxNrOfTimes; times++) {
            await increaseFakeTime(mixedSubscriptionRequest.qos.maxIntervalMs - 2);
            testAttribute.valueChanged(value + times);
            await increaseFakeTime(1);

            expect(testAttribute.get).not.toHaveBeenCalled();
            expect(dispatcherSpy.sendPublication.mock.calls.length).toEqual(times + 1);
        }

        // cleanup
        stopSubscription(mixedSubscriptionRequest);
        expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeFalsy();
    });
    it("creates a mixed subscription that publishes many valueChanges within minIntervalMs only once", async () => {
        publicationManager.addPublicationProvider(providerId, provider);
        publicationManager.handleSubscriptionRequest(proxyId, providerId, mixedSubscriptionRequest, callbackDispatcher);
        expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeTruthy();
        await increaseFakeTime(1);

        // reset first publication
        testAttribute.get.mockClear();
        dispatcherSpy.sendPublication.mockClear();

        const shortInterval = Math.round((mixedSubscriptionRequest.qos.minIntervalMs - 2) / maxNrOfTimes);
        for (let times = 0; times < maxNrOfTimes; times++) {
            expect(testAttribute.get).not.toHaveBeenCalled();
            expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();
            await increaseFakeTime(shortInterval);
            testAttribute.valueChanged(value + times);
        }

        // after minIntervalMs the publication works again
        await increaseFakeTime(mixedSubscriptionRequest.qos.minIntervalMs - shortInterval * maxNrOfTimes);
        testAttribute.valueChanged(value);
        expect(testAttribute.get.mock.calls.length).toEqual(1);
        expect(dispatcherSpy.sendPublication.mock.calls.length).toEqual(1);

        // cleanup
        stopSubscription(mixedSubscriptionRequest);
        expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeFalsy();
    });

    it("creates a periodic subscription without expiryDate and expects periodic publications", async () => {
        const periodMs = 400;
        const n = 10;

        const subscriptionRequestWithoutExpiryDate = createSubscriptionRequest(
            true,
            testAttributeName,
            periodMs,
            0,
            false,
            minIntervalMs
        );
        dispatcherSpy.sendPublication.mockClear();
        publicationManager.addPublicationProvider(providerId, provider);
        publicationManager.handleSubscriptionRequest(
            proxyId,
            providerId,
            subscriptionRequestWithoutExpiryDate,
            callbackDispatcher
        );
        expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeTruthy();
        await increaseFakeTime(1);

        expect(dispatcherSpy.sendPublication.mock.calls.length).toEqual(1);
        dispatcherSpy.sendPublication.mockClear();

        for (let i = 0; i < n; i++) {
            await increaseFakeTime(periodMs);
            expect(dispatcherSpy.sendPublication.mock.calls.length).toEqual(1 + i);
        }

        // cleanup
        stopSubscription(subscriptionRequestWithoutExpiryDate);
        expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeFalsy();
    });
    it("creates a mixed subscription that publishes correctly with all cases mixed", async () => {
        let i: any;

        publicationManager.addPublicationProvider(providerId, provider);
        publicationManager.handleSubscriptionRequest(proxyId, providerId, mixedSubscriptionRequest, callbackDispatcher);
        expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeTruthy();
        await increaseFakeTime(1);

        // reset first publication
        testAttribute.get.mockClear();
        dispatcherSpy.sendPublication.mockClear();

        // at time 0, a value change is not reported because value was
        // already published initially while
        // handling the subscription request
        testAttribute.valueChanged(value);
        expect(testAttribute.get).not.toHaveBeenCalled();
        expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();

        // minIntervalMs and a value change the value should be reported
        await increaseFakeTime(mixedSubscriptionRequest.qos.minIntervalMs);

        // due to minIntervalMs exceeded + valueChanged has been occured
        // within the minIntervalMs, the
        // PublicationManager
        // send the current attribute value to the subscriptions
        expect(testAttribute.get.mock.calls.length).toEqual(1);
        expect(dispatcherSpy.sendPublication.mock.calls.length).toEqual(1);
        testAttribute.get.mockClear();
        dispatcherSpy.sendPublication.mockClear();
        // minIntervalMs and no publication shall occur
        await increaseFakeTime(mixedSubscriptionRequest.qos.minIntervalMs);
        expect(testAttribute.get).not.toHaveBeenCalled();
        expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();

        // change value, and immediate publication shall occur
        testAttribute.valueChanged(value);
        expect(testAttribute.get).not.toHaveBeenCalled();
        expect(dispatcherSpy.sendPublication.mock.calls.length).toEqual(1);

        // at time 1:maxNrOfTimes, a value change is reported => NO
        // publication is sent
        for (i = 0; i < maxNrOfTimes; ++i) {
            dispatcherSpy.sendPublication.mockClear();
            await increaseFakeTime(1);
            testAttribute.valueChanged(value);
            expect(testAttribute.get).not.toHaveBeenCalled();
            expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();
        }

        // at time mixedSubscriptionRequest.qos.minIntervalMs the last
        // value of the test attribute is sent
        // to the subscribers as the publication timeout occurs
        dispatcherSpy.sendPublication.mockClear();
        await increaseFakeTime(mixedSubscriptionRequest.qos.minIntervalMs - maxNrOfTimes);
        expect(testAttribute.get.mock.calls.length).toEqual(1);

        expect(dispatcherSpy.sendPublication.mock.calls.length).toEqual(1);

        // value change after mixedSubscriptionRequest.qos.maxInterval - 2=> publication sent
        dispatcherSpy.sendPublication.mockClear();
        testAttribute.get.mockClear();

        await increaseFakeTime(mixedSubscriptionRequest.qos.maxIntervalMs - 1);
        testAttribute.valueChanged(value);
        expect(testAttribute.get).not.toHaveBeenCalled();

        expect(dispatcherSpy.sendPublication.mock.calls.length).toEqual(1);

        // after mixedSubscriptionRequest.qos.maxIntervalMs => interval
        // publication is sent
        dispatcherSpy.sendPublication.mockClear();
        await increaseFakeTime(mixedSubscriptionRequest.qos.maxIntervalMs);

        expect(testAttribute.get.mock.calls.length).toEqual(1);
        expect(dispatcherSpy.sendPublication.mock.calls.length).toEqual(1);

        // after another mixedSubscriptionRequest.qos.maxIntervalMs =>
        // interval publication is sent
        testAttribute.get.mockClear();
        dispatcherSpy.sendPublication.mockClear();
        await increaseFakeTime(mixedSubscriptionRequest.qos.maxIntervalMs);

        expect(testAttribute.get.mock.calls.length).toEqual(1);
        expect(dispatcherSpy.sendPublication.mock.calls.length).toEqual(1);

        // after subscription stop => NO publications are sent any more
        stopSubscription(mixedSubscriptionRequest);
        expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeFalsy();
        testAttribute.get.mockClear();
        dispatcherSpy.sendPublication.mockClear();
        await increaseFakeTime(mixedSubscriptionRequest.qos.minIntervalMs);
        testAttribute.valueChanged(value);
        expect(testAttribute.get).not.toHaveBeenCalled();
    });
    it("does not publish when mixed subscription has an endDate in the past", async () => {
        mixedSubscriptionRequest.qos.expiryDateMs = Date.now() - 1;

        publicationManager.addPublicationProvider(providerId, provider);
        publicationManager.handleSubscriptionRequest(proxyId, providerId, mixedSubscriptionRequest, callbackDispatcher);
        expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeFalsy();
        // reset first publication
        testAttribute.get.mockClear();
        dispatcherSpy.sendPublication.mockClear();

        testAttribute.valueChanged(value);
        await increaseFakeTime(subscriptionLength);
        expect(testAttribute.get).not.toHaveBeenCalled();
        expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();

        // cleanup
        stopSubscription(mixedSubscriptionRequest);
        expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeFalsy();
    });

    it("removes a mixed attribute subscription after endDate", async () => {
        publicationManager.addPublicationProvider(providerId, provider);
        publicationManager.handleSubscriptionRequest(proxyId, providerId, mixedSubscriptionRequest, callbackDispatcher);
        expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeTruthy();
        await increaseFakeTime(1);

        // reset first publication
        testAttribute.get.mockClear();
        dispatcherSpy.sendPublication.mockClear();

        await increaseFakeTime(subscriptionLength);
        testAttribute.get.mockClear();
        dispatcherSpy.sendPublication.mockClear();

        expect(testAttribute.get).not.toHaveBeenCalled();
        expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();
        testAttribute.valueChanged(value);
        expect(testAttribute.get).not.toHaveBeenCalled();
        expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();
        stopSubscription(mixedSubscriptionRequest);
        expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeFalsy();
    });

    it("removes a mixed broadcast subscription after subscription stop", async () => {
        const broadcastOutputParameters = testBroadcast.createBroadcastOutputParameters();
        broadcastOutputParameters.setParam1("param1");
        publicationManager.addPublicationProvider(providerId, provider);
        publicationManager.handleBroadcastSubscriptionRequest(
            proxyId,
            providerId,
            onChangeBroadcastSubscriptionRequest,
            callbackDispatcher
        );

        expect(publicationManager.hasSubscriptionsForProviderEvent(providerId, testBroadcastName)).toBeTruthy();
        await increaseFakeTime(1);

        testBroadcast.fire(broadcastOutputParameters);
        await increaseFakeTime(maxIntervalMs); // increase interval

        // reset first publication
        dispatcherSpy.sendPublication.mockClear();

        // after subscription stop, the methods should not have been called
        // again (ie subscription
        // terminated)
        publicationManager.handleSubscriptionStop(
            new SubscriptionStop({
                subscriptionId: onChangeBroadcastSubscriptionRequest.subscriptionId
            })
        );

        expect(publicationManager.hasSubscriptionsForProviderEvent(providerId, testBroadcastName)).toBeFalsy();

        await increaseFakeTime(maxIntervalMs); // increase interval
        testBroadcast.fire(broadcastOutputParameters);
        await increaseFakeTime(maxIntervalMs); // increase interval
        await testUtil.multipleSetImmediate();
        expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();
    });

    it("removes publication provider", async () => {
        jest.spyOn(testAttribute, "registerObserver");
        jest.spyOn(testAttribute, "unregisterObserver");
        publicationManager.addPublicationProvider(providerId, provider);
        expect(testAttribute.registerObserver).toHaveBeenCalled();
        jest.spyOn(publicationManager, "handleSubscriptionStop");

        expect(testAttribute.get).not.toHaveBeenCalled();
        expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();

        publicationManager.handleSubscriptionRequest(
            proxyId,
            providerId,
            intervalSubscriptionRequest,
            callbackDispatcher
        );
        expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeTruthy();
        //increase the fake time to ensure proper async processing of the subscription request
        await increaseFakeTime(1);

        expect(testAttribute.get.mock.calls.length).toEqual(1);
        expect(dispatcherSpy.sendPublication.mock.calls.length).toEqual(1);
        expect(testAttribute.unregisterObserver).not.toHaveBeenCalled();
        expect(publicationManager.handleSubscriptionStop).not.toHaveBeenCalled();
        publicationManager.removePublicationProvider(providerId, provider);
        expect(testAttribute.unregisterObserver).toHaveBeenCalled();
        expect(publicationManager.handleSubscriptionStop).toHaveBeenCalledWith(
            new SubscriptionStop({
                subscriptionId: intervalSubscriptionRequest.subscriptionId
            })
        );
        // cleanup
        stopSubscription(intervalSubscriptionRequest);
        expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeFalsy();
    });

    it("behaves correctly when resubscribing to a publication", async () => {
        jest.spyOn(testAttribute, "registerObserver");
        jest.spyOn(testAttribute, "unregisterObserver");
        publicationManager.addPublicationProvider(providerId, provider);
        expect(testAttribute.registerObserver).toHaveBeenCalled();
        jest.spyOn(publicationManager, "handleSubscriptionStop");

        publicationManager.handleSubscriptionRequest(
            proxyId,
            providerId,
            intervalSubscriptionRequest,
            callbackDispatcher
        );
        await testUtil.multipleSetImmediate();

        expect(testAttribute.get.mock.calls.length).toEqual(1);
        expect(dispatcherSpy.sendPublication.mock.calls.length).toEqual(1);
        expect(testAttribute.unregisterObserver).not.toHaveBeenCalled();
        expect(publicationManager.handleSubscriptionStop).not.toHaveBeenCalled();

        publicationManager.removePublicationProvider(providerId, provider);
        expect(testAttribute.unregisterObserver).toHaveBeenCalled();
        expect(publicationManager.handleSubscriptionStop).toHaveBeenCalledWith(
            new SubscriptionStop({
                subscriptionId: intervalSubscriptionRequest.subscriptionId
            })
        );

        expect(publicationManager.hasSubscriptions()).toBeFalsy();

        callbackDispatcher.mockClear();
        await increaseFakeTime(1000);
        publicationManager.addPublicationProvider(providerId, provider);
        publicationManager.handleSubscriptionRequest(
            proxyId,
            providerId,
            intervalSubscriptionRequest,
            callbackDispatcher
        );
        expect(callbackDispatcher.mock.calls.length).toEqual(1);

        // cleanup
        stopSubscription(intervalSubscriptionRequest);
        expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeFalsy();
    });

    it("rejects attribute subscription if expiryDateMs lies in the past", async () => {
        const request = new SubscriptionRequest({
            subscriptionId: `subscriptionId${nanoid()}`,
            subscribedToName: testAttributeName,
            qos: new OnChangeSubscriptionQos({
                expiryDateMs: Date.now() - 10000
            })
        });
        publicationManager.addPublicationProvider(providerId, provider);
        publicationManager.handleSubscriptionRequest(proxyId, providerId, request, callbackDispatcher);
        await testUtil.multipleSetImmediate();

        expect(callbackDispatcher).toHaveBeenCalled();
        const error = callbackDispatcher.mock.calls.slice(-1)[0][1].error;
        expect(error).toBeDefined();
        expect(error).toBeInstanceOf(SubscriptionException);
        expect(error.subscriptionId).toBeDefined();
        expect(error.subscriptionId).toEqual(callbackDispatcher.mock.calls.slice(-1)[0][1].subscriptionId);
        expect(error.detailMessage).toMatch(/lies in the past/);
    });

    it("rejects attribute subscription if attribute does not exist", async () => {
        const request = new SubscriptionRequest({
            subscriptionId: `subscriptionId${nanoid()}`,
            subscribedToName: "nonExistingAttribute",
            qos: new OnChangeSubscriptionQos()
        });
        publicationManager.addPublicationProvider(providerId, provider);
        publicationManager.handleSubscriptionRequest(proxyId, providerId, request, callbackDispatcher);
        await testUtil.multipleSetImmediate();

        expect(callbackDispatcher).toHaveBeenCalled();
        const error = callbackDispatcher.mock.calls.slice(-1)[0][1].error;
        expect(error).toBeDefined();
        expect(error).toBeInstanceOf(SubscriptionException);
        expect(error.subscriptionId).toBeDefined();
        expect(error.subscriptionId).toEqual(callbackDispatcher.mock.calls.slice(-1)[0][1].subscriptionId);
        expect(error.detailMessage).toMatch(/misses attribute/);
    });

    it("rejects attribute subscription if attribute is not notifiable", async () => {
        const request = new SubscriptionRequest({
            subscriptionId: `subscriptionId${nanoid()}`,
            subscribedToName: testAttributeNotNotifiableName,
            qos: new OnChangeSubscriptionQos()
        });
        publicationManager.addPublicationProvider(providerId, provider);
        publicationManager.handleSubscriptionRequest(proxyId, providerId, request, callbackDispatcher);

        await testUtil.multipleSetImmediate();

        expect(callbackDispatcher).toHaveBeenCalled();
        const error = callbackDispatcher.mock.calls.slice(-1)[0][1].error;
        expect(error).toBeDefined();
        expect(error).toBeInstanceOf(SubscriptionException);
        expect(error.subscriptionId).toBeDefined();
        expect(error.subscriptionId).toEqual(callbackDispatcher.mock.calls.slice(-1)[0][1].subscriptionId);
        expect(error.detailMessage).toMatch(/is not notifiable/);
        await increaseFakeTime(1);
    });

    it("rejects attribute subscription if periodMs is too small", async () => {
        const qosSettings = new PeriodicSubscriptionQos({
            expiryDateMs: 0,
            alertAfterIntervalMs: 0,
            publicationTtlMs: 1000
        });
        // forcibly fake it! The constructor throws, if using this directly
        qosSettings.periodMs = PeriodicSubscriptionQos.MIN_PERIOD_MS - 1;

        const request = new SubscriptionRequest({
            subscriptionId: `subscriptionId${nanoid()}`,
            subscribedToName: testAttributeName,
            qos: qosSettings
        });
        publicationManager.addPublicationProvider(providerId, provider);
        publicationManager.handleSubscriptionRequest(proxyId, providerId, request, callbackDispatcher);

        await testUtil.multipleSetImmediate();

        expect(callbackDispatcher).toHaveBeenCalled();
        const error = callbackDispatcher.mock.calls.slice(-1)[0][1].error;
        expect(error).toBeDefined();
        expect(error).toBeInstanceOf(SubscriptionException);
        expect(error.subscriptionId).toBeDefined();
        expect(error.subscriptionId).toEqual(callbackDispatcher.mock.calls.slice(-1)[0][1].subscriptionId);
        expect(error.detailMessage).toMatch(/is smaller than PeriodicSubscriptionQos/);
    });

    it("rejects broadcast subscription if expiryDateMs lies in the past", async () => {
        const request = new BroadcastSubscriptionRequest({
            subscriptionId: `subscriptionId${nanoid()}`,
            subscribedToName: testBroadcastName,
            qos: new OnChangeSubscriptionQos({
                expiryDateMs: Date.now() - 10000
            }),
            filterParameters: {} as any
        });
        publicationManager.addPublicationProvider(providerId, provider);
        publicationManager.handleBroadcastSubscriptionRequest(proxyId, providerId, request, callbackDispatcher);
        await testUtil.multipleSetImmediate();

        expect(callbackDispatcher).toHaveBeenCalled();
        const error = callbackDispatcher.mock.calls.slice(-1)[0][1].error;
        expect(error).toBeDefined();
        expect(error).toBeInstanceOf(SubscriptionException);
        expect(error.subscriptionId).toBeDefined();
        expect(error.subscriptionId).toEqual(callbackDispatcher.mock.calls.slice(-1)[0][1].subscriptionId);
        expect(error.detailMessage).toMatch(/lies in the past/);
    });

    it("rejects broadcast subscription if filter parameters are wrong", async () => {
        const request = new BroadcastSubscriptionRequest({
            subscriptionId: `subscriptionId${nanoid()}`,
            subscribedToName: testBroadcastName,
            qos: new OnChangeSubscriptionQos(),
            filterParameters: {
                filterParameters: {
                    corruptFilterParameter: "value"
                }
            } as any
        });
        publicationManager.addPublicationProvider(providerId, provider);
        publicationManager.handleBroadcastSubscriptionRequest(proxyId, providerId, request, callbackDispatcher);

        await testUtil.multipleSetImmediate();

        expect(callbackDispatcher).toHaveBeenCalled();
        const error = callbackDispatcher.mock.calls.slice(-1)[0][1].error;
        expect(error).toBeDefined();
        expect(error).toBeInstanceOf(SubscriptionException);
        expect(error.subscriptionId).toBeDefined();
        expect(error.subscriptionId).toEqual(callbackDispatcher.mock.calls.slice(-1)[0][1].subscriptionId);
        expect(error.detailMessage).toMatch(/Filter parameter positionOfInterest for broadcast/);
    });

    it("registers multicast subscription", async () => {
        expect(publicationManager.hasMulticastSubscriptions()).toBe(false);
        publicationManager.addPublicationProvider(providerId, provider);

        const request = handleMulticastSubscriptionRequest();

        await testUtil.multipleSetImmediate();

        expect(callbackDispatcher).toHaveBeenCalled();
        const response = callbackDispatcher.mock.calls.slice(-1)[0][1];
        expect(response.error).toBeUndefined();
        expect(response.subscriptionId).toEqual(request.subscriptionId);
        expect(publicationManager.hasMulticastSubscriptions()).toBe(true);
    });

    it("registers and unregisters multicast subscription", () => {
        expect(publicationManager.hasMulticastSubscriptions()).toBe(false);
        publicationManager.addPublicationProvider(providerId, provider);

        const request = handleMulticastSubscriptionRequest();
        expect(publicationManager.hasMulticastSubscriptions()).toBe(true);

        expect(publicationManager.hasSubscriptions()).toBe(true);
        publicationManager.handleSubscriptionStop({
            subscriptionId: request.subscriptionId
        } as any);
        expect(publicationManager.hasMulticastSubscriptions()).toBe(false);
        expect(publicationManager.hasSubscriptions()).toBe(false);
    });

    it("registers for multicast subscription and sends multicast publication", () => {
        const broadcastOutputParameters = testNonSelectiveBroadcast.createBroadcastOutputParameters();
        broadcastOutputParameters.setParam1("param1");

        expect(publicationManager.hasMulticastSubscriptions()).toBe(false);
        publicationManager.addPublicationProvider(providerId, provider);

        const request = handleMulticastSubscriptionRequest();
        expect(publicationManager.hasMulticastSubscriptions()).toBe(true);

        testNonSelectiveBroadcast.fire(broadcastOutputParameters);

        expect(dispatcherSpy.sendMulticastPublication).toHaveBeenCalled();

        const multicastPublication = dispatcherSpy.sendMulticastPublication.mock.calls[0][1];

        expect(multicastPublication.multicastId).toEqual(request.multicastId);

        publicationManager.handleSubscriptionStop({
            subscriptionId: request.subscriptionId
        } as any);
        expect(publicationManager.hasMulticastSubscriptions()).toBe(false);
    });

    it("rejects broadcast subscription if broadcast does not exist", async () => {
        const request = new BroadcastSubscriptionRequest({
            subscriptionId: `subscriptionId${nanoid()}`,
            subscribedToName: "nonExistingBroadcast",
            qos: new OnChangeSubscriptionQos(),
            filterParameters: {} as any
        });
        publicationManager.addPublicationProvider(providerId, provider);
        publicationManager.handleBroadcastSubscriptionRequest(proxyId, providerId, request, callbackDispatcher);

        await testUtil.multipleSetImmediate();

        expect(callbackDispatcher).toHaveBeenCalled();
        const error = callbackDispatcher.mock.calls.slice(-1)[0][1].error;
        expect(error).toBeDefined();
        expect(error).toBeInstanceOf(SubscriptionException);
        expect(error.subscriptionId).toBeDefined();
        expect(error.subscriptionId).toEqual(callbackDispatcher.mock.calls.slice(-1)[0][1].subscriptionId);
        expect(error.detailMessage).toMatch(/misses event/);
    });

    it(" throws exception when called while shut down", async () => {
        publicationManager.shutdown();
        const callbackDispatcherSpy = jest.fn();

        expect(() => {
            publicationManager.removePublicationProvider("providerParticipantId", {});
        }).toThrow();

        expect(() => {
            publicationManager.addPublicationProvider("providerParticipantId", {});
        }).toThrow();

        publicationManager.handleSubscriptionRequest(
            "proxyParticipantId",
            "providerParticipantId",
            {
                subscriptionId: "subscriptionId"
            } as any,
            callbackDispatcherSpy
        );
        await increaseFakeTime(1);

        expect(callbackDispatcherSpy).toHaveBeenCalled();
        expect(callbackDispatcherSpy.mock.calls[0][1]).toBeInstanceOf(SubscriptionReply);
        expect(callbackDispatcherSpy.mock.calls[0][1].error).toBeInstanceOf(SubscriptionException);
        callbackDispatcherSpy.mockClear();
        publicationManager.handleBroadcastSubscriptionRequest(
            "proxyParticipantId",
            "providerParticipantId",
            {
                subscriptionId: "subscriptionId"
            } as any,
            callbackDispatcherSpy
        );
        await increaseFakeTime(1);
        expect(callbackDispatcherSpy).toHaveBeenCalled();
        expect(callbackDispatcherSpy.mock.calls[0][1]).toBeInstanceOf(SubscriptionReply);
        expect(callbackDispatcherSpy.mock.calls[0][1].error).toBeInstanceOf(SubscriptionException);
    });
});
