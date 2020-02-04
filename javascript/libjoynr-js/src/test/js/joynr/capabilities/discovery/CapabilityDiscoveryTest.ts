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

import CapabilityDiscovery from "../../../../../main/js/joynr/capabilities/discovery/CapabilityDiscovery";
import DiscoveryQos from "../../../../../main/js/generated/joynr/types/DiscoveryQos";
import ProviderQos from "../../../../../main/js/generated/joynr/types/ProviderQos";
import CustomParameter from "../../../../../main/js/generated/joynr/types/CustomParameter";
import ProviderScope from "../../../../../main/js/generated/joynr/types/ProviderScope";
import DiscoveryScope from "../../../../../main/js/generated/joynr/types/DiscoveryScope";
import DiscoveryEntry from "../../../../../main/js/generated/joynr/types/DiscoveryEntry";
import GlobalDiscoveryEntry from "../../../../../main/js/generated/joynr/types/GlobalDiscoveryEntry";
import ChannelAddress from "../../../../../main/js/generated/joynr/system/RoutingTypes/ChannelAddress";
import Version from "../../../../../main/js/generated/joynr/types/Version";
import * as CapabilitiesUtil from "../../../../../main/js/joynr/util/CapabilitiesUtil";
import { multipleSetImmediate, reversePromise } from "../../../testUtil";
import testUtil = require("../../../testUtil");
import ApplicationException from "joynr/joynr/exceptions/ApplicationException";
import DiscoveryError from "joynr/generated/joynr/types/DiscoveryError";
const typeRegistry = require("../../../../../main/js/joynr/types/TypeRegistrySingleton").getInstance();
typeRegistry
    .addType(DiscoveryQos)
    .addType(ProviderQos)
    .addType(CustomParameter)
    .addType(ProviderScope)
    .addType(DiscoveryScope)
    .addType(DiscoveryEntry)
    .addType(GlobalDiscoveryEntry)
    .addType(ChannelAddress)
    .addType(Version);

let participantId: any, domain: any, interfaceName: string, discoveryQos: DiscoveryQos;
let discoveryEntries: any,
    discoveryEntriesReturned: any,
    globalDiscoveryEntries: any,
    globalDiscoveryEntriesReturned: any;
let capabilityDiscovery: CapabilityDiscovery, proxyBuilderSpy: any, address: any, localCapStoreSpy: any;
let globalCapCacheSpy: any, globalCapDirSpy: any;
let startDateMs: number;
const expiryDateMs = Date.now() + 1e10;
const knownGbids = ["joynrdefaultgbid", "someOtherGbid"];
const gbids = ["joynrdefaultgbid"];

const messageRouterSpy: any = {
    addNextHop: jest.fn(),
    resolveNextHop: jest.fn()
};

messageRouterSpy.addNextHop.mockReturnValue(Promise.resolve());
messageRouterSpy.resolveNextHop.mockReturnValue(Promise.resolve());

function getSpiedLookupObjWithReturnValue(returnValue: any) {
    const spyObj = {
        lookup: jest.fn(),
        add: jest.fn(),
        remove: jest.fn(),
        touch: jest.fn()
    };
    spyObj.lookup.mockReturnValue(returnValue);
    spyObj.add.mockReturnValue(returnValue);
    spyObj.remove.mockReturnValue(spyObj);
    return spyObj;
}

function getGlobalDiscoveryEntry(domain: any, interfaceName: string, newGlobalAddress: any) {
    return new GlobalDiscoveryEntry({
        providerVersion: new Version({ majorVersion: 47, minorVersion: 11 }),
        domain,
        interfaceName,
        lastSeenDateMs: Date.now(),
        qos: new ProviderQos({
            customParameters: [
                new CustomParameter({
                    name: "theName",
                    value: "theValue"
                })
            ],
            priority: 1234,
            scope:
                discoveryQos.discoveryScope === DiscoveryScope.LOCAL_ONLY ? ProviderScope.LOCAL : ProviderScope.GLOBAL,
            supportsOnChangeSubscriptions: true
        }),
        address: JSON.stringify(newGlobalAddress !== undefined ? newGlobalAddress : address),
        participantId: "700",
        publicKeyId: "",
        expiryDateMs
    });
}

