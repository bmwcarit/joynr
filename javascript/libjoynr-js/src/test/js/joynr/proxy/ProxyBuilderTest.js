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
const ProxyBuilder = require("../../../../main/js/joynr/proxy/ProxyBuilder");
const DiscoveryQos = require("../../../../main/js/joynr/proxy/DiscoveryQos");
const MessagingQos = require("../../../../main/js/joynr/messaging/MessagingQos");
const ProviderQos = require("../../../../main/js/generated/joynr/types/ProviderQos");
const ProviderScope = require("../../../../main/js/generated/joynr/types/ProviderScope");
const DiscoveryEntryWithMetaInfo = require("../../../../main/js/generated/joynr/types/DiscoveryEntryWithMetaInfo");
const ArbitrationStrategyCollection = require("../../../../main/js/joynr/types/ArbitrationStrategyCollection");
const DiscoveryScope = require("../../../../main/js/generated/joynr/types/DiscoveryScope");
const Version = require("../../../../main/js/generated/joynr/types/Version");
const RadioProxy = require("../../../generated/joynr/vehicle/RadioProxy");
const Promise = require("../../../../main/js/global/Promise");
const waitsFor = require("../../../../test/js/global/WaitsFor");

const safetyTimeoutDelta = 100;

let interfaceName = "io/joynr/apps/radio";

