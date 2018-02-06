/*jslint es5: true, node: true, node: true */
/*global fail: true, xit: true, sharedTests: true */
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
var Promise = require("../../../../classes/global/Promise");
var PublicationManager = require("../../../../classes/joynr/dispatching/subscription/PublicationManager");
var MessagingQos = require("../../../../classes/joynr/messaging/MessagingQos");
var SubscriptionReply = require("../../../../classes/joynr/dispatching/types/SubscriptionReply");
var SubscriptionRequest = require("../../../../classes/joynr/dispatching/types/SubscriptionRequest");
var BroadcastSubscriptionRequest = require("../../../../classes/joynr/dispatching/types/BroadcastSubscriptionRequest");
var MulticastSubscriptionRequest = require("../../../../classes/joynr/dispatching/types/MulticastSubscriptionRequest");
var SubscriptionStop = require("../../../../classes/joynr/dispatching/types/SubscriptionStop");
var ProviderAttribute = require("../../../../classes/joynr/provider/ProviderAttribute");
var ProviderEvent = require("../../../../classes/joynr/provider/ProviderEvent");
var PeriodicSubscriptionQos = require("../../../../classes/joynr/proxy/PeriodicSubscriptionQos");
var SubscriptionQos = require("../../../../classes/joynr/proxy/SubscriptionQos");
var OnChangeSubscriptionQos = require("../../../../classes/joynr/proxy/OnChangeSubscriptionQos");
var OnChangeWithKeepAliveSubscriptionQos = require("../../../../classes/joynr/proxy/OnChangeWithKeepAliveSubscriptionQos");
var ProviderQos = require("../../../../classes/joynr/types/ProviderQos");
var ProviderScope = require("../../../../classes/joynr/types/ProviderScope");
var SubscriptionPublication = require("../../../../classes/joynr/dispatching/types/SubscriptionPublication");
var SubscriptionUtil = require("../../../../classes/joynr/dispatching/subscription/util/SubscriptionUtil");
var SubscriptionException = require("../../../../classes/joynr/exceptions/SubscriptionException");
var LongTimer = require("../../../../classes/joynr/util/LongTimer");
var uuid = require("../../../../classes/lib/uuid-annotated");
var Date = require("../../../../test-classes/global/Date");
var waitsFor = require("../../../../test-classes/global/WaitsFor");
var LocalStorage = require("../../../../test-classes/global/LocalStorageNodeTests");
var originalSetTimeout = setTimeout;
describe("libjoynr-js.joynr.dispatching.subscription.PublicationManager", function() {
    var callbackDispatcher;
    var proxyId, providerId, publicationManager, joynrInstanceId, dispatcherSpy;
    var provider, asyncGetterCallDelay, fakeTime, intervalSubscriptionRequest;
    var onChangeSubscriptionRequest, mixedSubscriptionRequest, onChangeBroadcastSubscriptionRequest;
    var mixedSubscriptionRequestWithAsyncAttribute, testAttributeName;
    var asyncTestAttributeName, value, minIntervalMs, maxIntervalMs, maxNrOfTimes;
    var subscriptionLength, asyncTestAttribute, testAttribute, providerSettings;
    var testAttributeNotNotifiable, testAttributeNotNotifiableName;
    var testBroadcastName, testBroadcast;
    var testNonSelectiveBroadcastName, testNonSelectiveBroadcast;
    var persistency; // localStorage was renamed to persistency because it's impossible to reassign it because of jslint
    var callbackDispatcherSettings;

    function createSubscriptionRequest(
        isAttribute,
        subscribeToName,
        periodMs,
        subscriptionLength,
        onChange,
        minIntervalMs
    ) {
        var qosSettings, expiryDateMs, request;
        expiryDateMs =
            subscriptionLength === SubscriptionQos.NO_EXPIRY_DATE
                ? SubscriptionQos.NO_EXPIRY_DATE
                : Date.now() + subscriptionLength;
        if (onChange) {
            if (periodMs !== undefined) {
                qosSettings = new OnChangeWithKeepAliveSubscriptionQos({
                    minIntervalMs: minIntervalMs || 50,
                    maxIntervalMs: periodMs,
                    expiryDateMs: expiryDateMs,
                    alertAfterIntervalMs: 0,
                    publicationTtlMs: 1000
                });
            } else {
                qosSettings = new OnChangeSubscriptionQos({
                    minIntervalMs: minIntervalMs || 50,
                    expiryDateMs: expiryDateMs,
                    publicationTtlMs: 1000
                });
            }
        } else {
            qosSettings = new PeriodicSubscriptionQos({
                periodMs: periodMs,
                expiryDateMs: expiryDateMs,
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

    // have to define our own ProviderAttributeNotifyReadWrite because we need
    // the phrase "Notify" in the constructor
    // function name for the SubscriptionPublication Manager to work correctly,
    // but we can't use the
    // ProviderAttributeNotifyReadWrite implementation because it freezes its
    // members for safe public usage => we cannot
    // spy on functions of a real ProviderAttributeNotifyReadWrite instance
    function ProviderAttributeNotifyReadWrite(parent, settings, attributeName, attributeType) {
        var providerAttribute = new ProviderAttribute(
            parent,
            settings,
            attributeName,
            attributeType,
            "NOTIFYREADWRITE"
        );
        this.registerGetter = providerAttribute.registerGetter;
        this.get = providerAttribute.get;
        this.registerSetter = providerAttribute.registerSetter;
        this.set = providerAttribute.set;
        this.valueChanged = providerAttribute.valueChanged;
        this.registerObserver = providerAttribute.registerObserver;
        this.unregisterObserver = providerAttribute.unregisterObserver;
        this.isNotifiable = providerAttribute.isNotifiable;
    }

    function ProviderAttributeReadWrite(parent, settings, attributeName, attributeType) {
        var providerAttribute = new ProviderAttribute(parent, settings, attributeName, attributeType, "READWRITE");
        this.registerGetter = providerAttribute.registerGetter;
        this.get = providerAttribute.get;
        this.registerSetter = providerAttribute.registerSetter;
        this.set = providerAttribute.set;
        this.isNotifiable = providerAttribute.isNotifiable;
        //this.valueChanged = providerAttribute.valueChanged;
        //this.registerObserver = providerAttribute.registerObserver;
        //this.unregisterObserver = providerAttribute.unregisterObserver;
    }

    function stopSubscription(subscriptionInfo) {
        publicationManager.handleSubscriptionStop(
            new SubscriptionStop({
                subscriptionId: subscriptionInfo.subscriptionId
            })
        );
    }

    function handleMulticastSubscriptionRequest() {
        var request = new MulticastSubscriptionRequest({
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
        callbackDispatcherSettings = {};

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
                publicationManager: publicationManager
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

        testAttribute = new ProviderAttributeNotifyReadWrite(provider, providerSettings, testAttributeName, "Boolean");

        asyncTestAttribute = new ProviderAttributeNotifyReadWrite(
            provider,
            providerSettings,
            asyncTestAttributeName,
            "Boolean"
        );

        testAttributeNotNotifiable = new ProviderAttributeReadWrite(
            provider,
            providerSettings,
            testAttributeNotNotifiableName,
            "Boolean"
        );

        provider[testAttributeName] = testAttribute;
        provider[testBroadcastName] = testBroadcast;
        provider[testNonSelectiveBroadcastName] = testNonSelectiveBroadcast;
        provider[testAttributeNotNotifiableName] = testAttributeNotNotifiable;
        spyOn(testAttribute, "get").and.returnValue("attributeValue");
        spyOn(testAttributeNotNotifiable, "get").and.returnValue("attributeValue");

        provider[asyncTestAttributeName] = asyncTestAttribute;
        spyOn(asyncTestAttribute, "get").and.callFake(function() {
            return new Promise(function(resolve, reject) {
                LongTimer.setTimeout(function() {
                    resolve("asyncAttributeValue");
                }, asyncGetterCallDelay);
            });
        });

        jasmine.clock().install();
        spyOn(Date, "now").and.callFake(function() {
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

    describe("without localStorage", function() {
        sharedTests(function() {
            persistency = null;
        });
    });

    describe("with localStorage", function() {
        sharedTests(function() {
            persistency = new LocalStorage();
        });
    });

    function sharedTests(beforeTests) {
        beforeEach(function(done) {
            beforeTests();
            prepareTests(done);
        });

        afterEach(function(done) {
            jasmine.clock().uninstall();
            done();
        });

        it("is instantiable", function(done) {
            expect(publicationManager).toBeDefined();
            expect(function() {
                var p = new PublicationManager(dispatcherSpy, persistency);
            }).not.toThrow();
            done();
        });

        it("calls dispatcher with correct arguments", function(done) {
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
                function() {
                    return dispatcherSpy.sendPublication.calls.count() > 0;
                },
                "dispatcherSpy.sendPublication",
                asyncGetterCallDelay
            )
                .then(function() {
                    // reset first publication
                    testAttribute.get.calls.reset();
                    dispatcherSpy.sendPublication.calls.reset();

                    increaseFakeTime(50);

                    testAttribute.valueChanged(value);

                    return waitsFor(
                        function() {
                            return dispatcherSpy.sendPublication.calls.count() > 0;
                        },
                        "dispatcherSpy.sendPublication",
                        asyncGetterCallDelay
                    );
                })
                .then(function() {
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

        it("publishes first value once immediately after subscription", function(done) {
            var times;

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
                function() {
                    return dispatcherSpy.sendPublication.calls.count() > 0;
                },
                "dispatcherSpy.sendPublication",
                asyncGetterCallDelay
            )
                .then(function() {
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

        it("creates a working interval subscription", function(done) {
            var times;

            publicationManager.addPublicationProvider(providerId, provider);
            publicationManager.handleSubscriptionRequest(
                proxyId,
                providerId,
                intervalSubscriptionRequest,
                callbackDispatcher
            );
            expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeTruthy();
            increaseFakeTime(1);

            var promiseChain = waitsFor(function() {
                return dispatcherSpy.sendPublication.calls.count() > 0;
            }, asyncGetterCallDelay).then(function() {
                // reset first publication
                testAttribute.get.calls.reset();
                dispatcherSpy.sendPublication.calls.reset();
            });

            var internalCheck = function(times, promiseChain) {
                return promiseChain.then(function() {
                    // step the clock forward to 1 ms before the interval
                    increaseFakeTime(maxIntervalMs - 1);

                    // 1 ms later the poll should happen
                    increaseFakeTime(1);

                    return waitsFor(
                        function() {
                            return dispatcherSpy.sendPublication.calls.count() === times;
                        },
                        "dispatcherSpy.sendPublication " + times + " times",
                        asyncGetterCallDelay
                    ).then(function() {
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
                .then(function() {
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
            it("restores a persisted subscription request correctly", function(done) {
                publicationManager.addPublicationProvider(providerId, provider);
                publicationManager.handleSubscriptionRequest(
                    proxyId,
                    providerId,
                    intervalSubscriptionRequest,
                    callbackDispatcher
                );
                //now, a valid subscription should be correctly persisted -> let's restore

                var publicationManagerWithRestore = new PublicationManager(dispatcherSpy, persistency, joynrInstanceId);
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

            it("restores a persisted event subscription request correctly", function(done) {
                publicationManager.addPublicationProvider(providerId, provider);
                publicationManager.handleBroadcastSubscriptionRequest(
                    proxyId,
                    providerId,
                    onChangeBroadcastSubscriptionRequest,
                    callbackDispatcher
                );

                //now, a valid subscription should be correctly persisted -> let's restore

                var publicationManagerWithRestore = new PublicationManager(dispatcherSpy, persistency, joynrInstanceId);
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

            it("restores a persisted multicast subscription request correctly", function(done) {
                publicationManager.addPublicationProvider(providerId, provider);
                var request = handleMulticastSubscriptionRequest();

                //now, a valid subscription should be correctly persisted -> let's restore

                var publicationManagerWithRestore = new PublicationManager(dispatcherSpy, persistency, joynrInstanceId);
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

        it("does not publish when interval subscription has an endDate in the past", function(done) {
            var times;

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

            originalSetTimeout(function() {
                expect(testAttribute.get).not.toHaveBeenCalled();
                expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();
                // cleanup
                stopSubscription(intervalSubscriptionRequest);
                done();
            }, asyncGetterCallDelay);
        });

        it("removes an interval attribute subscription after endDate", function(done) {
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
                function() {
                    return dispatcherSpy.sendPublication.calls.count() > 0;
                },
                "dispatcherSpy.sendPublication",
                asyncGetterCallDelay
            )
                .then(function() {
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
                        function() {
                            return dispatcherSpy.sendPublication.calls.count() > 0;
                        },
                        "dispatcherSpy.sendPublication",
                        asyncGetterCallDelay
                    );
                })
                .then(function() {
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
                    originalSetTimeout(function() {
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

        it("removes an interval attribute subscription after subscription stop", function(done) {
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
                function() {
                    return dispatcherSpy.sendPublication.calls.count() > 0;
                },
                "timeout dispatcherSpy.sendPublication",
                asyncGetterCallDelay
            )
                .then(function() {
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

        it("creates a working onChange subscription", function(done) {
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

            for (times = 0; times < maxNrOfTimes; times++) {
                increaseFakeTime(50);
                testAttribute.valueChanged(value + times);
                expect(dispatcherSpy.sendPublication.calls.count()).toEqual(times + 1);
            }

            // cleanup
            stopSubscription(onChangeSubscriptionRequest);
            expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeFalsy();
            done();
        });

        it("does not publish when an onChange subscription has an endDate in the past", function(done) {
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

            testAttribute.valueChanged(value + times);

            increaseFakeTime(asyncGetterCallDelay);

            originalSetTimeout(function() {
                expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();

                // cleanup
                stopSubscription(onChangeSubscriptionRequest);
                expect(
                    publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)
                ).toBeFalsy();
                done();
            }, asyncGetterCallDelay);
        });

        it("removes an onChange attribute subscription after endDate", function(done) {
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
                function() {
                    return dispatcherSpy.sendPublication.calls.count() > 0;
                },
                "timeout dispatcherSpy.sendPublication",
                10
            )
                .then(function() {
                    // reset first publication
                    testAttribute.get.calls.reset();
                    dispatcherSpy.sendPublication.calls.reset();

                    increaseFakeTime(50);
                    testAttribute.valueChanged(value);
                    return waitsFor(
                        function() {
                            return dispatcherSpy.sendPublication.calls.count() > 0;
                        },
                        "timeout dispatcherSpy.sendPublication after valueChanged",
                        10
                    );
                })
                .then(function() {
                    expect(dispatcherSpy.sendPublication).toHaveBeenCalled();

                    // after another attribute change, the methods should not have been
                    // called again (ie subscription terminated)
                    increaseFakeTime(subscriptionLength);
                    dispatcherSpy.sendPublication.calls.reset();

                    testAttribute.valueChanged(value);

                    originalSetTimeout(function() {
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

        it("removes an onChange attribute subscription after subscription stop", function(done) {
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
                function() {
                    return dispatcherSpy.sendPublication.calls.count() > 0;
                },
                "timeout dispatcherSpy.sendPublication",
                10
            )
                .then(function() {
                    // reset first publication
                    testAttribute.get.calls.reset();
                    dispatcherSpy.sendPublication.calls.reset();

                    increaseFakeTime(50);
                    testAttribute.valueChanged(value);
                    return waitsFor(
                        function() {
                            return dispatcherSpy.sendPublication.calls.count() > 0;
                        },
                        "timeout dispatcherSpy.sendPublication",
                        10
                    );
                })
                .then(function() {
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
                    originalSetTimeout(function() {
                        expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();
                        done();
                    }, asyncGetterCallDelay);
                    return null;
                })
                .catch(fail);
        });

        it("creates a mixed subscription and does not send two publications within minintervalMs in case async getter calls and valueChanged occur at the same time", function(
            done
        ) {
            var times;

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
                .then(function() {
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
                        function() {
                            return dispatcherSpy.sendPublication.calls.count() === 1;
                        },
                        "timeout dispatcherSpy.sendPublication",
                        asyncGetterCallDelay
                    );
                })
                .then(function() {
                    expect(dispatcherSpy.sendPublication.calls.count()).toEqual(1);
                    // now, lets increas the time until mininterval
                    increaseFakeTime(minIntervalMs - 5);
                })
                .then(function() {
                    // now, the async timer has exceeded, and the PublicationManager
                    // invokes the get
                    expect(asyncTestAttribute.get.calls.count()).toEqual(1);

                    // now change the attribute value
                    asyncTestAttribute.valueChanged(value);
                })
                .then(function() {
                    return waitsFor(
                        function() {
                            return dispatcherSpy.sendPublication.calls.count() === 2;
                        },
                        "timeout dispatcherSpy.sendPublication",
                        asyncGetterCallDelay
                    );
                })
                .then(function() {
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
                        function() {
                            return asyncTestAttribute.get.calls.count() === 2;
                        },
                        "timeout asyncTestAttribute.get",
                        asyncGetterCallDelay
                    );
                })
                .then(function() {
                    expect(asyncTestAttribute.get.calls.count()).toEqual(2);

                    increaseFakeTime(asyncGetterCallDelay);
                    return waitsFor(
                        function() {
                            return dispatcherSpy.sendPublication.calls.count() === 3;
                        },
                        "timeout dispatcherSpy.sendPublication",
                        asyncGetterCallDelay
                    );
                })
                .then(function() {
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

        it("creates a mixed subscription that publishes every maxIntervalMs", function(done) {
            var times;

            publicationManager.addPublicationProvider(providerId, provider);
            publicationManager.handleSubscriptionRequest(
                proxyId,
                providerId,
                mixedSubscriptionRequest,
                callbackDispatcher
            );
            expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeTruthy();
            increaseFakeTime(1);

            var promiseChain = waitsFor(
                function() {
                    return dispatcherSpy.sendPublication.calls.count() > 0;
                },
                "timeout dispatcherSpy.sendPublication",
                asyncGetterCallDelay
            ).then(function() {
                // reset first publication
                testAttribute.get.calls.reset();
                dispatcherSpy.sendPublication.calls.reset();
                expect(testAttribute.get).not.toHaveBeenCalled();
                expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();
            });

            var internalCheck = function(times, promiseChain) {
                return promiseChain.then(function() {
                    // step the clock forward to 1 ms before the interval
                    increaseFakeTime(maxIntervalMs - 1);
                    // 1 ms later the poll should happen
                    increaseFakeTime(1);

                    return waitsFor(
                        function() {
                            return dispatcherSpy.sendPublication.calls.count() === times + 1;
                        },
                        "timeout testAttribute.get",
                        asyncGetterCallDelay
                    ).then(function() {
                        expect(testAttribute.get.calls.count()).toEqual(times + 1);
                        expect(dispatcherSpy.sendPublication.calls.count()).toEqual(times + 1);
                    });
                });
            };

            for (times = 0; times < maxNrOfTimes; ++times) {
                promiseChain = internalCheck(times, promiseChain);
            }

            promiseChain
                .then(function() {
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

        it("creates a mixed subscription that publishes valueChanges that occur each minIntervalMs", function(done) {
            publicationManager.addPublicationProvider(providerId, provider);
            publicationManager.handleSubscriptionRequest(
                proxyId,
                providerId,
                mixedSubscriptionRequest,
                callbackDispatcher
            );
            expect(publicationManager.hasSubscriptionsForProviderAttribute(providerId, testAttributeName)).toBeTruthy();
            increaseFakeTime(1);

            var promiseChain = waitsFor(
                function() {
                    return dispatcherSpy.sendPublication.calls.count() === 1;
                },
                "timeout dispatcherSpy.sendPublication",
                asyncGetterCallDelay
            ).then(function() {
                // reset first publication
                testAttribute.get.calls.reset();
                dispatcherSpy.sendPublication.calls.reset();
            });

            var internalCheck = function(times, promiseChain) {
                return promiseChain.then(function() {
                    increaseFakeTime(mixedSubscriptionRequest.qos.minIntervalMs);
                    testAttribute.valueChanged(value + times);

                    return waitsFor(
                        function() {
                            return dispatcherSpy.sendPublication.calls.count() === times + 1;
                        },
                        "timeout dispatcherSpy.sendPublication",
                        asyncGetterCallDelay
                    ).then(function() {
                        expect(testAttribute.get).not.toHaveBeenCalled();
                        expect(dispatcherSpy.sendPublication.calls.count()).toEqual(times + 1);
                    });
                });
            };

            for (times = 0; times < maxNrOfTimes; times++) {
                promiseChain = internalCheck(times, promiseChain);
            }

            promiseChain
                .then(function() {
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

        it("creates a mixed subscription that publishes valueChanges that occur each maxIntervalMs-1", function(done) {
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

            var promiseChain = waitsFor(
                function() {
                    return dispatcherSpy.sendPublication.calls.count() === 1;
                },
                "timeout dispatcherSpy.sendPublication",
                asyncGetterCallDelay
            ).then(function() {
                // reset first publication
                testAttribute.get.calls.reset();
                dispatcherSpy.sendPublication.calls.reset();
            });

            var internalCheck = function(times, promiseChain) {
                return promiseChain.then(function() {
                    increaseFakeTime(mixedSubscriptionRequest.qos.maxIntervalMs - 2);
                    testAttribute.valueChanged(value + times);
                    increaseFakeTime(1);

                    return waitsFor(
                        function() {
                            return dispatcherSpy.sendPublication.calls.count() === times + 1;
                        },
                        "timeout dispatcherSpy.sendPublication",
                        asyncGetterCallDelay
                    ).then(function() {
                        expect(testAttribute.get).not.toHaveBeenCalled();
                        expect(dispatcherSpy.sendPublication.calls.count()).toEqual(times + 1);
                    });
                });
            };
            for (times = 0; times < maxNrOfTimes; times++) {
                promiseChain = internalCheck(times, promiseChain);
            }

            promiseChain
                .then(function() {
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
        it("creates a mixed subscription that publishes many valueChanges within minIntervalMs only once", function(
            done
        ) {
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
                function() {
                    return dispatcherSpy.sendPublication.calls.count() === 1;
                },
                "timeout dispatcherSpy.sendPublication",
                asyncGetterCallDelay
            )
                .then(function() {
                    // reset first publication
                    testAttribute.get.calls.reset();
                    dispatcherSpy.sendPublication.calls.reset();

                    var shortInterval = Math.round((mixedSubscriptionRequest.qos.minIntervalMs - 2) / maxNrOfTimes);
                    for (times = 0; times < maxNrOfTimes; times++) {
                        expect(testAttribute.get).not.toHaveBeenCalled();
                        expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();
                        increaseFakeTime(shortInterval);
                        testAttribute.valueChanged(value + times);
                    }

                    // after minIntervalMs the publication works again
                    increaseFakeTime(mixedSubscriptionRequest.qos.minIntervalMs - shortInterval * maxNrOfTimes);
                })
                .then(function() {
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

        it("creates a periodic subscription without expiryDate and expects periodic publications", function(done) {
            var periodMs = 400,
                n = 10,
                subscriptionRequestWithoutExpiryDate;

            subscriptionRequestWithoutExpiryDate = createSubscriptionRequest(
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

            var promiseChain = waitsFor(
                function() {
                    return dispatcherSpy.sendPublication.calls.count() === 1;
                },
                "timeout dispatcherSpy.sendPublication",
                asyncGetterCallDelay
            ).then(function() {
                expect(dispatcherSpy.sendPublication.calls.count()).toEqual(1);
                dispatcherSpy.sendPublication.calls.reset();
            });

            var checkMaxIntervalCalls = function(i, promiseChain) {
                return promiseChain
                    .then(function() {
                        increaseFakeTime(periodMs);

                        return waitsFor(
                            function() {
                                return dispatcherSpy.sendPublication.calls.count() === 1 + i;
                            },
                            "timeout " + i + " times dispatcherSpy.sendPublication call",
                            asyncGetterCallDelay
                        );
                    })
                    .then(function() {
                        expect(dispatcherSpy.sendPublication.calls.count()).toEqual(1 + i);
                    });
            };

            var i;
            for (i = 0; i < n; i++) {
                promiseChain = checkMaxIntervalCalls(i, promiseChain);
            }

            promiseChain
                .then(function() {
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
        it("creates a mixed subscription that publishes correctly with all cases mixed", function(done) {
            var i;

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
                function() {
                    return dispatcherSpy.sendPublication.calls.count() === 1;
                },
                "timeout dispatcherSpy.sendPublication",
                asyncGetterCallDelay
            )
                .then(function() {
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
                        function() {
                            return dispatcherSpy.sendPublication.calls.count() === 1;
                        },
                        "timeout dispatcherSpy.sendPublication call",
                        asyncGetterCallDelay
                    );
                })
                .then(function() {
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
                .then(function() {
                    expect(testAttribute.get.calls.count()).toEqual(1);
                })
                .then(function() {
                    return waitsFor(
                        function() {
                            return dispatcherSpy.sendPublication.calls.count() === 1;
                        },
                        "timeout dispatcherSpy.sendPublication call ",
                        asyncGetterCallDelay
                    );
                })
                .then(function() {
                    expect(dispatcherSpy.sendPublication.calls.count()).toEqual(1);

                    // value change after mixedSubscriptionRequest.qos.maxInterval - 2=> publication sent
                    dispatcherSpy.sendPublication.calls.reset();
                    testAttribute.get.calls.reset();

                    increaseFakeTime(mixedSubscriptionRequest.qos.maxIntervalMs - 1);
                    testAttribute.valueChanged(value);
                    expect(testAttribute.get).not.toHaveBeenCalled();
                    return waitsFor(
                        function() {
                            return dispatcherSpy.sendPublication.calls.count() === 1;
                        },
                        "timeout dispatcherSpy.sendPublication call ",
                        asyncGetterCallDelay
                    );
                })
                .then(function() {
                    expect(dispatcherSpy.sendPublication.calls.count()).toEqual(1);

                    // after mixedSubscriptionRequest.qos.maxIntervalMs => interval
                    // publication is sent
                    dispatcherSpy.sendPublication.calls.reset();
                    increaseFakeTime(mixedSubscriptionRequest.qos.maxIntervalMs);
                    return waitsFor(
                        function() {
                            return dispatcherSpy.sendPublication.calls.count() === 1;
                        },
                        "timeout dispatcherSpy.sendPublication call ",
                        asyncGetterCallDelay
                    );
                })
                .then(function() {
                    expect(testAttribute.get.calls.count()).toEqual(1);
                    expect(dispatcherSpy.sendPublication.calls.count()).toEqual(1);

                    // after another mixedSubscriptionRequest.qos.maxIntervalMs =>
                    // interval publication is sent
                    testAttribute.get.calls.reset();
                    dispatcherSpy.sendPublication.calls.reset();
                    increaseFakeTime(mixedSubscriptionRequest.qos.maxIntervalMs);
                    return waitsFor(
                        function() {
                            return dispatcherSpy.sendPublication.calls.count() === 1;
                        },
                        "timeout dispatcherSpy.sendPublication call ",
                        asyncGetterCallDelay
                    );
                })
                .then(function() {
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
        it("does not publish when mixed subscription has an endDate in the past", function(done) {
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

        it("removes a mixed attribute subscription after endDate", function(done) {
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
                function() {
                    return dispatcherSpy.sendPublication.calls.count() > 0;
                },
                "dispatcherSpy.sendPublication",
                asyncGetterCallDelay
            )
                .then(function() {
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

        xit("removes a mixed attribute subscription after subscription stop", function(done) {
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
                function() {
                    return dispatcherSpy.sendPublication.calls.count() > 0;
                },
                "dispatcherSpy.sendPublication",
                asyncGetterCallDelay
            )
                .then(function() {
                    // reset first publication
                    testAttribute.get.calls.reset();
                    dispatcherSpy.sendPublication.calls.reset();

                    testAttribute.valueChanged(value); // do change
                    //increaseFakeTime(maxIntervalMs); // increase interval
                    increaseFakeTime(maxIntervalMs); // increase interval

                    return waitsFor(
                        function() {
                            return dispatcherSpy.sendPublication.calls.count() > 0;
                        },
                        "dispatcherSpy.sendPublication 2",
                        asyncGetterCallDelay
                    );
                })
                .then(function() {
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

                    originalSetTimeout(function() {
                        expect(testAttribute.get).not.toHaveBeenCalled();
                        expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();
                        stopSubscription(mixedSubscriptionRequest);
                        done();
                    }, asyncGetterCallDelay);
                    return null;
                })
                .catch(fail);
        });

        it("removes a mixed broadcast subscription after subscription stop", function(done) {
            var broadcastOutputParameters = testBroadcast.createBroadcastOutputParameters();
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
                function() {
                    return dispatcherSpy.sendPublication.calls.count() > 0;
                },
                "dispatcherSpy.sendPublication",
                asyncGetterCallDelay
            )
                .then(function() {
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

                    originalSetTimeout(function() {
                        expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();
                        done();
                    }, asyncGetterCallDelay);
                    return null;
                })
                .catch(fail);
        });

        it("removes publication provider", function(done) {
            var times;
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
                function() {
                    return dispatcherSpy.sendPublication.calls.count() > 0;
                },
                "dispatcherSpy.sendPublication",
                asyncGetterCallDelay
            )
                .then(function() {
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

        it("rejects attribute subscription if expiryDateMs lies in the past", function(done) {
            var request = new SubscriptionRequest({
                subscriptionId: "subscriptionId" + uuid(),
                subscribedToName: testAttributeName,
                qos: new OnChangeSubscriptionQos({
                    expiryDateMs: Date.now() - 10000
                })
            });
            publicationManager.addPublicationProvider(providerId, provider);
            publicationManager.handleSubscriptionRequest(proxyId, providerId, request, callbackDispatcher);

            waitsFor(
                function() {
                    return callbackDispatcher.calls.count() === 1;
                },
                "callbackDispatcher got called",
                1000
            )
                .then(function() {
                    expect(callbackDispatcher).toHaveBeenCalled();
                    var error = callbackDispatcher.calls.mostRecent().args[1].error;
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

        it("rejects attribute subscription if attribute does not exist", function(done) {
            var request = new SubscriptionRequest({
                subscriptionId: "subscriptionId" + uuid(),
                subscribedToName: "nonExistingAttribute",
                qos: new OnChangeSubscriptionQos()
            });
            publicationManager.addPublicationProvider(providerId, provider);
            publicationManager.handleSubscriptionRequest(proxyId, providerId, request, callbackDispatcher);

            waitsFor(
                function() {
                    return callbackDispatcher.calls.count() === 1;
                },
                "callbackDispatcher got called",
                1000
            )
                .then(function() {
                    expect(callbackDispatcher).toHaveBeenCalled();
                    var error = callbackDispatcher.calls.mostRecent().args[1].error;
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

        it("rejects attribute subscription if attribute is not notifiable", function(done) {
            var request = new SubscriptionRequest({
                subscriptionId: "subscriptionId" + uuid(),
                subscribedToName: testAttributeNotNotifiableName,
                qos: new OnChangeSubscriptionQos()
            });
            publicationManager.addPublicationProvider(providerId, provider);
            publicationManager.handleSubscriptionRequest(proxyId, providerId, request, callbackDispatcher);

            waitsFor(
                function() {
                    return callbackDispatcher.calls.count() === 1;
                },
                "callbackDispatcher got called",
                1000
            )
                .then(function() {
                    expect(callbackDispatcher).toHaveBeenCalled();
                    var error = callbackDispatcher.calls.mostRecent().args[1].error;
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

        it("rejects attribute subscription if periodMs is too small", function(done) {
            var qosSettings = new PeriodicSubscriptionQos({
                expiryDateMs: 0,
                alertAfterIntervalMs: 0,
                publicationTtlMs: 1000
            });
            // forcibly fake it! The constructor throws, if using this directly
            qosSettings.periodMs = PeriodicSubscriptionQos.MIN_PERIOD_MS - 1;

            var request = new SubscriptionRequest({
                subscriptionId: "subscriptionId" + uuid(),
                subscribedToName: testAttributeName,
                qos: qosSettings
            });
            publicationManager.addPublicationProvider(providerId, provider);
            publicationManager.handleSubscriptionRequest(proxyId, providerId, request, callbackDispatcher);

            waitsFor(
                function() {
                    return callbackDispatcher.calls.count() === 1;
                },
                "callbackDispatcher got called",
                1000
            )
                .then(function() {
                    expect(callbackDispatcher).toHaveBeenCalled();
                    var error = callbackDispatcher.calls.mostRecent().args[1].error;
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

        it("rejects broadcast subscription if expiryDateMs lies in the past", function(done) {
            var request = new BroadcastSubscriptionRequest({
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
                function() {
                    return callbackDispatcher.calls.count() === 1;
                },
                "callbackDispatcher got called",
                1000
            )
                .then(function() {
                    expect(callbackDispatcher).toHaveBeenCalled();
                    var error = callbackDispatcher.calls.mostRecent().args[1].error;
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

        it("rejects broadcast subscription if filter parameters are wrong", function(done) {
            var request = new BroadcastSubscriptionRequest({
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
                function() {
                    return callbackDispatcher.calls.count() === 1;
                },
                "callbackDispatcher got called",
                1000
            )
                .then(function() {
                    expect(callbackDispatcher).toHaveBeenCalled();
                    var error = callbackDispatcher.calls.mostRecent().args[1].error;
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

        it("registers multicast subscription", function(done) {
            expect(publicationManager.hasMulticastSubscriptions()).toBe(false);
            publicationManager.addPublicationProvider(providerId, provider);

            var request = handleMulticastSubscriptionRequest();

            waitsFor(
                function() {
                    return callbackDispatcher.calls.count() === 1;
                },
                "callbackDispatcher got called",
                1000
            )
                .then(function() {
                    expect(callbackDispatcher).toHaveBeenCalled();
                    var response = callbackDispatcher.calls.mostRecent().args[1];
                    expect(response.error).toBeUndefined();
                    expect(response.subscriptionId).toEqual(request.subscriptionId);
                    expect(publicationManager.hasMulticastSubscriptions()).toBe(true);
                    done();
                    return null;
                })
                .catch(fail);
            increaseFakeTime(1);
        });

        it("registers and unregisters multicast subscription", function() {
            expect(publicationManager.hasMulticastSubscriptions()).toBe(false);
            publicationManager.addPublicationProvider(providerId, provider);

            var request = handleMulticastSubscriptionRequest();
            expect(publicationManager.hasMulticastSubscriptions()).toBe(true);

            expect(publicationManager.hasSubscriptions()).toBe(true);
            publicationManager.handleSubscriptionStop({
                subscriptionId: request.subscriptionId
            });
            expect(publicationManager.hasMulticastSubscriptions()).toBe(false);
            expect(publicationManager.hasSubscriptions()).toBe(false);
        });

        it("registers for multicast subscription and sends multicast publication", function() {
            var broadcastOutputParameters = testNonSelectiveBroadcast.createBroadcastOutputParameters();
            broadcastOutputParameters.setParam1("param1");

            expect(publicationManager.hasMulticastSubscriptions()).toBe(false);
            publicationManager.addPublicationProvider(providerId, provider);

            var request = handleMulticastSubscriptionRequest();
            expect(publicationManager.hasMulticastSubscriptions()).toBe(true);

            testNonSelectiveBroadcast.fire(broadcastOutputParameters);

            expect(dispatcherSpy.sendMulticastPublication).toHaveBeenCalled();

            var settings = dispatcherSpy.sendMulticastPublication.calls.argsFor(0)[0];
            var multicastPublication = dispatcherSpy.sendMulticastPublication.calls.argsFor(0)[1];

            expect(multicastPublication.multicastId).toEqual(request.multicastId);

            publicationManager.handleSubscriptionStop({
                subscriptionId: request.subscriptionId
            });
            expect(publicationManager.hasMulticastSubscriptions()).toBe(false);
        });

        it("rejects broadcast subscription if broadcast does not exist", function(done) {
            var request = new BroadcastSubscriptionRequest({
                subscriptionId: "subscriptionId" + uuid(),
                subscribedToName: "nonExistingBroadcast",
                qos: new OnChangeSubscriptionQos(),
                filterParameters: {}
            });
            publicationManager.addPublicationProvider(providerId, provider);
            publicationManager.handleBroadcastSubscriptionRequest(proxyId, providerId, request, callbackDispatcher);

            waitsFor(
                function() {
                    return callbackDispatcher.calls.count() === 1;
                },
                "callbackDispatcher got called",
                1000
            )
                .then(function() {
                    expect(callbackDispatcher).toHaveBeenCalled();
                    var error = callbackDispatcher.calls.mostRecent().args[1].error;
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

        it(" throws exception when called while shut down", function(done) {
            publicationManager.shutdown();
            var callbackDispatcherSpy = jasmine.createSpy("callbackDispatcherSpy");

            expect(function() {
                publicationManager.removePublicationProvider("providerParticipantId", {});
            }).toThrow();

            expect(function() {
                publicationManager.addPublicationProvider("providerParticipantId", {});
            }).toThrow();

            expect(function() {
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
                function() {
                    return callbackDispatcherSpy.calls.count() === 1;
                },
                "callbackDispatcher for attributes got called",
                1000
            )
                .then(function() {
                    expect(callbackDispatcherSpy).toHaveBeenCalled();
                    expect(callbackDispatcherSpy.calls.argsFor(0)[1] instanceof SubscriptionReply);
                    expect(callbackDispatcherSpy.calls.argsFor(0)[1].error instanceof SubscriptionException);
                    return null;
                })
                .then(function() {
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
                        function() {
                            return callbackDispatcherSpy.calls.count() === 1;
                        },
                        "callbackDispatcher for events got called",
                        1000
                    ).then(function() {
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