function assertDiscoveryEntryEquals(expected: any, actual: any) {
    expect(actual.providerVersion).toEqual(expected.providerVersion);
    expect(actual.domain).toEqual(expected.domain);
    expect(actual.interfaceName).toEqual(expected.interfaceName);
    expect(actual.participantId).toEqual(expected.participantId);
    expect(actual.qos).toEqual(expected.qos);
    expect(actual.lastSeenDateMs).toEqual(expected.lastSeenDateMs);
    expect(actual.expiryDateMs).toEqual(expected.expiryDateMs);
    expect(actual.publicKeyId).toEqual(expected.publicKeyId);
    expect(actual.isLocal).toEqual(expected.isLocal);
    expect(actual.address).toEqual(expected.address);
}

function getDiscoveryEntry(domain: any, interfaceName: string) {
    return new DiscoveryEntry({
        providerVersion: new Version({ majorVersion: 47, minorVersion: 11 }),
        domain,
        interfaceName,
        qos: new ProviderQos({
            customParameters: [
                new CustomParameter({
                    name: "theName",
                    value: "theValue"
                })
            ],
            priority: 1234,
            scope:
                discoveryQos.discoveryScope === DiscoveryScope.LOCAL_ONLY ? ProviderScope.LOCAL : ProviderScope.GLOBAL,
            supportsOnChangeSubscriptions: true
        }),
        participantId: "700",
        lastSeenDateMs: Date.now(),
        publicKeyId: "",
        expiryDateMs
    });
}