describe("libjoynr-js.joynr.proxy.ProxyBuilder", () => {
    let proxyBuilder;
    let domain;
    let arbitratorSpy;
    let discoveryQos;
    let settings;
    let capInfo;
    let arbitratedCaps;
    let messagingQos;
    let messageRouterSpy;
    let libjoynrMessagingAddress;

    beforeEach(done => {
        domain = "myDomain";
        interfaceName = "vehicle/Radio";
        discoveryQos = new DiscoveryQos({
            discoveryTimeoutMs: 5000,
            discoveryRetryDelayMs: 900,
            arbitrationStrategy: ArbitrationStrategyCollection.Nothing,
            cacheMaxAgeMs: 0,
            discoveryScope: DiscoveryScope.LOCAL_THEN_GLOBAL,
            additionalParameters: {}
        });
        messagingQos = new MessagingQos();
        settings = {
            domain,
            discoveryQos,
            messagingQos,
            staticArbitration: false
        };

        capInfo = new DiscoveryEntryWithMetaInfo({
            providerVersion: new Version({ majorVersion: 47, minorVersion: 11 }),
            domain,
            interfaceName,
            participantId: "myParticipantId",
            qos: new ProviderQos({
                customParameter: [],
                priority: 1,
                scope: ProviderScope.GLOBAL,
                supportsOnChangeSubscriptions: true
            }),
            lastSeenDateMs: Date.now(),
            expiryDateMs: Date.now() + 60000,
            publicKeyId: "",
            isLocal: false
        });

        arbitratedCaps = [capInfo];

        arbitratorSpy = jasmine.createSpyObj("arbitratorSpy", ["startArbitration"]);
        messageRouterSpy = jasmine.createSpyObj("messageRouterSpy", ["addNextHop", "setToKnown"]);
        libjoynrMessagingAddress = {
            key: "libjoynrMessagingAddress"
        };

        const resolvedPromise = Promise.resolve(arbitratedCaps);
        arbitratorSpy.startArbitration.and.returnValue(resolvedPromise);
        messageRouterSpy.addNextHop.and.returnValue(resolvedPromise);
        proxyBuilder = new ProxyBuilder(
            {
                arbitrator: arbitratorSpy
            },
            {
                messageRouter: messageRouterSpy,
                libjoynrMessagingAddress
            }
        );
        done();
    });

    it("is defined and of correct type", done => {
        expect(proxyBuilder).toBeDefined();
        expect(typeof proxyBuilder.build === "function").toBe(true);
        done();
    });

    it("throws exceptions upon missing or wrongly typed arguments", done => {
        // settings is undefined
        expect(() => {
            proxyBuilder.build(RadioProxy);
        }).toThrow();
        // settings is not of type object
        expect(() => {
            proxyBuilder.build(RadioProxy, "notObject");
        }).toThrow();
        // domain is undefined
        expect(() => {
            proxyBuilder.build(RadioProxy, {
                discoveryQos: new DiscoveryQos(),
                messagingQos: new MessagingQos()
            });
        }).toThrow();
        // domain is not of type string
        expect(() => {
            proxyBuilder.build(RadioProxy, {
                domain: 1234,
                discoveryQos: new DiscoveryQos(),
                messagingQos: new MessagingQos()
            });
        }).toThrow();
        // discoveryQos is not of type DiscoveryQos
        expect(() => {
            proxyBuilder.build(RadioProxy, {
                domain: "",
                discoveryQos: {},
                messagingQos: new MessagingQos()
            });
        }).not.toThrow();
        // messagingQos is is not of type MessagingQos
        expect(() => {
            proxyBuilder.build(RadioProxy, {
                domain: "",
                discoveryQos: new DiscoveryQos(),
                messagingQos: {}
            });
        }).not.toThrow();
        // discoveryQos is missing
        expect(() => {
            proxyBuilder.build(RadioProxy, {
                domain: "",
                messagingQos: new MessagingQos()
            });
        }).not.toThrow();
        // messagingQos is missing
        expect(() => {
            proxyBuilder.build(RadioProxy, {
                domain: "",
                discoveryQos: new DiscoveryQos()
            });
        }).not.toThrow();
        // messagingQos and discoveryQos are missing
        expect(() => {
            proxyBuilder.build(RadioProxy, {
                domain: ""
            });
        }).not.toThrow();
        // ok
        expect(() => {
            proxyBuilder.build(RadioProxy, {
                domain: "",
                discoveryQos: new DiscoveryQos(),
                messagingQos: new MessagingQos()
            });
        }).not.toThrow();
        done();
    });

    it("does not throw", done => {
        expect(() => {
            proxyBuilder.build(RadioProxy, settings);
        }).not.toThrow();
        done();
    });

    it("returns an A+ Promise object", done => {
        const promise = proxyBuilder.build(RadioProxy, settings);
        expect(promise).toBeDefined();
        expect(promise).not.toBeNull();
        expect(typeof promise === "object").toBeTruthy();
        expect(promise.then).toBeDefined();
        done();
    });

    it("calls arbitrator with correct arguments", done => {
        proxyBuilder.build(RadioProxy, settings);

        waitsFor(
            () => {
                return arbitratorSpy.startArbitration.calls.count() > 0;
            },
            "startArbitration invoked",
            100
        )
            .then(() => {
                expect(arbitratorSpy.startArbitration).toHaveBeenCalled();
                expect(arbitratorSpy.startArbitration).toHaveBeenCalledWith({
                    domains: [settings.domain],
                    interfaceName,
                    discoveryQos: settings.discoveryQos,
                    staticArbitration: settings.staticArbitration,
                    proxyVersion: new Version({ majorVersion: 47, minorVersion: 11 })
                });
                done();
                return null;
            })
            .catch(fail);
    });

    it("returned promise is resolved with a frozen proxy object by default", done => {
        const spy = jasmine.createSpyObj("spy", ["onFulfilled", "onRejected"]);

        proxyBuilder
            .build(RadioProxy, settings)
            .then(argument => {
                spy.onFulfilled(argument);
            })
            .catch(spy.onRejected);

        waitsFor(
            () => {
                return spy.onFulfilled.calls.count() > 0;
            },
            "until the ProxyBuilder promise is not pending any more",
            safetyTimeoutDelta
        )
            .then(() => {
                expect(spy.onFulfilled).toHaveBeenCalled();
                expect(spy.onFulfilled).toHaveBeenCalledWith(jasmine.any(RadioProxy));
                expect(spy.onRejected).not.toHaveBeenCalled();
                const proxy = spy.onFulfilled.calls.mostRecent().args[0];
                proxy.adaptfrozenObjectShouldNotWork = "adaptfrozenObjectShouldNotWork";
                expect(proxy.adaptfrozenObjectShouldNotWork).not.toBeDefined();
                done();
                return null;
            })
            .catch(fail);
    });

    it("returned promise is resolved with an unfrozen proxy object if set accordingly", done => {
        const spy = jasmine.createSpyObj("spy", ["onFulfilled", "onRejected"]);

        settings.freeze = false;
        proxyBuilder
            .build(RadioProxy, settings)
            .then(argument => {
                spy.onFulfilled(argument);
            })
            .catch(spy.onRejected);

        waitsFor(
            () => {
                return spy.onFulfilled.calls.count() > 0;
            },
            "until the ProxyBuilder promise is not pending any more",
            safetyTimeoutDelta
        )
            .then(() => {
                expect(spy.onFulfilled).toHaveBeenCalled();
                expect(spy.onFulfilled).toHaveBeenCalledWith(jasmine.any(RadioProxy));
                expect(spy.onRejected).not.toHaveBeenCalled();
                const proxy = spy.onFulfilled.calls.mostRecent().args[0];
                proxy.adaptUnfrozenObjectShouldWork = "adaptUnfrozenObjectShouldWork";
                expect(proxy.adaptUnfrozenObjectShouldWork).toEqual("adaptUnfrozenObjectShouldWork");
                done();
                return null;
            })
            .catch(fail);
    });

    it("returned promise is rejected with error", done => {
        const spy = jasmine.createSpyObj("spy", ["onFulfilled", "onRejected"]);
        const error = new Error("MyError");

        let onRejectedCalled = false;

        arbitratorSpy.startArbitration.and.returnValue(Promise.reject(error));
        proxyBuilder = new ProxyBuilder({
            arbitrator: arbitratorSpy
        });
        proxyBuilder
            .build(RadioProxy, settings)
            .then(spy.onFulfilled)
            .catch(error => {
                onRejectedCalled = true;
                spy.onRejected(error);
            });

        waitsFor(
            () => {
                return onRejectedCalled;
            },
            "until the ProxyBuilder promise is not pending any more",
            safetyTimeoutDelta
        )
            .then(() => {
                expect(spy.onFulfilled).not.toHaveBeenCalled();
                expect(spy.onRejected).toHaveBeenCalled();
                expect(spy.onRejected).toHaveBeenCalledWith(error);
                done();
                return null;
            })
            .catch(fail);
    });

    it("returned promise is resolved with proxy object with injected providerParticipantId", done => {
        const spy = jasmine.createSpyObj("spy", ["onFulfilled", "onRejected"]);

        let onFulfilledCalled = false;

        proxyBuilder
            .build(RadioProxy, settings)
            .then(argument => {
                onFulfilledCalled = true;
                spy.onFulfilled(argument);
            })
            .catch(spy.onRejected);

        waitsFor(
            () => {
                return onFulfilledCalled;
            },
            "until the ProxyBuilder promise is not pending any more",
            safetyTimeoutDelta
        )
            .then(() => {
                expect(spy.onFulfilled.calls.argsFor(0)[0].providerDiscoveryEntry).toEqual(arbitratedCaps[0]);
                done();
                return null;
            })
            .catch(fail);
    });

    it("adds a routing table entry for proxy and knows provider", done => {
        let spy = jasmine.createSpyObj("spy", ["onFulfilled", "onRejected"]);
        let onFulfilledCalled = false;

        proxyBuilder
            .build(RadioProxy, settings)
            .then(argument => {
                onFulfilledCalled = true;
                spy.onFulfilled(argument);
                return null;
            })
            .catch(() => {
                spy.onRejected();
                return null;
            });

        waitsFor(
            () => {
                return onFulfilledCalled;
            },
            "until the ProxyBuilder promise is not pending any more",
            safetyTimeoutDelta
        )
            .then(() => {
                expect(spy.onFulfilled.calls.argsFor(0)[0].providerDiscoveryEntry).toEqual(arbitratedCaps[0]);
                expect(typeof messageRouterSpy.addNextHop.calls.mostRecent().args[0] === "string").toBeTruthy();
                expect(messageRouterSpy.setToKnown.calls.mostRecent().args[0]).toEqual(arbitratedCaps[0].participantId);
                expect(messageRouterSpy.addNextHop.calls.mostRecent().args[1]).toEqual(libjoynrMessagingAddress);
                expect(messageRouterSpy.addNextHop.calls.mostRecent().args[2]).toEqual(true);
                done();
                return null;
            })
            .catch(fail);
    });
});
