/*global joynrTestRequire: true, xit: true */

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
            "joynr/dispatching/types/SubscriptionStop",
            "joynr/provider/ProviderAttribute",
            "joynr/proxy/PeriodicSubscriptionQos",
            "joynr/proxy/SubscriptionQos",
            "joynr/proxy/OnChangeSubscriptionQos",
            "joynr/proxy/OnChangeWithKeepAliveSubscriptionQos",
            "joynr/types/ProviderQos",
            "joynr/types/ProviderScope",
            "joynr/dispatching/types/SubscriptionPublication",
            "joynr/TypesEnum",
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
                SubscriptionStop,
                ProviderAttribute,
                PeriodicSubscriptionQos,
                SubscriptionQos,
                OnChangeSubscriptionQos,
                OnChangeWithKeepAliveSubscriptionQos,
                ProviderQos,
                ProviderScope,
                SubscriptionPublication,
                TypesEnum,
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
                        var onChangeSubscriptionRequest, mixedSubscriptionRequest;
                        var mixedSubscriptionRequestWithAsyncAttribute, testAttributeName;
                        var asyncTestAttributeName, value, minInterval, maxInterval, maxNrOfTimes;
                        var subscriptionLength, asyncTestAttribute, testAttribute, providerSettings;

                        function createSubscriptionRequest(
                                requestAttributeName,
                                period,
                                subscriptionLength,
                                onChange,
                                minInterval) {
                            var qosSettings, expiryDate;
                            expiryDate =
                                    subscriptionLength === SubscriptionQos.NO_EXPIRY_DATE
                                            ? SubscriptionQos.NO_EXPIRY_DATE
                                            : Date.now() + subscriptionLength;
                            if (onChange) {
                                if (period !== undefined) {
                                    qosSettings = new OnChangeWithKeepAliveSubscriptionQos({
                                        minInterval : minInterval || 50,
                                        maxInterval : period,
                                        expiryDate : expiryDate,
                                        alertAfterInterval : 0,
                                        publicationTtl : 1000
                                    });
                                } else {
                                    qosSettings = new OnChangeSubscriptionQos({
                                        minInterval : minInterval || 50,
                                        expiryDate : expiryDate,
                                        publicationTtl : 1000
                                    });
                                }
                            } else {
                                qosSettings = new PeriodicSubscriptionQos({
                                    period : period,
                                    expiryDate : expiryDate,
                                    alertAfterInterval : 0,
                                    publicationTtl : 1000
                                });
                            }

                            return new SubscriptionRequest({
                                subscriptionId : "subscriptionId" + uuid(),
                                subscribedToName : requestAttributeName,
                                qos : qosSettings
                            });
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
                            asyncTestAttributeName = "asyncTestAttribute";
                            value = "the value";
                            minInterval = 100;
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

                            testAttribute =
                                    new ProviderAttributeNotifyReadWrite(
                                            provider,
                                            providerSettings,
                                            testAttributeName,
                                            TypesEnum.BOOL);

                            asyncTestAttribute =
                                    new ProviderAttributeNotifyReadWrite(
                                            provider,
                                            providerSettings,
                                            asyncTestAttributeName,
                                            TypesEnum.BOOL);

                            provider[testAttributeName] = testAttribute;
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
                                            testAttributeName,
                                            maxInterval,
                                            subscriptionLength);
                            onChangeSubscriptionRequest =
                                    createSubscriptionRequest(
                                            testAttributeName,
                                            undefined,
                                            subscriptionLength,
                                            true);
                            mixedSubscriptionRequest =
                                    createSubscriptionRequest(
                                            testAttributeName,
                                            maxInterval,
                                            subscriptionLength,
                                            true,
                                            minInterval);
                            mixedSubscriptionRequestWithAsyncAttribute =
                                    createSubscriptionRequest(
                                            asyncTestAttributeName,
                                            maxInterval,
                                            subscriptionLength,
                                            true,
                                            minInterval);
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
                                                            expiryDate : Date.now()
                                                                + onChangeSubscriptionRequest.qos.publicationTtl
                                                        },
                                                        new SubscriptionPublication(
                                                                {
                                                                    response : [ value
                                                                    ],
                                                                    subscriptionId : onChangeSubscriptionRequest.subscriptionId
                                                                }));
                                        stopSubscription(onChangeSubscriptionRequest);
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
                                    });
                                });

                        xit(
                                "creates a working interval subscription with an interval timeout > Math.pow(2, 31) - 1",
                                function() {
                                    var times;
                                    var largeInterval = Math.pow(2, 40);
                                    var largeIntervalSubscriptionRequest =
                                            createSubscriptionRequest(
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
                                    });
                                });

                        it(
                                "does not publish when interval subscription has an endDate in the past",
                                function() {
                                    var times;

                                    runs(function() {
                                        intervalSubscriptionRequest.qos.expiryDate = Date.now() - 1;
                                        publicationManager.addPublicationProvider(
                                                providerId,
                                                provider);
                                        publicationManager.handleSubscriptionRequest(
                                                proxyId,
                                                providerId,
                                                intervalSubscriptionRequest);

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

                        it("removes an interval subscription after endDate", function() {

                            runs(function() {
                                publicationManager.addPublicationProvider(providerId, provider);
                                publicationManager.handleSubscriptionRequest(
                                        proxyId,
                                        providerId,
                                        intervalSubscriptionRequest);
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
                                }, asyncGetterCallDelay);
                            });
                        });

                        it("removes an interval subscription after subscription stop", function() {
                            runs(function() {
                                publicationManager.addPublicationProvider(providerId, provider);
                                publicationManager.handleSubscriptionRequest(
                                        proxyId,
                                        providerId,
                                        intervalSubscriptionRequest);

                                // reset first publication
                                testAttribute.get.reset();
                                dispatcherSpy.sendPublication.reset();

                                increaseFakeTime(maxInterval + asyncGetterCallDelay);
                            });

                            waitsFor(function() {
                                return dispatcherSpy.sendPublication.callCount > 0;
                            }, "timeout dispatcherSpy.sendPublication", asyncGetterCallDelay);

                            runs(function() {
                                expect(testAttribute.get).toHaveBeenCalled();
                                expect(dispatcherSpy.sendPublication).toHaveBeenCalled();

                                // after subscription stop, the methods should not have been called
                                // again (ie subscription terminated)
                                publicationManager.handleSubscriptionStop(new SubscriptionStop({
                                    subscriptionId : intervalSubscriptionRequest.subscriptionId
                                }));
                                testAttribute.get.reset();
                                dispatcherSpy.sendPublication.reset();

                                increaseFakeTime(maxInterval + 1);
                                expect(testAttribute.get).not.toHaveBeenCalled();
                                expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();
                            });
                        });

                        it("creates a working onChange subscription", function() {
                            publicationManager.addPublicationProvider(providerId, provider);
                            publicationManager.handleSubscriptionRequest(
                                    proxyId,
                                    providerId,
                                    onChangeSubscriptionRequest);

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
                        });

                        it(
                                "does not publish when an onChange subscription has an endDate in the past",
                                function() {
                                    onChangeSubscriptionRequest.qos.expiryDate = Date.now() - 1;

                                    runs(function() {
                                        publicationManager.addPublicationProvider(
                                                providerId,
                                                provider);
                                        publicationManager.handleSubscriptionRequest(
                                                proxyId,
                                                providerId,
                                                onChangeSubscriptionRequest);
                                        // reset first publication
                                        testAttribute.get.reset();
                                        dispatcherSpy.sendPublication.reset();

                                        testAttribute.valueChanged(value + times);

                                        increaseFakeTime(asyncGetterCallDelay);

                                        setTimeout(function() {
                                            expect(dispatcherSpy.sendPublication).not
                                                    .toHaveBeenCalled();

                                            // cleanup
                                            stopSubscription(onChangeSubscriptionRequest);
                                        }, asyncGetterCallDelay);
                                    });
                                });

                        it("removes an onChange subscription after endDate", function() {

                            runs(function() {
                                publicationManager.addPublicationProvider(providerId, provider);
                                publicationManager.handleSubscriptionRequest(
                                        proxyId,
                                        providerId,
                                        onChangeSubscriptionRequest);
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
                                }, asyncGetterCallDelay);
                            });
                        });

                        it("removes an onChange subscription after subscription stop", function() {

                            runs(function() {
                                publicationManager.addPublicationProvider(providerId, provider);
                                publicationManager.handleSubscriptionRequest(
                                        proxyId,
                                        providerId,
                                        onChangeSubscriptionRequest);
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
                                publicationManager.handleSubscriptionStop(new SubscriptionStop({
                                    subscriptionId : onChangeSubscriptionRequest.subscriptionId
                                }));
                                dispatcherSpy.sendPublication.reset();

                                testAttribute.valueChanged(value);
                                increaseFakeTime(1);
                                setTimeout(function() {
                                    expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();
                                }, asyncGetterCallDelay);
                            });
                        });

                        it(
                                "creates a mixed subscription and does not send two publications within mininterval in case async getter calls and valueChanged occur at the same time",
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
                                        // wait until the first publication occurs
                                        increaseFakeTime(asyncGetterCallDelay);

                                        // reset first publication
                                        asyncTestAttribute.get.reset();
                                        dispatcherSpy.sendPublication.reset();

                                        // let the minInterval exceed, so that new value changes
                                        // immediately lead to publications
                                        increaseFakeTime(minInterval);
                                        expect(asyncTestAttribute.get).not.toHaveBeenCalled();
                                        expect(dispatcherSpy.sendPublication).not
                                                .toHaveBeenCalled();

                                        asyncTestAttribute.valueChanged(value);
                                        increaseFakeTime(5);

                                        // this should cause an async timer, which sends a publication
                                        // after mininterval-5
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
                                        // now, lets increas the time until mininterval
                                        increaseFakeTime(minInterval - 5);

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
                                        // asyncGetterCallDelay<minInterval and the time
                                        // delay between two publications must be at least minInterval
                                        expect(dispatcherSpy.sendPublication.callCount).toEqual(2);

                                        increaseFakeTime(minInterval - asyncGetterCallDelay);
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
                                    });
                                });

                        it(
                                "creates a mixed subscription that publishes valueChanges that occur each minInterval",
                                function() {
                                    runs(function() {
                                        publicationManager.addPublicationProvider(
                                                providerId,
                                                provider);
                                        publicationManager.handleSubscriptionRequest(
                                                proxyId,
                                                providerId,
                                                mixedSubscriptionRequest);
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
                                                    increaseFakeTime(mixedSubscriptionRequest.qos.minInterval);
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
                                    });
                                });
                        it(
                                "creates a mixed subscription that publishes many valueChanges within minInterval only once",
                                function() {

                                    runs(function() {
                                        publicationManager.addPublicationProvider(
                                                providerId,
                                                provider);
                                        publicationManager.handleSubscriptionRequest(
                                                proxyId,
                                                providerId,
                                                mixedSubscriptionRequest);
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
                                                        .round((mixedSubscriptionRequest.qos.minInterval - 2)
                                                            / maxNrOfTimes);
                                        for (times = 0; times < maxNrOfTimes; times++) {
                                            expect(testAttribute.get).not.toHaveBeenCalled();
                                            expect(dispatcherSpy.sendPublication).not
                                                    .toHaveBeenCalled();
                                            increaseFakeTime(shortInterval);
                                            testAttribute.valueChanged(value + times);
                                        }

                                        // after minInterval the publication works again
                                        increaseFakeTime(mixedSubscriptionRequest.qos.minInterval
                                            - shortInterval
                                            * maxNrOfTimes);
                                        testAttribute.valueChanged(value);
                                        expect(testAttribute.get.callCount).toEqual(1);
                                        expect(dispatcherSpy.sendPublication.callCount).toEqual(1);

                                        // cleanup
                                        stopSubscription(mixedSubscriptionRequest);
                                    });
                                });

                        it(
                                "creates a periodic subscription without expiryDate and expects periodic publications",
                                function() {
                                    var period = 400, n = 10, subscriptionRequestWithoutExpiryDate;

                                    runs(function() {
                                        subscriptionRequestWithoutExpiryDate =
                                                createSubscriptionRequest(
                                                        testAttributeName,
                                                        period,
                                                        0,
                                                        false,
                                                        minInterval);
                                        dispatcherSpy.sendPublication.reset();
                                        publicationManager.addPublicationProvider(
                                                providerId,
                                                provider);
                                        publicationManager.handleSubscriptionRequest(
                                                proxyId,
                                                providerId,
                                                subscriptionRequestWithoutExpiryDate);
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

                                        // minInterval and a value change the value should be reported
                                        increaseFakeTime(mixedSubscriptionRequest.qos.minInterval);
                                    });

                                    waitsFor(
                                            function() {
                                                return dispatcherSpy.sendPublication.callCount === 1;
                                            },
                                            "timeout dispatcherSpy.sendPublication call",
                                            asyncGetterCallDelay);

                                    runs(function() {
                                        // due to minInterval exceeded + valueChanged has been occured
                                        // within the minInterval, the
                                        // PublicationManager
                                        // send the current attribute value to the subscriptions
                                        expect(testAttribute.get.callCount).toEqual(1);
                                        expect(dispatcherSpy.sendPublication.callCount).toEqual(1);
                                        testAttribute.get.reset();
                                        dispatcherSpy.sendPublication.reset();
                                        // minInterval and no publication shall occur
                                        increaseFakeTime(mixedSubscriptionRequest.qos.minInterval);
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

                                        // at time mixedSubscriptionRequest.qos.minInterval the last
                                        // value of the test attribute is sent
                                        // to the subscribers as the publication timeout occurs
                                        dispatcherSpy.sendPublication.reset();
                                        increaseFakeTime(mixedSubscriptionRequest.qos.minInterval
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
                                        testAttribute.get.reset();
                                        dispatcherSpy.sendPublication.reset();
                                        increaseFakeTime(mixedSubscriptionRequest.qos.minInterval);
                                        testAttribute.valueChanged(value);
                                        expect(testAttribute.get).not.toHaveBeenCalled();
                                    });
                                });
                        it(
                                "does not publish when mixed subscription has an endDate in the past",
                                function() {
                                    mixedSubscriptionRequest.qos.expiryDate = Date.now() - 1;

                                    runs(function() {
                                        publicationManager.addPublicationProvider(
                                                providerId,
                                                provider);
                                        publicationManager.handleSubscriptionRequest(
                                                proxyId,
                                                providerId,
                                                mixedSubscriptionRequest);
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
                                    });
                                });

                        it("removes a mixed subscription after endDate", function() {
                            runs(function() {
                                publicationManager.addPublicationProvider(providerId, provider);
                                publicationManager.handleSubscriptionRequest(
                                        proxyId,
                                        providerId,
                                        mixedSubscriptionRequest);
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
                            });
                        });

                        it("removes a mixed subscription after subscription stop", function() {
                            runs(function() {
                                publicationManager.addPublicationProvider(providerId, provider);
                                publicationManager.handleSubscriptionRequest(
                                        proxyId,
                                        providerId,
                                        mixedSubscriptionRequest);
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
                                publicationManager.handleSubscriptionStop(new SubscriptionStop({
                                    subscriptionId : mixedSubscriptionRequest.subscriptionId
                                }));
                                testAttribute.get.reset();
                                dispatcherSpy.sendPublication.reset();

                                increaseFakeTime(maxInterval); // increase interval
                                testAttribute.valueChanged(value); // do change
                                increaseFakeTime(maxInterval); // increase interval

                                setTimeout(function() {
                                    expect(testAttribute.get).not.toHaveBeenCalled();
                                    expect(dispatcherSpy.sendPublication).not.toHaveBeenCalled();
                                    stopSubscription(mixedSubscriptionRequest);
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
                                    });
                                });
                    });

        });
