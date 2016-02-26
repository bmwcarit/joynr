/*global joynrTestRequire: true, xit: true */

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
        "joynr/dispatching/subscription/TestPublicationManager",
        [
            "global/Promise",
            "joynr/dispatching/subscription/PublicationManager",
            "joynr/messaging/MessagingQos",
            "joynr/dispatching/types/SubscriptionRequest",
            "joynr/dispatching/types/BroadcastSubscriptionRequest",
            "joynr/dispatching/types/SubscriptionStop",
            "joynr/provider/ProviderAttribute",
            "joynr/provider/ProviderEvent",
            "joynr/proxy/PeriodicSubscriptionQos",
            "joynr/proxy/SubscriptionQos",
            "joynr/proxy/OnChangeSubscriptionQos",
            "joynr/proxy/OnChangeWithKeepAliveSubscriptionQos",
            "joynr/types/ProviderQos",
            "joynr/types/ProviderScope",
            "joynr/dispatching/types/SubscriptionPublication",
            "joynr/util/LongTimer",
            "uuid",
            "Date",
            "global/LocalStorage"
        ],
        function(
                Promise,
                PublicationManager,
                MessagingQos,
                SubscriptionRequest,
                BroadcastSubscriptionRequest,
                SubscriptionStop,
                ProviderAttribute,
                ProviderEvent,
                PeriodicSubscriptionQos,
                SubscriptionQos,
                OnChangeSubscriptionQos,
                OnChangeWithKeepAliveSubscriptionQos,
                ProviderQos,
                ProviderScope,
                SubscriptionPublication,
                LongTimer,
                uuid,
                Date,
                LocalStorage) {
            var localStorage = new LocalStorage();
            describe(
                    "libjoynr-js.joynr.dispatching.subscription.PublicationManager",
                    function() {
                        var proxyId, providerId, publicationManager, joynrInstanceId, dispatcherSpy;
                        var provider, asyncGetterCallDelay, fakeTime, intervalSubscriptionRequest;
                        var onChangeSubscriptionRequest, mixedSubscriptionRequest, onChangeBroadcastSubscriptionRequest;
                        var mixedSubscriptionRequestWithAsyncAttribute, testAttributeName;
                        var asyncTestAttributeName, value, minIntervalMs, maxInterval, maxNrOfTimes;
                        var subscriptionLength, asyncTestAttribute, testAttribute, providerSettings;
                        var testBroadcastName, testBroadcast;

                        function createSubscriptionRequest(
                                isAttribute,
                                subscribeToName,
                                period,
                                subscriptionLength,
                                onChange,
                                minIntervalMs) {
                            var qosSettings, expiryDateMs, request;
                            expiryDateMs =
                                    subscriptionLength === SubscriptionQos.NO_EXPIRY_DATE
                                            ? SubscriptionQos.NO_EXPIRY_DATE
                                            : Date.now() + subscriptionLength;
                            if (onChange) {
                                if (period !== undefined) {
                                    qosSettings = new OnChangeWithKeepAliveSubscriptionQos({
                                        minIntervalMs : minIntervalMs || 50,
                                        maxInterval : period,
                                        expiryDateMs : expiryDateMs,
                                        alertAfterInterval : 0,
                                        publicationTtlMs : 1000
                                    });
                                } else {
                                    qosSettings = new OnChangeSubscriptionQos({
                                        minIntervalMs : minIntervalMs || 50,
                                        expiryDateMs : expiryDateMs,
                                        publicationTtlMs : 1000
                                    });
                                }
                            } else {
                                qosSettings = new PeriodicSubscriptionQos({
                                    period : period,
                                    expiryDateMs : expiryDateMs,
                                    alertAfterInterval : 0,
                                    publicationTtlMs : 1000
                                });
                            }

                            if (isAttribute) {
                                request = new SubscriptionRequest({
                                    subscriptionId : "subscriptionId" + uuid(),
                                    subscribedToName : subscribeToName,
                                    qos : qosSettings
                                });
                            } else {
                                request = new BroadcastSubscriptionRequest({
                                    subscriptionId : "subscriptionId" + uuid(),
                                    subscribedToName : subscribeToName,
                                    qos : qosSettings,
                                    filterParameters : {}
                                });
                            }
                            return request;
                        }

                        function resetFakeTime() {
                            fakeTime = 0;
                            jasmine.Clock.reset();
                        }

                        function increaseFakeTime(time_ms) {
                            fakeTime += time_ms;
                            jasmine.Clock.tick(time_ms);
                        }

                        // have to define our own ProviderAttributeNotifyReadWrite because we need
                        // the phrase "Notify" in the constructor
                        // function name for the SubscriptionPublication Manager to work correctly,
                        // but we can't use the
                        // ProviderAttributeNotifyReadWrite implementation because it freezes its
                        // members for safe public usage => we cannot
                        // spy on functions of a real ProviderAttributeNotifyReadWrite instance
                        function ProviderAttributeNotifyReadWrite(
                                parent,
                                settings,
                                attributeName,
                                attributeType) {
                            var providerAttribute =
                                    new ProviderAttribute(
                                            parent,
                                            settings,
                                            attributeName,
                                            attributeType,
                                            "NOTIFYREADWRITE");
                            this.registerGetter = providerAttribute.registerGetter;
                            this.get = providerAttribute.get;
                            this.registerSetter = providerAttribute.registerSetter;
                            this.set = providerAttribute.set;
                            this.valueChanged = providerAttribute.valueChanged;
                            this.registerObserver = providerAttribute.registerObserver;
                            this.unregisterObserver = providerAttribute.unregisterObserver;
                        }

                        function stopSubscription(subscriptionInfo) {
                            publicationManager.handleSubscriptionStop(new SubscriptionStop({
                                subscriptionId : subscriptionInfo.subscriptionId
                            }));
                        }

                        /**
                         * Called before each test.
                         */
                        beforeEach(function() {
                            //jasmine.getEnv().currentSpec.description;
                            proxyId = "proxy" + uuid();
                            providerId = "provider" + uuid();
                            joynrInstanceId = uuid();
                            fakeTime = 123456789;
                            testAttributeName = "testAttribute";
                            testBroadcastName = "testBroadcast";
                            asyncTestAttributeName = "asyncTestAttribute";
                            value = "the value";
                            minIntervalMs = 100;
                            maxInterval = 1000;
                            maxNrOfTimes = 5;
                            asyncGetterCallDelay = 10;
                            subscriptionLength = (maxNrOfTimes + 1) * maxInterval;

                            dispatcherSpy = jasmine.createSpyObj("Dispatcher", [ "sendPublication"
                            ]);
                            publicationManager =
                                    new PublicationManager(
                                            dispatcherSpy,
                                            localStorage,
                                            joynrInstanceId);

                            provider =
                                    jasmine.createSpyObj("Provider", [ "registerOnChangeListener"
                                    ]);
                            provider.id = uuid();

                            providerSettings = {
                                id : provider.id,
                                providerQos : new ProviderQos({
                                    version : 123,
                                    priority : 1234,
                                    scope : ProviderScope.GLOBAL
                                }),
                                dependencies : {
                                    publicationManager : publicationManager
                                }
                            };

                            testBroadcast =
                                    new ProviderEvent(
                                            provider,
                                            providerSettings,
                                            testBroadcastName,
                                            [ {
                                                name : "param1",
                                                type : "String"
                                            }
                                            ],
                                            {});
                            testAttribute =
                                    new ProviderAttributeNotifyReadWrite(
                                            provider,
                                            providerSettings,
                                            testAttributeName,
                                            "Boolean");

                            asyncTestAttribute =
                                    new ProviderAttributeNotifyReadWrite(
                                            provider,
                                            providerSettings,
                                            asyncTestAttributeName,
                                            "Boolean");

                            provider[testAttributeName] = testAttribute;
                            provider[testBroadcastName] = testBroadcast;
                            spyOn(testAttribute, "get").andReturn("attributeValue");

                            provider[asyncTestAttributeName] = asyncTestAttribute;
                            spyOn(asyncTestAttribute, "get").andCallFake(function() {
                                return new Promise(function(resolve, reject) {
                                    LongTimer.setTimeout(function() {
                                        resolve("asyncAttributeValue");
                                    }, asyncGetterCallDelay);
                                });
                            });

                            jasmine.Clock.useMock();
                            jasmine.Clock.reset();
                            spyOn(Date, "now").andCallFake(function() {
                                return fakeTime;
                            });

                            intervalSubscriptionRequest =
                                    createSubscriptionRequest(
                                            true,
                                            testAttributeName,
                                            maxInterval,
                                            subscriptionLength);
                            onChangeSubscriptionRequest =
                                    createSubscriptionRequest(
                                            true,
                                            testAttributeName,
                                            undefined,
                                            subscriptionLength,
                                            true);
                            mixedSubscriptionRequest =
                                    createSubscriptionRequest(
                                            true,
                                            testAttributeName,
                                            maxInterval,
                                            subscriptionLength,
                                            true,
                                            minIntervalMs);
                            onChangeBroadcastSubscriptionRequest =
                                    createSubscriptionRequest(
                                            false,
                                            testBroadcastName,
                                            undefined,
                                            subscriptionLength,
                                            true);
                            mixedSubscriptionRequestWithAsyncAttribute =
                                    createSubscriptionRequest(
                                            true,
                                            asyncTestAttributeName,
                                            maxInterval,
                                            subscriptionLength,
                                            true,
                                            minIntervalMs);
                        });
                        it("is instantiable", function() {
                            expect(publicationManager).toBeDefined();
                            expect(function() {
                                var p = new PublicationManager(dispatcherSpy, localStorage);
                            }).not.toThrow();
                        });

                        it(
                                "calls dispatcher with correct arguments",
                                function() {
                                    runs(function() {
                                        publicationManager.addPublicationProvider(
                                                providerId,
                                                provider);
                                        publicationManager.handleSubscriptionRequest(
                                                proxyId,
                                                providerId,
                                                onChangeSubscriptionRequest);
                                        expect(
                                                publicationManager
                                                        .hasSubscriptionsForProviderAttribute(
                                                                provider.id,
                                                                testAttributeName)).toBeTruthy();
                                        increaseFakeTime(1);
                                    });

                                    waitsFor(function() {
                                        return dispatcherSpy.sendPublication.callCount > 0;
                                    }, "dispatcherSpy.sendPublication", asyncGetterCallDelay);

                                    runs(function() {
                                        // reset first publication
                                        testAttribute.get.reset();
                                        dispatcherSpy.sendPublication.reset();

                                        increaseFakeTime(50);

                                        testAttribute.valueChanged(value);
                                    });

                                    waitsFor(function() {
                                        return dispatcherSpy.sendPublication.callCount > 0;
                                    }, "dispatcherSpy.sendPublication", asyncGetterCallDelay);

                                    runs(function() {
                                        expect(dispatcherSpy.sendPublication).toHaveBeenCalled();
                                        expect(dispatcherSpy.sendPublication)
                                                .toHaveBeenCalledWith(
                                                        {
                                                            from : providerId,
                                                            to : proxyId,
                                                            expiryDate : (Date.now() + onChangeSubscriptionRequest.qos.publicationTtlMs)
                                                                    .toString()
                                                        },
                                                        new SubscriptionPublication(
                                                                {
                                                                    response : [ value
                                                                    ],
                                                                    subscriptionId : onChangeSubscriptionRequest.subscriptionId
                                                                }));
                                        stopSubscription(onChangeSubscriptionRequest);
                                        expect(
                                                publicationManager
                                                        .hasSubscriptionsForProviderAttribute(
                                                                provider.id,
                                                                testAttributeName)).toBeFalsy();
                                    });
                                });

                        it("publishes first value once immediately after subscription", function() {
                            var times;

                            runs(function() {
                                publicationManager.addPublicationProvider(providerId, provider);
                                expect(testAttribute.get).not.toHaveBeenCalled();
                                expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();
                                publicationManager.handleSubscriptionRequest(
                                        proxyId,
                                        providerId,
                                        onChangeSubscriptionRequest);
                                expect(
                                        publicationManager.hasSubscriptionsForProviderAttribute(
                                                provider.id,
                                                testAttributeName)).toBeTruthy();
                                increaseFakeTime(1);

                            });

                            waitsFor(function() {
                                return dispatcherSpy.sendPublication.callCount > 0;
                            }, "dispatcherSpy.sendPublication", asyncGetterCallDelay);

                            runs(function() {
                                expect(testAttribute.get.callCount).toEqual(1);
                                expect(dispatcherSpy.sendPublication.callCount).toEqual(1);
                                // cleanup
                                stopSubscription(onChangeSubscriptionRequest);
                                expect(
                                        publicationManager.hasSubscriptionsForProviderAttribute(
                                                provider.id,
                                                testAttributeName)).toBeFalsy();
                            });

                        });

                        it(
                                "creates a working interval subscription",
                                function() {
                                    var times;

                                    runs(function() {
                                        publicationManager.addPublicationProvider(
                                                providerId,
                                                provider);
                                        publicationManager.handleSubscriptionRequest(
                                                proxyId,
                                                providerId,
                                                intervalSubscriptionRequest);
                                        expect(
                                                publicationManager
                                                        .hasSubscriptionsForProviderAttribute(
                                                                provider.id,
                                                                testAttributeName)).toBeTruthy();
                                        increaseFakeTime(1);
                                    });

                                    waitsFor(function() {
                                        return dispatcherSpy.sendPublication.callCount > 0;
                                    }, "dispatcherSpy.sendPublication", asyncGetterCallDelay);

                                    runs(function() {
                                        // reset first publication
                                        testAttribute.get.reset();
                                        dispatcherSpy.sendPublication.reset();
                                    });

                                    var internalCheck =
                                            function(times) {
                                                runs(function() {
                                                    // step the clock forward to 1 ms before the interval
                                                    increaseFakeTime(maxInterval - 1);

                                                    // 1 ms later the poll should happen
                                                    increaseFakeTime(1);
                                                });

                                                waitsFor(
                                                        function() {
                                                            return dispatcherSpy.sendPublication.callCount === times;
                                                        },
                                                        "dispatcherSpy.sendPublication "
                                                            + times
                                                            + " times",
                                                        asyncGetterCallDelay);

                                                runs(function() {
                                                    expect(testAttribute.get.callCount).toEqual(
                                                            times);
                                                    expect(dispatcherSpy.sendPublication.callCount)
                                                            .toEqual(times);
                                                });
                                            };

                                    for (times = 1; times < maxNrOfTimes + 1; ++times) {
                                        internalCheck(times);
                                    }

                                    runs(function() {
                                        // cleanup
                                        stopSubscription(intervalSubscriptionRequest);
                                        expect(
                                                publicationManager
                                                        .hasSubscriptionsForProviderAttribute(
                                                                provider.id,
                                                                testAttributeName)).toBeFalsy();
                                    });
                                });

                        xit(
                                "creates a working interval subscription with an interval timeout > Math.pow(2, 31) - 1",
                                function() {
                                    var times;
                                    var largeInterval = Math.pow(2, 40);
                                    var largeIntervalSubscriptionRequest =
                                            createSubscriptionRequest(
                                                    true,
                                                    testAttributeName,
                                                    largeInterval,
                                                    largeInterval * times);

                                    runs(function() {
                                        resetFakeTime();
                                        publicationManager.addPublicationProvider(
                                                providerId,
                                                provider);

                                        publicationManager.handleSubscriptionRequest(
                                                proxyId,
                                                providerId,
                                                largeIntervalSubscriptionRequest);
                                        expect(
                                                publicationManager
                                                        .hasSubscriptionsForProviderAttribute(
                                                                provider.id,
                                                                testAttributeName)).toBeTruthy();
                                        increaseFakeTime(1);
                                    });

                                    waitsFor(function() {
                                        return dispatcherSpy.sendPublication.callCount > 0;
                                    }, "dispatcherSpy.sendPublication", asyncGetterCallDelay);

                                    runs(function() {
                                        // reset first publication
                                        testAttribute.get.reset();
                                        dispatcherSpy.sendPublication.reset();
                                    });

                                    var internalCheck =
                                            function(times) {
                                                runs(function() {
                                                    // step the clock forward to 1 ms before the interval
                                                    increaseFakeTime(largeInterval - 1);

                                                    // 1 ms later the poll should happen
                                                    increaseFakeTime(1);
                                                });

                                                waitsFor(
                                                        function() {
                                                            return dispatcherSpy.sendPublication.callCount === times;
                                                        },
                                                        "dispatcherSpy.sendPublication",
                                                        asyncGetterCallDelay);

                                                runs(function() {
                                                    expect(testAttribute.get.callCount).toEqual(
                                                            times + 1);
                                                    expect(dispatcherSpy.sendPublication.callCount)
                                                            .toEqual(times + 1);
                                                });
                                            };

                                    for (times = 1; times < maxNrOfTimes + 1; ++times) {
                                        internalCheck(times);
                                    }

                                    runs(function() {
                                        // cleanup
                                        stopSubscription(largeIntervalSubscriptionRequest);
                                        expect(
                                                publicationManager
                                                        .hasSubscriptionsForProviderAttribute(
                                                                provider.id,
                                                                testAttributeName)).toBeFalsy();
                                    });
                                });

                        it(
                                "does not publish when interval subscription has an endDate in the past",
                                function() {
                                    var times;

                                    runs(function() {
                                        intervalSubscriptionRequest.qos.expiryDateMs =
                                                Date.now() - 1;
                                        publicationManager.addPublicationProvider(
                                                providerId,
                                                provider);
                                        publicationManager.handleSubscriptionRequest(
                                                proxyId,
                                                providerId,
                                                intervalSubscriptionRequest);

                                        expect(
                                                publicationManager
                                                        .hasSubscriptionsForProviderAttribute(
                                                                provider.id,
                                                                testAttributeName)).toBeFalsy();
                                        // reset first publication
                                        testAttribute.get.reset();
                                        dispatcherSpy.sendPublication.reset();

                                        expect(testAttribute.get).not.toHaveBeenCalled();
                                        expect(dispatcherSpy.sendPublication).not
                                                .toHaveBeenCalled();
                                        increaseFakeTime(subscriptionLength);
                                    });

                                    runs(function() {
                                        setTimeout(function() {
                                            expect(testAttribute.get).not.toHaveBeenCalled();
                                            expect(dispatcherSpy.sendPublication).not
                                                    .toHaveBeenCalled();
                                            // cleanup
                                            stopSubscription(intervalSubscriptionRequest);
                                        }, asyncGetterCallDelay);
                                    });
                                });

                        it("removes an interval attribute subscription after endDate", function() {

                            runs(function() {
                                publicationManager.addPublicationProvider(providerId, provider);
                                publicationManager.handleSubscriptionRequest(
                                        proxyId,
                                        providerId,
                                        intervalSubscriptionRequest);
                                expect(
                                        publicationManager.hasSubscriptionsForProviderAttribute(
                                                provider.id,
                                                testAttributeName)).toBeTruthy();
                                increaseFakeTime(1);
                            });

                            waitsFor(function() {
                                return dispatcherSpy.sendPublication.callCount > 0;
                            }, "dispatcherSpy.sendPublication", asyncGetterCallDelay);

                            runs(function() {
                                // reset first publication
                                testAttribute.get.reset();
                                dispatcherSpy.sendPublication.reset();
                                increaseFakeTime(subscriptionLength);
                            });

                            waitsFor(function() {
                                return dispatcherSpy.sendPublication.callCount > 0;
                            }, "dispatcherSpy.sendPublication", asyncGetterCallDelay);

                            runs(function() {
                                expect(testAttribute.get).toHaveBeenCalled();
                                expect(dispatcherSpy.sendPublication).toHaveBeenCalled();
                                // after another interval, the methods should not have been called again
                                // (ie subscription terminated)
                                testAttribute.get.reset();
                                dispatcherSpy.sendPublication.reset();
                                increaseFakeTime(maxInterval);
                            });

                            runs(function() {
                                setTimeout(function() {
                                    expect(testAttribute.get).not.toHaveBeenCalled();
                                    expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();
                                    stopSubscription(intervalSubscriptionRequest);
                                    expect(
                                            publicationManager
                                                    .hasSubscriptionsForProviderAttribute(
                                                            provider.id,
                                                            testAttributeName)).toBeFalsy();
                                }, asyncGetterCallDelay);
                            });
                        });

                        it(
                                "removes an interval attribute subscription after subscription stop",
                                function() {
                                    runs(function() {
                                        publicationManager.addPublicationProvider(
                                                providerId,
                                                provider);
                                        publicationManager.handleSubscriptionRequest(
                                                proxyId,
                                                providerId,
                                                intervalSubscriptionRequest);

                                        expect(
                                                publicationManager
                                                        .hasSubscriptionsForProviderAttribute(
                                                                provider.id,
                                                                testAttributeName)).toBeTruthy();

                                        // reset first publication
                                        testAttribute.get.reset();
                                        dispatcherSpy.sendPublication.reset();

                                        increaseFakeTime(maxInterval + asyncGetterCallDelay);
                                    });

                                    waitsFor(
                                            function() {
                                                return dispatcherSpy.sendPublication.callCount > 0;
                                            },
                                            "timeout dispatcherSpy.sendPublication",
                                            asyncGetterCallDelay);

                                    runs(function() {
                                        expect(testAttribute.get).toHaveBeenCalled();
                                        expect(dispatcherSpy.sendPublication).toHaveBeenCalled();

                                        // after subscription stop, the methods should not have been called
                                        // again (ie subscription terminated)
                                        publicationManager
                                                .handleSubscriptionStop(new SubscriptionStop(
                                                        {
                                                            subscriptionId : intervalSubscriptionRequest.subscriptionId
                                                        }));
                                        expect(
                                                publicationManager
                                                        .hasSubscriptionsForProviderAttribute(
                                                                provider.id,
                                                                testAttributeName)).toBeFalsy();
                                        testAttribute.get.reset();
                                        dispatcherSpy.sendPublication.reset();

                                        increaseFakeTime(maxInterval + 1);
                                        expect(testAttribute.get).not.toHaveBeenCalled();
                                        expect(dispatcherSpy.sendPublication).not
                                                .toHaveBeenCalled();
                                    });
                                });

                        it("creates a working onChange subscription", function() {
                            publicationManager.addPublicationProvider(providerId, provider);
                            publicationManager.handleSubscriptionRequest(
                                    proxyId,
                                    providerId,
                                    onChangeSubscriptionRequest);

                            expect(
                                    publicationManager.hasSubscriptionsForProviderAttribute(
                                            provider.id,
                                            testAttributeName)).toBeTruthy();

                            // reset first publication
                            testAttribute.get.reset();
                            dispatcherSpy.sendPublication.reset();

                            for (times = 0; times < maxNrOfTimes; times++) {
                                increaseFakeTime(50);
                                testAttribute.valueChanged(value + times);
                                expect(dispatcherSpy.sendPublication.callCount).toEqual(times + 1);
                            }

                            // cleanup
                            stopSubscription(onChangeSubscriptionRequest);
                            expect(
                                    publicationManager.hasSubscriptionsForProviderAttribute(
                                            provider.id,
                                            testAttributeName)).toBeFalsy();
                        });

                        it(
                                "does not publish when an onChange subscription has an endDate in the past",
                                function() {
                                    onChangeSubscriptionRequest.qos.expiryDateMs = Date.now() - 1;

                                    runs(function() {
                                        publicationManager.addPublicationProvider(
                                                providerId,
                                                provider);
                                        publicationManager.handleSubscriptionRequest(
                                                proxyId,
                                                providerId,
                                                onChangeSubscriptionRequest);
                                        expect(
                                                publicationManager
                                                        .hasSubscriptionsForProviderAttribute(
                                                                provider.id,
                                                                testAttributeName)).toBeFalsy();
                                        // reset first publication
                                        testAttribute.get.reset();
                                        dispatcherSpy.sendPublication.reset();

                                        testAttribute.valueChanged(value + times);

                                        increaseFakeTime(asyncGetterCallDelay);

                                        setTimeout(
                                                function() {
                                                    expect(dispatcherSpy.sendPublication).not
                                                            .toHaveBeenCalled();

                                                    // cleanup
                                                    stopSubscription(onChangeSubscriptionRequest);
                                                    expect(
                                                            publicationManager
                                                                    .hasSubscriptionsForProviderAttribute(
                                                                            provider.id,
                                                                            testAttributeName))
                                                            .toBeFalsy();
                                                },
                                                asyncGetterCallDelay);
                                    });
                                });

                        it("removes an onChange attribute subscription after endDate", function() {

                            runs(function() {
                                publicationManager.addPublicationProvider(providerId, provider);
                                publicationManager.handleSubscriptionRequest(
                                        proxyId,
                                        providerId,
                                        onChangeSubscriptionRequest);
                                expect(
                                        publicationManager.hasSubscriptionsForProviderAttribute(
                                                provider.id,
                                                testAttributeName)).toBeTruthy();
                                increaseFakeTime(1);
                            });

                            waitsFor(function() {
                                return dispatcherSpy.sendPublication.callCount > 0;
                            }, "timeout dispatcherSpy.sendPublication", 10);

                            runs(function() {
                                // reset first publication
                                testAttribute.get.reset();
                                dispatcherSpy.sendPublication.reset();

                                increaseFakeTime(50);
                                testAttribute.valueChanged(value);
                            });

                            waitsFor(function() {
                                return dispatcherSpy.sendPublication.callCount > 0;
                            }, "timeout dispatcherSpy.sendPublication after valueChanged", 10);

                            runs(function() {
                                expect(dispatcherSpy.sendPublication).toHaveBeenCalled();

                                // after another attribute change, the methods should not have been
                                // called again (ie subscription terminated)
                                increaseFakeTime(subscriptionLength);
                                dispatcherSpy.sendPublication.reset();

                                testAttribute.valueChanged(value);

                                setTimeout(function() {
                                    expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();
                                    expect(
                                            publicationManager
                                                    .hasSubscriptionsForProviderAttribute(
                                                            provider.id,
                                                            testAttributeName)).toBeFalsy();
                                }, asyncGetterCallDelay);
                            });
                        });

                        it(
                                "removes an onChange attribute subscription after subscription stop",
                                function() {

                                    runs(function() {
                                        publicationManager.addPublicationProvider(
                                                providerId,
                                                provider);
                                        publicationManager.handleSubscriptionRequest(
                                                proxyId,
                                                providerId,
                                                onChangeSubscriptionRequest);
                                        expect(
                                                publicationManager
                                                        .hasSubscriptionsForProviderAttribute(
                                                                provider.id,
                                                                testAttributeName)).toBeTruthy();
                                        increaseFakeTime(1);
                                    });

                                    waitsFor(function() {
                                        return dispatcherSpy.sendPublication.callCount > 0;
                                    }, "timeout dispatcherSpy.sendPublication", 10);

                                    runs(function() {
                                        // reset first publication
                                        testAttribute.get.reset();
                                        dispatcherSpy.sendPublication.reset();

                                        increaseFakeTime(50);
                                        testAttribute.valueChanged(value);
                                    });

                                    waitsFor(function() {
                                        return dispatcherSpy.sendPublication.callCount > 0;
                                    }, "timeout dispatcherSpy.sendPublication", 10);

                                    runs(function() {
                                        expect(dispatcherSpy.sendPublication).toHaveBeenCalled();

                                        // after subscription stop, the methods should not have been called
                                        // again (ie subscription terminated)
                                        publicationManager
                                                .handleSubscriptionStop(new SubscriptionStop(
                                                        {
                                                            subscriptionId : onChangeSubscriptionRequest.subscriptionId
                                                        }));
                                        expect(
                                                publicationManager
                                                        .hasSubscriptionsForProviderAttribute(
                                                                provider.id,
                                                                testAttributeName)).toBeFalsy();
                                        dispatcherSpy.sendPublication.reset();

                                        testAttribute.valueChanged(value);
                                        increaseFakeTime(1);
                                        setTimeout(function() {
                                            expect(dispatcherSpy.sendPublication).not
                                                    .toHaveBeenCalled();
                                        }, asyncGetterCallDelay);
                                    });
                                });

                        it(
                                "creates a mixed subscription and does not send two publications within minIntervalMs in case async getter calls and valueChanged occur at the same time",
                                function() {
                                    var times;

                                    runs(function() {
                                        publicationManager.addPublicationProvider(
                                                providerId,
                                                provider);
                                        publicationManager.handleSubscriptionRequest(
                                                proxyId,
                                                providerId,
                                                mixedSubscriptionRequestWithAsyncAttribute);
                                        expect(
                                                publicationManager
                                                        .hasSubscriptionsForProviderAttribute(
                                                                provider.id,
                                                                asyncTestAttributeName))
                                                .toBeTruthy();
                                        // wait until the first publication occurs
                                        increaseFakeTime(asyncGetterCallDelay);

                                        // reset first publication
                                        asyncTestAttribute.get.reset();
                                        dispatcherSpy.sendPublication.reset();

                                        // let the minIntervalMs exceed, so that new value changes
                                        // immediately lead to publications
                                        increaseFakeTime(minIntervalMs);
                                        expect(asyncTestAttribute.get).not.toHaveBeenCalled();
                                        expect(dispatcherSpy.sendPublication).not
                                                .toHaveBeenCalled();

                                        asyncTestAttribute.valueChanged(value);
                                        increaseFakeTime(5);

                                        // this should cause an async timer, which sends a publication
                                        // after minIntervalMs-5
                                        asyncTestAttribute.valueChanged(value);

                                        // the getter has not been invoked so far
                                        expect(asyncTestAttribute.get).not.toHaveBeenCalled();
                                    });

                                    waitsFor(
                                            function() {
                                                return dispatcherSpy.sendPublication.callCount === 1;
                                            },
                                            "timeout dispatcherSpy.sendPublication",
                                            asyncGetterCallDelay);

                                    runs(function() {
                                        expect(dispatcherSpy.sendPublication.callCount).toEqual(1);
                                        // now, lets increas the time until minIntervalMs
                                        increaseFakeTime(minIntervalMs - 5);

                                        // now, the async timer has exceeded, and the PublicationManager
                                        // invokes the get
                                        expect(asyncTestAttribute.get.callCount).toEqual(1);

                                        // now change the attribute value
                                        asyncTestAttribute.valueChanged(value);
                                    });

                                    waitsFor(
                                            function() {
                                                return dispatcherSpy.sendPublication.callCount === 2;
                                            },
                                            "timeout dispatcherSpy.sendPublication",
                                            asyncGetterCallDelay);

                                    runs(function() {
                                        expect(dispatcherSpy.sendPublication.callCount).toEqual(2);
                                        // lets exceed the async getter delay, causing the async getter
                                        // to resolve the promise object
                                        increaseFakeTime(asyncGetterCallDelay);

                                        // now change the attribute value
                                        asyncTestAttribute.valueChanged(value);

                                        // this shall not result in a new sendPublication, as
                                        // asyncGetterCallDelay<minIntervalMs and the time
                                        // delay between two publications must be at least minIntervalMs
                                        expect(dispatcherSpy.sendPublication.callCount).toEqual(2);

                                        increaseFakeTime(minIntervalMs - asyncGetterCallDelay);
                                    });

                                    waitsFor(function() {
                                        return asyncTestAttribute.get.callCount === 2;
                                    }, "timeout asyncTestAttribute.get", asyncGetterCallDelay);

                                    runs(function() {
                                        expect(asyncTestAttribute.get.callCount).toEqual(2);

                                        increaseFakeTime(asyncGetterCallDelay);
                                    });

                                    waitsFor(
                                            function() {
                                                return dispatcherSpy.sendPublication.callCount === 3;
                                            },
                                            "timeout dispatcherSpy.sendPublication",
                                            asyncGetterCallDelay);

                                    runs(function() {
                                        expect(asyncTestAttribute.get.callCount).toEqual(2);
                                        expect(dispatcherSpy.sendPublication.callCount).toEqual(3);

                                        stopSubscription(mixedSubscriptionRequestWithAsyncAttribute);
                                        expect(
                                                publicationManager
                                                        .hasSubscriptionsForProviderAttribute(
                                                                provider.id,
                                                                asyncTestAttributeName))
                                                .toBeFalsy();
                                    });
                                });

                        it(
                                "creates a mixed subscription that publishes every maxInterval",
                                function() {
                                    var times;

                                    runs(function() {
                                        publicationManager.addPublicationProvider(
                                                providerId,
                                                provider);
                                        publicationManager.handleSubscriptionRequest(
                                                proxyId,
                                                providerId,
                                                mixedSubscriptionRequest);
                                        expect(
                                                publicationManager
                                                        .hasSubscriptionsForProviderAttribute(
                                                                provider.id,
                                                                testAttributeName)).toBeTruthy();
                                        increaseFakeTime(1);
                                    });

                                    waitsFor(
                                            function() {
                                                return dispatcherSpy.sendPublication.callCount > 0;
                                            },
                                            "timeout dispatcherSpy.sendPublication",
                                            asyncGetterCallDelay);

                                    runs(function() {
                                        // reset first publication
                                        testAttribute.get.reset();
                                        dispatcherSpy.sendPublication.reset();
                                        expect(testAttribute.get).not.toHaveBeenCalled();
                                        expect(dispatcherSpy.sendPublication).not
                                                .toHaveBeenCalled();
                                    });

                                    var internalCheck =
                                            function(times) {
                                                runs(function() {
                                                    // step the clock forward to 1 ms before the interval
                                                    increaseFakeTime(maxInterval - 1);
                                                    // 1 ms later the poll should happen
                                                    increaseFakeTime(1);
                                                });

                                                waitsFor(
                                                        function() {
                                                            return dispatcherSpy.sendPublication.callCount === (times + 1);
                                                        },
                                                        "timeout testAttribute.get",
                                                        asyncGetterCallDelay);

                                                runs(function() {
                                                    expect(testAttribute.get.callCount).toEqual(
                                                            times + 1);
                                                    expect(dispatcherSpy.sendPublication.callCount)
                                                            .toEqual(times + 1);
                                                });
                                            };

                                    for (times = 0; times < maxNrOfTimes; ++times) {
                                        internalCheck(times);
                                    }

                                    runs(function() {
                                        // cleanup
                                        stopSubscription(mixedSubscriptionRequest);
                                        expect(
                                                publicationManager
                                                        .hasSubscriptionsForProviderAttribute(
                                                                provider.id,
                                                                testAttributeName)).toBeFalsy();
                                    });
                                });

                        it(
                                "creates a mixed subscription that publishes valueChanges that occur each minIntervalMs",
                                function() {
                                    runs(function() {
                                        publicationManager.addPublicationProvider(
                                                providerId,
                                                provider);
                                        publicationManager.handleSubscriptionRequest(
                                                proxyId,
                                                providerId,
                                                mixedSubscriptionRequest);
                                        expect(
                                                publicationManager
                                                        .hasSubscriptionsForProviderAttribute(
                                                                provider.id,
                                                                testAttributeName)).toBeTruthy();
                                        increaseFakeTime(1);
                                    });

                                    waitsFor(
                                            function() {
                                                return dispatcherSpy.sendPublication.callCount === 1;
                                            },
                                            "timeout dispatcherSpy.sendPublication",
                                            asyncGetterCallDelay);

                                    runs(function() {
                                        // reset first publication
                                        testAttribute.get.reset();
                                        dispatcherSpy.sendPublication.reset();
                                    });

                                    var internalCheck =
                                            function(times) {
                                                runs(function() {
                                                    increaseFakeTime(mixedSubscriptionRequest.qos.minIntervalMs);
                                                    testAttribute.valueChanged(value + times);
                                                });

                                                waitsFor(
                                                        function() {
                                                            return dispatcherSpy.sendPublication.callCount === times + 1;
                                                        },
                                                        "timeout dispatcherSpy.sendPublication",
                                                        asyncGetterCallDelay);

                                                runs(function() {
                                                    expect(testAttribute.get).not
                                                            .toHaveBeenCalled();
                                                    expect(dispatcherSpy.sendPublication.callCount)
                                                            .toEqual(times + 1);
                                                });
                                            };

                                    for (times = 0; times < maxNrOfTimes; times++) {
                                        internalCheck(times);
                                    }

                                    runs(function() {
                                        // cleanup
                                        stopSubscription(mixedSubscriptionRequest);
                                        expect(
                                                publicationManager
                                                        .hasSubscriptionsForProviderAttribute(
                                                                provider.id,
                                                                testAttributeName)).toBeFalsy();
                                    });
                                });

                        it(
                                "creates a mixed subscription that publishes valueChanges that occur each maxInterval-1",
                                function() {
                                    runs(function() {
                                        dispatcherSpy.sendPublication.reset();
                                        publicationManager.addPublicationProvider(
                                                providerId,
                                                provider);
                                        publicationManager.handleSubscriptionRequest(
                                                proxyId,
                                                providerId,
                                                mixedSubscriptionRequest);
                                        expect(
                                                publicationManager
                                                        .hasSubscriptionsForProviderAttribute(
                                                                provider.id,
                                                                testAttributeName)).toBeTruthy();
                                        increaseFakeTime(1);
                                    });

                                    waitsFor(
                                            function() {
                                                return dispatcherSpy.sendPublication.callCount === 1;
                                            },
                                            "timeout dispatcherSpy.sendPublication",
                                            asyncGetterCallDelay);

                                    runs(function() {
                                        // reset first publication
                                        testAttribute.get.reset();
                                        dispatcherSpy.sendPublication.reset();
                                    });

                                    var internalCheck =
                                            function(times) {
                                                runs(function() {
                                                    increaseFakeTime(mixedSubscriptionRequest.qos.maxInterval - 2);
                                                    testAttribute.valueChanged(value + times);
                                                    increaseFakeTime(1);
                                                });

                                                waitsFor(
                                                        function() {
                                                            return dispatcherSpy.sendPublication.callCount === times + 1;
                                                        },
                                                        "timeout dispatcherSpy.sendPublication",
                                                        asyncGetterCallDelay);

                                                runs(function() {
                                                    expect(testAttribute.get).not
                                                            .toHaveBeenCalled();
                                                    expect(dispatcherSpy.sendPublication.callCount)
                                                            .toEqual(times + 1);
                                                });
                                            };
                                    for (times = 0; times < maxNrOfTimes; times++) {
                                        internalCheck(times);
                                    }

                                    runs(function() {
                                        // cleanup
                                        stopSubscription(mixedSubscriptionRequest);
                                        expect(
                                                publicationManager
                                                        .hasSubscriptionsForProviderAttribute(
                                                                provider.id,
                                                                testAttributeName)).toBeFalsy();
                                    });
                                });
                        it(
                                "creates a mixed subscription that publishes many valueChanges within minIntervalMs only once",
                                function() {

                                    runs(function() {
                                        publicationManager.addPublicationProvider(
                                                providerId,
                                                provider);
                                        publicationManager.handleSubscriptionRequest(
                                                proxyId,
                                                providerId,
                                                mixedSubscriptionRequest);
                                        expect(
                                                publicationManager
                                                        .hasSubscriptionsForProviderAttribute(
                                                                provider.id,
                                                                testAttributeName)).toBeTruthy();
                                        increaseFakeTime(1);
                                    });

                                    waitsFor(
                                            function() {
                                                return dispatcherSpy.sendPublication.callCount === 1;
                                            },
                                            "timeout dispatcherSpy.sendPublication",
                                            asyncGetterCallDelay);

                                    runs(function() {
                                        // reset first publication
                                        testAttribute.get.reset();
                                        dispatcherSpy.sendPublication.reset();

                                        var shortInterval =
                                                Math
                                                        .round((mixedSubscriptionRequest.qos.minIntervalMs - 2)
                                                            / maxNrOfTimes);
                                        for (times = 0; times < maxNrOfTimes; times++) {
                                            expect(testAttribute.get).not.toHaveBeenCalled();
                                            expect(dispatcherSpy.sendPublication).not
                                                    .toHaveBeenCalled();
                                            increaseFakeTime(shortInterval);
                                            testAttribute.valueChanged(value + times);
                                        }

                                        // after minIntervalMs the publication works again
                                        increaseFakeTime(mixedSubscriptionRequest.qos.minIntervalMs
                                            - shortInterval
                                            * maxNrOfTimes);
                                        testAttribute.valueChanged(value);
                                        expect(testAttribute.get.callCount).toEqual(1);
                                        expect(dispatcherSpy.sendPublication.callCount).toEqual(1);

                                        // cleanup
                                        stopSubscription(mixedSubscriptionRequest);
                                        expect(
                                                publicationManager
                                                        .hasSubscriptionsForProviderAttribute(
                                                                provider.id,
                                                                testAttributeName)).toBeFalsy();
                                    });
                                });

                        it(
                                "creates a periodic subscription without expiryDate and expects periodic publications",
                                function() {
                                    var period = 400, n = 10, subscriptionRequestWithoutExpiryDate;

                                    runs(function() {
                                        subscriptionRequestWithoutExpiryDate =
                                                createSubscriptionRequest(
                                                        true,
                                                        testAttributeName,
                                                        period,
                                                        0,
                                                        false,
                                                        minIntervalMs);
                                        dispatcherSpy.sendPublication.reset();
                                        publicationManager.addPublicationProvider(
                                                providerId,
                                                provider);
                                        publicationManager.handleSubscriptionRequest(
                                                proxyId,
                                                providerId,
                                                subscriptionRequestWithoutExpiryDate);
                                        expect(
                                                publicationManager
                                                        .hasSubscriptionsForProviderAttribute(
                                                                provider.id,
                                                                testAttributeName)).toBeTruthy();
                                        increaseFakeTime(1);
                                    });

                                    waitsFor(
                                            function() {
                                                return dispatcherSpy.sendPublication.callCount === 1;
                                            },
                                            "timeout dispatcherSpy.sendPublication",
                                            asyncGetterCallDelay);

                                    runs(function() {
                                        expect(dispatcherSpy.sendPublication.callCount).toEqual(1);
                                        dispatcherSpy.sendPublication.reset();
                                    });

                                    var checkMaxIntervalCalls =
                                            function(i) {
                                                runs(function() {
                                                    increaseFakeTime(period);
                                                });

                                                waitsFor(
                                                        function() {
                                                            return dispatcherSpy.sendPublication.callCount === (1 + i);
                                                        },
                                                        "timeout "
                                                            + i
                                                            + " times dispatcherSpy.sendPublication call",
                                                        asyncGetterCallDelay);

                                                runs(function() {
                                                    expect(dispatcherSpy.sendPublication.callCount)
                                                            .toEqual(1 + i);
                                                });
                                            };

                                    var i;
                                    for (i = 0; i < n; i++) {
                                        checkMaxIntervalCalls(i);
                                    }

                                    runs(function() {
                                        // cleanup
                                        stopSubscription(subscriptionRequestWithoutExpiryDate);
                                        expect(
                                                publicationManager
                                                        .hasSubscriptionsForProviderAttribute(
                                                                provider.id,
                                                                testAttributeName)).toBeFalsy();
                                    });
                                });
                        it(
                                "creates a mixed subscription that publishes correctly with all cases mixed",
                                function() {
                                    var i;

                                    runs(function() {
                                        publicationManager.addPublicationProvider(
                                                providerId,
                                                provider);
                                        publicationManager.handleSubscriptionRequest(
                                                proxyId,
                                                providerId,
                                                mixedSubscriptionRequest);
                                        expect(
                                                publicationManager
                                                        .hasSubscriptionsForProviderAttribute(
                                                                provider.id,
                                                                testAttributeName)).toBeTruthy();
                                        increaseFakeTime(1);
                                    });

                                    waitsFor(
                                            function() {
                                                return dispatcherSpy.sendPublication.callCount === 1;
                                            },
                                            "timeout dispatcherSpy.sendPublication",
                                            asyncGetterCallDelay);

                                    runs(function() {
                                        // reset first publication
                                        testAttribute.get.reset();
                                        dispatcherSpy.sendPublication.reset();

                                        // at time 0, a value change is not reported because value was
                                        // already published initially while
                                        // handling the subscription request
                                        testAttribute.valueChanged(value);
                                        expect(testAttribute.get).not.toHaveBeenCalled();
                                        expect(dispatcherSpy.sendPublication).not
                                                .toHaveBeenCalled();

                                        // minIntervalMs and a value change the value should be reported
                                        increaseFakeTime(mixedSubscriptionRequest.qos.minIntervalMs);
                                    });

                                    waitsFor(
                                            function() {
                                                return dispatcherSpy.sendPublication.callCount === 1;
                                            },
                                            "timeout dispatcherSpy.sendPublication call",
                                            asyncGetterCallDelay);

                                    runs(function() {
                                        // due to minIntervalMs exceeded + valueChanged has been occured
                                        // within the minIntervalMs, the
                                        // PublicationManager
                                        // send the current attribute value to the subscriptions
                                        expect(testAttribute.get.callCount).toEqual(1);
                                        expect(dispatcherSpy.sendPublication.callCount).toEqual(1);
                                        testAttribute.get.reset();
                                        dispatcherSpy.sendPublication.reset();
                                        // minIntervalMs and no publication shall occur
                                        increaseFakeTime(mixedSubscriptionRequest.qos.minIntervalMs);
                                        expect(testAttribute.get).not.toHaveBeenCalled();
                                        expect(dispatcherSpy.sendPublication).not
                                                .toHaveBeenCalled();

                                        // change value, and immediate publication shall occur
                                        testAttribute.valueChanged(value);
                                        expect(testAttribute.get).not.toHaveBeenCalled();
                                        expect(dispatcherSpy.sendPublication.callCount).toEqual(1);

                                        // at time 1:maxNrOfTimes, a value change is reported => NO
                                        // publication is sent
                                        for (i = 0; i < maxNrOfTimes; ++i) {
                                            dispatcherSpy.sendPublication.reset();
                                            increaseFakeTime(1);
                                            testAttribute.valueChanged(value);
                                            expect(testAttribute.get).not.toHaveBeenCalled();
                                            expect(dispatcherSpy.sendPublication).not
                                                    .toHaveBeenCalled();
                                        }

                                        // at time mixedSubscriptionRequest.qos.minIntervalMs the last
                                        // value of the test attribute is sent
                                        // to the subscribers as the publication timeout occurs
                                        dispatcherSpy.sendPublication.reset();
                                        increaseFakeTime(mixedSubscriptionRequest.qos.minIntervalMs
                                            - maxNrOfTimes);
                                        expect(testAttribute.get.callCount).toEqual(1);
                                    });

                                    waitsFor(
                                            function() {
                                                return dispatcherSpy.sendPublication.callCount === 1;
                                            },
                                            "timeout dispatcherSpy.sendPublication call ",
                                            asyncGetterCallDelay);

                                    runs(function() {
                                        expect(dispatcherSpy.sendPublication.callCount).toEqual(1);

                                        // value change after mixedSubscriptionRequest.qos.maxInterval - 2=> publication sent
                                        dispatcherSpy.sendPublication.reset();
                                        testAttribute.get.reset();

                                        increaseFakeTime(mixedSubscriptionRequest.qos.maxInterval - 1);
                                        testAttribute.valueChanged(value);
                                        expect(testAttribute.get).not.toHaveBeenCalled();
                                    });

                                    waitsFor(
                                            function() {
                                                return dispatcherSpy.sendPublication.callCount === 1;
                                            },
                                            "timeout dispatcherSpy.sendPublication call ",
                                            asyncGetterCallDelay);

                                    runs(function() {
                                        expect(dispatcherSpy.sendPublication.callCount).toEqual(1);

                                        // after mixedSubscriptionRequest.qos.maxInterval => interval
                                        // publication is sent
                                        dispatcherSpy.sendPublication.reset();
                                        increaseFakeTime(mixedSubscriptionRequest.qos.maxInterval);
                                    });

                                    waitsFor(
                                            function() {
                                                return dispatcherSpy.sendPublication.callCount === 1;
                                            },
                                            "timeout dispatcherSpy.sendPublication call ",
                                            asyncGetterCallDelay);

                                    runs(function() {
                                        expect(testAttribute.get.callCount).toEqual(1);
                                        expect(dispatcherSpy.sendPublication.callCount).toEqual(1);

                                        // after another mixedSubscriptionRequest.qos.maxInterval =>
                                        // interval publication is sent
                                        testAttribute.get.reset();
                                        dispatcherSpy.sendPublication.reset();
                                        increaseFakeTime(mixedSubscriptionRequest.qos.maxInterval);
                                    });

                                    waitsFor(
                                            function() {
                                                return dispatcherSpy.sendPublication.callCount === 1;
                                            },
                                            "timeout dispatcherSpy.sendPublication call ",
                                            asyncGetterCallDelay);

                                    runs(function() {
                                        expect(testAttribute.get.callCount).toEqual(1);
                                        expect(dispatcherSpy.sendPublication.callCount).toEqual(1);

                                        // after subscription stop => NO publications are sent any more
                                        stopSubscription(mixedSubscriptionRequest);
                                        expect(
                                                publicationManager
                                                        .hasSubscriptionsForProviderAttribute(
                                                                provider.id,
                                                                testAttributeName)).toBeFalsy();
                                        testAttribute.get.reset();
                                        dispatcherSpy.sendPublication.reset();
                                        increaseFakeTime(mixedSubscriptionRequest.qos.minIntervalMs);
                                        testAttribute.valueChanged(value);
                                        expect(testAttribute.get).not.toHaveBeenCalled();
                                    });
                                });
                        it(
                                "does not publish when mixed subscription has an endDate in the past",
                                function() {
                                    mixedSubscriptionRequest.qos.expiryDateMs = Date.now() - 1;

                                    runs(function() {
                                        publicationManager.addPublicationProvider(
                                                providerId,
                                                provider);
                                        publicationManager.handleSubscriptionRequest(
                                                proxyId,
                                                providerId,
                                                mixedSubscriptionRequest);
                                        expect(
                                                publicationManager
                                                        .hasSubscriptionsForProviderAttribute(
                                                                provider.id,
                                                                testAttributeName)).toBeFalsy();
                                        // reset first publication
                                        testAttribute.get.reset();
                                        dispatcherSpy.sendPublication.reset();

                                        testAttribute.valueChanged(value);
                                        increaseFakeTime(subscriptionLength);
                                        expect(testAttribute.get).not.toHaveBeenCalled();
                                        expect(dispatcherSpy.sendPublication).not
                                                .toHaveBeenCalled();

                                        // cleanup
                                        stopSubscription(mixedSubscriptionRequest);
                                        expect(
                                                publicationManager
                                                        .hasSubscriptionsForProviderAttribute(
                                                                provider.id,
                                                                testAttributeName)).toBeFalsy();
                                    });
                                });

                        it("removes a mixed attribute subscription after endDate", function() {
                            runs(function() {
                                publicationManager.addPublicationProvider(providerId, provider);
                                publicationManager.handleSubscriptionRequest(
                                        proxyId,
                                        providerId,
                                        mixedSubscriptionRequest);
                                expect(
                                        publicationManager.hasSubscriptionsForProviderAttribute(
                                                provider.id,
                                                testAttributeName)).toBeTruthy();
                                increaseFakeTime(1);
                            });

                            waitsFor(function() {
                                return dispatcherSpy.sendPublication.callCount > 0;
                            }, "dispatcherSpy.sendPublication", asyncGetterCallDelay);

                            runs(function() {
                                // reset first publication
                                testAttribute.get.reset();
                                dispatcherSpy.sendPublication.reset();

                                increaseFakeTime(subscriptionLength);
                                testAttribute.get.reset();
                                dispatcherSpy.sendPublication.reset();

                                expect(testAttribute.get).not.toHaveBeenCalled();
                                expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();
                                testAttribute.valueChanged(value);
                                expect(testAttribute.get).not.toHaveBeenCalled();
                                expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();
                                stopSubscription(mixedSubscriptionRequest);
                                expect(
                                        publicationManager.hasSubscriptionsForProviderAttribute(
                                                provider.id,
                                                testAttributeName)).toBeFalsy();
                            });
                        });

                        it(
                                "removes a mixed attribute subscription after subscription stop",
                                function() {
                                    runs(function() {
                                        publicationManager.addPublicationProvider(
                                                providerId,
                                                provider);
                                        publicationManager.handleSubscriptionRequest(
                                                proxyId,
                                                providerId,
                                                mixedSubscriptionRequest);
                                        expect(
                                                publicationManager
                                                        .hasSubscriptionsForProviderAttribute(
                                                                provider.id,
                                                                testAttributeName)).toBeTruthy();
                                        increaseFakeTime(1);
                                    });

                                    waitsFor(function() {
                                        return dispatcherSpy.sendPublication.callCount > 0;
                                    }, "dispatcherSpy.sendPublication", asyncGetterCallDelay);

                                    runs(function() {
                                        // reset first publication
                                        testAttribute.get.reset();
                                        dispatcherSpy.sendPublication.reset();

                                        testAttribute.valueChanged(value); // do change
                                        increaseFakeTime(maxInterval); // increase interval

                                        // after subscription stop, the methods should not have been called
                                        // again (ie subscription
                                        // terminated)
                                        publicationManager
                                                .handleSubscriptionStop(new SubscriptionStop(
                                                        {
                                                            subscriptionId : mixedSubscriptionRequest.subscriptionId
                                                        }));
                                        expect(
                                                publicationManager
                                                        .hasSubscriptionsForProviderAttribute(
                                                                provider.id,
                                                                testAttributeName)).toBeFalsy();
                                        testAttribute.get.reset();
                                        dispatcherSpy.sendPublication.reset();

                                        increaseFakeTime(maxInterval); // increase interval
                                        testAttribute.valueChanged(value); // do change
                                        increaseFakeTime(maxInterval); // increase interval

                                        setTimeout(function() {
                                            expect(testAttribute.get).not.toHaveBeenCalled();
                                            expect(dispatcherSpy.sendPublication).not
                                                    .toHaveBeenCalled();
                                            stopSubscription(mixedSubscriptionRequest);
                                        }, asyncGetterCallDelay);
                                    });
                                });

                        it(
                                "removes a mixed broadcast subscription after subscription stop",
                                function() {
                                    var broadcastOutputParameters =
                                            testBroadcast.createBroadcastOutputParameters();
                                    broadcastOutputParameters.setParam1("param1");
                                    runs(function() {
                                        publicationManager.addPublicationProvider(
                                                providerId,
                                                provider);
                                        publicationManager.handleEventSubscriptionRequest(
                                                proxyId,
                                                providerId,
                                                onChangeBroadcastSubscriptionRequest);

                                        expect(
                                                publicationManager
                                                        .hasSubscriptionsForProviderEvent(
                                                                provider.id,
                                                                testBroadcastName)).toBeTruthy();
                                        increaseFakeTime(1);

                                        testBroadcast.fire(broadcastOutputParameters);
                                        increaseFakeTime(maxInterval); // increase interval
                                    });

                                    waitsFor(function() {
                                        return dispatcherSpy.sendPublication.callCount > 0;
                                    }, "dispatcherSpy.sendPublication", asyncGetterCallDelay);

                                    runs(function() {
                                        // reset first publication
                                        dispatcherSpy.sendPublication.reset();

                                        // after subscription stop, the methods should not have been called
                                        // again (ie subscription
                                        // terminated)
                                        publicationManager
                                                .handleSubscriptionStop(new SubscriptionStop(
                                                        {
                                                            subscriptionId : onChangeBroadcastSubscriptionRequest.subscriptionId
                                                        }));

                                        expect(
                                                publicationManager
                                                        .hasSubscriptionsForProviderEvent(
                                                                provider.id,
                                                                testBroadcastName)).toBeFalsy();

                                        increaseFakeTime(maxInterval); // increase interval
                                        testBroadcast.fire(broadcastOutputParameters);
                                        increaseFakeTime(maxInterval); // increase interval

                                        setTimeout(function() {
                                            expect(dispatcherSpy.sendPublication).not
                                                    .toHaveBeenCalled();
                                        }, asyncGetterCallDelay);
                                    });
                                });

                        it(
                                "removes publication provider",
                                function() {
                                    var times;
                                    spyOn(testAttribute, "registerObserver").andCallThrough();
                                    spyOn(testAttribute, "unregisterObserver").andCallThrough();
                                    publicationManager.addPublicationProvider(providerId, provider);
                                    expect(testAttribute.registerObserver).toHaveBeenCalled();
                                    spyOn(publicationManager, "handleSubscriptionStop")
                                            .andCallThrough();

                                    expect(testAttribute.get).not.toHaveBeenCalled();
                                    expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();

                                    runs(function() {
                                        publicationManager.handleSubscriptionRequest(
                                                proxyId,
                                                providerId,
                                                intervalSubscriptionRequest);
                                        expect(
                                                publicationManager
                                                        .hasSubscriptionsForProviderAttribute(
                                                                provider.id,
                                                                testAttributeName)).toBeTruthy();
                                        //increase the fake time to ensure proper async processing of the subscription request
                                        increaseFakeTime(1);
                                    });

                                    waitsFor(function() {
                                        return dispatcherSpy.sendPublication.callCount > 0;
                                    }, "dispatcherSpy.sendPublication", asyncGetterCallDelay);

                                    runs(function() {
                                        expect(testAttribute.get.callCount).toEqual(1);
                                        expect(dispatcherSpy.sendPublication.callCount).toEqual(1);
                                        expect(testAttribute.unregisterObserver).not
                                                .toHaveBeenCalled();
                                        expect(publicationManager.handleSubscriptionStop).not
                                                .toHaveBeenCalled();
                                        publicationManager.removePublicationProvider(
                                                providerId,
                                                provider);
                                        expect(testAttribute.unregisterObserver).toHaveBeenCalled();
                                        expect(publicationManager.handleSubscriptionStop)
                                                .toHaveBeenCalledWith(
                                                        new SubscriptionStop(
                                                                {
                                                                    subscriptionId : intervalSubscriptionRequest.subscriptionId
                                                                }));
                                        // cleanup
                                        stopSubscription(intervalSubscriptionRequest);
                                        expect(
                                                publicationManager
                                                        .hasSubscriptionsForProviderAttribute(
                                                                provider.id,
                                                                testAttributeName)).toBeFalsy();
                                    });
                                });
                    });

        });
