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

import {
    ProxyReadAttribute,
    ProxyReadNotifyAttribute,
    ProxyReadWriteAttribute,
    ProxyReadWriteNotifyAttribute
} from "../../../../main/js/joynr/proxy/ProxyAttribute";
import MessagingQos from "../../../../main/js/joynr/messaging/MessagingQos";
import OnChangeWithKeepAliveSubscriptionQos from "../../../../main/js/joynr/proxy/OnChangeWithKeepAliveSubscriptionQos";
import * as Request from "../../../../main/js/joynr/dispatching/types/Request";
import TestEnum from "../../../generated/joynr/tests/testTypes/TestEnum";
import TypeRegistrySingleton from "../../../../main/js/joynr/types/TypeRegistrySingleton";
import DiscoveryEntryWithMetaInfo from "../../../../main/js/generated/joynr/types/DiscoveryEntryWithMetaInfo";
import Version from "../../../../main/js/generated/joynr/types/Version";
import ProviderQos from "../../../../main/js/generated/joynr/types/ProviderQos";
import ComplexTestType = require("../../../generated/joynr/tests/testTypes/ComplexTestType");

describe("libjoynr-js.joynr.proxy.ProxyAttribute", () => {
    let isOn: ProxyReadWriteNotifyAttribute<boolean>;
    let isOnNotifyReadOnly: ProxyReadNotifyAttribute<boolean>;
    let isOnReadWrite: ProxyReadWriteAttribute<boolean>;
    let isOnReadOnly: ProxyReadAttribute<boolean>;
    let subscriptionQos: any;
    let messagingQos: any;
    let requestReplyManagerSpy: any;
    let subscriptionId: any;
    let subscriptionManagerSpy: any;
    let proxyParticipantId: any;
    let providerParticipantId: any;
    let providerDiscoveryEntry: any;
    let isOnType: string;
    let settings: any;
    let proxy: any;

    function RadioStation(this: any, name: any, station: any, source: any) {
        this.name = name;
        this.station = station;
        this.source = source;
        this.checkMembers = jest.fn();

        Object.defineProperty(this, "_typeName", {
            configurable: false,
            writable: false,
            enumerable: true,
            value: "test.RadioStation"
        });
    }

    beforeEach(() => {
        subscriptionQos = new OnChangeWithKeepAliveSubscriptionQos();
        messagingQos = new MessagingQos();

        requestReplyManagerSpy = {
            sendRequest: jest.fn()
        };
        requestReplyManagerSpy.sendRequest.mockImplementation((_settings: any, callbackSettings: any) => {
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
        subscriptionManagerSpy = {
            registerSubscription: jest.fn(),
            unregisterSubscription: jest.fn()
        };
        subscriptionManagerSpy.registerSubscription.mockReturnValue(Promise.resolve(subscriptionId));
        subscriptionManagerSpy.unregisterSubscription.mockReturnValue(Promise.resolve(subscriptionId));

        proxyParticipantId = "proxyParticipantId";
        providerParticipantId = "providerParticipantId";
        providerDiscoveryEntry = new DiscoveryEntryWithMetaInfo({
            providerVersion: new Version({ majorVersion: 0, minorVersion: 23 }),
            domain: "testProviderDomain",
            interfaceName: "interfaceName",
            participantId: providerParticipantId,
            qos: new (ProviderQos as any)(),
            lastSeenDateMs: Date.now(),
            expiryDateMs: Date.now() + 60000,
            publicKeyId: "publicKeyId",
            isLocal: true
        });

        settings = {
            dependencies: {
                requestReplyManager: requestReplyManagerSpy,
                subscriptionManager: subscriptionManagerSpy
            }
        };
        proxy = {
            proxyParticipantId,
            providerDiscoveryEntry,
            settings
        };

        isOnType = "Boolean";
        isOn = new ProxyReadWriteNotifyAttribute(proxy, "isOn", isOnType);
        isOnNotifyReadOnly = new ProxyReadNotifyAttribute(proxy, "isOnNotifyReadOnly", "Boolean");
        isOnReadOnly = new ProxyReadAttribute(proxy, "isOnReadOnly", "Boolean");
        isOnReadWrite = new ProxyReadWriteAttribute(proxy, "isOnReadWrite", "Boolean");

        TypeRegistrySingleton.getInstance().addType(TestEnum);
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
        expect((isOnNotifyReadOnly as any).set).toBeUndefined();
        expect(isOnNotifyReadOnly.subscribe).toBeDefined();
        expect(isOnNotifyReadOnly.unsubscribe).toBeDefined();
        done();
    });

    it("has correct members (ProxyAttribute with READWRITE)", done => {
        expect(isOnReadWrite.get).toBeDefined();
        expect(isOnReadWrite.set).toBeDefined();
        expect((isOnReadWrite as any).subscribe).toBeUndefined();
        expect((isOnReadWrite as any).unsubscribe).toBeUndefined();
        done();
    });

    it("has correct members (ProxyAttribute with READONLY)", done => {
        expect(isOnReadOnly.get).toBeDefined();
        expect((isOnReadOnly as any).set).toBeUndefined();
        expect((isOnReadOnly as any).subscribe).toBeUndefined();
        expect((isOnReadOnly as any).unsubscribe).toBeUndefined();
        done();
    });

    it("get calls through to RequestReplyManager", async () => {
        await isOn.get();
        expect(requestReplyManagerSpy.sendRequest).toHaveBeenCalled();
        const requestReplyId = requestReplyManagerSpy.sendRequest.mock.calls[0][0].request.requestReplyId;
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
    });

    it("get notifies", async () => {
        expect(isOn.get).toBeDefined();
        expect(typeof isOn.get === "function").toBeTruthy();

        await isOn.get();
    });

    it("get returns correct joynr objects", done => {
        const fixture = new ProxyReadWriteNotifyAttribute(
            {
                proxyParticipantId: "proxy",
                providerDiscoveryEntry,
                settings
            },
            "attributeOfTypeTestEnum",
            TestEnum.ZERO._typeName
        );

        expect(fixture.get).toBeDefined();
        expect(typeof fixture.get === "function").toBeTruthy();

        requestReplyManagerSpy.sendRequest.mockImplementation((_settings: any, callbackSettings: any) => {
            return Promise.resolve({
                response: ["ZERO"],
                settings: callbackSettings
            });
        });

        fixture.get().then((data: any) => {
            expect(data).toEqual(TestEnum.ZERO);
            done();
        });
    });

    it("expect correct error reporting after attribute set with invalid value", done => {
        TypeRegistrySingleton.getInstance().addType(ComplexTestType);
        const enumAttribute = new ProxyReadWriteNotifyAttribute(
            {
                proxyParticipantId: "proxy",
                providerParticipantId: "provider",
                settings
            },
            "enumAttribute",
            "joynr.tests.testTypes.ComplexTestType"
        );
        return enumAttribute
            .set({
                value: {
                    _typeName: "joynr.tests.testTypes.ComplexTestType",
                    a: "notANumber"
                }
            })
            .then(() => {
                done.fail("unexpected resolve from setter with invalid value");
            })
            .catch((error: any) => {
                expect(error.message).toEqual(
                    "error setting attribute: enumAttribute: Error: members.a is not of type Number. Actual type is String"
                );
                done();
            });
    });

    it("set calls through to RequestReplyManager", async () => {
        await isOn.set({
            value: true
        });

        expect(requestReplyManagerSpy.sendRequest).toHaveBeenCalled();
        const requestReplyId = requestReplyManagerSpy.sendRequest.mock.calls[0][0].request.requestReplyId;
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
    });

    it("subscribe calls through to SubscriptionManager", done => {
        const spy = {
            onReceive: jest.fn(),
            onError: jest.fn(),
            onSubscribed: jest.fn()
        };

        isOn.subscribe({
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

    it("subscribe notifies", async () => {
        expect(isOn.subscribe).toBeDefined();
        expect(typeof isOn.subscribe === "function").toBeTruthy();

        await isOn.subscribe({
            subscriptionQos,
            onReceive() {
                // do nothing
            },
            onError() {
                // do nothing
            }
        });
    });

    it("subscribe provides a subscriptionId", done => {
        isOn.subscribe({
            subscriptionQos,
            onReceive() {
                // Do nothing
            },
            onError() {
                // Do nothing
            }
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

    it("unsubscribe notifies", async () => {
        expect(isOn.unsubscribe).toBeDefined();
        expect(typeof isOn.unsubscribe === "function").toBeTruthy();

        const subscriptionId = await isOn.subscribe({
            subscriptionQos,
            onReceive() {
                // do nothing
            },
            onError() {
                // do nothing
            }
        });
        await isOn.unsubscribe({
            subscriptionId
        });
    });

    it("rejects if caller sets a generic object without a declared _typeName attribute with the name of a registrered type", done => {
        const radioStationProxyAttributeWrite = new ProxyReadWriteAttribute(
            proxy,
            "radioStationProxyAttributeWrite",
            RadioStation
        );

        radioStationProxyAttributeWrite
            .set({
                value: {
                    name: "radiostationname",
                    station: "station"
                }
            })
            .then(() => done.fail())
            .catch(() => done());
    });
});
