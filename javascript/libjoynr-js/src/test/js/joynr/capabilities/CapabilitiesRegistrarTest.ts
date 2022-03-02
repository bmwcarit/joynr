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

import DiscoveryEntry from "../../../../main/js/generated/joynr/types/DiscoveryEntry";
import CapabilitiesRegistrar from "../../../../main/js/joynr/capabilities/CapabilitiesRegistrar";
import ProviderQos from "../../../../main/js/generated/joynr/types/ProviderQos";
import * as ProviderAttribute from "../../../../main/js/joynr/provider/ProviderAttribute";
import ProviderScope from "../../../../main/js/generated/joynr/types/ProviderScope";
import { nanoid } from "nanoid";

describe("libjoynr-js.joynr.capabilities.CapabilitiesRegistrar", () => {
    let capabilitiesRegistrar: CapabilitiesRegistrar;
    let requestReplyManagerSpy: any;
    let publicationManagerSpy: any;
    let participantId: string;
    let domain: string;
    let participantIdStorageSpy: any;
    let discoveryStubSpy: any;
    let messageRouterSpy: any;
    let libjoynrMessagingAddress: any;
    let provider: any;
    let providerQos: ProviderQos;
    const gbids = ["joynrdefaultgbid"];

    class TestProvider {
        public static MAJOR_VERSION = 47;
        public static MINOR_VERSION = 11;
        public id = nanoid();
        public interfaceName = "myInterfaceName";
        public checkImplementation = jest.fn().mockReturnValue([]);
    }

    beforeEach(done => {
        publicationManagerSpy = {
            addPublicationProvider: jest.fn(),
            removePublicationProvider: jest.fn(),
            registerOnChangedProvider: jest.fn()
        };

        provider = new TestProvider();

        providerQos = new ProviderQos({
            customParameters: [],
            priority: Date.now(),
            scope: ProviderScope.GLOBAL,
            supportsOnChangeSubscriptions: true
        });

        provider.myAttribute = new ProviderAttribute.ProviderReadWriteNotifyAttribute(
            provider,
            {
                dependencies: {
                    publicationManager: publicationManagerSpy
                }
            },
            "myAttribute",
            "Boolean"
        );
        domain = "testdomain";
        participantId = "myParticipantId";
        participantIdStorageSpy = {
            getParticipantId: jest.fn(),
            setParticipantId: jest.fn()
        };
        participantIdStorageSpy.getParticipantId.mockReturnValue(participantId);
        requestReplyManagerSpy = {
            addRequestCaller: jest.fn(),
            removeRequestCaller: jest.fn()
        };
        discoveryStubSpy = {
            add: jest.fn(),
            addToAll: jest.fn(),
            remove: jest.fn()
        };
        discoveryStubSpy.add.mockReturnValue(Promise.resolve());
        discoveryStubSpy.remove.mockReturnValue(Promise.resolve());
        discoveryStubSpy.addToAll.mockReturnValue(Promise.resolve());
        messageRouterSpy = {
            addNextHop: jest.fn(),
            removeNextHop: jest.fn()
        };

        messageRouterSpy.addNextHop.mockReturnValue(Promise.resolve());
        libjoynrMessagingAddress = {
            someKey: "someValue",
            toBe: "a",
            object: {}
        };
        messageRouterSpy.removeNextHop.mockReturnValue(Promise.resolve());

        capabilitiesRegistrar = new CapabilitiesRegistrar({
            discoveryStub: discoveryStubSpy,
            messageRouter: messageRouterSpy,
            participantIdStorage: participantIdStorageSpy,
            libjoynrMessagingAddress,
            requestReplyManager: requestReplyManagerSpy,
            publicationManager: publicationManagerSpy
        });

        done();
    });

    it("is instantiable", () => {
        expect(capabilitiesRegistrar).toBeDefined();
    });

    it("has all members", () => {
        expect(capabilitiesRegistrar.registerProvider).toBeDefined();
        expect(typeof capabilitiesRegistrar.registerProvider === "function").toBeTruthy();
        expect(typeof capabilitiesRegistrar.register === "function").toBeTruthy();
    });

    it("checks the provider's implementation", async () => {
        await capabilitiesRegistrar.registerProvider(domain, provider, providerQos);
        expect(provider.checkImplementation).toHaveBeenCalled();
    });

    it("supports configuring defaultDelayMs", async () => {
        const overwrittenDelay = 100000;

        jest.useFakeTimers();
        const baseTime = Date.now();
        jest.spyOn(Date, "now").mockImplementationOnce(() => {
            return baseTime;
        });

        CapabilitiesRegistrar.setDefaultExpiryIntervalMs(overwrittenDelay);

        await capabilitiesRegistrar.registerProvider(domain, provider, providerQos).catch((error: any) => {
            jest.useRealTimers();
            throw error;
        });

        expect(discoveryStubSpy.add).toHaveBeenCalled();
        const actualDiscoveryEntry = discoveryStubSpy.add.mock.calls[0][0];
        expect(actualDiscoveryEntry.expiryDateMs).toEqual(baseTime + overwrittenDelay);

        jest.useRealTimers();
    });

    it("checks the provider's implementation, and rejects if incomplete", async () => {
        provider.checkImplementation = function() {
            return ["Operation:addFavoriteStation"];
        };

        const e = await reversePromise(capabilitiesRegistrar.registerProvider(domain, provider, providerQos));
        expect(e).toEqual(
            new Error(
                `provider: ${domain}/${provider.interfaceName}.v${
                    provider.constructor.MAJOR_VERSION
                } is missing: Operation:addFavoriteStation`
            )
        );
    });

    it("fetches participantId from the participantIdStorage", async () => {
        await capabilitiesRegistrar.registerProvider(domain, provider, providerQos);
        expect(participantIdStorageSpy.getParticipantId).toHaveBeenCalled();
        expect(participantIdStorageSpy.getParticipantId).toHaveBeenCalledWith(domain, provider);
    });

    it("registers next hop with routing table", async () => {
        await capabilitiesRegistrar.registerProvider(domain, provider, providerQos);
        const isGloballyVisible = providerQos.scope === ProviderScope.GLOBAL;
        expect(messageRouterSpy.addNextHop).toHaveBeenCalled();
        expect(messageRouterSpy.addNextHop).toHaveBeenCalledWith(
            participantId,
            libjoynrMessagingAddress,
            isGloballyVisible
        );
    });

    it("registers provider at RequestReplyManager", async () => {
        await capabilitiesRegistrar.registerProvider(domain, provider, providerQos);
        expect(requestReplyManagerSpy.addRequestCaller).toHaveBeenCalled();
        expect(requestReplyManagerSpy.addRequestCaller).toHaveBeenCalledWith(participantId, provider);
    });

    it("handles calls to function register", () => {
        capabilitiesRegistrar
            .register({
                domain,
                provider,
                providerQos
            })
            .then(() => {
                return null;
            })
            .catch(() => {
                return null;
            });
        expect(requestReplyManagerSpy.addRequestCaller).toHaveBeenCalled();
        expect(requestReplyManagerSpy.addRequestCaller).toHaveBeenCalledWith(participantId, provider);
    });

    it("uses passed-in participantId", async () => {
        const myParticipantId = "myFixedParticipantId";
        await capabilitiesRegistrar.register({
            domain,
            provider,
            providerQos,
            participantId: myParticipantId
        });
        expect(participantIdStorageSpy.setParticipantId).toHaveBeenCalledWith(domain, provider, myParticipantId);
        expect(requestReplyManagerSpy.addRequestCaller).toHaveBeenCalled();
        expect(requestReplyManagerSpy.addRequestCaller).toHaveBeenCalledWith(myParticipantId, provider);
    });

    it("registers a provider with PublicationManager if it has an attribute", async () => {
        await capabilitiesRegistrar.registerProvider(domain, provider, providerQos);
        expect(publicationManagerSpy.addPublicationProvider).toHaveBeenCalled();
        expect(publicationManagerSpy.addPublicationProvider).toHaveBeenCalledWith(participantId, provider);
    });

    it("register calls discoveryStub with gbids", async () => {
        await capabilitiesRegistrar.register({
            domain,
            provider,
            providerQos,
            gbids
        });
        expect(discoveryStubSpy.add).toHaveBeenCalledWith(expect.any(Object), expect.any(Boolean), gbids);
    });

    it("register calls discoveryStub with empty array if gbids aren't provided", async () => {
        const myParticipantId = "myFixedParticipantId";
        const myDomain = "myDomain";
        await capabilitiesRegistrar.register({
            domain: myDomain,
            provider,
            providerQos,
            participantId: myParticipantId
        });
        expect(discoveryStubSpy.add).toHaveBeenCalledWith(expect.any(Object), expect.any(Boolean), []);
    });

    it("registers capability at capabilities stub", async () => {
        const lowerBound = Date.now();
        await capabilitiesRegistrar.registerProvider(domain, provider, providerQos);
        const upperBound = Date.now();
        expect(discoveryStubSpy.add).toHaveBeenCalled();
        const actualDiscoveryEntry = discoveryStubSpy.add.mock.calls[0][0];
        expect(actualDiscoveryEntry.domain).toEqual(domain);
        expect(actualDiscoveryEntry.interfaceName).toEqual(provider.interfaceName);
        expect(actualDiscoveryEntry.participantId).toEqual(participantId);
        expect(actualDiscoveryEntry.qos).toEqual(providerQos);
        expect(actualDiscoveryEntry.lastSeenDateMs).not.toBeLessThan(lowerBound);
        expect(actualDiscoveryEntry.lastSeenDateMs).not.toBeGreaterThan(upperBound);
        expect(actualDiscoveryEntry.providerVersion.majorVersion).toEqual(provider.constructor.MAJOR_VERSION);
        expect(actualDiscoveryEntry.providerVersion.minorVersion).toEqual(provider.constructor.MINOR_VERSION);
    });

    async function testAwaitGlobalRegistrationScenario(awaitGlobalRegistration: boolean) {
        await capabilitiesRegistrar.register({
            domain,
            provider,
            providerQos,
            awaitGlobalRegistration
        });
        const actualAwaitGlobalRegistration = discoveryStubSpy.add.mock.calls[0][1];
        expect(actualAwaitGlobalRegistration).toEqual(awaitGlobalRegistration);
    }

    it("calls discoveryProxy.add() with same awaitGlobalRegistration parameter true used in call to registerProvider", async () => {
        const awaitGlobalRegistration = true;
        await testAwaitGlobalRegistrationScenario(awaitGlobalRegistration);
    });

    it("calls discoveryProxy.add() with same awaitGlobalRegistration parameter false used in call to registerProvider", async () => {
        const awaitGlobalRegistration = false;
        await testAwaitGlobalRegistrationScenario(awaitGlobalRegistration);
    });

    it("calls discoveryProxy.add() with awaitGlobalRegistration parameter false on default call of registerProvider", async () => {
        await capabilitiesRegistrar.registerProvider(domain, provider, providerQos);
        const expectedAwaitGlobalRegistration = false;
        const actualAwaitGlobalRegistration = discoveryStubSpy.add.mock.calls[0][1];
        expect(actualAwaitGlobalRegistration).toEqual(expectedAwaitGlobalRegistration);
    });

    it("returns the provider participant ID", async () => {
        const result = await capabilitiesRegistrar.registerProvider(domain, provider, providerQos);
        expect(result).toEqual(participantId);
    });

    it("returns the promise onRejected from capabilites stub", async () => {
        discoveryStubSpy.add.mockReturnValue(Promise.reject(new Error("Some error.")));

        const error = await reversePromise(capabilitiesRegistrar.registerProvider(domain, provider, providerQos));
        expect(Object.prototype.toString.call(error) === "[object Error]").toBeTruthy();
    });

    function reversePromise(promise: Promise<any>) {
        return promise.then(suc => Promise.reject(suc)).catch((e: any) => e);
    }

    it("rejects with an exception when called while shutting down", async () => {
        capabilitiesRegistrar.shutdown();
        await reversePromise(capabilitiesRegistrar.registerProvider(domain, provider, providerQos));
        await reversePromise(capabilitiesRegistrar.unregisterProvider(domain, provider));
    });

    it("deletes the next hop when discoveryStub.add fails", async () => {
        const error = new Error("some Error");
        discoveryStubSpy.add.mockReturnValue(Promise.reject(error));
        const e = await reversePromise(capabilitiesRegistrar.registerProvider(domain, provider, providerQos));
        expect(e).toEqual(error);
        expect(messageRouterSpy.removeNextHop).toHaveBeenCalled();
    });

    it("removes capability at discoveryStub and removes next hop in routing table when unregistering provider", async () => {
        await capabilitiesRegistrar.registerProvider(domain, provider, providerQos);

        await capabilitiesRegistrar.unregisterProvider(domain, provider);

        expect(messageRouterSpy.removeNextHop).toHaveBeenCalled();
        expect(discoveryStubSpy.remove).toHaveBeenCalled();
    });

    describe(`registerInAllKnownBackends`, () => {
        it(`calls discoveryStub.addToAll`, async () => {
            await capabilitiesRegistrar.registerInAllKnownBackends({
                domain,
                provider,
                providerQos
            });
            expect(discoveryStubSpy.addToAll).toHaveBeenCalled();
            const discoveryEntry = discoveryStubSpy.addToAll.mock.calls.slice(-1)[0][0];
            expect(discoveryEntry).toBeInstanceOf(DiscoveryEntry);
            expect(discoveryEntry.domain).toEqual(domain);
            expect(discoveryEntry.interface).toEqual(provider.interfacename);
            expect(discoveryEntry.participandId).toEqual(provider.participantId);
            expect(discoveryEntry.qos).toEqual(providerQos);
        });
    });
});
