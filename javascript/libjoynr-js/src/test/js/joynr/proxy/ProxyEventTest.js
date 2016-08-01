/*jslint es5: true */
/*global fail: true */

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
    "global/Promise",
    "global/WaitsFor"
], function(
        ProxyAttribute,
        ProxyOperation,
        ProxyEvent,
        DiscoveryQos,
        MessagingQos,
        OnChangeSubscriptionQos,
        Promise, waitsFor) {

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

        beforeEach(function(done) {
            var fakeProxy = {
                fromParticipantId : "proxyParticipantId",
                toParticipantId : "providerParticipantId"
            };

            subscriptionManagerSpy = jasmine.createSpyObj("subscriptionManager", [
                "registerBroadcastSubscription",
                "unregisterSubscription"
            ]);
            subscriptionId = "subscriptionId";

            subscriptionManagerSpy.registerBroadcastSubscription.and.returnValue(Promise.resolve(subscriptionId));
            subscriptionManagerSpy.unregisterSubscription.and.returnValue(Promise.resolve());

            weakSignal = new ProxyEvent(fakeProxy, {
                broadcastName : "weakSignal",
                discoveryQos : new DiscoveryQos(),
                messagingQos : new MessagingQos(),
                dependencies : {
                    subscriptionManager : subscriptionManagerSpy
                },
                filterParameters: {
                    "a": "reservedForTypeInfo",
                    "b": "reservedForTypeInfo",
                    "c": "reservedForTypeInfo"
                }
            });
            done();
        });

        it("is of correct type", function(done) {
            expect(weakSignal).toBeDefined();
            expect(weakSignal).not.toBeNull();
            expect(typeof weakSignal === "object").toBeTruthy();
            expect(weakSignal instanceof ProxyEvent).toBeTruthy();
            done();
        });

        it("has correct members", function(done) {
            expect(weakSignal.subscribe).toBeDefined();
            expect(weakSignal.unsubscribe).toBeDefined();
            done();
        });

        it("subscribe provides a subscriptionId", function(done) {
            var spy = jasmine.createSpyObj("spy", [
                "onFulfilled",
                "onRejected"
            ]);

            spy.onFulfilled.and.callFake(function(subscriptionId) {
                return subscriptionId;
            });

            weakSignal.subscribe({
                subscriptionQos : subscriptionQos,
                receive : function(value) {}
            }).then(spy.onFulfilled).catch(spy.onRejected).then(function(passedId) {
                expect(passedId).toBeDefined();
                expect(typeof passedId === "string").toBeTruthy();
            });

            waitsFor(function() {
                return spy.onFulfilled.calls.count() > 0;
            }, "The promise is not pending any more", asyncTimeout).then(function() {
                checkSpy(spy);
                done();
                return null;
            }).catch(fail);
        });

        it("subscribe and unsubscribe notify on success", function(done) {

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
            spySubscribePromise.onFulfilled.and.callFake(function(passedSubscriptionId) {
                return passedSubscriptionId;
            });

            var spyUnsubscribePromise = jasmine.createSpyObj("spyUnsubscribePromise", [
                "onFulfilled",
                "onRejected"
            ]);

            weakSignal.subscribe({
                subscriptionQos : subscriptionQos,
                receive : function(value) {}
            }).then(spySubscribePromise.onFulfilled).catch(spySubscribePromise.onRejected).then(function(passedSubscriptionId) {
                return weakSignal.unsubscribe({
                    subscriptionId : passedSubscriptionId
                });
            }).then(function() {
                spyUnsubscribePromise.onFulfilled();
                return null;
            }).catch(function() {
                spyUnsubscribePromise.onRejected();
                return null;
            });

            waitsFor(function() {
                return spyUnsubscribePromise.onFulfilled.calls.count() > 0;
            }, "The promise is not pending any more", asyncTimeout).then(function() {
                expect(spySubscribePromise.onFulfilled).toHaveBeenCalled();
                expect(spySubscribePromise.onRejected).not.toHaveBeenCalled();

                expect(spyUnsubscribePromise.onFulfilled).toHaveBeenCalled();
                expect(spyUnsubscribePromise.onRejected).not.toHaveBeenCalled();
                done();
                return null;
            }).catch(fail);
        });

        it("subscribe rejects if filter parameters are only partially filled", function(done) {
            subscriptionManagerSpy.registerBroadcastSubscription.and.returnValue(Promise.resolve());

            //subscribing without filter parameters should work
            weakSignal.subscribe({
                subscriptionQos : subscriptionQos,
                receive : function(value) {}
            }).then(function() {
                //subscribing with empty filter parameters should work
                return weakSignal.subscribe({
                    subscriptionQos : subscriptionQos,
                    receive : function(value) {},
                    filterParameters : {}
                });
            }).then(function() {
                //subscribing with filter parameters having value null should work
                return weakSignal.subscribe({
                    subscriptionQos : subscriptionQos,
                    receive : function(value) {},
                    filterParameters : null
                });
            }).then(function() {
                //subscribing with partially defined filter parameters should fail
                return weakSignal.subscribe({
                    subscriptionQos : subscriptionQos,
                    receive : function(value) {},
                    filterParameters : {
                        a : "a",
                        b : "b"
                    }
                }).then(fail).catch(done);
            }).catch(fail);
        });

        it("subscribe notifies on failure", function(done) {

            // precondition: the broadcast object has already been created
            expect(weakSignal.subscribe).toBeDefined();
            expect(typeof weakSignal.subscribe === "function").toBeTruthy();

            var promise; // saved to allow jasmine to "waitsFor" its status

            var spySubscribePromise = jasmine.createSpyObj("spySubscribePromise", [
                "onFulfilled",
                "onRejected"
            ]);

            var expectedError = new Error("error registering broadcast");
            subscriptionManagerSpy.registerBroadcastSubscription.and.returnValue(Promise.reject(expectedError));

            promise = weakSignal.subscribe({
                subscriptionQos : subscriptionQos,
                receive : function(value) {}
            }).then(spySubscribePromise.onFulfilled).catch(spySubscribePromise.onRejected);

            waitsFor(function() {
                return spySubscribePromise.onRejected.calls.count() > 0;
            }, "The promise is not pending any more", asyncTimeout).then(function() {
                expect(spySubscribePromise.onFulfilled).not.toHaveBeenCalled();
                expect(spySubscribePromise.onRejected).toHaveBeenCalledWith(expectedError);
                done();
                return null;
            }).catch(fail);
        });

        it("unsubscribe notifies on failure", function(done) {

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
            subscriptionManagerSpy.unregisterSubscription.and.returnValue(Promise.reject(expectedError));

            weakSignal.subscribe({
                subscriptionQos : subscriptionQos,
                receive : function(value) {}
            }).then(function(passedSubscriptionId) {
                return weakSignal.unsubscribe({
                    subscriptionId : passedSubscriptionId
                });
            }).then(spyUnsubscribePromise.onFulfilled).catch(spyUnsubscribePromise.onRejected);

            waitsFor(function() {
                return spyUnsubscribePromise.onRejected.calls.count() > 0;
            }, "The promise is not pending any more", asyncTimeout).then(function() {
                expect(spyUnsubscribePromise.onFulfilled).not.toHaveBeenCalled();
                expect(spyUnsubscribePromise.onRejected).toHaveBeenCalledWith(expectedError);
                done();
                return null;
            }).catch(fail);
        });
    });

}); // require
