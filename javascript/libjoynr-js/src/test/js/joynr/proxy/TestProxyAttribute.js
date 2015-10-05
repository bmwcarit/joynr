/*jslint es5: true, nomen: true */
/*global joynrTestRequire: true */

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

//TODO: some of this relies on the dummy implementation, change accordingly when implementating
joynrTestRequire(
        "joynr/proxy/TestProxyAttribute",
        [
            "joynr/TypesEnum",
            "joynr/proxy/ProxyAttribute",
            "joynr/proxy/ProxyAttributeNotifyReadWrite",
            "joynr/proxy/ProxyAttributeNotifyRead",
            "joynr/proxy/ProxyAttributeNotifyWrite",
            "joynr/proxy/ProxyAttributeNotify",
            "joynr/proxy/ProxyAttributeReadWrite",
            "joynr/proxy/ProxyAttributeRead",
            "joynr/proxy/ProxyAttributeWrite",
            "joynr/proxy/DiscoveryQos",
            "joynr/messaging/MessagingQos",
            "joynr/proxy/OnChangeWithKeepAliveSubscriptionQos",
            "joynr/dispatching/RequestReplyManager",
            "joynr/dispatching/types/Request",
            "global/Promise"
        ],
        function(
                TypesEnum,
                ProxyAttribute,
                ProxyAttributeNotifyReadWrite,
                ProxyAttributeNotifyRead,
                ProxyAttributeNotifyWrite,
                ProxyAttributeNotify,
                ProxyAttributeReadWrite,
                ProxyAttributeRead,
                ProxyAttributeWrite,
                DiscoveryQos,
                MessagingQos,
                OnChangeWithKeepAliveSubscriptionQos,
                RequestReplyManager,
                Request,
                Promise) {

            var asyncTimeout = 5000;

            describe(
                    "libjoynr-js.joynr.proxy.ProxyAttribute",
                    function() {
                        var settings;
                        var isOn;
                        var isOnNotifyReadOnly;
                        var isOnNotifyWriteOnly;
                        var isOnNotify;
                        var isOnReadWrite;
                        var isOnReadOnly;
                        var isOnWriteOnly;
                        var isOnProxyAttributeNotifyReadWrite;
                        var isOnProxyAttributeNotifyRead;
                        var isOnProxyAttributeNotifyWrite;
                        var isOnProxyAttributeNotify;
                        var isOnProxyAttributeReadWrite;
                        var isOnProxyAttributeRead;
                        var isOnProxyAttributeWrite;
                        var subscriptionQos;
                        var messagingQos;
                        var requestReplyManagerSpy;
                        var subscriptionId;
                        var subscriptionManagerSpy;
                        var proxyParticipantId;
                        var providerParticipantId;

                        function checkSpy(spy, errorExpected, successArg, errorArg) {
                            if (errorExpected) {
                                if (spy.onFulfilled) {
                                    expect(spy.onFulfilled).not.toHaveBeenCalled();
                                }

                                if (spy.onRejected) {
                                    expect(spy.onRejected).toHaveBeenCalled();
                                }
                                if (errorArg) {
                                    if (spy.onRejected) {
                                        expect(spy.onRejected).toHaveBeenCalledWith(errorArg);
                                    }
                                }
                            } else {
                                if (spy.onFulfilled) {
                                    expect(spy.onFulfilled).toHaveBeenCalled();
                                }

                                if (spy.onRejected) {
                                    expect(spy.onRejected).not.toHaveBeenCalled();
                                }
                            }
                        }

                        function RadioStation(name, station, source) {
                            if (!(this instanceof RadioStation)) {
                                // in case someone calls constructor without new keyword (e.g. var c
                                // = Constructor({..}))
                                return new RadioStation(name, station, source);
                            }
                            this.name = name;
                            this.station = station;
                            this.source = source;
                            this.checkMembers = jasmine.createSpy("checkMembers");

                            Object.defineProperty(this, "_typeName", {
                                configurable : false,
                                writable : false,
                                enumerable : true,
                                value : "test.RadioStation"
                            });
                        }

                        beforeEach(function() {
                            subscriptionQos = new OnChangeWithKeepAliveSubscriptionQos();
                            messagingQos = new MessagingQos();

                            requestReplyManagerSpy =
                                    jasmine.createSpyObj("requestReplyManager", [ "sendRequest"
                                    ]);
                            requestReplyManagerSpy.sendRequest.andReturn(Promise.resolve({
                                result : {
                                    resultKey : "resultValue"
                                }
                            }));

                            subscriptionId = {
                                tokenId : "someId",
                                tokenUserData : "some additional data, do not touch!"
                            };
                            subscriptionManagerSpy = jasmine.createSpyObj("subscriptionManager", [
                                "registerSubscription",
                                "unregisterSubscription"
                            ]);
                            subscriptionManagerSpy.registerSubscription.andReturn(Promise.resolve(subscriptionId));
                            subscriptionManagerSpy.unregisterSubscription.andReturn(Promise.resolve(subscriptionId));

                            settings = {
                                dependencies : {
                                    requestReplyManager : requestReplyManagerSpy,
                                    subscriptionManager : subscriptionManagerSpy
                                }
                            };

                            proxyParticipantId = "proxyParticipantId";
                            providerParticipantId = "providerParticipantId";
                            var proxy = {
                                proxyParticipantId : proxyParticipantId,
                                providerParticipantId : providerParticipantId
                            };

                            isOn =
                                    new ProxyAttribute(
                                            proxy,
                                            settings,
                                            "isOn",
                                            TypesEnum.BOOL,
                                            "NOTIFYREADWRITE");
                            isOnNotifyReadOnly =
                                    new ProxyAttribute(
                                            proxy,
                                            settings,
                                            "isOnNotifyReadOnly",
                                            TypesEnum.BOOL,
                                            "NOTIFYREADONLY");
                            isOnNotifyWriteOnly =
                                    new ProxyAttribute(
                                            proxy,
                                            settings,
                                            "isOnNotifyWriteOnly",
                                            TypesEnum.BOOL,
                                            "NOTIFYWRITEONLY");
                            isOnNotify =
                                    new ProxyAttribute(
                                            proxy,
                                            settings,
                                            "isOnNotify",
                                            TypesEnum.BOOL,
                                            "NOTIFY");
                            isOnReadWrite =
                                    new ProxyAttribute(
                                            proxy,
                                            settings,
                                            "isOnReadWrite",
                                            TypesEnum.BOOL,
                                            "READWRITE");
                            isOnReadOnly =
                                    new ProxyAttribute(
                                            proxy,
                                            settings,
                                            "isOnReadOnly",
                                            TypesEnum.BOOL,
                                            "READONLY");
                            isOnWriteOnly =
                                    new ProxyAttribute(
                                            proxy,
                                            settings,
                                            "isOnWriteOnly",
                                            TypesEnum.BOOL,
                                            "WRITEONLY");

                            isOnProxyAttributeNotifyReadWrite =
                                    new ProxyAttributeNotifyReadWrite(
                                            proxy,
                                            settings,
                                            "isOnProxyAttributeNotifyReadWrite",
                                            TypesEnum.BOOL);
                            isOnProxyAttributeNotifyRead =
                                    new ProxyAttributeNotifyRead(
                                            proxy,
                                            settings,
                                            "isOnProxyAttributeNotifyRead",
                                            TypesEnum.BOOL);
                            isOnProxyAttributeNotifyWrite =
                                    new ProxyAttributeNotifyWrite(
                                            proxy,
                                            settings,
                                            "isOnProxyAttributeNotifyWrite",
                                            TypesEnum.BOOL);
                            isOnProxyAttributeNotify =
                                    new ProxyAttributeNotify(
                                            proxy,
                                            settings,
                                            "isOnProxyAttributeNotify",
                                            TypesEnum.BOOL);
                            isOnProxyAttributeReadWrite =
                                    new ProxyAttributeReadWrite(
                                            proxy,
                                            settings,
                                            "isOnProxyAttributeReadWrite",
                                            TypesEnum.BOOL);
                            isOnProxyAttributeRead =
                                    new ProxyAttributeRead(
                                            proxy,
                                            settings,
                                            "isOnProxyAttributeRead",
                                            TypesEnum.BOOL);
                            isOnProxyAttributeWrite =
                                    new ProxyAttributeWrite(
                                            proxy,
                                            settings,
                                            "isOnProxyAttributeWrite",
                                            TypesEnum.BOOL);
                        });

                        it("is of correct type (ProxyAttribute)", function() {
                            expect(isOn).toBeDefined();
                            expect(isOn).not.toBeNull();
                            expect(typeof isOn === "object").toBeTruthy();
                            expect(isOn instanceof ProxyAttribute).toBeTruthy();
                        });

                        it("has correct members (ProxyAttribute with NOTIFYREADWRITE)", function() {
                            expect(isOn.get).toBeDefined();
                            expect(isOn.set).toBeDefined();
                            expect(isOn.subscribe).toBeDefined();
                            expect(isOn.unsubscribe).toBeDefined();
                        });

                        it("has correct members (ProxyAttribute with NOTIFYREADONLY)", function() {
                            expect(isOnNotifyReadOnly.get).toBeDefined();
                            expect(isOnNotifyReadOnly.set).toBeUndefined();
                            expect(isOnNotifyReadOnly.subscribe).toBeDefined();
                            expect(isOnNotifyReadOnly.unsubscribe).toBeDefined();
                        });

                        it("has correct members (ProxyAttribute with NOTIFYWRITEONLY)", function() {
                            expect(isOnNotifyWriteOnly.get).toBeUndefined();
                            expect(isOnNotifyWriteOnly.set).toBeDefined();
                            expect(isOnNotifyWriteOnly.subscribe).toBeDefined();
                            expect(isOnNotifyWriteOnly.unsubscribe).toBeDefined();
                        });

                        it("has correct members (ProxyAttribute with NOTIFY)", function() {
                            expect(isOnNotify.get).toBeUndefined();
                            expect(isOnNotify.set).toBeUndefined();
                            expect(isOnNotify.subscribe).toBeDefined();
                            expect(isOnNotify.unsubscribe).toBeDefined();
                        });

                        it("has correct members (ProxyAttribute with READWRITE)", function() {
                            expect(isOnReadWrite.get).toBeDefined();
                            expect(isOnReadWrite.set).toBeDefined();
                            expect(isOnReadWrite.subscribe).toBeUndefined();
                            expect(isOnReadWrite.unsubscribe).toBeUndefined();
                        });

                        it("has correct members (ProxyAttribute with READONLY)", function() {
                            expect(isOnReadOnly.get).toBeDefined();
                            expect(isOnReadOnly.set).toBeUndefined();
                            expect(isOnReadOnly.subscribe).toBeUndefined();
                            expect(isOnReadOnly.unsubscribe).toBeUndefined();
                        });

                        it("has correct members (ProxyAttribute with WRITEONLY)", function() {
                            expect(isOnWriteOnly.get).toBeUndefined();
                            expect(isOnWriteOnly.set).toBeDefined();
                            expect(isOnWriteOnly.subscribe).toBeUndefined();
                            expect(isOnWriteOnly.unsubscribe).toBeUndefined();
                        });

                        it("has correct members (ProxyAttributeNotifyReadWrite)", function() {
                            expect(isOnProxyAttributeNotifyReadWrite.get).toBeDefined();
                            expect(isOnProxyAttributeNotifyReadWrite.set).toBeDefined();
                            expect(isOnProxyAttributeNotifyReadWrite.subscribe).toBeDefined();
                            expect(isOnProxyAttributeNotifyReadWrite.unsubscribe).toBeDefined();
                        });

                        it("has correct members (ProxyAttributeNotifyRead)", function() {
                            expect(isOnProxyAttributeNotifyRead.get).toBeDefined();
                            expect(isOnProxyAttributeNotifyRead.set).toBeUndefined();
                            expect(isOnProxyAttributeNotifyRead.subscribe).toBeDefined();
                            expect(isOnProxyAttributeNotifyRead.unsubscribe).toBeDefined();
                        });

                        it("has correct members (ProxyAttributeNotifyWrite)", function() {
                            expect(isOnProxyAttributeNotifyWrite.get).toBeUndefined();
                            expect(isOnProxyAttributeNotifyWrite.set).toBeDefined();
                            expect(isOnProxyAttributeNotifyWrite.subscribe).toBeDefined();
                            expect(isOnProxyAttributeNotifyWrite.unsubscribe).toBeDefined();
                        });

                        it("has correct members (ProxyAttributeNotify)", function() {
                            expect(isOnProxyAttributeNotify.get).toBeUndefined();
                            expect(isOnProxyAttributeNotify.set).toBeUndefined();
                            expect(isOnProxyAttributeNotify.subscribe).toBeDefined();
                            expect(isOnProxyAttributeNotify.unsubscribe).toBeDefined();
                        });

                        it("has correct members (ProxyAttributeReadWrite)", function() {
                            expect(isOnProxyAttributeReadWrite.get).toBeDefined();
                            expect(isOnProxyAttributeReadWrite.set).toBeDefined();
                            expect(isOnProxyAttributeReadWrite.subscribe).toBeUndefined();
                            expect(isOnProxyAttributeReadWrite.unsubscribe).toBeUndefined();
                        });

                        it("has correct members (ProxyAttributeRead)", function() {
                            expect(isOnProxyAttributeRead.get).toBeDefined();
                            expect(isOnProxyAttributeRead.set).toBeUndefined();
                            expect(isOnProxyAttributeRead.subscribe).toBeUndefined();
                            expect(isOnProxyAttributeRead.unsubscribe).toBeUndefined();
                        });

                        it("has correct members (ProxyAttributeWrite)", function() {
                            expect(isOnProxyAttributeWrite.get).toBeUndefined();
                            expect(isOnProxyAttributeWrite.set).toBeDefined();
                            expect(isOnProxyAttributeWrite.subscribe).toBeUndefined();
                            expect(isOnProxyAttributeWrite.unsubscribe).toBeUndefined();
                        });

                        it(
                                "get calls through to RequestReplyManager",
                                function() {
                                    var requestReplyId;
                                    isOn.get();

                                    expect(requestReplyManagerSpy.sendRequest).toHaveBeenCalled();
                                    requestReplyId =
                                            requestReplyManagerSpy.sendRequest.calls[0].args[0].request.requestReplyId;
                                    expect(requestReplyManagerSpy.sendRequest)
                                            .toHaveBeenCalledWith({
                                                to : providerParticipantId,
                                                from : proxyParticipantId,
                                                messagingQos : messagingQos,
                                                request : new Request({
                                                    methodName : "getIsOn",
                                                    requestReplyId : requestReplyId
                                                })
                                            }

                                            );
                                });

                        it("get notifies", function() {
                            expect(isOn.get).toBeDefined();
                            expect(typeof isOn.get === "function").toBeTruthy();

                            var spy = jasmine.createSpyObj("spy", [
                                "onFulfilled",
                                "onRejected"
                            ]);

                            runs(function() {
                                isOn.get().then(spy.onFulfilled).catch(spy.onRejected);
                            });

                            waitsFor(function() {
                                return spy.onFulfilled.callCount > 0;
                            }, "The promise is not pending any more", asyncTimeout);

                            runs(function() {
                                checkSpy(spy);
                            });
                        });

                        it(
                                "set calls through to RequestReplyManager",
                                function() {
                                    var requestReplyId;
                                    isOn.set({
                                        value : true
                                    });

                                    expect(requestReplyManagerSpy.sendRequest).toHaveBeenCalled();
                                    requestReplyId =
                                            requestReplyManagerSpy.sendRequest.calls[0].args[0].request.requestReplyId;
                                    expect(requestReplyManagerSpy.sendRequest)
                                            .toHaveBeenCalledWith({
                                                to : providerParticipantId,
                                                from : proxyParticipantId,
                                                messagingQos : messagingQos,
                                                request : new Request({
                                                    methodName : "setIsOn",
                                                    requestReplyId : requestReplyId,
                                                    paramDatatypes : [ "Boolean"
                                                    ],
                                                    params : [ true
                                                    ]
                                                })
                                            }

                                            );
                                });

                        it("set notifies", function() {
                            expect(isOn.set).toBeDefined();
                            expect(typeof isOn.set === "function").toBeTruthy();

                            var spy = jasmine.createSpyObj("spy", [
                                "onFulfilled",
                                "onRejected"
                            ]);

                            runs(function() {
                                isOn.set().then(spy.onFulfilled).catch(spy.onRejected);
                            });

                            waitsFor(function() {
                                return spy.onFulfilled.callCount > 0;
                            }, "The promise is not pending any more", asyncTimeout);

                            runs(function() {
                                checkSpy(spy);
                            });
                        });

                        it("subscribe calls through to SubscriptionManager", function() {
                            var spy = jasmine.createSpyObj("spy", [
                                "publication",
                                "publicationMissed"
                            ]);

                            isOn.subscribe({
                                messagingQos : messagingQos,
                                subscriptionQos : subscriptionQos,
                                onReceive : spy.publication,
                                onError : spy.publicationMissed
                            });

                            expect(subscriptionManagerSpy.registerSubscription).toHaveBeenCalled();
                            expect(subscriptionManagerSpy.registerSubscription)
                                    .toHaveBeenCalledWith({
                                        proxyId : proxyParticipantId,
                                        providerId : providerParticipantId,
                                        messagingQos : messagingQos,
                                        attributeName : "isOn",
                                        attributeType : TypesEnum.BOOL,
                                        qos : subscriptionQos,
                                        onReceive : spy.publication,
                                        onError : spy.publicationMissed
                                    });
                        });

                        it("subscribe notifies", function() {
                            expect(isOn.subscribe).toBeDefined();
                            expect(typeof isOn.subscribe === "function").toBeTruthy();

                            var spy = jasmine.createSpyObj("spy", [
                                "onFulfilled",
                                "onRejected"
                            ]);

                            runs(function() {
                                isOn.subscribe({
                                    subscriptionQos : subscriptionQos,
                                    onReceive : function(value) {},
                                    onError : function(value) {}
                                }).then(spy.onFulfilled).catch(spy.onRejected);
                            });

                            waitsFor(function() {
                                return spy.onFulfilled.callCount > 0;
                            }, "The promise is not pending any more", asyncTimeout);

                            runs(function() {
                                checkSpy(spy);
                            });
                        });

                        it("subscribe provides a subscriptionId", function() {
                            var spy = jasmine.createSpyObj("spy", [
                                "onFulfilled",
                                "onRejected"
                            ]);

                            runs(function() {
                                isOn.subscribe({
                                    subscriptionQos : subscriptionQos,
                                    onReceive : function(value) {},
                                    onError : function(value) {}
                                }).then(spy.onFulfilled).catch(spy.onRejected);
                            });

                            waitsFor(function() {
                                return spy.onFulfilled.callCount > 0;
                            }, "The promise is not pending any more", asyncTimeout);

                            runs(function() {
                                checkSpy(spy, false, subscriptionId);
                            });
                        });

                        // TODO: implement mock for publication of the value
                        // it("subscribe publishes a value", function () {
                        // var publishedValue;
                        //
                        // runs(function () {
                        // isOn.subscribe({subscriptionQos: subscriptionQos, publication: function
                        // (value) { publishedValue = value; } });
                        // });
                        //
                        // waitsFor(function () {
                        // return publishedValue !== undefined;
                        // }, "The publication callback is fired and provides a value !==
                        // undefined", asyncTimeout);
                        // });

                        it("unsubscribe calls through to SubscriptionManager", function() {
                            var spy = jasmine.createSpyObj("spy", [
                                "onFulfilled",
                                "onRejected"
                            ]);

                            runs(function() {
                                isOn.unsubscribe({
                                    subscriptionId : subscriptionId
                                }).then(spy.onFulfilled).catch(spy.onRejected);
                            });

                            waitsFor(function() {
                                return spy.onFulfilled.callCount > 0;
                            }, "The promise is not pending any more", asyncTimeout);

                            runs(function() {
                                expect(subscriptionManagerSpy.unregisterSubscription)
                                        .toHaveBeenCalled();
                                expect(subscriptionManagerSpy.unregisterSubscription)
                                        .toHaveBeenCalledWith({
                                            messagingQos : new MessagingQos(),
                                            subscriptionId : subscriptionId
                                        });
                            });
                        });

                        it("unsubscribe notifies", function() {
                            expect(isOn.unsubscribe).toBeDefined();
                            expect(typeof isOn.unsubscribe === "function").toBeTruthy();

                            var spy1 = jasmine.createSpyObj("spy1", [
                                "fulfill",
                                "onRejected"
                            ]);
                            var spy2 = jasmine.createSpyObj("spy2", [
                                "fulfill",
                                "onRejected"
                            ]);

                            runs(function() {
                                isOn.subscribe({
                                    subscriptionQos : subscriptionQos,
                                    onReceive : function(value) {},
                                    onError : function(value) {}
                                }).then(function(subscriptionId) {
                                    spy1.fulfill(subscriptionId);
                                    isOn.unsubscribe({
                                        subscriptionId : subscriptionId
                                    }).then(spy2.fulfill).catch(spy2.onRejected);
                                }).catch(spy1.onRejected);
                            });

                            waitsFor(function() {
                                return spy1.fulfill.callCount > 0;
                            }, "The promises 1 is not pending any more", asyncTimeout);

                            waitsFor(function() {
                                return spy2.fulfill.callCount > 0;
                            }, "The promises 2 is not pending any more", asyncTimeout);

                            runs(function() {
                                checkSpy(spy1);
                                checkSpy(spy2);
                            });
                        });

                        it(
                                "throws if caller sets a generic object without a declared _typeName attribute with the name of a registrered type",
                                function() {
                                    var proxy = {};
                                    var radioStationProxyAttributeWrite =
                                            new ProxyAttributeWrite(
                                                    proxy,
                                                    settings,
                                                    "radioStationProxyAttributeWrite",
                                                    RadioStation);

                                    expect(function() {
                                        radioStationProxyAttributeWrite.set({
                                            value : {
                                                name : "radiostationname",
                                                station : "station"
                                            }
                                        });
                                    }).toThrow();
                                });
                    });

        }); // require
/*jslint nomen: false */