describe("libjoynr-js.joynr.capabilities.discovery.CapabilityDiscovery", () => {
    beforeEach(done => {
        let i: number, discoveryEntry: DiscoveryEntry;
        startDateMs = Date.now();
        domain = "myDomain";
        interfaceName = "myInterfaceName";
        address = new ChannelAddress({
            channelId: `${domain}TestCapabilityDiscoveryChannel`,
            messagingEndpointUrl: "http://testUrl"
        });
        discoveryQos = new DiscoveryQos({
            cacheMaxAge: 0,
            discoveryScope: DiscoveryScope.LOCAL_THEN_GLOBAL
        } as any);

        discoveryEntries = [];
        discoveryEntriesReturned = [];
        for (i = 0; i < 12; ++i) {
            discoveryEntry = getDiscoveryEntry(domain + i.toString(), interfaceName + i.toString());
            discoveryEntries.push(discoveryEntry);
            discoveryEntriesReturned.push(CapabilitiesUtil.convertToDiscoveryEntryWithMetaInfo(true, discoveryEntry));
        }

        globalDiscoveryEntries = [];
        globalDiscoveryEntriesReturned = [];
        for (i = 0; i < 12; ++i) {
            discoveryEntry = getGlobalDiscoveryEntry(
                domain + i.toString(),
                interfaceName + i.toString(),
                new ChannelAddress({
                    channelId: `globalCapInfo${i.toString()}`,
                    messagingEndpointUrl: "http://testurl"
                })
            );
            globalDiscoveryEntries.push(discoveryEntry);
            globalDiscoveryEntriesReturned.push(
                CapabilitiesUtil.convertToDiscoveryEntryWithMetaInfo(false, discoveryEntry)
            );
        }

        localCapStoreSpy = getSpiedLookupObjWithReturnValue([]);
        globalCapCacheSpy = getSpiedLookupObjWithReturnValue([]);
        globalCapDirSpy = getSpiedLookupObjWithReturnValue(Promise.resolve({ result: [] }));
        proxyBuilderSpy = {
            build: jest.fn()
        };
        proxyBuilderSpy.build.mockReturnValue(Promise.resolve(globalCapDirSpy));
        capabilityDiscovery = new CapabilityDiscovery(localCapStoreSpy, globalCapCacheSpy, "io.joynr", knownGbids);
        capabilityDiscovery.setDependencies(messageRouterSpy, proxyBuilderSpy);
        capabilityDiscovery.globalAddressReady(address);
        done();
    });

    describe("lookupByParticipantId", () => {
        beforeEach(done => {
            globalCapDirSpy = getSpiedLookupObjWithReturnValue(Promise.resolve({ result: undefined }));
            proxyBuilderSpy.build.mockReturnValue(Promise.resolve(globalCapDirSpy));
            done();
        });

        it("calls local capabilities directory according to discoveryQos.discoveryScope LOCAL_THEN_GLOBAL when local cache provides non-empty result", () => {
            localCapStoreSpy.lookup.mockReturnValue(discoveryEntries);

            capabilityDiscovery.lookupByParticipantId(participantId, discoveryQos, gbids);
            expect(localCapStoreSpy.lookup).toHaveBeenCalledWith({
                participantId
            });
            expect(globalCapDirSpy.lookup).not.toHaveBeenCalled();
            expect(globalCapCacheSpy.lookup).not.toHaveBeenCalled();
        });

        it("calls local capabilities directory according to discoveryQos.discoveryScope LOCAL_THEN_GLOBAL when global cache provides non-empty result", () => {
            // discoveryEntries cached in globalCapabilitiesCache
            globalCapCacheSpy.lookup.mockReturnValue(discoveryEntries);

            capabilityDiscovery.lookupByParticipantId(participantId, discoveryQos, gbids);
            expect(globalCapCacheSpy.lookup).toHaveBeenCalledWith({
                participantId,
                cacheMaxAge: discoveryQos.cacheMaxAge
            });
            expect(localCapStoreSpy.lookup).toHaveBeenCalledWith({
                participantId
            });
            expect(globalCapDirSpy.lookup).not.toHaveBeenCalled();
        });

        it("calls local and global capabilities directory according to discoveryQos.discoveryScope LOCAL_THEN_GLOBAL when local store and global cache provide empty result", async done => {
            discoveryQos.discoveryScope = DiscoveryScope.LOCAL_THEN_GLOBAL;
            try {
                await capabilityDiscovery.lookupByParticipantId(participantId, discoveryQos, gbids);
                done.fail("lookupByParticipantId should throw ApplicationException.");
            } catch (e) {
                expect(e).toBeInstanceOf(ApplicationException);
                expect(e.error).toBeInstanceOf(DiscoveryError);
                expect(e.error).toEqual(DiscoveryError.INTERNAL_ERROR);
            }
            await multipleSetImmediate();
            expect(localCapStoreSpy.lookup).toHaveBeenCalledWith({
                participantId
            });
            expect(globalCapCacheSpy.lookup).toHaveBeenCalledWith({
                participantId,
                cacheMaxAge: discoveryQos.cacheMaxAge
            });
            expect(globalCapDirSpy.lookup).toHaveBeenCalledWith({
                participantId,
                gbids
            });
            done();
        });
    });

    it(`throws if instantiated without knownGbids`, () => {
        expect(() => {
            new CapabilityDiscovery(localCapStoreSpy, globalCapCacheSpy, "io.joynr", []);
        }).toThrow();
    });

    it("is instantiable, of correct type and has all members", () => {
        expect(capabilityDiscovery).toBeDefined();
        expect(capabilityDiscovery instanceof CapabilityDiscovery).toBeTruthy();
        expect(capabilityDiscovery.lookup).toBeDefined();
        expect(typeof capabilityDiscovery.lookup === "function").toBeTruthy();
        expect(capabilityDiscovery.lookupByParticipantId).toBeDefined();
        expect(typeof capabilityDiscovery.lookupByParticipantId === "function").toBeTruthy();
    });

    it("calls local capabilities directory according to discoveryQos.discoveryScope LOCAL_THEN_GLOBAL when local store provides non-empty result", () => {
        localCapStoreSpy = getSpiedLookupObjWithReturnValue(discoveryEntries);
        globalCapCacheSpy = getSpiedLookupObjWithReturnValue([]);
        capabilityDiscovery = new CapabilityDiscovery(localCapStoreSpy, globalCapCacheSpy, "io.joynr", knownGbids);
        capabilityDiscovery.setDependencies(messageRouterSpy, proxyBuilderSpy);
        capabilityDiscovery.globalAddressReady(address);

        capabilityDiscovery.lookup([domain], interfaceName, discoveryQos, gbids);
        expect(localCapStoreSpy.lookup).toHaveBeenCalledWith({
            domains: [domain],
            interfaceName
        });
        expect(globalCapDirSpy.lookup).not.toHaveBeenCalled();
        expect(globalCapCacheSpy.lookup).not.toHaveBeenCalled();
    });

    it("calls local and global capabilities directory according to discoveryQos.discoveryScope LOCAL_THEN_GLOBAL when local store and global cache provide empty result", async () => {
        discoveryQos.discoveryScope = DiscoveryScope.LOCAL_THEN_GLOBAL;
        await capabilityDiscovery.lookup([domain], interfaceName, discoveryQos, gbids);
        await multipleSetImmediate();
        expect(localCapStoreSpy.lookup).toHaveBeenCalledWith({
            domains: [domain],
            interfaceName
        });
        expect(globalCapCacheSpy.lookup).toHaveBeenCalledWith({
            domains: [domain],
            interfaceName,
            cacheMaxAge: discoveryQos.cacheMaxAge
        });
        expect(globalCapDirSpy.lookup).toHaveBeenCalledWith({
            domains: [domain],
            interfaceName,
            gbids
        });
    });

    it("calls local and not global cache and not global capabilities directory according to discoveryQos.discoveryScope LOCAL_THEN_GLOBAL when local store provides non-empty result", () => {
        localCapStoreSpy.lookup.mockReturnValue([getDiscoveryEntry(domain, interfaceName)]);
        capabilityDiscovery.lookup([domain], interfaceName, discoveryQos, gbids);
        expect(localCapStoreSpy.lookup).toHaveBeenCalledWith({
            domains: [domain],
            interfaceName
        });
        expect(globalCapCacheSpy.lookup).not.toHaveBeenCalled();
        expect(globalCapDirSpy.lookup).not.toHaveBeenCalled();
    });

    it("calls local capabilities directory according to discoveryQos.discoveryScope LOCAL_ONLY", () => {
        discoveryQos.discoveryScope = DiscoveryScope.LOCAL_ONLY;
        capabilityDiscovery.lookup([domain], interfaceName, discoveryQos, gbids);
        expect(localCapStoreSpy.lookup).toHaveBeenCalledWith({
            domains: [domain],
            interfaceName
        });
        expect(globalCapCacheSpy.lookup).not.toHaveBeenCalled();
        expect(globalCapDirSpy.lookup).not.toHaveBeenCalled();
    });

    it("calls global capabilities directory according to discoveryQos.discoveryScope GLOBAL_ONLY", async () => {
        discoveryQos.discoveryScope = DiscoveryScope.GLOBAL_ONLY;
        capabilityDiscovery.lookup([domain], interfaceName, discoveryQos, gbids);
        await testUtil.multipleSetImmediate();

        expect(localCapStoreSpy.lookup).not.toHaveBeenCalled();
        expect(globalCapCacheSpy.lookup).toHaveBeenCalledWith({
            domains: [domain],
            interfaceName,
            cacheMaxAge: discoveryQos.cacheMaxAge
        });
        expect(globalCapDirSpy.lookup).toHaveBeenCalledWith({
            domains: [domain],
            interfaceName,
            gbids
        });
    });

    it("does not call global capabilities directory according to discoveryQos.discoveryScope GLOBAL_ONLY, if global cache is non-empty", () => {
        globalCapCacheSpy.lookup.mockReturnValue([getDiscoveryEntry(domain, interfaceName)]);
        discoveryQos.discoveryScope = DiscoveryScope.GLOBAL_ONLY;
        capabilityDiscovery.lookup([domain], interfaceName, discoveryQos, gbids);
        expect(localCapStoreSpy.lookup).not.toHaveBeenCalled();
        expect(globalCapCacheSpy.lookup).toHaveBeenCalledWith({
            domains: [domain],
            interfaceName,
            cacheMaxAge: discoveryQos.cacheMaxAge
        });
        expect(globalCapDirSpy.lookup).not.toHaveBeenCalled();
    });

    function testDiscoveryResult(
        _descriptor: string,
        discoveryScope: DiscoveryScope,
        localdiscoveryEntries: any,
        globalCapCacheEntries: any,
        globalCapabilityInfos: any,
        expectedReturnValue: any
    ) {
        const localCapStoreSpy = getSpiedLookupObjWithReturnValue(localdiscoveryEntries);
        const globalCapCacheSpy = getSpiedLookupObjWithReturnValue(globalCapCacheEntries);
        const globalCapDirSpy = getSpiedLookupObjWithReturnValue(
            Promise.resolve({
                result: globalCapabilityInfos
            })
        );

        const proxyBuilderSpy = {
            build: jest.fn()
        };
        proxyBuilderSpy.build.mockReturnValue(Promise.resolve(globalCapDirSpy));
        const capabilityDiscovery = new CapabilityDiscovery(
            localCapStoreSpy as any,
            globalCapCacheSpy as any,
            "io.joynr",
            knownGbids
        );
        capabilityDiscovery.setDependencies(messageRouterSpy, proxyBuilderSpy as any);
        capabilityDiscovery.globalAddressReady(address);
        const discoveryQos = new DiscoveryQos({
            cacheMaxAge: 0,
            discoveryScope
        } as any);

        return capabilityDiscovery
            .lookup([domain], interfaceName, discoveryQos, gbids)
            .then(fulfilledWith => {
                const endDateMs = Date.now();
                if (expectedReturnValue === undefined) {
                    throw new Error("no return value was expected");
                } else {
                    expect(fulfilledWith.length).toEqual(expectedReturnValue.length);
                    for (let i = 0; i < fulfilledWith.length; i++) {
                        assertDiscoveryEntryEquals(expectedReturnValue[i], fulfilledWith[i]);
                        expect(fulfilledWith[i].lastSeenDateMs >= startDateMs).toBeTruthy();
                        expect(fulfilledWith[i].lastSeenDateMs <= endDateMs).toBeTruthy();
                    }
                }
            })
            .catch(() => {
                if (expectedReturnValue !== undefined) {
                    // eslint-disable-next-line prefer-promise-reject-errors
                    throw new Error(`a return value was expected: ${expectedReturnValue}`);
                }
            });
    }

    it("discovers correct discoveryEntries according to discoveryScope", async () => {
        await testDiscoveryResult("00", DiscoveryScope.LOCAL_THEN_GLOBAL, [], [], [], []);
        await testDiscoveryResult(
            "01",
            DiscoveryScope.LOCAL_THEN_GLOBAL,
            [discoveryEntries[1]],
            [],
            [],
            [discoveryEntries[1]]
        );
        await testDiscoveryResult(
            "02",
            DiscoveryScope.LOCAL_THEN_GLOBAL,
            [],
            [globalDiscoveryEntries[1]],
            [],
            [globalDiscoveryEntriesReturned[1]]
        );

        await testDiscoveryResult(
            "03",
            DiscoveryScope.LOCAL_THEN_GLOBAL,
            [],
            [],
            [globalDiscoveryEntries[2]],
            [globalDiscoveryEntriesReturned[2]]
        );
        await testDiscoveryResult(
            "04",
            DiscoveryScope.LOCAL_THEN_GLOBAL,
            [discoveryEntries[3]],
            [],
            [globalDiscoveryEntries[3]],
            [discoveryEntriesReturned[3]]
        );
        await testDiscoveryResult(
            "05",
            DiscoveryScope.LOCAL_THEN_GLOBAL,
            [discoveryEntries[3]],
            [globalDiscoveryEntries[1]],
            [globalDiscoveryEntries[3]],
            [discoveryEntriesReturned[3]]
        );
        await testDiscoveryResult("06", DiscoveryScope.LOCAL_ONLY, [], [], [], []);
        await testDiscoveryResult(
            "07",
            DiscoveryScope.LOCAL_ONLY,
            [discoveryEntries[5]],
            [],
            [],
            [discoveryEntriesReturned[5]]
        );
        await testDiscoveryResult("08", DiscoveryScope.LOCAL_ONLY, [], [], [globalDiscoveryEntries[6]], []);
        await testDiscoveryResult("09", DiscoveryScope.LOCAL_ONLY, [], [globalDiscoveryEntries[5]], [], []);
        await testDiscoveryResult(
            "10",
            DiscoveryScope.LOCAL_ONLY,
            [],
            [globalDiscoveryEntries[5]],
            [globalDiscoveryEntries[6]],
            []
        );
        await testDiscoveryResult(
            "11",
            DiscoveryScope.LOCAL_ONLY,
            [discoveryEntries[7]],
            [],
            [globalDiscoveryEntries[7]],
            [discoveryEntriesReturned[7]]
        );
        await testDiscoveryResult(
            "12",
            DiscoveryScope.LOCAL_ONLY,
            [discoveryEntries[7]],
            [globalDiscoveryEntries[1]],
            [globalDiscoveryEntries[7]],
            [discoveryEntriesReturned[7]]
        );
        await testDiscoveryResult("13", DiscoveryScope.GLOBAL_ONLY, [], [], [], []);
        await testDiscoveryResult(
            "14",
            DiscoveryScope.GLOBAL_ONLY,
            [],
            [globalDiscoveryEntries[9]],
            [],
            [globalDiscoveryEntriesReturned[9]]
        );
        await testDiscoveryResult("15", DiscoveryScope.GLOBAL_ONLY, [discoveryEntries[9]], [], [], []);
        await testDiscoveryResult(
            "16",
            DiscoveryScope.GLOBAL_ONLY,
            [],
            [globalDiscoveryEntries[10]],
            [globalDiscoveryEntries[10]],
            [globalDiscoveryEntriesReturned[10]]
        );
        await testDiscoveryResult(
            "17",
            DiscoveryScope.GLOBAL_ONLY,
            [],
            [],
            [globalDiscoveryEntries[10]],
            [globalDiscoveryEntriesReturned[10]]
        );
        await testDiscoveryResult(
            "18",
            DiscoveryScope.GLOBAL_ONLY,
            [],
            [globalDiscoveryEntries[11]],
            [globalDiscoveryEntries[11]],
            [globalDiscoveryEntriesReturned[11]]
        );
        await testDiscoveryResult(
            "19",
            DiscoveryScope.GLOBAL_ONLY,
            [discoveryEntries[10]],
            [globalDiscoveryEntries[11]],
            [globalDiscoveryEntries[11]],
            [globalDiscoveryEntriesReturned[11]]
        );
        await testDiscoveryResult("20", DiscoveryScope.LOCAL_AND_GLOBAL, [], [], [], []);
        await testDiscoveryResult(
            "21",
            DiscoveryScope.LOCAL_AND_GLOBAL,
            [discoveryEntries[1]],
            [],
            [],
            [discoveryEntriesReturned[1]]
        );
        await testDiscoveryResult(
            "22",
            DiscoveryScope.LOCAL_AND_GLOBAL,
            [],
            [globalDiscoveryEntries[1]],
            [],
            [globalDiscoveryEntriesReturned[1]]
        );
        await testDiscoveryResult(
            "23",
            DiscoveryScope.LOCAL_AND_GLOBAL,
            [],
            [],
            [globalDiscoveryEntries[2]],
            [globalDiscoveryEntriesReturned[2]]
        );
        await testDiscoveryResult(
            "24",
            DiscoveryScope.LOCAL_AND_GLOBAL,
            [discoveryEntries[3]],
            [globalDiscoveryEntries[4]],
            [],
            [discoveryEntriesReturned[3], globalDiscoveryEntriesReturned[4]]
        );
        await testDiscoveryResult(
            "25",
            DiscoveryScope.LOCAL_AND_GLOBAL,
            [discoveryEntries[3]],
            [],
            [globalDiscoveryEntries[4]],
            [discoveryEntriesReturned[3], globalDiscoveryEntriesReturned[4]]
        );
        await testDiscoveryResult(
            "26",
            DiscoveryScope.LOCAL_AND_GLOBAL,
            [discoveryEntries[3]],
            [globalDiscoveryEntries[1]],
            [globalDiscoveryEntries[3]],
            [discoveryEntriesReturned[3], globalDiscoveryEntriesReturned[1]]
        );
    });

    function getDiscoveryEntryWithScope(scope: any) {
        return new DiscoveryEntry({
            providerVersion: new Version({ majorVersion: 47, minorVersion: 11 }),
            domain: "domain",
            interfaceName: "interfaceName",
            qos: new ProviderQos({
                customParameters: [
                    new CustomParameter({
                        name: "theName",
                        value: "theValue"
                    })
                ],
                priority: 1234,
                scope,
                supportsOnChangeSubscriptions: true
            }),
            participantId: "700",
            lastSeenDateMs: 123,
            publicKeyId: "",
            expiryDateMs
        });
    }

    it("add calls cap dir correctly with ProviderScope.LOCAL", async () => {
        const discoveryEntry = getDiscoveryEntryWithScope(ProviderScope.LOCAL);

        await capabilityDiscovery.add(discoveryEntry);

        expect(localCapStoreSpy.add).toHaveBeenCalled();
        expect(localCapStoreSpy.add).toHaveBeenCalledWith({
            discoveryEntry,
            remote: false
        });
        expect(globalCapDirSpy.add).not.toHaveBeenCalledWith(undefined);
    });

    it("add calls cap dir correctly with ProviderScope.GLOBAL", async () => {
        const discoveryEntry = getDiscoveryEntryWithScope(ProviderScope.GLOBAL);
        const expectedDiscoveryEntry = CapabilitiesUtil.discoveryEntry2GlobalDiscoveryEntry(discoveryEntry, address);

        await capabilityDiscovery.add(discoveryEntry);

        expect(globalCapDirSpy.add).toHaveBeenCalled();
        const actualDiscoveryEntry = globalCapDirSpy.add.mock.calls[0][0].globalDiscoveryEntry;
        // lastSeenDate is set to Date.now() in CapabilityDiscovery.add
        expectedDiscoveryEntry.lastSeenDateMs = actualDiscoveryEntry.lastSeenDateMs;
        assertDiscoveryEntryEquals(expectedDiscoveryEntry, actualDiscoveryEntry);

        expect(localCapStoreSpy.add).not.toHaveBeenCalledWith();
    });

    it("touch calls global cap dir correctly", async () => {
        const expectedTtl = 1337;
        const expectedClusterControllerId = "testTouchClusterControllerId";

        await capabilityDiscovery.touch(expectedClusterControllerId, expectedTtl);

        expect(globalCapDirSpy.touch).toHaveBeenCalled();
        const actualTtl = proxyBuilderSpy.build.mock.calls[0][1].messagingQos.ttl;
        const actualClusterControllerId = globalCapDirSpy.touch.mock.calls[0][0].clusterControllerId;
        expect(actualTtl).toEqual(expectedTtl);
        expect(actualClusterControllerId).toEqual(expectedClusterControllerId);
    });

    it("reports error from global cap dir", async () => {
        const discoveryEntry = getDiscoveryEntryWithScope(ProviderScope.GLOBAL);
        const expectedDiscoveryEntry = CapabilitiesUtil.discoveryEntry2GlobalDiscoveryEntry(discoveryEntry, address);
        globalCapDirSpy = getSpiedLookupObjWithReturnValue(Promise.reject(new Error("Some error.")));

        proxyBuilderSpy.build.mockReturnValue(Promise.resolve(globalCapDirSpy));
        const error = await reversePromise(capabilityDiscovery.add(discoveryEntry));
        expect(globalCapDirSpy.add).toHaveBeenCalled();
        const actualDiscoveryEntry = globalCapDirSpy.add.mock.calls[0][0].globalDiscoveryEntry;
        // lastSeenDate is set to Date.now() in CapabilityDiscovery.add
        expectedDiscoveryEntry.lastSeenDateMs = actualDiscoveryEntry.lastSeenDateMs;
        assertDiscoveryEntryEquals(expectedDiscoveryEntry, actualDiscoveryEntry);
        expect(localCapStoreSpy.add).not.toHaveBeenCalledWith();
        expect(Object.prototype.toString.call(error === "[object Error]")).toBeTruthy();
    });

    it("throws on unknown provider scope", async () => {
        const discoveryEntry = getDiscoveryEntryWithScope("UnknownScope");
        const error = await reversePromise(capabilityDiscovery.add(discoveryEntry));
        expect(globalCapDirSpy.add).not.toHaveBeenCalled();
        expect(localCapStoreSpy.add).not.toHaveBeenCalledWith();
        expect(Object.prototype.toString.call(error) === "[object Error]").toBeTruthy();
    });

    it("lookup with multiple domains should throw an exception", async () => {
        localCapStoreSpy = getSpiedLookupObjWithReturnValue(discoveryEntries);
        globalCapCacheSpy = getSpiedLookupObjWithReturnValue([]);
        capabilityDiscovery = new CapabilityDiscovery(localCapStoreSpy, globalCapCacheSpy, "io.joynr", knownGbids);
        capabilityDiscovery.setDependencies(messageRouterSpy, proxyBuilderSpy);
        capabilityDiscovery.globalAddressReady(address);
        await expect(
            capabilityDiscovery.lookup([domain, domain], interfaceName, discoveryQos, gbids)
        ).rejects.toBeInstanceOf(Error);
    });
});
