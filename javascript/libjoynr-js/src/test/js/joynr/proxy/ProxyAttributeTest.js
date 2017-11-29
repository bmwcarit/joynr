/*jslint es5: true, node: true, nomen: true */
/*global fail: true */
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
var ProxyAttribute = require("../../../classes/joynr/proxy/ProxyAttribute");
var DiscoveryQos = require("../../../classes/joynr/proxy/DiscoveryQos");
var MessagingQos = require("../../../classes/joynr/messaging/MessagingQos");
var OnChangeWithKeepAliveSubscriptionQos = require("../../../classes/joynr/proxy/OnChangeWithKeepAliveSubscriptionQos");
var RequestReplyManager = require("../../../classes/joynr/dispatching/RequestReplyManager");
var Request = require("../../../classes/joynr/dispatching/types/Request");
var TestEnum = require("../../../test-classes/joynr/tests/testTypes/TestEnum");
var ComplexTestType = require("../../../test-classes/joynr/tests/testTypes/ComplexTestType");
var TypeRegistrySingleton = require("../../../classes/joynr/types/TypeRegistrySingleton");
var Promise = require("../../../classes/global/Promise");
var DiscoveryEntryWithMetaInfo = require("../../../classes/joynr/types/DiscoveryEntryWithMetaInfo");
var Version = require("../../../classes/joynr/types/Version");
var ProviderQos = require("../../../classes/joynr/types/ProviderQos");

var asyncTimeout = 5000;

