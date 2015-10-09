/*global joynrTestRequire: true, xit: true */
/*jslint es5: true */

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
        "joynr/dispatching/subscription/TestSubscriptionManager",
        [
            "joynr/dispatching/subscription/SubscriptionManager",
            "joynr/TypesEnum",
            "joynr/messaging/MessagingQos",
            "joynr/dispatching/types/SubscriptionRequest",
            "joynr/dispatching/types/SubscriptionStop",
            "joynr/proxy/OnChangeWithKeepAliveSubscriptionQos",
            "joynr/proxy/OnChangeSubscriptionQos",
            "joynr/proxy/SubscriptionQos",
            "joynr/dispatching/types/SubscriptionPublication",
            "global/Promise",
            "joynr/dispatching/types/Reply",
            "joynr/system/LoggerFactory",
            "Date",
            "joynr/tests/testTypes/TestEnum"
        ],
        function(
                SubscriptionManager,
                TypesEnum,
                MessagingQos,
                SubscriptionRequest,
                SubscriptionStop,
                OnChangeWithKeepAliveSubscriptionQos,
                OnChangeSubscriptionQos,
                SubscriptionQos,
                SubscriptionPublication,
                Promise,
                Reply,
                LoggerFactory,
                Date,
                TestEnum) {

            describe(
                    "libjoynr-js.joynr.dispatching.subscription.SubscriptionManager",
                    function() {
                        var subscriptionManager;
                        var log =
                                LoggerFactory
                                        .getLogger("joynr.dispatching.TestSubscriptionManager");
                        var fakeTime = 1371553100000;
                        var dispatcherSpy;

                        /**
                         * Called before each test.
                         */
                        beforeEach(function() {
                            dispatcherSpy = jasmine.createSpyObj("DispatcherSpy", [
                                "sendSubscriptionRequest",
                                "sendBroadcastSubscriptionRequest",
                                "sendSubscriptionStop"
                            ]);

                            dispatcherSpy.sendBroadcastSubscriptionRequest
                                    .andCallFake(function(settings) {
                                        subscriptionManager
                                                .handleSubscriptionReply({
                                                    subscriptionId : settings.subscriptionRequest.subscriptionId
                                                });
                                        return Promise.resolve(settings.subscriptionRequest.subscriptionId);
                                    });

                            dispatcherSpy.sendSubscriptionRequest
                                    .andCallFake(function(settings) {
                                        subscriptionManager
                                                .handleSubscriptionReply({
                                                    subscriptionId : settings.subscriptionRequest.subscriptionId
                                                });
                                        return Promise.resolve(settings.subscriptionRequest.subscriptionId);
                                    });

                            subscriptionManager = new SubscriptionManager(dispatcherSpy);
                            jasmine.Clock.useMock();
                            jasmine.Clock.reset();
                            spyOn(Date, 'now').andCallFake(function() {
                                return fakeTime;
                            });
                        });

                        function increaseFakeTime(time_ms) {
                            fakeTime = fakeTime + time_ms;
                            jasmine.Clock.tick(time_ms);
                        }

                        it("is instantiable", function() {
                            expect(subscriptionManager).toBeDefined();
                        });

                        it(
                                "sends broadcast subscription requests",
                                function() {
                                    var ttl = 250;
                                    var parameters = {
                                        proxyId : "subscriber",
                                        providerId : "provider",
                                        messagingQos : new MessagingQos({
                                            ttl : ttl
                                        }),
                                        broadcastName : "broadcastName",
                                        subscriptionQos : new OnChangeSubscriptionQos({
                                            expiryDate : Date.now() + ttl
                                        })
                                    };

                                    runs(function() {
                                        dispatcherSpy.sendBroadcastSubscriptionRequest.reset();
                                        var spySubscribePromise =
                                                jasmine.createSpyObj("spySubscribePromise", [
                                                    "resolve",
                                                    "reject"
                                                ]);

                                        subscriptionManager.registerBroadcastSubscription(
                                                parameters).then(
                                                spySubscribePromise.resolve).catch(spySubscribePromise.reject);
                                        increaseFakeTime(1);
                                    });

                                    waitsFor(
                                            function() {
                                                return dispatcherSpy.sendBroadcastSubscriptionRequest.callCount === 1;
                                            },
                                            "dispatcherSpy.sendBroadcastSubscriptionRequest called",
                                            100);

                                    runs(function() {
                                        expect(dispatcherSpy.sendBroadcastSubscriptionRequest)
                                                .toHaveBeenCalled();
                                        expect(
                                                dispatcherSpy.sendBroadcastSubscriptionRequest.calls[0].args[0].messagingQos.ttl)
                                                .toEqual(ttl);
                                        expect(
                                                dispatcherSpy.sendBroadcastSubscriptionRequest.calls[0].args[0].to)
                                                .toEqual(parameters.providerId);
                                        expect(
                                                dispatcherSpy.sendBroadcastSubscriptionRequest.calls[0].args[0].from)
                                                .toEqual(parameters.proxyId);
                                        expect(
                                                dispatcherSpy.sendBroadcastSubscriptionRequest.calls[0].args[0].subscriptionRequest.subscribedToName)
                                                .toEqual(parameters.broadcastName);
                                    });
                                });

                        it("alerts on missed publication and stops", function() {
                            var publicationReceivedSpy =
                                    jasmine.createSpy('publicationReceivedSpy');
                            var publicationMissedSpy = jasmine.createSpy('publicationMissedSpy');

                            runs(function() {
                                //log.debug("registering subscription");
                                subscriptionManager.registerSubscription({
                                    proxyId : "subscriber",
                                    providerId : "provider",
                                    messagingQos : new MessagingQos(),
                                    attributeName : "testAttribute",
                                    qos : new OnChangeWithKeepAliveSubscriptionQos({
                                        alertAfterInterval : 100,
                                        expiryDate : Date.now() + 250
                                    }),
                                    onReceive : publicationReceivedSpy,
                                    onError : publicationMissedSpy
                                }).catch(function(error) {
                                    log.error("Error in sendSubscriptionRequest :" + error);
                                });
                                increaseFakeTime(50);
                                expect(publicationMissedSpy).not.toHaveBeenCalled();
                                increaseFakeTime(51);
                                expect(publicationMissedSpy).toHaveBeenCalled();
                                increaseFakeTime(101);
                                expect(publicationMissedSpy.callCount).toEqual(2);
                                // expiryDate should be reached, expect no more interactions
                                increaseFakeTime(101);
                                expect(publicationMissedSpy.callCount).toEqual(2);
                                increaseFakeTime(101);
                                expect(publicationMissedSpy.callCount).toEqual(2);
                            });

                        });

                        it(
                                "sets messagingQos.ttl correctly according to subscriptionQos.expiryDate",
                                function() {
                                    var ttl = 250;
                                    var subscriptionSettings = {
                                        proxyId : "subscriber",
                                        providerId : "provider",
                                        messagingQos : new MessagingQos(),
                                        attributeName : "testAttribute",
                                        qos : new OnChangeWithKeepAliveSubscriptionQos({
                                            alertAfterInterval : 100,
                                            expiryDate : Date.now() + ttl
                                        }),
                                        onReceive : function() {},
                                        onError : function() {}
                                    };

                                    runs(function() {
                                        dispatcherSpy.sendSubscriptionRequest.reset();
                                        subscriptionManager.registerSubscription(
                                                subscriptionSettings).catch(function(error) {
                                                    expect(
                                                            "Error in sendSubscriptionRequest :"
                                                                + error).toBeTruthy();
                                                });
                                        increaseFakeTime(1);
                                    });

                                    waitsFor(
                                            function() {
                                                return dispatcherSpy.sendSubscriptionRequest.callCount === 1;
                                            },
                                            "dispatcherSpy.sendSubscriptionRequest called the first time",
                                            100);

                                    runs(function() {
                                        expect(
                                                dispatcherSpy.sendSubscriptionRequest.calls[0].args[0].messagingQos.ttl)
                                                .toEqual(ttl);
                                        subscriptionSettings.qos.expiryDate =
                                                SubscriptionQos.NO_EXPIRY_DATE;
                                        subscriptionManager.registerSubscription(
                                                subscriptionSettings).catch(function(error) {
                                                    expect(
                                                            "Error in sendSubscriptionRequest :"
                                                                + error).toBeTruthy();
                                                });
                                        increaseFakeTime(1);
                                    });

                                    waitsFor(
                                            function() {
                                                return dispatcherSpy.sendSubscriptionRequest.callCount === 2;
                                            },
                                            "dispatcherSpy.sendSubscriptionRequest called the first time",
                                            100);

                                    runs(function() {
                                        expect(dispatcherSpy.sendSubscriptionRequest.callCount)
                                                .toEqual(2);
                                        expect(
                                                dispatcherSpy.sendSubscriptionRequest.calls[1].args[0].messagingQos.ttl)
                                                .toEqual(
                                                        SubscriptionQos.NO_EXPIRY_DATE_TTL
                                                            - (fakeTime - 1));
                                    });

                                });

                        it("forwards publication payload", function() {
                            var publicationReceivedSpy =
                                    jasmine.createSpy('publicationReceivedSpy');
                            var publicationMissedSpy = jasmine.createSpy('publicationMissedSpy');

                            var resolveSpy =
                                    {
                                        // called when the subscription is registered successfully (see below)
                                        resolveMethod : function(subscriptionId) {
                                            // increase time by 50ms and see if alert was triggered
                                            increaseFakeTime(50);
                                            expect(publicationMissedSpy).not.toHaveBeenCalled();
                                            var publication = new SubscriptionPublication({
                                                response : ["test"],
                                                subscriptionId : subscriptionId
                                            });
                                            // simulate incoming publication
                                            subscriptionManager.handlePublication(publication);
                                            // make sure publication payload is forwarded
                                            expect(publicationReceivedSpy).toHaveBeenCalledWith(
                                                    publication.response[0]);
                                            increaseFakeTime(51);
                                            // make sure no alert is triggered if publication is received
                                            expect(publicationMissedSpy).not.toHaveBeenCalled();
                                            increaseFakeTime(101);
                                            // if no publications follow alert should be triggered again
                                            expect(publicationMissedSpy).toHaveBeenCalled();

                                        }
                                    };

                            spyOn(resolveSpy, 'resolveMethod').andCallThrough();

                            runs(function() {
                                //log.debug("registering subscription");
                                // register the subscription and call the resolve method when ready
                                subscriptionManager.registerSubscription({
                                    proxyId : "subscriber",
                                    providerId : "provider",
                                    messagingQos : new MessagingQos(),
                                    attributeName : "testAttribute",
                                    qos : new OnChangeWithKeepAliveSubscriptionQos({
                                        alertAfterInterval : 100,
                                        expiryDate : Date.now() + 250
                                    }),
                                    onReceive : publicationReceivedSpy,
                                    onError : publicationMissedSpy
                                }).then(resolveSpy.resolveMethod);
                                increaseFakeTime(1);
                            });

                            waitsFor(function() {
                                // wait until the subscriptionReply was received
                                return resolveSpy.resolveMethod.callCount > 0;
                            }, "resolveSpy.resolveMethod called", 100);

                        });

                        it("augments incoming publications with information from the joynr type system", function() {
                            var publicationReceivedSpy =
                                    jasmine.createSpy('publicationReceivedSpy');
                            var publicationMissedSpy = jasmine.createSpy('publicationMissedSpy');

                            var resolveSpy =
                                    {
                                        // called when the subscription is registered successfully (see below)
                                        resolveMethod : function(subscriptionId) {
                                            // increase time by 50ms and see if alert was triggered
                                            increaseFakeTime(50);
                                            expect(publicationMissedSpy).not.toHaveBeenCalled();
                                            var publication = new SubscriptionPublication({
                                                response : [ "ZERO" ],
                                                subscriptionId : subscriptionId
                                            });
                                            // simulate incoming publication
                                            subscriptionManager.handlePublication(publication);
                                            // make sure publication payload is forwarded
                                            expect(publicationReceivedSpy).toHaveBeenCalledWith(
                                                    TestEnum.ZERO);

                                        }
                                    };

                            spyOn(resolveSpy, 'resolveMethod').andCallThrough();

                            runs(function() {
                                //log.debug("registering subscription");
                                // register the subscription and call the resolve method when ready
                                /*jslint nomen: true */
                                subscriptionManager.registerSubscription({
                                    proxyId : "subscriber",
                                    providerId : "provider",
                                    messagingQos : new MessagingQos(),
                                    attributeName : "testAttribute",
                                    attributeType : TestEnum.ZERO._typeName,
                                    qos : new OnChangeSubscriptionQos({
                                        expiryDate : Date.now() + 250
                                    }),
                                    onReceive : publicationReceivedSpy,
                                    onError : publicationMissedSpy
                                }).then(resolveSpy.resolveMethod);
                                increaseFakeTime(1);
                                /*jslint nomen: false */
                            });

                            waitsFor(function() {
                                // wait until the subscriptionReply was received
                                return resolveSpy.resolveMethod.callCount > 0;
                            }, "resolveSpy.resolveMethod called", 100);

                        });

                        it("augments incoming broadcasts with information from the joynr type system", function() {
                            var publicationReceivedSpy =
                                    jasmine.createSpy('publicationReceivedSpy');
                            var onErrorSpy = jasmine.createSpy('onErrorSpy');

                            var resolveSpy =
                                    {
                                        // called when the subscription is registered successfully (see below)
                                        resolveMethod : function(subscriptionId) {
                                            var testString = "testString";
                                            var testInt = 2;
                                            var testEnum = TestEnum.ZERO;
                                            expect(onErrorSpy).not.toHaveBeenCalled();
                                            var publication = new SubscriptionPublication({
                                                response : [ testString, testInt, testEnum.name ],
                                                subscriptionId : subscriptionId
                                            });
                                            // simulate incoming publication
                                            subscriptionManager.handlePublication(publication);
                                            // make sure publication payload is forwarded
                                            expect(publicationReceivedSpy).toHaveBeenCalledWith(
                                                    [ testString, testInt, testEnum ]);

                                        }
                                    };

                            spyOn(resolveSpy, 'resolveMethod').andCallThrough();

                            runs(function() {
                                //log.debug("registering subscription");
                                // register the subscription and call the resolve method when ready
                                /*jslint nomen: true */
                                subscriptionManager.registerBroadcastSubscription({
                                    proxyId : "subscriber",
                                    providerId : "provider",
                                    messagingQos : new MessagingQos(),
                                    broadcastName : "broadcastName",
                                    broadcastParameter : [
                                         {
                                             name : "param1",
                                             type : TypesEnum.STRING,
                                         },
                                         {
                                             name : "param2",
                                             type : TypesEnum.INT,
                                         },
                                         {
                                             name : "param3",
                                             type : TestEnum.ZERO._typeName,
                                         }
                                    ],
                                    qos : new OnChangeSubscriptionQos({
                                        expiryDate : Date.now() + 250
                                    }),
                                    onReceive : publicationReceivedSpy,
                                    onError : onErrorSpy
                                }).then(resolveSpy.resolveMethod);
                                increaseFakeTime(1);
                                /*jslint nomen: false */
                            });

                            waitsFor(function() {
                                // wait until the subscriptionReply was received
                                return resolveSpy.resolveMethod.callCount > 0;
                            }, "resolveSpy.resolveMethod called", 100);

                        });

                        it(
                                "sends out subscriptionStop and stops alerts on unsubscribe",
                                function() {
                                    var publicationReceivedSpy =
                                            jasmine.createSpy('publicationReceivedSpy');
                                    var publicationMissedSpy =
                                            jasmine.createSpy('publicationMissedSpy');
                                    var subscriptionId;

                                    runs(function() {
                                        //log.debug("registering subscription");
                                        subscriptionManager.registerSubscription({
                                            proxyId : "subscriber",
                                            providerId : "provider",
                                            messagingQos : new MessagingQos(),
                                            attributeName : "testAttribute",
                                            qos : new OnChangeWithKeepAliveSubscriptionQos({
                                                alertAfterInterval : 100,
                                                expiryDate : Date.now() + 10000
                                            }),
                                            onReceive : publicationReceivedSpy,
                                            onError : publicationMissedSpy
                                        }).then(
                                                function(returnedSubscriptionId) {
                                                    subscriptionId = returnedSubscriptionId;
                                                }).catch(function(error) {
                                                    log.error("Error in sendSubscriptionRequest :"
                                                        + error);
                                                });
                                        increaseFakeTime(1);
                                    });

                                    waitsFor(
                                            function() {
                                                return subscriptionId !== undefined;
                                            },
                                            "SubscriptionId should be set by the SubscriptionManager",
                                            500);

                                    runs(function() {
                                        increaseFakeTime(50);
                                        expect(publicationMissedSpy).not.toHaveBeenCalled();
                                        increaseFakeTime(51);
                                        expect(publicationMissedSpy).toHaveBeenCalled();
                                        increaseFakeTime(101);
                                        expect(publicationMissedSpy.callCount).toEqual(2);

                                        // unsubscribe and expect no more missed publication alerts
                                        var unsubscrMsgQos = new MessagingQos();
                                        subscriptionManager.unregisterSubscription({
                                            subscriptionId : subscriptionId,
                                            messagingQos : unsubscrMsgQos
                                        });

                                        var subscriptionStop = new SubscriptionStop({
                                            subscriptionId : subscriptionId
                                        });

                                        expect(dispatcherSpy.sendSubscriptionStop)
                                                .toHaveBeenCalledWith({
                                                    from : "subscriber",
                                                    to : "provider",
                                                    subscriptionStop : subscriptionStop,
                                                    messagingQos : unsubscrMsgQos
                                                });
                                        increaseFakeTime(101);
                                        expect(publicationMissedSpy.callCount).toEqual(2);
                                        increaseFakeTime(101);
                                        expect(publicationMissedSpy.callCount).toEqual(2);
                                        increaseFakeTime(101);
                                        expect(publicationMissedSpy.callCount).toEqual(2);
                                    });
                                });

                    xit(
                    "returns a rejected promise when unsubscribing with a non-existant subscriptionId",
                    function(done) {
                            subscriptionManager.unregisterSubscription({
                                subscriptionId : "non-existant"
                            }).then().catch(function(value){
                                expect(value).toBeDefined();
                                var className = Object.prototype.toString.call(value).slice(8, -1);
                                expect(className).toMatch("Error");
                                done();
                            });
                    });
              });
        });