/*jslint es5: true */

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

//TODO: some of this relies on the dummy implementation, change accordingly when implementing
define([
    "joynr/proxy/ProxyAttribute",
    "joynr/proxy/ProxyOperation",
    "joynr/proxy/ProxyEvent",
    "joynr/proxy/DiscoveryQos",
    "joynr/messaging/MessagingQos",
    "joynr/proxy/OnChangeSubscriptionQos",
    "global/Promise"
], function(
        ProxyAttribute,
        ProxyOperation,
        ProxyEvent,
        DiscoveryQos,
        MessagingQos,
        OnChangeSubscriptionQos,
        Promise) {

    var asyncTimeout = 5000;

    describe("libjoynr-js.joynr.proxy.ProxyEvent", function() {

        var weakSignal;
        var subscriptionId;
        var subscriptionQos = new OnChangeSubscriptionQos();
        var subscriptionManagerSpy;

        var qosSettings = {
            periodMs : 50,
            expiryDateMs : 3,
            alertAfterIntervalMs : 80,
            publicationTtlMs : 100
        };

        function checkSpy(spy, errorExpected) {

            if (errorExpected) {
                expect(spy.onFulfilled).not.toHaveBeenCalled();
                expect(spy.onRejected).toHaveBeenCalled();
            } else {
                expect(spy.onFulfilled).toHaveBeenCalled();
                expect(spy.onRejected).not.toHaveBeenCalled();
            }
        }

        beforeEach(function() {
            var fakeProxy = {
                fromParticipantId : "proxyParticipantId",
                toParticipantId : "providerParticipantId"
            };

            subscriptionManagerSpy = jasmine.createSpyObj("subscriptionManager", [
                "registerBroadcastSubscription",
                "unregisterSubscription"
            ]);
            subscriptionId = "subscriptionId";

            subscriptionManagerSpy.registerBroadcastSubscription.andReturn(Promise.resolve(subscriptionId));
            subscriptionManagerSpy.unregisterSubscription.andReturn(Promise.resolve());

            weakSignal = new ProxyEvent(fakeProxy, {
                broadcastName : "weakSignal",
                discoveryQos : new DiscoveryQos(),
                messagingQos : new MessagingQos(),
                dependencies : {
                    subscriptionManager : subscriptionManagerSpy
                }
            });
        });

        it("is of correct type", function() {
            expect(weakSignal).toBeDefined();
            expect(weakSignal).not.toBeNull();
            expect(typeof weakSignal === "object").toBeTruthy();
            expect(weakSignal instanceof ProxyEvent).toBeTruthy();
        });

        it("has correct members", function() {
            expect(weakSignal.subscribe).toBeDefined();
            expect(weakSignal.unsubscribe).toBeDefined();
        });

        it("subscribe provides a subscriptionId", function() {
            var spy = jasmine.createSpyObj("spy", [
                "onFulfilled",
                "onRejected"
            ]);

            spy.onFulfilled.andCallFake(function(subscriptionId) {
                return subscriptionId;
            });

            runs(function() {
                weakSignal.subscribe({
                    subscriptionQos : subscriptionQos,
                    receive : function(value) {}
                }).then(spy.onFulfilled).catch(spy.onRejected).then(function(passedId) {
                    expect(passedId).toBeDefined();
                    expect(typeof passedId === "string").toBeTruthy();
                });
            });

            waitsFor(function() {
                return spy.onFulfilled.callCount > 0;
            }, "The promise is not pending any more", asyncTimeout);

            runs(function() {
                checkSpy(spy);
            });
        });

        it("subscribe and unsubscribe notify on success", function() {

            // precondition: the broadcast object has already been created
            expect(weakSignal.subscribe).toBeDefined();
            expect(typeof weakSignal.subscribe === "function").toBeTruthy();
            expect(weakSignal.unsubscribe).toBeDefined();
            expect(typeof weakSignal.unsubscribe === "function").toBeTruthy();

            var spySubscribePromise = jasmine.createSpyObj("spySubscribePromise", [
                "onFulfilled",
                "onRejected"
            ]);
            // the spy returns the subscriptionId that it was called with,
            // so that the next then in the promise chain also can see it
            spySubscribePromise.onFulfilled.andCallFake(function(passedSubscriptionId) {
                return passedSubscriptionId;
            });

            var spyUnsubscribePromise = jasmine.createSpyObj("spyUnsubscribePromise", [
                "onFulfilled",
                "onRejected"
            ]);

            runs(function() {
                weakSignal.subscribe({
                    subscriptionQos : subscriptionQos,
                    receive : function(value) {}
                }).then(spySubscribePromise.onFulfilled).catch(spySubscribePromise.onRejected).then(
                        function(passedSubscriptionId) {
                            return weakSignal.unsubscribe({
                                subscriptionId : passedSubscriptionId
                            });
                        })
                        .then(spyUnsubscribePromise.onFulfilled).catch(spyUnsubscribePromise.onRejected);

            });

            waitsFor(function() {
                return spyUnsubscribePromise.onFulfilled.callCount > 0;
            }, "The promise is not pending any more", asyncTimeout);

            runs(function() {
                expect(spySubscribePromise.onFulfilled).toHaveBeenCalled();
                expect(spySubscribePromise.onRejected).not.toHaveBeenCalled();

                expect(spyUnsubscribePromise.onFulfilled).toHaveBeenCalled();
                expect(spyUnsubscribePromise.onRejected).not.toHaveBeenCalled();

            });
        });
        it("subscribe notifies on failure", function() {

            // precondition: the broadcast object has already been created
            expect(weakSignal.subscribe).toBeDefined();
            expect(typeof weakSignal.subscribe === "function").toBeTruthy();

            var promise; // saved to allow jasmine to "waitsFor" its status

            var spySubscribePromise = jasmine.createSpyObj("spySubscribePromise", [
                "onFulfilled",
                "onRejected"
            ]);

            var expectedError = new Error("error registering broadcast");
            subscriptionManagerSpy.registerBroadcastSubscription.andReturn(Promise.reject(expectedError));

            runs(function() {
                promise = weakSignal.subscribe({
                    subscriptionQos : subscriptionQos,
                    receive : function(value) {}
                }).then(spySubscribePromise.onFulfilled).catch(spySubscribePromise.onRejected);

            });

            waitsFor(function() {
                return spySubscribePromise.onRejected.callCount > 0;
            }, "The promise is not pending any more", asyncTimeout);

            runs(function() {
                expect(spySubscribePromise.onFulfilled).not.toHaveBeenCalled();
                expect(spySubscribePromise.onRejected).toHaveBeenCalledWith(expectedError);
            });
        });

        it("unsubscribe notifies on failure", function() {

            // precondition: the broadcast object has already been created
            expect(weakSignal.subscribe).toBeDefined();
            expect(typeof weakSignal.subscribe === "function").toBeTruthy();
            expect(weakSignal.unsubscribe).toBeDefined();
            expect(typeof weakSignal.unsubscribe === "function").toBeTruthy();

            var spyUnsubscribePromise = jasmine.createSpyObj("spyUnsubscribePromise", [
                "onFulfilled",
                "onRejected"
            ]);

            var expectedError = new Error("error unsubscribing from broadcast");
            subscriptionManagerSpy.unregisterSubscription.andReturn(Promise.reject(expectedError));

            runs(function() {
                weakSignal.subscribe({
                    subscriptionQos : subscriptionQos,
                    receive : function(value) {}
                }).then(function(passedSubscriptionId) {
                    return weakSignal.unsubscribe({
                        subscriptionId : passedSubscriptionId
                    });
                }).then(spyUnsubscribePromise.onFulfilled).catch(spyUnsubscribePromise.onRejected);

            });

            waitsFor(function() {
                return spyUnsubscribePromise.onRejected.callCount > 0;
            }, "The promise is not pending any more", asyncTimeout);

            runs(function() {
                expect(spyUnsubscribePromise.onFulfilled).not.toHaveBeenCalled();
                expect(spyUnsubscribePromise.onRejected).toHaveBeenCalledWith(expectedError);
            });
        });
    });

}); // require