describe("libjoynr-js.joynr.proxy.ProxyAttribute", function() {
    var settings;
    var isOn;
    var isOnNotifyReadOnly;
    var isOnNotifyWriteOnly;
    var isOnNotify;
    var isOnReadWrite;
    var isOnReadOnly;
    var isOnWriteOnly;
    var subscriptionQos;
    var messagingQos;
    var requestReplyManagerSpy;
    var subscriptionId;
    var subscriptionManagerSpy;
    var proxyParticipantId;
    var providerParticipantId;
    var providerDiscoveryEntry;

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
            configurable: false,
            writable: false,
            enumerable: true,
            value: "test.RadioStation"
        });
    }

    beforeEach(function(done) {
        subscriptionQos = new OnChangeWithKeepAliveSubscriptionQos();
        messagingQos = new MessagingQos();

        requestReplyManagerSpy = jasmine.createSpyObj("requestReplyManager", ["sendRequest"]);
        requestReplyManagerSpy.sendRequest.and.returnValue(
            Promise.resolve({
                result: {
                    resultKey: "resultValue"
                }
            })
        );

        subscriptionId = {
            tokenId: "someId",
            tokenUserData: "some additional data, do not touch!"
        };
        subscriptionManagerSpy = jasmine.createSpyObj("subscriptionManager", [
            "registerSubscription",
            "unregisterSubscription"
        ]);
        subscriptionManagerSpy.registerSubscription.and.returnValue(Promise.resolve(subscriptionId));
        subscriptionManagerSpy.unregisterSubscription.and.returnValue(Promise.resolve(subscriptionId));

        settings = {
            dependencies: {
                requestReplyManager: requestReplyManagerSpy,
                subscriptionManager: subscriptionManagerSpy
            }
        };

        proxyParticipantId = "proxyParticipantId";
        providerParticipantId = "providerParticipantId";
        providerDiscoveryEntry = new DiscoveryEntryWithMetaInfo({
            providerVersion: new Version({ majorVersion: 0, minorVersion: 23 }),
            domain: "testProviderDomain",
            interfaceName: "interfaceName",
            participantId: providerParticipantId,
            qos: new ProviderQos(),
            lastSeenDateMs: Date.now(),
            expiryDateMs: Date.now() + 60000,
            publicKeyId: "publicKeyId",
            isLocal: true
        });
        var proxy = {
            proxyParticipantId: proxyParticipantId,
            providerDiscoveryEntry: providerDiscoveryEntry
        };

        isOn = new ProxyAttribute(proxy, settings, "isOn", "Boolean", "NOTIFYREADWRITE");
        isOnNotifyReadOnly = new ProxyAttribute(proxy, settings, "isOnNotifyReadOnly", "Boolean", "NOTIFYREADONLY");
        isOnNotifyWriteOnly = new ProxyAttribute(proxy, settings, "isOnNotifyWriteOnly", "Boolean", "NOTIFYWRITEONLY");
        isOnNotify = new ProxyAttribute(proxy, settings, "isOnNotify", "Boolean", "NOTIFY");
        isOnReadWrite = new ProxyAttribute(proxy, settings, "isOnReadWrite", "Boolean", "READWRITE");
        isOnReadOnly = new ProxyAttribute(proxy, settings, "isOnReadOnly", "Boolean", "READONLY");
        isOnWriteOnly = new ProxyAttribute(proxy, settings, "isOnWriteOnly", "Boolean", "WRITEONLY");

        TypeRegistrySingleton.getInstance()
            .getTypeRegisteredPromise("joynr.tests.testTypes.TestEnum", 1000)
            .then(function() {
                done();
                return null;
            })
            .catch(fail);
    });

    it("got initialized", function(done) {
        expect(isOn).toBeDefined();
        expect(isOn).not.toBeNull();
        expect(typeof isOn === "object").toBeTruthy();
        done();
    });

    it("has correct members (ProxyAttribute with NOTIFYREADWRITE)", function(done) {
        expect(isOn.get).toBeDefined();
        expect(isOn.set).toBeDefined();
        expect(isOn.subscribe).toBeDefined();
        expect(isOn.unsubscribe).toBeDefined();
        done();
    });

    it("has correct members (ProxyAttribute with NOTIFYREADONLY)", function(done) {
        expect(isOnNotifyReadOnly.get).toBeDefined();
        expect(isOnNotifyReadOnly.set).toBeUndefined();
        expect(isOnNotifyReadOnly.subscribe).toBeDefined();
        expect(isOnNotifyReadOnly.unsubscribe).toBeDefined();
        done();
    });

    it("has correct members (ProxyAttribute with NOTIFYWRITEONLY)", function(done) {
        expect(isOnNotifyWriteOnly.get).toBeUndefined();
        expect(isOnNotifyWriteOnly.set).toBeDefined();
        expect(isOnNotifyWriteOnly.subscribe).toBeDefined();
        expect(isOnNotifyWriteOnly.unsubscribe).toBeDefined();
        done();
    });

    it("has correct members (ProxyAttribute with NOTIFY)", function(done) {
        expect(isOnNotify.get).toBeUndefined();
        expect(isOnNotify.set).toBeUndefined();
        expect(isOnNotify.subscribe).toBeDefined();
        expect(isOnNotify.unsubscribe).toBeDefined();
        done();
    });

    it("has correct members (ProxyAttribute with READWRITE)", function(done) {
        expect(isOnReadWrite.get).toBeDefined();
        expect(isOnReadWrite.set).toBeDefined();
        expect(isOnReadWrite.subscribe).toBeUndefined();
        expect(isOnReadWrite.unsubscribe).toBeUndefined();
        done();
    });

    it("has correct members (ProxyAttribute with READONLY)", function(done) {
        expect(isOnReadOnly.get).toBeDefined();
        expect(isOnReadOnly.set).toBeUndefined();
        expect(isOnReadOnly.subscribe).toBeUndefined();
        expect(isOnReadOnly.unsubscribe).toBeUndefined();
        done();
    });

    it("has correct members (ProxyAttribute with WRITEONLY)", function(done) {
        expect(isOnWriteOnly.get).toBeUndefined();
        expect(isOnWriteOnly.set).toBeDefined();
        expect(isOnWriteOnly.subscribe).toBeUndefined();
        expect(isOnWriteOnly.unsubscribe).toBeUndefined();
        done();
    });

    it("get calls through to RequestReplyManager", function(done) {
        var requestReplyId;
        isOn
            .get()
            .then(function() {
                expect(requestReplyManagerSpy.sendRequest).toHaveBeenCalled();
                requestReplyId = requestReplyManagerSpy.sendRequest.calls.argsFor(0)[0].request.requestReplyId;
                expect(requestReplyManagerSpy.sendRequest).toHaveBeenCalledWith({
                    toDiscoveryEntry: providerDiscoveryEntry,
                    from: proxyParticipantId,
                    messagingQos: messagingQos,
                    request: new Request({
                        methodName: "getIsOn",
                        requestReplyId: requestReplyId
                    })
                });
                done();
                return null;
            })
            .catch(function() {
                fail("get call unexpectedly failed.");
                return null;
            });
    });

    it("get notifies", function(done) {
        expect(isOn.get).toBeDefined();
        expect(typeof isOn.get === "function").toBeTruthy();

        isOn
            .get()
            .then(function() {
                done();
                return null;
            })
            .catch(function() {
                fail("get notifies rejected unexpectedly");
                return null;
            });
    });

    it("get returns correct joynr objects", function(done) {
        var fixture = new ProxyAttribute(
            {
                proxyParticipantId: "proxy",
                providerDiscoveryEntry: providerDiscoveryEntry
            },
            settings,
            "attributeOfTypeTestEnum",
            TestEnum.ZERO._typeName,
            "NOTIFYREADWRITE"
        );

        expect(fixture.get).toBeDefined();
        expect(typeof fixture.get === "function").toBeTruthy();

        requestReplyManagerSpy.sendRequest.and.returnValue(Promise.resolve(["ZERO"]));
        fixture
            .get()
            .then(function(data) {
                expect(data).toEqual(TestEnum.ZERO);
                done();
                return null;
            })
            .catch(function() {
                return null;
            });
    });

    it("expect correct error reporting after attribute set with invalid value", function(done) {
        TypeRegistrySingleton.getInstance()
            .getTypeRegisteredPromise("joynr.tests.testTypes.ComplexTestType", 1000)
            .then(function() {
                return null;
            })
            .catch(fail)
            .then(function() {
                var enumAttribute = new ProxyAttribute(
                    {
                        proxyParticipantId: "proxy",
                        providerParticipantId: "provider"
                    },
                    settings,
                    "enumAttribute",
                    "joynr.tests.testTypes.ComplexTestType",
                    "NOTIFYREADWRITE"
                );
                return enumAttribute.set({
                    value: {
                        _typeName: "joynr.tests.testTypes.ComplexTestType",
                        a: "notANumber"
                    }
                });
            })
            .then(function() {
                fail("unexpected resolve from setter with invalid value");
                return null;
            })
            .catch(function(error) {
                expect(error.message).toEqual(
                    "error setting attribute: enumAttribute: Error: members.a is not of type Number. Actual type is String"
                );
                done();
                return null;
            });
    });

    it("set calls through to RequestReplyManager", function(done) {
        var requestReplyId;
        isOn
            .set({
                value: true
            })
            .then(function() {
                expect(requestReplyManagerSpy.sendRequest).toHaveBeenCalled();
                requestReplyId = requestReplyManagerSpy.sendRequest.calls.argsFor(0)[0].request.requestReplyId;
                expect(requestReplyManagerSpy.sendRequest).toHaveBeenCalledWith({
                    toDiscoveryEntry: providerDiscoveryEntry,
                    from: proxyParticipantId,
                    messagingQos: messagingQos,
                    request: new Request({
                        methodName: "setIsOn",
                        requestReplyId: requestReplyId,
                        paramDatatypes: ["Boolean"],
                        params: [true]
                    })
                });
                done();
                return null;
            })
            .catch(function() {
                fail("got unexpected reject from setter");
                return null;
            });
    });

    it("subscribe calls through to SubscriptionManager", function(done) {
        var spy = jasmine.createSpyObj("spy", ["onReceive", "onError", "onSubscribed"]);

        isOn.subscribe({
            messagingQos: messagingQos,
            subscriptionQos: subscriptionQos,
            onReceive: spy.onReceive,
            onError: spy.onError,
            onSubscribed: spy.onSubscribed
        });

        expect(subscriptionManagerSpy.registerSubscription).toHaveBeenCalled();
        expect(subscriptionManagerSpy.registerSubscription).toHaveBeenCalledWith({
            proxyId: proxyParticipantId,
            providerDiscoveryEntry: providerDiscoveryEntry,
            attributeName: "isOn",
            attributeType: "Boolean",
            qos: subscriptionQos,
            subscriptionId: undefined,
            onReceive: spy.onReceive,
            onError: spy.onError,
            onSubscribed: spy.onSubscribed
        });
        done();
    });

    it("subscribe notifies", function(done) {
        expect(isOn.subscribe).toBeDefined();
        expect(typeof isOn.subscribe === "function").toBeTruthy();

        isOn
            .subscribe({
                subscriptionQos: subscriptionQos,
                onReceive: function(value) {},
                onError: function(value) {}
            })
            .then(function() {
                done();
                return null;
            })
            .catch(function() {
                fail("got reject from subscribe operation");
                return null;
            });
    });

    it("subscribe provides a subscriptionId", function(done) {
        var spy = jasmine.createSpyObj("spy", ["onFulfilled", "onRejected"]);

        isOn
            .subscribe({
                subscriptionQos: subscriptionQos,
                onReceive: function(value) {},
                onError: function(value) {}
            })
            .then(function(id) {
                expect(id).toEqual(subscriptionId);
                done();
                return null;
            })
            .catch(function() {
                return null;
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

    it("unsubscribe calls through to SubscriptionManager", function(done) {
        isOn
            .unsubscribe({
                subscriptionId: subscriptionId
            })
            .then(function() {
                expect(subscriptionManagerSpy.unregisterSubscription).toHaveBeenCalled();
                expect(subscriptionManagerSpy.unregisterSubscription).toHaveBeenCalledWith({
                    messagingQos: new MessagingQos(),
                    subscriptionId: subscriptionId
                });
                done();
                return null;
            })
            .catch(function() {
                return null;
            });
    });

    it("unsubscribe notifies", function(done) {
        expect(isOn.unsubscribe).toBeDefined();
        expect(typeof isOn.unsubscribe === "function").toBeTruthy();

        isOn
            .subscribe({
                subscriptionQos: subscriptionQos,
                onReceive: function(value) {},
                onError: function(value) {}
            })
            .then(function(subscriptionId) {
                return isOn.unsubscribe({
                    subscriptionId: subscriptionId
                });
            })
            .then(function() {
                done();
                return null;
            })
            .catch(function() {
                fail("subscribe or unsubscribe unexpectedly failed");
                return null;
            });
    });

    it("throws if caller sets a generic object without a declared _typeName attribute with the name of a registrered type", function(
        done
    ) {
        var proxy = {};
        var radioStationProxyAttributeWrite = new ProxyAttribute(
            proxy,
            settings,
            "radioStationProxyAttributeWrite",
            RadioStation,
            "WRITE"
        );

        expect(function() {
            radioStationProxyAttributeWrite.set({
                value: {
                    name: "radiostationname",
                    station: "station"
                }
            });
        }).toThrow();
        done();
    });
}); /*jslint nomen: false */
