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
require("../../../node-unit-test-helper");
const Promise = require("../../../../../main/js/global/Promise");
const PublicationManager = require("../../../../../main/js/joynr/dispatching/subscription/PublicationManager");
const SubscriptionReply = require("../../../../../main/js/joynr/dispatching/types/SubscriptionReply");
const SubscriptionRequest = require("../../../../../main/js/joynr/dispatching/types/SubscriptionRequest");
const BroadcastSubscriptionRequest = require("../../../../../main/js/joynr/dispatching/types/BroadcastSubscriptionRequest");
const MulticastSubscriptionRequest = require("../../../../../main/js/joynr/dispatching/types/MulticastSubscriptionRequest");
const SubscriptionStop = require("../../../../../main/js/joynr/dispatching/types/SubscriptionStop");
const ProviderAttribute = require("../../../../../main/js/joynr/provider/ProviderAttribute");
const ProviderEvent = require("../../../../../main/js/joynr/provider/ProviderEvent");
const PeriodicSubscriptionQos = require("../../../../../main/js/joynr/proxy/PeriodicSubscriptionQos");
const SubscriptionQos = require("../../../../../main/js/joynr/proxy/SubscriptionQos");
const OnChangeSubscriptionQos = require("../../../../../main/js/joynr/proxy/OnChangeSubscriptionQos");
const OnChangeWithKeepAliveSubscriptionQos = require("../../../../../main/js/joynr/proxy/OnChangeWithKeepAliveSubscriptionQos");
const ProviderQos = require("../../../../../main/js/generated/joynr/types/ProviderQos");
const ProviderScope = require("../../../../../main/js/generated/joynr/types/ProviderScope");
const SubscriptionPublication = require("../../../../../main/js/joynr/dispatching/types/SubscriptionPublication");
const SubscriptionUtil = require("../../../../../main/js/joynr/dispatching/subscription/util/SubscriptionUtil");
const SubscriptionException = require("../../../../../main/js/joynr/exceptions/SubscriptionException");
const LongTimer = require("../../../../../main/js/joynr/util/LongTimer");
const uuid = require("uuid/v4");
const Date = require("../../../../../test/js/global/Date");
const waitsFor = require("../../../../../test/js/global/WaitsFor");
const LocalStorage = require("../../../../../test/js/global/LocalStorageNodeTests");
const originalSetTimeout = setTimeout;
describe("libjoynr-js.joynr.dispatching.subscription.PublicationManager", () => {
    let callbackDispatcher;
    let proxyId, providerId, publicationManager, joynrInstanceId, dispatcherSpy;
    let provider, asyncGetterCallDelay, fakeTime, intervalSubscriptionRequest;
    let onChangeSubscriptionRequest, mixedSubscriptionRequest, onChangeBroadcastSubscriptionRequest;
    let mixedSubscriptionRequestWithAsyncAttribute, testAttributeName;
    let asyncTestAttributeName, value, minIntervalMs, maxIntervalMs, maxNrOfTimes;
    let subscriptionLength, asyncTestAttribute, testAttribute, providerSettings;
    let testAttributeNotNotifiable, testAttributeNotNotifiableName;
    let testBroadcastName, testBroadcast;
    let testNonSelectiveBroadcastName, testNonSelectiveBroadcast;
    let persistency; // localStorage was renamed to persistency because it's impossible to reassign it because of jslint

    function createSubscriptionRequest(
        isAttribute,
        subscribeToName,
        periodMs,
        subscriptionLength,
        onChange,
        minIntervalMs
    ) {
        let qosSettings, request;
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
                subscriptionId: "subscriptionId" + uuid(),
                subscribedToName: subscribeToName,
                qos: qosSettings
            });
        } else {
            request = new BroadcastSubscriptionRequest({
                subscriptionId: "subscriptionId" + uuid(),
                subscribedToName: subscribeToName,
                qos: qosSettings,
                filterParameters: {}
            });
        }
        return request;
    }

    function increaseFakeTime(time_ms) {
        fakeTime += time_ms;
        jasmine.clock().tick(time_ms);
    }

    function stopSubscription(subscriptionInfo) {
        publicationManager.handleSubscriptionStop(
            new SubscriptionStop({
                subscriptionId: subscriptionInfo.subscriptionId
            })
        );
    }

    function handleMulticastSubscriptionRequest() {
        const request = new MulticastSubscriptionRequest({
            subscriptionId: "subscriptionId" + uuid(),
            multicastId: SubscriptionUtil.createMulticastId(providerId, testNonSelectiveBroadcastName, []),
            subscribedToName: testNonSelectiveBroadcastName,
            qos: new OnChangeSubscriptionQos()
        });
        publicationManager.handleMulticastSubscriptionRequest(proxyId, providerId, request, callbackDispatcher);
        return request;
    }

    function prepareTests(done) {
        callbackDispatcher = jasmine.createSpy("callbackDispatcher");
        proxyId = "proxy" + uuid();
        providerId = "provider" + uuid();
        joynrInstanceId = uuid();
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

        dispatcherSpy = jasmine.createSpyObj("Dispatcher", ["sendPublication", "sendMulticastPublication"]);
        publicationManager = new PublicationManager(dispatcherSpy, persistency, joynrInstanceId);

        provider = jasmine.createSpyObj("Provider", ["registerOnChangeListener"]);

        providerSettings = {
            providerQos: new ProviderQos({
                version: 123,
                priority: 1234,
                scope: ProviderScope.GLOBAL
            }),
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
            selective: false
        });

        testAttribute = new ProviderAttribute(
            provider,
            providerSettings,
            testAttributeName,
            "Boolean",
            "NOTIFYREADWRITE"
        );

        asyncTestAttribute = new ProviderAttribute(
            provider,
            providerSettings,
            asyncTestAttributeName,
            "Boolean",
            "NOTIFYREADWRITE"
        );

        testAttributeNotNotifiable = new ProviderAttribute(
            provider,
            providerSettings,
            testAttributeNotNotifiableName,
            "Boolean",
            "READWRITE"
        );

        provider[testAttributeName] = testAttribute;
        provider[testBroadcastName] = testBroadcast;
        provider[testNonSelectiveBroadcastName] = testNonSelectiveBroadcast;
        provider[testAttributeNotNotifiableName] = testAttributeNotNotifiable;
        spyOn(testAttribute, "get").and.callFake(() => {
            return Promise.resolve("attributeValue");
        });
        spyOn(testAttributeNotNotifiable, "get").and.callFake(() => {
            return Promise.resolve("attributeValue");
        });

        provider[asyncTestAttributeName] = asyncTestAttribute;
        spyOn(asyncTestAttribute, "get").and.callFake(() => {
            /* eslint-disable no-unused-vars*/
            return new Promise((resolve, reject) => {
                LongTimer.setTimeout(() => {
                    resolve("asyncAttributeValue");
                }, asyncGetterCallDelay);
            });
            /* eslint-enable no-unused-vars*/
        });

        jasmine.clock().install();
        spyOn(Date, "now").and.callFake(() => {
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
        done();
    }

    describe("without localStorage", () => {
        sharedTests(() => {
            persistency = null;
            return Promise.resolve();
        });
    });

    describe("with localStorage", () => {
        sharedTests(() => {
            persistency = new LocalStorage();
            return persistency.init();
        });
    });

    function sharedTests(beforeTests) {
        beforeEach(done => {
            beforeTests().then(() => prepareTests(done));
        });

        afterEach(done => {
            jasmine.clock().uninstall();
            done();
        });

        it("is instantiable", done => {
            expect(publicationManager).toBeDefined();
            expect(() => {
                /*eslint-disable*/
                const p = new PublicationManager(dispatcherSpy, persistency);
                /*eslint-enable*/
            }).not.toThrow();
            done();
        });

        it("calls dispatcher with correct arguments", done => {
            publicationManager.addPublicationProvider(providerId, provider);
            publicationManager.handleSubscriptionRequest(
                proxyId,
                providerId,
                onChangeSubscriptionRequest,
                callbackDispatcher
            );
            expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeTruthy();
            increaseFakeTime(1);

            waitsFor(
                () => {
                    return dispatcherSpy.sendPublication.calls.count() > 0;
                },
                "dispatcherSpy.sendPublication",
                asyncGetterCallDelay
            )
                .then(() => {
                    // reset first publication
                    testAttribute.get.calls.reset();
                    dispatcherSpy.sendPublication.calls.reset();

                    increaseFakeTime(50);

                    testAttribute.valueChanged(value);

                    return waitsFor(
                        () => {
                            return dispatcherSpy.sendPublication.calls.count() > 0;
                        },
                        "dispatcherSpy.sendPublication",
                        asyncGetterCallDelay
                    );
                })
                .then(() => {
                    expect(dispatcherSpy.sendPublication).toHaveBeenCalled();
                    expect(dispatcherSpy.sendPublication).toHaveBeenCalledWith(
                        {
                            from: providerId,
                            to: proxyId,
                            expiryDate: Date.now() + onChangeSubscriptionRequest.qos.publicationTtlMs
                        },
                        new SubscriptionPublication({
                            response: [value],
                            subscriptionId: onChangeSubscriptionRequest.subscriptionId
                        })
                    );
                    stopSubscription(onChangeSubscriptionRequest);
                    expect(
                        publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)
                    ).toBeFalsy();
                    done();
                    return null;
                })
                .catch(fail);
        });

        it("publishes first value once immediately after subscription", done => {
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
            increaseFakeTime(1);

            waitsFor(
                () => {
                    return dispatcherSpy.sendPublication.calls.count() > 0;
                },
                "dispatcherSpy.sendPublication",
                asyncGetterCallDelay
            )
                .then(() => {
                    expect(testAttribute.get.calls.count()).toEqual(1);
                    expect(dispatcherSpy.sendPublication.calls.count()).toEqual(1);
                    // cleanup
                    stopSubscription(onChangeSubscriptionRequest);
                    expect(
                        publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)
                    ).toBeFalsy();
                    done();
                    return null;
                })
                .catch(fail);
        });

        it("creates a working interval subscription", done => {
            let times;

            publicationManager.addPublicationProvider(providerId, provider);
            publicationManager.handleSubscriptionRequest(
                proxyId,
                providerId,
                intervalSubscriptionRequest,
                callbackDispatcher
            );
            expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeTruthy();
            increaseFakeTime(1);

            let promiseChain = waitsFor(() => {
                return dispatcherSpy.sendPublication.calls.count() > 0;
            }, asyncGetterCallDelay).then(() => {
                // reset first publication
                testAttribute.get.calls.reset();
                dispatcherSpy.sendPublication.calls.reset();
            });

            const internalCheck = function(times, promiseChain) {
                return promiseChain.then(() => {
                    // step the clock forward to 1 ms before the interval
                    increaseFakeTime(maxIntervalMs - 1);

                    // 1 ms later the poll should happen
                    increaseFakeTime(1);

                    return waitsFor(
                        () => {
                            return dispatcherSpy.sendPublication.calls.count() === times;
                        },
                        "dispatcherSpy.sendPublication " + times + " times",
                        asyncGetterCallDelay
                    ).then(() => {
                        expect(testAttribute.get.calls.count()).toEqual(times);
                        expect(dispatcherSpy.sendPublication.calls.count()).toEqual(times);
                        expect(dispatcherSpy.sendPublication.calls.count()).toEqual(times);
                    });
                });
            };

            for (times = 1; times < maxNrOfTimes + 1; ++times) {
                promiseChain = internalCheck(times, promiseChain);
            }

            promiseChain
                .then(() => {
                    // cleanup
                    stopSubscription(intervalSubscriptionRequest);
                    expect(
                        publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)
                    ).toBeFalsy();
                    done();
                    return null;
                })
                .catch(fail);
        });

        if (persistency) {
            it("restores a persisted subscription request correctly", done => {
                publicationManager.addPublicationProvider(providerId, provider);
                publicationManager.handleSubscriptionRequest(
                    proxyId,
                    providerId,
                    intervalSubscriptionRequest,
                    callbackDispatcher
                );
                //now, a valid subscription should be correctly persisted -> let's restore

                const publicationManagerWithRestore = new PublicationManager(
                    dispatcherSpy,
                    persistency,
                    joynrInstanceId
                );
                publicationManagerWithRestore.addPublicationProvider(providerId, provider);
                publicationManagerWithRestore.restore(callbackDispatcher);
                // increasing the time by one tick ensures all async callbacks within the
                // publication manager are invoked
                increaseFakeTime(1);
                expect(callbackDispatcher).toHaveBeenCalled();
                expect(callbackDispatcher.calls.mostRecent().args[1].subscriptionId).toBe(
                    intervalSubscriptionRequest.subscriptionId
                );
                expect(callbackDispatcher.calls.mostRecent().args[1].error).toBeUndefined();
                done();
            });

            it("restores a persisted event subscription request correctly", done => {
                publicationManager.addPublicationProvider(providerId, provider);
                publicationManager.handleBroadcastSubscriptionRequest(
                    proxyId,
                    providerId,
                    onChangeBroadcastSubscriptionRequest,
                    callbackDispatcher
                );

                //now, a valid subscription should be correctly persisted -> let's restore

                const publicationManagerWithRestore = new PublicationManager(
                    dispatcherSpy,
                    persistency,
                    joynrInstanceId
                );
                publicationManagerWithRestore.addPublicationProvider(providerId, provider);
                publicationManagerWithRestore.restore(callbackDispatcher);
                // increasing the time by one tick ensures all async callbacks within the
                // publication manager are invoked
                increaseFakeTime(1);
                expect(callbackDispatcher).toHaveBeenCalled();
                expect(callbackDispatcher.calls.mostRecent().args[1].subscriptionId).toBe(
                    onChangeBroadcastSubscriptionRequest.subscriptionId
                );
                expect(callbackDispatcher.calls.mostRecent().args[1].error).toBeUndefined();
                done();
            });

            it("restores a persisted multicast subscription request correctly", done => {
                publicationManager.addPublicationProvider(providerId, provider);
                const request = handleMulticastSubscriptionRequest();

                //now, a valid subscription should be correctly persisted -> let's restore

                const publicationManagerWithRestore = new PublicationManager(
                    dispatcherSpy,
                    persistency,
                    joynrInstanceId
                );
                publicationManagerWithRestore.addPublicationProvider(providerId, provider);
                publicationManagerWithRestore.restore(callbackDispatcher);
                // increasing the time by one tick ensures all async callbacks within the
                // publication manager are invoked
                increaseFakeTime(1);
                expect(callbackDispatcher).toHaveBeenCalled();
                expect(callbackDispatcher.calls.mostRecent().args[1].subscriptionId).toBe(request.subscriptionId);
                expect(callbackDispatcher.calls.mostRecent().args[1].error).toBeUndefined();
                done();
            });
        }

        it("does not publish when interval subscription has an endDate in the past", done => {
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
            testAttribute.get.calls.reset();
            dispatcherSpy.sendPublication.calls.reset();

            expect(testAttribute.get).not.toHaveBeenCalled();
            expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();
            increaseFakeTime(subscriptionLength);

            originalSetTimeout(() => {
                expect(testAttribute.get).not.toHaveBeenCalled();
                expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();
                // cleanup
                stopSubscription(intervalSubscriptionRequest);
                done();
            }, asyncGetterCallDelay);
        });

        it("removes an interval attribute subscription after endDate", done => {
            publicationManager.addPublicationProvider(providerId, provider);
            publicationManager.handleSubscriptionRequest(
                proxyId,
                providerId,
                intervalSubscriptionRequest,
                callbackDispatcher
            );
            expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeTruthy();
            increaseFakeTime(1);

            waitsFor(
                () => {
                    return dispatcherSpy.sendPublication.calls.count() > 0;
                },
                "dispatcherSpy.sendPublication",
                asyncGetterCallDelay
            )
                .then(() => {
                    // reset first publication
                    testAttribute.get.calls.reset();
                    dispatcherSpy.sendPublication.calls.reset();
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
                    increaseFakeTime(subscriptionLength - 2);
                    return waitsFor(
                        () => {
                            return dispatcherSpy.sendPublication.calls.count() > 0;
                        },
                        "dispatcherSpy.sendPublication",
                        asyncGetterCallDelay
                    );
                })
                .then(() => {
                    // The following increase of time will only trigger the end timer
                    // while the interval period timer remains waiting. So it
                    // can be cleared ok.
                    increaseFakeTime(2);

                    expect(testAttribute.get).toHaveBeenCalled();
                    expect(dispatcherSpy.sendPublication).toHaveBeenCalled();
                    // after another interval, the methods should not have been called again
                    // (ie subscription terminated)
                    testAttribute.get.calls.reset();
                    dispatcherSpy.sendPublication.calls.reset();
                    increaseFakeTime(maxIntervalMs);
                    originalSetTimeout(() => {
                        expect(testAttribute.get).not.toHaveBeenCalled();
                        expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();
                        stopSubscription(intervalSubscriptionRequest);
                        expect(
                            publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)
                        ).toBeFalsy();
                        done();
                    }, asyncGetterCallDelay);
                    return null;
                })
                .catch(fail);
        });

        it("removes an interval attribute subscription after subscription stop", done => {
            publicationManager.addPublicationProvider(providerId, provider);
            publicationManager.handleSubscriptionRequest(
                proxyId,
                providerId,
                intervalSubscriptionRequest,
                callbackDispatcher
            );

            expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeTruthy();
            // reset first publication
            testAttribute.get.calls.reset();
            dispatcherSpy.sendPublication.calls.reset();

            increaseFakeTime(maxIntervalMs + asyncGetterCallDelay);

            waitsFor(
                () => {
                    return dispatcherSpy.sendPublication.calls.count() > 0;
                },
                "timeout dispatcherSpy.sendPublication",
                asyncGetterCallDelay
            )
                .then(() => {
                    expect(testAttribute.get).toHaveBeenCalled();
                    expect(dispatcherSpy.sendPublication).toHaveBeenCalled();
                    // after subscription stop, the methods should not have been called
                    // again (ie subscription terminated)
                    publicationManager.handleSubscriptionStop(
                        new SubscriptionStop({
                            subscriptionId: intervalSubscriptionRequest.subscriptionId
                        })
                    );
                    testAttribute.get.calls.reset();
                    dispatcherSpy.sendPublication.calls.reset();

                    increaseFakeTime(maxIntervalMs + 1);
                    expect(testAttribute.get).not.toHaveBeenCalled();
                    expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();
                    done();
                    return null;
                })
                .catch(fail);
        });

        it("creates a working onChange subscription", done => {
            publicationManager.addPublicationProvider(providerId, provider);
            publicationManager.handleSubscriptionRequest(
                proxyId,
                providerId,
                onChangeSubscriptionRequest,
                callbackDispatcher
            );

            expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeTruthy();

            // reset first publication
            testAttribute.get.calls.reset();
            dispatcherSpy.sendPublication.calls.reset();

            for (let times = 0; times < maxNrOfTimes; times++) {
                increaseFakeTime(50);
                testAttribute.valueChanged(value + times);
                expect(dispatcherSpy.sendPublication.calls.count()).toEqual(times + 1);
            }

            // cleanup
            stopSubscription(onChangeSubscriptionRequest);
            expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeFalsy();
            done();
        });

        it("does not publish when an onChange subscription has an endDate in the past", done => {
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
            testAttribute.get.calls.reset();
            dispatcherSpy.sendPublication.calls.reset();

            testAttribute.valueChanged(value + "someChange");

            increaseFakeTime(asyncGetterCallDelay);

            originalSetTimeout(() => {
                expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();

                // cleanup
                stopSubscription(onChangeSubscriptionRequest);
                expect(
                    publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)
                ).toBeFalsy();
                done();
            }, asyncGetterCallDelay);
        });

        it("removes an onChange attribute subscription after endDate", done => {
            publicationManager.addPublicationProvider(providerId, provider);
            publicationManager.handleSubscriptionRequest(
                proxyId,
                providerId,
                onChangeSubscriptionRequest,
                callbackDispatcher
            );
            expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeTruthy();
            increaseFakeTime(1);

            waitsFor(
                () => {
                    return dispatcherSpy.sendPublication.calls.count() > 0;
                },
                "timeout dispatcherSpy.sendPublication",
                10
            )
                .then(() => {
                    // reset first publication
                    testAttribute.get.calls.reset();
                    dispatcherSpy.sendPublication.calls.reset();

                    increaseFakeTime(50);
                    testAttribute.valueChanged(value);
                    return waitsFor(
                        () => {
                            return dispatcherSpy.sendPublication.calls.count() > 0;
                        },
                        "timeout dispatcherSpy.sendPublication after valueChanged",
                        10
                    );
                })
                .then(() => {
                    expect(dispatcherSpy.sendPublication).toHaveBeenCalled();

                    // after another attribute change, the methods should not have been
                    // called again (ie subscription terminated)
                    increaseFakeTime(subscriptionLength);
                    dispatcherSpy.sendPublication.calls.reset();

                    testAttribute.valueChanged(value);

                    originalSetTimeout(() => {
                        expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();
                        expect(
                            publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)
                        ).toBeFalsy();
                        done();
                    }, asyncGetterCallDelay);
                    return null;
                })
                .catch(fail);
        });

        it("removes an onChange attribute subscription after subscription stop", done => {
            publicationManager.addPublicationProvider(providerId, provider);
            publicationManager.handleSubscriptionRequest(
                proxyId,
                providerId,
                onChangeSubscriptionRequest,
                callbackDispatcher
            );
            expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeTruthy();
            increaseFakeTime(1);

            waitsFor(
                () => {
                    return dispatcherSpy.sendPublication.calls.count() > 0;
                },
                "timeout dispatcherSpy.sendPublication",
                10
            )
                .then(() => {
                    // reset first publication
                    testAttribute.get.calls.reset();
                    dispatcherSpy.sendPublication.calls.reset();

                    increaseFakeTime(50);
                    testAttribute.valueChanged(value);
                    return waitsFor(
                        () => {
                            return dispatcherSpy.sendPublication.calls.count() > 0;
                        },
                        "timeout dispatcherSpy.sendPublication",
                        10
                    );
                })
                .then(() => {
                    expect(dispatcherSpy.sendPublication).toHaveBeenCalled();

                    // after subscription stop, the methods should not have been called
                    // again (ie subscription terminated)
                    publicationManager.handleSubscriptionStop(
                        new SubscriptionStop({
                            subscriptionId: onChangeSubscriptionRequest.subscriptionId
                        })
                    );
                    expect(
                        publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)
                    ).toBeFalsy();
                    dispatcherSpy.sendPublication.calls.reset();

                    testAttribute.valueChanged(value);
                    increaseFakeTime(1);
                    originalSetTimeout(() => {
                        expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();
                        done();
                    }, asyncGetterCallDelay);
                    return null;
                })
                .catch(fail);
        });

        it("creates a mixed subscription and does not send two publications within minintervalMs in case async getter calls and valueChanged occur at the same time", done => {
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
            increaseFakeTime(asyncGetterCallDelay);

            return Promise.resolve()
                .then(() => {
                    // reset first publication
                    asyncTestAttribute.get.calls.reset();
                    dispatcherSpy.sendPublication.calls.reset();

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

                    return waitsFor(
                        () => {
                            return dispatcherSpy.sendPublication.calls.count() === 1;
                        },
                        "timeout dispatcherSpy.sendPublication",
                        asyncGetterCallDelay
                    );
                })
                .then(() => {
                    expect(dispatcherSpy.sendPublication.calls.count()).toEqual(1);
                    // now, lets increas the time until mininterval
                    increaseFakeTime(minIntervalMs - 5);
                })
                .then(() => {
                    // now, the async timer has exceeded, and the PublicationManager
                    // invokes the get
                    expect(asyncTestAttribute.get.calls.count()).toEqual(1);

                    // now change the attribute value
                    asyncTestAttribute.valueChanged(value);
                })
                .then(() => {
                    return waitsFor(
                        () => {
                            return dispatcherSpy.sendPublication.calls.count() === 2;
                        },
                        "timeout dispatcherSpy.sendPublication",
                        asyncGetterCallDelay
                    );
                })
                .then(() => {
                    expect(dispatcherSpy.sendPublication.calls.count()).toEqual(2);
                    // lets exceed the async getter delay, causing the async getter
                    // to resolve the promise object
                    increaseFakeTime(asyncGetterCallDelay);

                    // now change the attribute value
                    asyncTestAttribute.valueChanged(value);

                    // this shall not result in a new sendPublication, as
                    // asyncGetterCallDelay<minIntervalMs and the time
                    // delay between two publications must be at least minIntervalMs
                    expect(dispatcherSpy.sendPublication.calls.count()).toEqual(2);

                    increaseFakeTime(minIntervalMs - asyncGetterCallDelay);
                    return waitsFor(
                        () => {
                            return asyncTestAttribute.get.calls.count() === 2;
                        },
                        "timeout asyncTestAttribute.get",
                        asyncGetterCallDelay
                    );
                })
                .then(() => {
                    expect(asyncTestAttribute.get.calls.count()).toEqual(2);

                    increaseFakeTime(asyncGetterCallDelay);
                    return waitsFor(
                        () => {
                            return dispatcherSpy.sendPublication.calls.count() === 3;
                        },
                        "timeout dispatcherSpy.sendPublication",
                        asyncGetterCallDelay
                    );
                })
                .then(() => {
                    expect(asyncTestAttribute.get.calls.count()).toEqual(2);
                    expect(dispatcherSpy.sendPublication.calls.count()).toEqual(3);

                    stopSubscription(mixedSubscriptionRequestWithAsyncAttribute);
                    expect(
                        publicationManager.hasSubscriptionsForProviderAttribute(providerId, asyncTestAttributeName)
                    ).toBeFalsy();
                    done();
                    return null;
                })
                .catch(fail);
        });

        it("creates a mixed subscription that publishes every maxIntervalMs", done => {
            let times;

            publicationManager.addPublicationProvider(providerId, provider);
            publicationManager.handleSubscriptionRequest(
                proxyId,
                providerId,
                mixedSubscriptionRequest,
                callbackDispatcher
            );
            expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeTruthy();
            increaseFakeTime(1);

            let promiseChain = waitsFor(
                () => {
                    return dispatcherSpy.sendPublication.calls.count() > 0;
                },
                "timeout dispatcherSpy.sendPublication",
                asyncGetterCallDelay
            ).then(() => {
                // reset first publication
                testAttribute.get.calls.reset();
                dispatcherSpy.sendPublication.calls.reset();
                expect(testAttribute.get).not.toHaveBeenCalled();
                expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();
            });

            const internalCheck = function(times, promiseChain) {
                return promiseChain.then(() => {
                    // step the clock forward to 1 ms before the interval
                    increaseFakeTime(maxIntervalMs - 1);
                    // 1 ms later the poll should happen
                    increaseFakeTime(1);

                    return waitsFor(
                        () => {
                            return dispatcherSpy.sendPublication.calls.count() === times + 1;
                        },
                        "timeout testAttribute.get",
                        asyncGetterCallDelay
                    ).then(() => {
                        expect(testAttribute.get.calls.count()).toEqual(times + 1);
                        expect(dispatcherSpy.sendPublication.calls.count()).toEqual(times + 1);
                    });
                });
            };

            for (times = 0; times < maxNrOfTimes; ++times) {
                promiseChain = internalCheck(times, promiseChain);
            }

            promiseChain
                .then(() => {
                    // cleanup
                    stopSubscription(mixedSubscriptionRequest);
                    expect(
                        publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)
                    ).toBeFalsy();
                    done();
                    return null;
                })
                .catch(fail);
        });

        it("creates a mixed subscription that publishes valueChanges that occur each minIntervalMs", done => {
            publicationManager.addPublicationProvider(providerId, provider);
            publicationManager.handleSubscriptionRequest(
                proxyId,
                providerId,
                mixedSubscriptionRequest,
                callbackDispatcher
            );
            expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeTruthy();
            increaseFakeTime(1);

            let promiseChain = waitsFor(
                () => {
                    return dispatcherSpy.sendPublication.calls.count() === 1;
                },
                "timeout dispatcherSpy.sendPublication",
                asyncGetterCallDelay
            ).then(() => {
                // reset first publication
                testAttribute.get.calls.reset();
                dispatcherSpy.sendPublication.calls.reset();
            });

            const internalCheck = function(times, promiseChain) {
                return promiseChain.then(() => {
                    increaseFakeTime(mixedSubscriptionRequest.qos.minIntervalMs);
                    testAttribute.valueChanged(value + times);

                    return waitsFor(
                        () => {
                            return dispatcherSpy.sendPublication.calls.count() === times + 1;
                        },
                        "timeout dispatcherSpy.sendPublication",
                        asyncGetterCallDelay
                    ).then(() => {
                        expect(testAttribute.get).not.toHaveBeenCalled();
                        expect(dispatcherSpy.sendPublication.calls.count()).toEqual(times + 1);
                    });
                });
            };

            for (let times = 0; times < maxNrOfTimes; times++) {
                promiseChain = internalCheck(times, promiseChain);
            }

            promiseChain
                .then(() => {
                    // cleanup
                    stopSubscription(mixedSubscriptionRequest);
                    expect(
                        publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)
                    ).toBeFalsy();
                    done();
                    return null;
                })
                .catch(fail);
        });

        it("creates a mixed subscription that publishes valueChanges that occur each maxIntervalMs-1", done => {
            dispatcherSpy.sendPublication.calls.reset();
            publicationManager.addPublicationProvider(providerId, provider);
            publicationManager.handleSubscriptionRequest(
                proxyId,
                providerId,
                mixedSubscriptionRequest,
                callbackDispatcher
            );
            expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeTruthy();
            increaseFakeTime(1);

            let promiseChain = waitsFor(
                () => {
                    return dispatcherSpy.sendPublication.calls.count() === 1;
                },
                "timeout dispatcherSpy.sendPublication",
                asyncGetterCallDelay
            ).then(() => {
                // reset first publication
                testAttribute.get.calls.reset();
                dispatcherSpy.sendPublication.calls.reset();
            });

            const internalCheck = function(times, promiseChain) {
                return promiseChain.then(() => {
                    increaseFakeTime(mixedSubscriptionRequest.qos.maxIntervalMs - 2);
                    testAttribute.valueChanged(value + times);
                    increaseFakeTime(1);

                    return waitsFor(
                        () => {
                            return dispatcherSpy.sendPublication.calls.count() === times + 1;
                        },
                        "timeout dispatcherSpy.sendPublication",
                        asyncGetterCallDelay
                    ).then(() => {
                        expect(testAttribute.get).not.toHaveBeenCalled();
                        expect(dispatcherSpy.sendPublication.calls.count()).toEqual(times + 1);
                    });
                });
            };
            for (let times = 0; times < maxNrOfTimes; times++) {
                promiseChain = internalCheck(times, promiseChain);
            }

            promiseChain
                .then(() => {
                    // cleanup
                    stopSubscription(mixedSubscriptionRequest);
                    expect(
                        publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)
                    ).toBeFalsy();
                    done();
                    return null;
                })
                .catch(fail);
        });
        it("creates a mixed subscription that publishes many valueChanges within minIntervalMs only once", done => {
            publicationManager.addPublicationProvider(providerId, provider);
            publicationManager.handleSubscriptionRequest(
                proxyId,
                providerId,
                mixedSubscriptionRequest,
                callbackDispatcher
            );
            expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeTruthy();
            increaseFakeTime(1);

            waitsFor(
                () => {
                    return dispatcherSpy.sendPublication.calls.count() === 1;
                },
                "timeout dispatcherSpy.sendPublication",
                asyncGetterCallDelay
            )
                .then(() => {
                    // reset first publication
                    testAttribute.get.calls.reset();
                    dispatcherSpy.sendPublication.calls.reset();

                    const shortInterval = Math.round((mixedSubscriptionRequest.qos.minIntervalMs - 2) / maxNrOfTimes);
                    for (let times = 0; times < maxNrOfTimes; times++) {
                        expect(testAttribute.get).not.toHaveBeenCalled();
                        expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();
                        increaseFakeTime(shortInterval);
                        testAttribute.valueChanged(value + times);
                    }

                    // after minIntervalMs the publication works again
                    increaseFakeTime(mixedSubscriptionRequest.qos.minIntervalMs - shortInterval * maxNrOfTimes);
                })
                .then(() => {
                    testAttribute.valueChanged(value);
                    expect(testAttribute.get.calls.count()).toEqual(1);
                    expect(dispatcherSpy.sendPublication.calls.count()).toEqual(1);

                    // cleanup
                    stopSubscription(mixedSubscriptionRequest);
                    expect(
                        publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)
                    ).toBeFalsy();
                    done();
                    return null;
                })
                .catch(fail);
        });

        it("creates a periodic subscription without expiryDate and expects periodic publications", done => {
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
            dispatcherSpy.sendPublication.calls.reset();
            publicationManager.addPublicationProvider(providerId, provider);
            publicationManager.handleSubscriptionRequest(
                proxyId,
                providerId,
                subscriptionRequestWithoutExpiryDate,
                callbackDispatcher
            );
            expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeTruthy();
            increaseFakeTime(1);

            let promiseChain = waitsFor(
                () => {
                    return dispatcherSpy.sendPublication.calls.count() === 1;
                },
                "timeout dispatcherSpy.sendPublication",
                asyncGetterCallDelay
            ).then(() => {
                expect(dispatcherSpy.sendPublication.calls.count()).toEqual(1);
                dispatcherSpy.sendPublication.calls.reset();
            });

            const checkMaxIntervalCalls = function(i, promiseChain) {
                return promiseChain
                    .then(() => {
                        increaseFakeTime(periodMs);

                        return waitsFor(
                            () => {
                                return dispatcherSpy.sendPublication.calls.count() === 1 + i;
                            },
                            "timeout " + i + " times dispatcherSpy.sendPublication call",
                            asyncGetterCallDelay
                        );
                    })
                    .then(() => {
                        expect(dispatcherSpy.sendPublication.calls.count()).toEqual(1 + i);
                    });
            };

            let i;
            for (i = 0; i < n; i++) {
                promiseChain = checkMaxIntervalCalls(i, promiseChain);
            }

            promiseChain
                .then(() => {
                    // cleanup
                    stopSubscription(subscriptionRequestWithoutExpiryDate);
                    expect(
                        publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)
                    ).toBeFalsy();
                    done();
                    return null;
                })
                .catch(fail);
        });
        it("creates a mixed subscription that publishes correctly with all cases mixed", done => {
            let i;

            publicationManager.addPublicationProvider(providerId, provider);
            publicationManager.handleSubscriptionRequest(
                proxyId,
                providerId,
                mixedSubscriptionRequest,
                callbackDispatcher
            );
            expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeTruthy();
            increaseFakeTime(1);

            waitsFor(
                () => {
                    return dispatcherSpy.sendPublication.calls.count() === 1;
                },
                "timeout dispatcherSpy.sendPublication",
                asyncGetterCallDelay
            )
                .then(() => {
                    // reset first publication
                    testAttribute.get.calls.reset();
                    dispatcherSpy.sendPublication.calls.reset();

                    // at time 0, a value change is not reported because value was
                    // already published initially while
                    // handling the subscription request
                    testAttribute.valueChanged(value);
                    expect(testAttribute.get).not.toHaveBeenCalled();
                    expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();

                    // minIntervalMs and a value change the value should be reported
                    increaseFakeTime(mixedSubscriptionRequest.qos.minIntervalMs);

                    return waitsFor(
                        () => {
                            return dispatcherSpy.sendPublication.calls.count() === 1;
                        },
                        "timeout dispatcherSpy.sendPublication call",
                        asyncGetterCallDelay
                    );
                })
                .then(() => {
                    // due to minIntervalMs exceeded + valueChanged has been occured
                    // within the minIntervalMs, the
                    // PublicationManager
                    // send the current attribute value to the subscriptions
                    expect(testAttribute.get.calls.count()).toEqual(1);
                    expect(dispatcherSpy.sendPublication.calls.count()).toEqual(1);
                    testAttribute.get.calls.reset();
                    dispatcherSpy.sendPublication.calls.reset();
                    // minIntervalMs and no publication shall occur
                    increaseFakeTime(mixedSubscriptionRequest.qos.minIntervalMs);
                    expect(testAttribute.get).not.toHaveBeenCalled();
                    expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();

                    // change value, and immediate publication shall occur
                    testAttribute.valueChanged(value);
                    expect(testAttribute.get).not.toHaveBeenCalled();
                    expect(dispatcherSpy.sendPublication.calls.count()).toEqual(1);

                    // at time 1:maxNrOfTimes, a value change is reported => NO
                    // publication is sent
                    for (i = 0; i < maxNrOfTimes; ++i) {
                        dispatcherSpy.sendPublication.calls.reset();
                        increaseFakeTime(1);
                        testAttribute.valueChanged(value);
                        expect(testAttribute.get).not.toHaveBeenCalled();
                        expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();
                    }

                    // at time mixedSubscriptionRequest.qos.minIntervalMs the last
                    // value of the test attribute is sent
                    // to the subscribers as the publication timeout occurs
                    dispatcherSpy.sendPublication.calls.reset();
                    increaseFakeTime(mixedSubscriptionRequest.qos.minIntervalMs - maxNrOfTimes);
                })
                .then(() => {
                    expect(testAttribute.get.calls.count()).toEqual(1);
                })
                .then(() => {
                    return waitsFor(
                        () => {
                            return dispatcherSpy.sendPublication.calls.count() === 1;
                        },
                        "timeout dispatcherSpy.sendPublication call ",
                        asyncGetterCallDelay
                    );
                })
                .then(() => {
                    expect(dispatcherSpy.sendPublication.calls.count()).toEqual(1);

                    // value change after mixedSubscriptionRequest.qos.maxInterval - 2=> publication sent
                    dispatcherSpy.sendPublication.calls.reset();
                    testAttribute.get.calls.reset();

                    increaseFakeTime(mixedSubscriptionRequest.qos.maxIntervalMs - 1);
                    testAttribute.valueChanged(value);
                    expect(testAttribute.get).not.toHaveBeenCalled();
                    return waitsFor(
                        () => {
                            return dispatcherSpy.sendPublication.calls.count() === 1;
                        },
                        "timeout dispatcherSpy.sendPublication call ",
                        asyncGetterCallDelay
                    );
                })
                .then(() => {
                    expect(dispatcherSpy.sendPublication.calls.count()).toEqual(1);

                    // after mixedSubscriptionRequest.qos.maxIntervalMs => interval
                    // publication is sent
                    dispatcherSpy.sendPublication.calls.reset();
                    increaseFakeTime(mixedSubscriptionRequest.qos.maxIntervalMs);
                    return waitsFor(
                        () => {
                            return dispatcherSpy.sendPublication.calls.count() === 1;
                        },
                        "timeout dispatcherSpy.sendPublication call ",
                        asyncGetterCallDelay
                    );
                })
                .then(() => {
                    expect(testAttribute.get.calls.count()).toEqual(1);
                    expect(dispatcherSpy.sendPublication.calls.count()).toEqual(1);

                    // after another mixedSubscriptionRequest.qos.maxIntervalMs =>
                    // interval publication is sent
                    testAttribute.get.calls.reset();
                    dispatcherSpy.sendPublication.calls.reset();
                    increaseFakeTime(mixedSubscriptionRequest.qos.maxIntervalMs);
                    return waitsFor(
                        () => {
                            return dispatcherSpy.sendPublication.calls.count() === 1;
                        },
                        "timeout dispatcherSpy.sendPublication call ",
                        asyncGetterCallDelay
                    );
                })
                .then(() => {
                    expect(testAttribute.get.calls.count()).toEqual(1);
                    expect(dispatcherSpy.sendPublication.calls.count()).toEqual(1);

                    // after subscription stop => NO publications are sent any more
                    stopSubscription(mixedSubscriptionRequest);
                    expect(
                        publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)
                    ).toBeFalsy();
                    testAttribute.get.calls.reset();
                    dispatcherSpy.sendPublication.calls.reset();
                    increaseFakeTime(mixedSubscriptionRequest.qos.minIntervalMs);
                    testAttribute.valueChanged(value);
                    expect(testAttribute.get).not.toHaveBeenCalled();
                    done();
                    return null;
                })
                .catch(fail);
        });
        it("does not publish when mixed subscription has an endDate in the past", done => {
            mixedSubscriptionRequest.qos.expiryDateMs = Date.now() - 1;

            publicationManager.addPublicationProvider(providerId, provider);
            publicationManager.handleSubscriptionRequest(
                proxyId,
                providerId,
                mixedSubscriptionRequest,
                callbackDispatcher
            );
            expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeFalsy();
            // reset first publication
            testAttribute.get.calls.reset();
            dispatcherSpy.sendPublication.calls.reset();

            testAttribute.valueChanged(value);
            increaseFakeTime(subscriptionLength);
            expect(testAttribute.get).not.toHaveBeenCalled();
            expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();

            // cleanup
            stopSubscription(mixedSubscriptionRequest);
            expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeFalsy();
            done();
        });

        it("removes a mixed attribute subscription after endDate", done => {
            publicationManager.addPublicationProvider(providerId, provider);
            publicationManager.handleSubscriptionRequest(
                proxyId,
                providerId,
                mixedSubscriptionRequest,
                callbackDispatcher
            );
            expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeTruthy();
            increaseFakeTime(1);

            waitsFor(
                () => {
                    return dispatcherSpy.sendPublication.calls.count() > 0;
                },
                "dispatcherSpy.sendPublication",
                asyncGetterCallDelay
            )
                .then(() => {
                    // reset first publication
                    testAttribute.get.calls.reset();
                    dispatcherSpy.sendPublication.calls.reset();

                    increaseFakeTime(subscriptionLength);
                    testAttribute.get.calls.reset();
                    dispatcherSpy.sendPublication.calls.reset();

                    expect(testAttribute.get).not.toHaveBeenCalled();
                    expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();
                    testAttribute.valueChanged(value);
                    expect(testAttribute.get).not.toHaveBeenCalled();
                    expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();
                    stopSubscription(mixedSubscriptionRequest);
                    expect(
                        publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)
                    ).toBeFalsy();
                    done();
                    return null;
                })
                .catch(fail);
        });

        xit("removes a mixed attribute subscription after subscription stop", done => {
            publicationManager.addPublicationProvider(providerId, provider);
            publicationManager.handleSubscriptionRequest(
                proxyId,
                providerId,
                mixedSubscriptionRequest,
                callbackDispatcher
            );
            expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeTruthy();
            increaseFakeTime(1);

            waitsFor(
                () => {
                    return dispatcherSpy.sendPublication.calls.count() > 0;
                },
                "dispatcherSpy.sendPublication",
                asyncGetterCallDelay
            )
                .then(() => {
                    // reset first publication
                    testAttribute.get.calls.reset();
                    dispatcherSpy.sendPublication.calls.reset();

                    testAttribute.valueChanged(value); // do change
                    //increaseFakeTime(maxIntervalMs); // increase interval
                    increaseFakeTime(maxIntervalMs); // increase interval

                    return waitsFor(
                        () => {
                            return dispatcherSpy.sendPublication.calls.count() > 0;
                        },
                        "dispatcherSpy.sendPublication 2",
                        asyncGetterCallDelay
                    );
                })
                .then(() => {
                    // after subscription stop, the methods should not have been called
                    // again (ie subscription
                    // terminated)
                    publicationManager.handleSubscriptionStop(
                        new SubscriptionStop({
                            subscriptionId: mixedSubscriptionRequest.subscriptionId
                        })
                    );
                    expect(
                        publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)
                    ).toBeFalsy();
                    testAttribute.get.calls.reset();
                    dispatcherSpy.sendPublication.calls.reset();

                    increaseFakeTime(maxIntervalMs); // increase interval
                    testAttribute.valueChanged(value); // do change
                    increaseFakeTime(maxIntervalMs); // increase interval

                    originalSetTimeout(() => {
                        expect(testAttribute.get).not.toHaveBeenCalled();
                        expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();
                        stopSubscription(mixedSubscriptionRequest);
                        done();
                    }, asyncGetterCallDelay);
                    return null;
                })
                .catch(fail);
        });

        it("removes a mixed broadcast subscription after subscription stop", done => {
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
            increaseFakeTime(1);

            testBroadcast.fire(broadcastOutputParameters);
            increaseFakeTime(maxIntervalMs); // increase interval

            waitsFor(
                () => {
                    return dispatcherSpy.sendPublication.calls.count() > 0;
                },
                "dispatcherSpy.sendPublication",
                asyncGetterCallDelay
            )
                .then(() => {
                    // reset first publication
                    dispatcherSpy.sendPublication.calls.reset();

                    // after subscription stop, the methods should not have been called
                    // again (ie subscription
                    // terminated)
                    publicationManager.handleSubscriptionStop(
                        new SubscriptionStop({
                            subscriptionId: onChangeBroadcastSubscriptionRequest.subscriptionId
                        })
                    );

                    expect(
                        publicationManager.hasSubscriptionsForProviderEvent(providerId, testBroadcastName)
                    ).toBeFalsy();

                    increaseFakeTime(maxIntervalMs); // increase interval
                    testBroadcast.fire(broadcastOutputParameters);
                    increaseFakeTime(maxIntervalMs); // increase interval

                    originalSetTimeout(() => {
                        expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();
                        done();
                    }, asyncGetterCallDelay);
                    return null;
                })
                .catch(fail);
        });

        it("removes publication provider", done => {
            spyOn(testAttribute, "registerObserver").and.callThrough();
            spyOn(testAttribute, "unregisterObserver").and.callThrough();
            publicationManager.addPublicationProvider(providerId, provider);
            expect(testAttribute.registerObserver).toHaveBeenCalled();
            spyOn(publicationManager, "handleSubscriptionStop").and.callThrough();

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
            increaseFakeTime(1);

            waitsFor(
                () => {
                    return dispatcherSpy.sendPublication.calls.count() > 0;
                },
                "dispatcherSpy.sendPublication",
                asyncGetterCallDelay
            )
                .then(() => {
                    expect(testAttribute.get.calls.count()).toEqual(1);
                    expect(dispatcherSpy.sendPublication.calls.count()).toEqual(1);
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
                    expect(
                        publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)
                    ).toBeFalsy();
                    done();
                    return null;
                })
                .catch(fail);
        });

        it("behaves correctly when resubscribing to a publication", done => {
            spyOn(testAttribute, "registerObserver").and.callThrough();
            spyOn(testAttribute, "unregisterObserver").and.callThrough();
            publicationManager.addPublicationProvider(providerId, provider);
            expect(testAttribute.registerObserver).toHaveBeenCalled();
            spyOn(publicationManager, "handleSubscriptionStop").and.callThrough();

            publicationManager.handleSubscriptionRequest(
                proxyId,
                providerId,
                intervalSubscriptionRequest,
                callbackDispatcher
            );

            // get to the bottom of the event loop
            Promise.resolve()
                .then(() => {
                    expect(testAttribute.get.calls.count()).toEqual(1);
                    expect(dispatcherSpy.sendPublication.calls.count()).toEqual(1);
                    expect(testAttribute.unregisterObserver).not.toHaveBeenCalled();
                    expect(publicationManager.handleSubscriptionStop).not.toHaveBeenCalled();

                    publicationManager.removePublicationProvider(providerId, provider);
                })
                .then(() => {
                    expect(testAttribute.unregisterObserver).toHaveBeenCalled();
                    expect(publicationManager.handleSubscriptionStop).toHaveBeenCalledWith(
                        new SubscriptionStop({
                            subscriptionId: intervalSubscriptionRequest.subscriptionId
                        })
                    );

                    expect(publicationManager.hasSubscriptions()).toBeFalsy();

                    callbackDispatcher.calls.reset();
                    increaseFakeTime(1000);
                    publicationManager.addPublicationProvider(providerId, provider);
                    publicationManager.handleSubscriptionRequest(
                        proxyId,
                        providerId,
                        intervalSubscriptionRequest,
                        callbackDispatcher
                    );
                })
                .then(() => {
                    expect(callbackDispatcher.calls.count()).toEqual(1);

                    // cleanup
                    stopSubscription(intervalSubscriptionRequest);
                    expect(
                        publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)
                    ).toBeFalsy();
                    done();
                })
                .catch(fail);
        });

        it("rejects attribute subscription if expiryDateMs lies in the past", done => {
            const request = new SubscriptionRequest({
                subscriptionId: "subscriptionId" + uuid(),
                subscribedToName: testAttributeName,
                qos: new OnChangeSubscriptionQos({
                    expiryDateMs: Date.now() - 10000
                })
            });
            publicationManager.addPublicationProvider(providerId, provider);
            publicationManager.handleSubscriptionRequest(proxyId, providerId, request, callbackDispatcher);

            waitsFor(
                () => {
                    return callbackDispatcher.calls.count() === 1;
                },
                "callbackDispatcher got called",
                1000
            )
                .then(() => {
                    expect(callbackDispatcher).toHaveBeenCalled();
                    const error = callbackDispatcher.calls.mostRecent().args[1].error;
                    expect(error).toBeDefined();
                    expect(error instanceof SubscriptionException);
                    expect(error.subscriptionId).toBeDefined();
                    expect(error.subscriptionId).toEqual(callbackDispatcher.calls.mostRecent().args[1].subscriptionId);
                    expect(error.detailMessage).toMatch(/lies in the past/);
                    done();
                    return null;
                })
                .catch(fail);
            increaseFakeTime(1);
        });

        it("rejects attribute subscription if attribute does not exist", done => {
            const request = new SubscriptionRequest({
                subscriptionId: "subscriptionId" + uuid(),
                subscribedToName: "nonExistingAttribute",
                qos: new OnChangeSubscriptionQos()
            });
            publicationManager.addPublicationProvider(providerId, provider);
            publicationManager.handleSubscriptionRequest(proxyId, providerId, request, callbackDispatcher);

            waitsFor(
                () => {
                    return callbackDispatcher.calls.count() === 1;
                },
                "callbackDispatcher got called",
                1000
            )
                .then(() => {
                    expect(callbackDispatcher).toHaveBeenCalled();
                    const error = callbackDispatcher.calls.mostRecent().args[1].error;
                    expect(error).toBeDefined();
                    expect(error instanceof SubscriptionException);
                    expect(error.subscriptionId).toBeDefined();
                    expect(error.subscriptionId).toEqual(callbackDispatcher.calls.mostRecent().args[1].subscriptionId);
                    expect(error.detailMessage).toMatch(/misses attribute/);
                    done();
                    return null;
                })
                .catch(fail);
            increaseFakeTime(1);
        });

        it("rejects attribute subscription if attribute is not notifiable", done => {
            const request = new SubscriptionRequest({
                subscriptionId: "subscriptionId" + uuid(),
                subscribedToName: testAttributeNotNotifiableName,
                qos: new OnChangeSubscriptionQos()
            });
            publicationManager.addPublicationProvider(providerId, provider);
            publicationManager.handleSubscriptionRequest(proxyId, providerId, request, callbackDispatcher);

            waitsFor(
                () => {
                    return callbackDispatcher.calls.count() === 1;
                },
                "callbackDispatcher got called",
                1000
            )
                .then(() => {
                    expect(callbackDispatcher).toHaveBeenCalled();
                    const error = callbackDispatcher.calls.mostRecent().args[1].error;
                    expect(error).toBeDefined();
                    expect(error instanceof SubscriptionException);
                    expect(error.subscriptionId).toBeDefined();
                    expect(error.subscriptionId).toEqual(callbackDispatcher.calls.mostRecent().args[1].subscriptionId);
                    expect(error.detailMessage).toMatch(/is not notifiable/);
                    done();
                    return null;
                })
                .catch(fail);
            increaseFakeTime(1);
        });

        it("rejects attribute subscription if periodMs is too small", done => {
            const qosSettings = new PeriodicSubscriptionQos({
                expiryDateMs: 0,
                alertAfterIntervalMs: 0,
                publicationTtlMs: 1000
            });
            // forcibly fake it! The constructor throws, if using this directly
            qosSettings.periodMs = PeriodicSubscriptionQos.MIN_PERIOD_MS - 1;

            const request = new SubscriptionRequest({
                subscriptionId: "subscriptionId" + uuid(),
                subscribedToName: testAttributeName,
                qos: qosSettings
            });
            publicationManager.addPublicationProvider(providerId, provider);
            publicationManager.handleSubscriptionRequest(proxyId, providerId, request, callbackDispatcher);

            waitsFor(
                () => {
                    return callbackDispatcher.calls.count() === 1;
                },
                "callbackDispatcher got called",
                1000
            )
                .then(() => {
                    expect(callbackDispatcher).toHaveBeenCalled();
                    const error = callbackDispatcher.calls.mostRecent().args[1].error;
                    expect(error).toBeDefined();
                    expect(error instanceof SubscriptionException);
                    expect(error.subscriptionId).toBeDefined();
                    expect(error.subscriptionId).toEqual(callbackDispatcher.calls.mostRecent().args[1].subscriptionId);
                    expect(error.detailMessage).toMatch(/is smaller than PeriodicSubscriptionQos/);
                    done();
                    return null;
                })
                .catch(fail);
            increaseFakeTime(1);
        });

        it("rejects broadcast subscription if expiryDateMs lies in the past", done => {
            const request = new BroadcastSubscriptionRequest({
                subscriptionId: "subscriptionId" + uuid(),
                subscribedToName: testBroadcastName,
                qos: new OnChangeSubscriptionQos({
                    expiryDateMs: Date.now() - 10000
                }),
                filterParameters: {}
            });
            publicationManager.addPublicationProvider(providerId, provider);
            publicationManager.handleBroadcastSubscriptionRequest(proxyId, providerId, request, callbackDispatcher);

            waitsFor(
                () => {
                    return callbackDispatcher.calls.count() === 1;
                },
                "callbackDispatcher got called",
                1000
            )
                .then(() => {
                    expect(callbackDispatcher).toHaveBeenCalled();
                    const error = callbackDispatcher.calls.mostRecent().args[1].error;
                    expect(error).toBeDefined();
                    expect(error instanceof SubscriptionException);
                    expect(error.subscriptionId).toBeDefined();
                    expect(error.subscriptionId).toEqual(callbackDispatcher.calls.mostRecent().args[1].subscriptionId);
                    expect(error.detailMessage).toMatch(/lies in the past/);
                    done();
                    return null;
                })
                .catch(fail);
            increaseFakeTime(1);
        });

        it("rejects broadcast subscription if filter parameters are wrong", done => {
            const request = new BroadcastSubscriptionRequest({
                subscriptionId: "subscriptionId" + uuid(),
                subscribedToName: testBroadcastName,
                qos: new OnChangeSubscriptionQos(),
                filterParameters: {
                    filterParameters: {
                        corruptFilterParameter: "value"
                    }
                }
            });
            publicationManager.addPublicationProvider(providerId, provider);
            publicationManager.handleBroadcastSubscriptionRequest(proxyId, providerId, request, callbackDispatcher);

            waitsFor(
                () => {
                    return callbackDispatcher.calls.count() === 1;
                },
                "callbackDispatcher got called",
                1000
            )
                .then(() => {
                    expect(callbackDispatcher).toHaveBeenCalled();
                    const error = callbackDispatcher.calls.mostRecent().args[1].error;
                    expect(error).toBeDefined();
                    expect(error instanceof SubscriptionException);
                    expect(error.subscriptionId).toBeDefined();
                    expect(error.subscriptionId).toEqual(callbackDispatcher.calls.mostRecent().args[1].subscriptionId);
                    expect(error.detailMessage).toMatch(/Filter parameter positionOfInterest for broadcast/);
                    done();
                    return null;
                })
                .catch(fail);
            increaseFakeTime(1);
        });

        it("registers multicast subscription", done => {
            expect(publicationManager.hasMulticastSubscriptions()).toBe(false);
            publicationManager.addPublicationProvider(providerId, provider);

            const request = handleMulticastSubscriptionRequest();

            waitsFor(
                () => {
                    return callbackDispatcher.calls.count() === 1;
                },
                "callbackDispatcher got called",
                1000
            )
                .then(() => {
                    expect(callbackDispatcher).toHaveBeenCalled();
                    const response = callbackDispatcher.calls.mostRecent().args[1];
                    expect(response.error).toBeUndefined();
                    expect(response.subscriptionId).toEqual(request.subscriptionId);
                    expect(publicationManager.hasMulticastSubscriptions()).toBe(true);
                    done();
                    return null;
                })
                .catch(fail);
            increaseFakeTime(1);
        });

        it("registers and unregisters multicast subscription", () => {
            expect(publicationManager.hasMulticastSubscriptions()).toBe(false);
            publicationManager.addPublicationProvider(providerId, provider);

            const request = handleMulticastSubscriptionRequest();
            expect(publicationManager.hasMulticastSubscriptions()).toBe(true);

            expect(publicationManager.hasSubscriptions()).toBe(true);
            publicationManager.handleSubscriptionStop({
                subscriptionId: request.subscriptionId
            });
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

            const multicastPublication = dispatcherSpy.sendMulticastPublication.calls.argsFor(0)[1];

            expect(multicastPublication.multicastId).toEqual(request.multicastId);

            publicationManager.handleSubscriptionStop({
                subscriptionId: request.subscriptionId
            });
            expect(publicationManager.hasMulticastSubscriptions()).toBe(false);
        });

        it("rejects broadcast subscription if broadcast does not exist", done => {
            const request = new BroadcastSubscriptionRequest({
                subscriptionId: "subscriptionId" + uuid(),
                subscribedToName: "nonExistingBroadcast",
                qos: new OnChangeSubscriptionQos(),
                filterParameters: {}
            });
            publicationManager.addPublicationProvider(providerId, provider);
            publicationManager.handleBroadcastSubscriptionRequest(proxyId, providerId, request, callbackDispatcher);

            waitsFor(
                () => {
                    return callbackDispatcher.calls.count() === 1;
                },
                "callbackDispatcher got called",
                1000
            )
                .then(() => {
                    expect(callbackDispatcher).toHaveBeenCalled();
                    const error = callbackDispatcher.calls.mostRecent().args[1].error;
                    expect(error).toBeDefined();
                    expect(error instanceof SubscriptionException);
                    expect(error.subscriptionId).toBeDefined();
                    expect(error.subscriptionId).toEqual(callbackDispatcher.calls.mostRecent().args[1].subscriptionId);
                    expect(error.detailMessage).toMatch(/misses event/);
                    done();
                    return null;
                })
                .catch(fail);
            increaseFakeTime(1);
        });

        it(" throws exception when called while shut down", done => {
            publicationManager.shutdown();
            const callbackDispatcherSpy = jasmine.createSpy("callbackDispatcherSpy");

            expect(() => {
                publicationManager.removePublicationProvider("providerParticipantId", {});
            }).toThrow();

            expect(() => {
                publicationManager.addPublicationProvider("providerParticipantId", {});
            }).toThrow();

            expect(() => {
                publicationManager.restore();
            }).toThrow();

            publicationManager.handleSubscriptionRequest(
                "proxyParticipantId",
                "providerParticipantId",
                {
                    subscriptionId: "subscriptionId"
                },
                callbackDispatcherSpy
            );
            increaseFakeTime(1);
            waitsFor(
                () => {
                    return callbackDispatcherSpy.calls.count() === 1;
                },
                "callbackDispatcher for attributes got called",
                1000
            )
                .then(() => {
                    expect(callbackDispatcherSpy).toHaveBeenCalled();
                    expect(callbackDispatcherSpy.calls.argsFor(0)[1] instanceof SubscriptionReply);
                    expect(callbackDispatcherSpy.calls.argsFor(0)[1].error instanceof SubscriptionException);
                    return null;
                })
                .then(() => {
                    callbackDispatcherSpy.calls.reset();
                    publicationManager.handleBroadcastSubscriptionRequest(
                        "proxyParticipantId",
                        "providerParticipantId",
                        {
                            subscriptionId: "subscriptionId"
                        },
                        callbackDispatcherSpy
                    );
                    increaseFakeTime(1);
                    waitsFor(
                        () => {
                            return callbackDispatcherSpy.calls.count() === 1;
                        },
                        "callbackDispatcher for events got called",
                        1000
                    ).then(() => {
                        expect(callbackDispatcherSpy).toHaveBeenCalled();
                        expect(callbackDispatcherSpy.calls.argsFor(0)[1] instanceof SubscriptionReply);
                        expect(callbackDispatcherSpy.calls.argsFor(0)[1].error instanceof SubscriptionException);
                        done();
                        return null;
                    });
                })
                .catch(fail);
        });
    }
});
