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
const Promise = require("../../../../main/js/global/Promise");
const CapabilitiesRegistrar = require("../../../../main/js/joynr/capabilities/CapabilitiesRegistrar");
const ProviderQos = require("../../../../main/js/generated/joynr/types/ProviderQos");
const ProviderAttribute = require("../../../../main/js/joynr/provider/ProviderAttribute");
const ProviderScope = require("../../../../main/js/generated/joynr/types/ProviderScope");
const uuid = require("../../../../main/js/lib/uuid-annotated");
describe("libjoynr-js.joynr.capabilities.CapabilitiesRegistrar", () => {
    let capabilitiesRegistrar;
    let requestReplyManagerSpy;
    let publicationManagerSpy;
    let participantId;
    let domain;
    let participantIdStorageSpy;
    let discoveryStubSpy;
    let messageRouterSpy;
    let libjoynrMessagingAddress;
    let provider;
    let providerQos;
    let checkImplementation;
    let TestProvider;

    beforeEach(done => {
        // default checkImplemenation, can be overwritten by individual tests as
        // needed
        checkImplementation = function checkImplementationDefault() {
            return [];
        };

        publicationManagerSpy = jasmine.createSpyObj("PublicationManager", [
            "addPublicationProvider",
            "registerOnChangedProvider"
        ]);

        TestProvider = function() {
            this.id = uuid();
            this.interfaceName = "myInterfaceName";
            this.checkImplementation = checkImplementation;
        };

        TestProvider.MAJOR_VERSION = 47;
        TestProvider.MINOR_VERSION = 11;
        provider = new TestProvider();

        spyOn(provider, "checkImplementation").and.callThrough();

        providerQos = new ProviderQos({
            customParameters: [],
            priority: Date.now(),
            scope: ProviderScope.GLOBAL,
            supportsOnChangeSubscriptions: true
        });

        provider.myAttribute = new ProviderAttribute(
            provider,
            {
                dependencies: {
                    publicationManager: publicationManagerSpy
                }
            },
            "myAttribute",
            "Boolean",
            "NOTIFYREADWRITE"
        );
        domain = "testdomain";
        participantId = "myParticipantId";
        participantIdStorageSpy = jasmine.createSpyObj("participantIdStorage", ["getParticipantId"]);
        participantIdStorageSpy.getParticipantId.and.returnValue(participantId);
        requestReplyManagerSpy = jasmine.createSpyObj("RequestReplyManager", ["addRequestCaller"]);
        discoveryStubSpy = jasmine.createSpyObj("discoveryStub", ["add"]);
        discoveryStubSpy.add.and.returnValue(Promise.resolve());
        messageRouterSpy = jasmine.createSpyObj("messageRouter", ["addNextHop"]);

        messageRouterSpy.addNextHop.and.returnValue(Promise.resolve());
        libjoynrMessagingAddress = {
            someKey: "someValue",
            toBe: "a",
            object: {}
        };

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

    it("is instantiable", done => {
        expect(capabilitiesRegistrar).toBeDefined();
        expect(capabilitiesRegistrar instanceof CapabilitiesRegistrar).toBeTruthy();
        done();
    });

    it("is has all members", done => {
        expect(capabilitiesRegistrar.registerProvider).toBeDefined();
        expect(typeof capabilitiesRegistrar.registerProvider === "function").toBeTruthy();
        expect(typeof capabilitiesRegistrar.register === "function").toBeTruthy();
        done();
    });

    it("is checks the provider's implementation", done => {
        capabilitiesRegistrar
            .registerProvider(domain, provider, providerQos)
            .then(() => {
                return null;
            })
            .catch(() => {
                return null;
            });
        expect(provider.checkImplementation).toHaveBeenCalled();
        done();
    });

    it("defaultDelayMs can be configured", done => {
        const overwrittenDelay = 100000;

        jasmine.clock().install();
        const baseTime = new Date();
        jasmine.clock().mockDate(baseTime);

        CapabilitiesRegistrar.setDefaultExpiryIntervalMs(overwrittenDelay);

        capabilitiesRegistrar
            .registerProvider(domain, provider, providerQos)
            .then(() => {
                return null;
            })
            .catch(() => {
                return null;
            });

        expect(discoveryStubSpy.add).toHaveBeenCalledWith(
            jasmine.objectContaining({
                expiryDateMs: baseTime.getTime() + overwrittenDelay
            })
        );

        jasmine.clock().uninstall();
        done();
    });

    it("is checks the provider's implementation, and throws if incomplete", done => {
        provider.checkImplementation = function() {
            return ["Operation:addFavoriteStation"];
        };

        expect(() => {
            capabilitiesRegistrar.registerProvider(domain, provider, providerQos);
        }).toThrow(
            new Error(
                "provider: " + domain + "/" + provider.interfaceName + " is missing: Operation:addFavoriteStation"
            )
        );
        done();
    });

    it("fetches participantId from the participantIdStorage", done => {
        capabilitiesRegistrar
            .registerProvider(domain, provider, providerQos)
            .then(() => {
                return null;
            })
            .catch(() => {
                return null;
            });
        expect(participantIdStorageSpy.getParticipantId).toHaveBeenCalled();
        expect(participantIdStorageSpy.getParticipantId).toHaveBeenCalledWith(domain, provider);
        done();
    });

    it("registers next hop with routing table", done => {
        capabilitiesRegistrar
            .registerProvider(domain, provider, providerQos)
            .then(() => {
                return null;
            })
            .catch(() => {
                return null;
            });
        const isGloballyVisible = providerQos.scope === ProviderScope.GLOBAL;
        expect(messageRouterSpy.addNextHop).toHaveBeenCalled();
        expect(messageRouterSpy.addNextHop).toHaveBeenCalledWith(
            participantId,
            libjoynrMessagingAddress,
            isGloballyVisible
        );
        done();
    });

    it("registers provider at RequestReplyManager", done => {
        capabilitiesRegistrar
            .registerProvider(domain, provider, providerQos)
            .then(() => {
                return null;
            })
            .catch(() => {
                return null;
            });
        expect(requestReplyManagerSpy.addRequestCaller).toHaveBeenCalled();
        expect(requestReplyManagerSpy.addRequestCaller).toHaveBeenCalledWith(participantId, provider);
        done();
    });

    it("handles calls to function register", done => {
        capabilitiesRegistrar
            .register({
                domain: "domain",
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
        done();
    });

    it("uses passed-in participantId", done => {
        const myParticipantId = "myParticipantId";
        capabilitiesRegistrar
            .register({
                domain: "domain",
                provider,
                providerQos,
                participantId: myParticipantId
            })
            .then(() => {
                return null;
            })
            .catch(() => {
                return null;
            });
        expect(requestReplyManagerSpy.addRequestCaller).toHaveBeenCalled();
        expect(requestReplyManagerSpy.addRequestCaller).toHaveBeenCalledWith(myParticipantId, provider);
        done();
    });

    it("registers a provider with PublicationManager if it has an attribute", done => {
        capabilitiesRegistrar
            .registerProvider(domain, provider, providerQos)
            .then(() => {
                return null;
            })
            .catch(() => {
                return null;
            });
        expect(publicationManagerSpy.addPublicationProvider).toHaveBeenCalled();
        expect(publicationManagerSpy.addPublicationProvider).toHaveBeenCalledWith(participantId, provider);
        done();
    });

    it("registers capability at capabilities stub", done => {
        let actualDiscoveryEntry;
        let upperBound;
        const lowerBound = Date.now();
        capabilitiesRegistrar
            .registerProvider(domain, provider, providerQos)
            .then(() => {
                return null;
            })
            .catch(() => {
                return null;
            });
        upperBound = Date.now();
        expect(discoveryStubSpy.add).toHaveBeenCalled();
        actualDiscoveryEntry = discoveryStubSpy.add.calls.argsFor(0)[0];
        expect(actualDiscoveryEntry.domain).toEqual(domain);
        expect(actualDiscoveryEntry.interfaceName).toEqual(provider.interfaceName);
        expect(actualDiscoveryEntry.participantId).toEqual(participantId);
        expect(actualDiscoveryEntry.qos).toEqual(providerQos);
        expect(actualDiscoveryEntry.lastSeenDateMs).not.toBeLessThan(lowerBound);
        expect(actualDiscoveryEntry.lastSeenDateMs).not.toBeGreaterThan(upperBound);
        expect(actualDiscoveryEntry.providerVersion.majorVersion).toEqual(provider.constructor.MAJOR_VERSION);
        expect(actualDiscoveryEntry.providerVersion.minorVersion).toEqual(provider.constructor.MINOR_VERSION);
        done();
    });

    it("returns the provider participant ID", done => {
        capabilitiesRegistrar
            .registerProvider(domain, provider, providerQos)
            .then(result => {
                expect(result).toEqual(participantId);
                done();
                return null;
            })
            .catch(error => {
                fail("unexpected error: " + error);
                return null;
            });
    });

    it("returns the promise onRejected from capabilites stub", done => {
        discoveryStubSpy.add.and.returnValue(Promise.reject(new Error("Some error.")));

        capabilitiesRegistrar
            .registerProvider(domain, provider, providerQos)
            .then(() => {
                fail("expected an error");
                return null;
            })
            .catch(error => {
                expect(Object.prototype.toString.call(error) === "[object Error]").toBeTruthy();
                done();
                return null;
            });
    });

    it("CapabilitiesRegistrar throws exception when called while shut down", done => {
        capabilitiesRegistrar.shutdown();
        expect(() => {
            capabilitiesRegistrar.registerProvider(domain, provider, providerQos);
        }).toThrow();
        expect(() => {
            capabilitiesRegistrar.unregisterProvider(domain, provider);
        }).toThrow();
        done();
    });
});
