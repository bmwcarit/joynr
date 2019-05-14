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
require("../../node-unit-test-helper");
const ProxyAttribute = require("../../../../main/js/joynr/proxy/ProxyAttribute");
const MessagingQos = require("../../../../main/js/joynr/messaging/MessagingQos");
const OnChangeWithKeepAliveSubscriptionQos = require("../../../../main/js/joynr/proxy/OnChangeWithKeepAliveSubscriptionQos");
const Request = require("../../../../main/js/joynr/dispatching/types/Request");
const TestEnum = require("../../../generated/joynr/tests/testTypes/TestEnum");
const TypeRegistrySingleton = require("../../../../main/js/joynr/types/TypeRegistrySingleton");
const DiscoveryEntryWithMetaInfo = require("../../../../main/js/generated/joynr/types/DiscoveryEntryWithMetaInfo");
const Version = require("../../../../main/js/generated/joynr/types/Version");
const ProviderQos = require("../../../../main/js/generated/joynr/types/ProviderQos");

describe("libjoynr-js.joynr.proxy.ProxyAttribute", () => {
    let settings;
    let isOn;
    let isOnNotifyReadOnly;
    let isOnNotifyWriteOnly;
    let isOnNotify;
    let isOnReadWrite;
    let isOnReadOnly;
    let isOnWriteOnly;
    let subscriptionQos;
    let messagingQos;
    let requestReplyManagerSpy;
    let subscriptionId;
    let subscriptionManagerSpy;
    let proxyParticipantId;
    let providerParticipantId;
    let providerDiscoveryEntry;
    let isOnType;

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

    beforeEach(done => {
        subscriptionQos = new OnChangeWithKeepAliveSubscriptionQos();
        messagingQos = new MessagingQos();

        requestReplyManagerSpy = jasmine.createSpyObj("requestReplyManager", ["sendRequest"]);
        requestReplyManagerSpy.sendRequest.and.callFake((settings, callbackSettings) => {
            const response = { result: { resultKey: "resultValue" } };

            return Promise.resolve({
                response,
                settings: callbackSettings
            });
        });

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
        const proxy = {
            proxyParticipantId,
            providerDiscoveryEntry
        };

        isOnType = "Boolean";
        isOn = new ProxyAttribute(proxy, settings, "isOn", isOnType, "NOTIFYREADWRITE");
        isOnNotifyReadOnly = new ProxyAttribute(proxy, settings, "isOnNotifyReadOnly", "Boolean", "NOTIFYREADONLY");
        isOnNotifyWriteOnly = new ProxyAttribute(proxy, settings, "isOnNotifyWriteOnly", "Boolean", "NOTIFYWRITEONLY");
        isOnNotify = new ProxyAttribute(proxy, settings, "isOnNotify", "Boolean", "NOTIFY");
        isOnReadWrite = new ProxyAttribute(proxy, settings, "isOnReadWrite", "Boolean", "READWRITE");
        isOnReadOnly = new ProxyAttribute(proxy, settings, "isOnReadOnly", "Boolean", "READONLY");
        isOnWriteOnly = new ProxyAttribute(proxy, settings, "isOnWriteOnly", "Boolean", "WRITEONLY");

        TypeRegistrySingleton.getInstance()
            .getTypeRegisteredPromise("joynr.tests.testTypes.TestEnum", 1000)
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("got initialized", done => {
        expect(isOn).toBeDefined();
        expect(isOn).not.toBeNull();
        expect(typeof isOn === "object").toBeTruthy();
        done();
    });

    it("has correct members (ProxyAttribute with NOTIFYREADWRITE)", done => {
        expect(isOn.get).toBeDefined();
        expect(isOn.set).toBeDefined();
        expect(isOn.subscribe).toBeDefined();
        expect(isOn.unsubscribe).toBeDefined();
        done();
    });

    it("has correct members (ProxyAttribute with NOTIFYREADONLY)", done => {
        expect(isOnNotifyReadOnly.get).toBeDefined();
        expect(isOnNotifyReadOnly.set).toBeUndefined();
        expect(isOnNotifyReadOnly.subscribe).toBeDefined();
        expect(isOnNotifyReadOnly.unsubscribe).toBeDefined();
        done();
    });

    it("has correct members (ProxyAttribute with NOTIFYWRITEONLY)", done => {
        expect(isOnNotifyWriteOnly.get).toBeUndefined();
        expect(isOnNotifyWriteOnly.set).toBeDefined();
        expect(isOnNotifyWriteOnly.subscribe).toBeDefined();
        expect(isOnNotifyWriteOnly.unsubscribe).toBeDefined();
        done();
    });

    it("has correct members (ProxyAttribute with NOTIFY)", done => {
        expect(isOnNotify.get).toBeUndefined();
        expect(isOnNotify.set).toBeUndefined();
        expect(isOnNotify.subscribe).toBeDefined();
        expect(isOnNotify.unsubscribe).toBeDefined();
        done();
    });

    it("has correct members (ProxyAttribute with READWRITE)", done => {
        expect(isOnReadWrite.get).toBeDefined();
        expect(isOnReadWrite.set).toBeDefined();
        expect(isOnReadWrite.subscribe).toBeUndefined();
        expect(isOnReadWrite.unsubscribe).toBeUndefined();
        done();
    });

    it("has correct members (ProxyAttribute with READONLY)", done => {
        expect(isOnReadOnly.get).toBeDefined();
        expect(isOnReadOnly.set).toBeUndefined();
        expect(isOnReadOnly.subscribe).toBeUndefined();
        expect(isOnReadOnly.unsubscribe).toBeUndefined();
        done();
    });

    it("has correct members (ProxyAttribute with WRITEONLY)", done => {
        expect(isOnWriteOnly.get).toBeUndefined();
        expect(isOnWriteOnly.set).toBeDefined();
        expect(isOnWriteOnly.subscribe).toBeUndefined();
        expect(isOnWriteOnly.unsubscribe).toBeUndefined();
        done();
    });

    it("get calls through to RequestReplyManager", done => {
        let requestReplyId;
        isOn.get()
            .then(() => {
                expect(requestReplyManagerSpy.sendRequest).toHaveBeenCalled();
                requestReplyId = requestReplyManagerSpy.sendRequest.calls.argsFor(0)[0].request.requestReplyId;
                expect(requestReplyManagerSpy.sendRequest).toHaveBeenCalledWith(
                    {
                        toDiscoveryEntry: providerDiscoveryEntry,
                        from: proxyParticipantId,
                        messagingQos,
                        request: Request.create({
                            methodName: "getIsOn",
                            requestReplyId
                        })
                    },
                    isOnType
                );
                done();
                return null;
            })
            .catch(() => {
                fail("get call unexpectedly failed.");
                return null;
            });
    });

    it("get notifies", done => {
        expect(isOn.get).toBeDefined();
        expect(typeof isOn.get === "function").toBeTruthy();

        isOn.get()
            .then(() => {
                done();
                return null;
            })
            .catch(() => {
                fail("get notifies rejected unexpectedly");
                return null;
            });
    });

    it("get returns correct joynr objects", done => {
        const fixture = new ProxyAttribute(
            {
                proxyParticipantId: "proxy",
                providerDiscoveryEntry
            },
            settings,
            "attributeOfTypeTestEnum",
            TestEnum.ZERO._typeName,
            "NOTIFYREADWRITE"
        );

        expect(fixture.get).toBeDefined();
        expect(typeof fixture.get === "function").toBeTruthy();

        requestReplyManagerSpy.sendRequest.and.callFake((settings, callbackSettings) => {
            return Promise.resolve({
                response: ["ZERO"],
                settings: callbackSettings
            });
        });

        fixture
            .get()
            .then(data => {
                expect(data).toEqual(TestEnum.ZERO);
                done();
                return null;
            })
            .catch(() => {
                return null;
            });
    });

    it("expect correct error reporting after attribute set with invalid value", done => {
        TypeRegistrySingleton.getInstance()
            .getTypeRegisteredPromise("joynr.tests.testTypes.ComplexTestType", 1000)
            .then(() => {
                return null;
            })
            .catch(fail)
            .then(() => {
                const enumAttribute = new ProxyAttribute(
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
            .then(() => {
                fail("unexpected resolve from setter with invalid value");
                return null;
            })
            .catch(error => {
                expect(error.message).toEqual(
                    "error setting attribute: enumAttribute: Error: members.a is not of type Number. Actual type is String"
                );
                done();
                return null;
            });
    });

    it("set calls through to RequestReplyManager", done => {
        let requestReplyId;
        isOn.set({
            value: true
        })
            .then(() => {
                expect(requestReplyManagerSpy.sendRequest).toHaveBeenCalled();
                requestReplyId = requestReplyManagerSpy.sendRequest.calls.argsFor(0)[0].request.requestReplyId;
                expect(requestReplyManagerSpy.sendRequest).toHaveBeenCalledWith(
                    {
                        toDiscoveryEntry: providerDiscoveryEntry,
                        from: proxyParticipantId,
                        messagingQos,
                        request: Request.create({
                            methodName: "setIsOn",
                            requestReplyId,
                            paramDatatypes: ["Boolean"],
                            params: [true]
                        })
                    },
                    isOnType
                );
                done();
                return null;
            })
            .catch(() => {
                fail("got unexpected reject from setter");
                return null;
            });
    });

    it("subscribe calls through to SubscriptionManager", done => {
        const spy = jasmine.createSpyObj("spy", ["onReceive", "onError", "onSubscribed"]);

        isOn.subscribe({
            messagingQos,
            subscriptionQos,
            onReceive: spy.onReceive,
            onError: spy.onError,
            onSubscribed: spy.onSubscribed
        });

        expect(subscriptionManagerSpy.registerSubscription).toHaveBeenCalled();
        expect(subscriptionManagerSpy.registerSubscription).toHaveBeenCalledWith({
            proxyId: proxyParticipantId,
            providerDiscoveryEntry,
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

    it("subscribe notifies", done => {
        expect(isOn.subscribe).toBeDefined();
        expect(typeof isOn.subscribe === "function").toBeTruthy();

        isOn.subscribe({
            subscriptionQos,
            onReceive() {},
            onError() {}
        })
            .then(() => {
                done();
                return null;
            })
            .catch(() => {
                fail("got reject from subscribe operation");
                return null;
            });
    });

    it("subscribe provides a subscriptionId", done => {
        isOn.subscribe({
            subscriptionQos,
            onReceive() {},
            onError() {}
        })
            .then(id => {
                expect(id).toEqual(subscriptionId);
                done();
                return null;
            })
            .catch(() => {
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

    it("unsubscribe calls through to SubscriptionManager", done => {
        isOn.unsubscribe({
            subscriptionId
        })
            .then(() => {
                expect(subscriptionManagerSpy.unregisterSubscription).toHaveBeenCalled();
                expect(subscriptionManagerSpy.unregisterSubscription).toHaveBeenCalledWith({
                    messagingQos: new MessagingQos(),
                    subscriptionId
                });
                done();
                return null;
            })
            .catch(() => {
                return null;
            });
    });

    it("unsubscribe notifies", done => {
        expect(isOn.unsubscribe).toBeDefined();
        expect(typeof isOn.unsubscribe === "function").toBeTruthy();

        isOn.subscribe({
            subscriptionQos,
            onReceive() {},
            onError() {}
        })
            .then(subscriptionId => {
                return isOn.unsubscribe({
                    subscriptionId
                });
            })
            .then(() => {
                done();
                return null;
            })
            .catch(() => {
                fail("subscribe or unsubscribe unexpectedly failed");
                return null;
            });
    });

    it("rejects if caller sets a generic object without a declared _typeName attribute with the name of a registrered type", done => {
        const proxy = {};
        const radioStationProxyAttributeWrite = new ProxyAttribute(
            proxy,
            settings,
            "radioStationProxyAttributeWrite",
            RadioStation,
            "WRITE"
        );

        radioStationProxyAttributeWrite
            .set({
                value: {
                    name: "radiostationname",
                    station: "station"
                }
            })
            .then(fail)
            .catch(() => done());
    });
}); /*jslint nomen: false */
