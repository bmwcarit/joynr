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
require("../../../node-unit-test-helper");
const CapabilityDiscovery = require("../../../../../main/js/joynr/capabilities/discovery/CapabilityDiscovery");
const DiscoveryQos = require("../../../../../main/js/generated/joynr/types/DiscoveryQos");
const ProviderQos = require("../../../../../main/js/generated/joynr/types/ProviderQos");
const CustomParameter = require("../../../../../main/js/generated/joynr/types/CustomParameter");
const ProviderScope = require("../../../../../main/js/generated/joynr/types/ProviderScope");
const DiscoveryScope = require("../../../../../main/js/generated/joynr/types/DiscoveryScope");
const DiscoveryEntry = require("../../../../../main/js/generated/joynr/types/DiscoveryEntry");
const GlobalDiscoveryEntry = require("../../../../../main/js/generated/joynr/types/GlobalDiscoveryEntry");
const ChannelAddress = require("../../../../../main/js/generated/joynr/system/RoutingTypes/ChannelAddress");
const Version = require("../../../../../main/js/generated/joynr/types/Version");
const Promise = require("../../../../../main/js/global/Promise");
const waitsFor = require("../../../../../test/js/global/WaitsFor");
const CapabilitiesUtil = require("../../../../../main/js/joynr/util/CapabilitiesUtil");

let domain, interfaceName, discoveryQos;
let discoveryEntries, discoveryEntriesReturned, globalDiscoveryEntries, globalDiscoveryEntriesReturned;
let capabilityDiscovery, messageRouterSpy, proxyBuilderSpy, address, localCapStoreSpy;
let globalCapCacheSpy, globalCapDirSpy;
let startDateMs;

messageRouterSpy = jasmine.createSpyObj("routingTable", ["addNextHop", "resolveNextHop"]);

messageRouterSpy.addNextHop.and.returnValue(Promise.resolve());
messageRouterSpy.resolveNextHop.and.returnValue(Promise.resolve());

function getSpiedLookupObjWithReturnValue(name, returnValue) {
    const spyObj = jasmine.createSpyObj(name, ["lookup", "add", "remove", "touch"]);
    spyObj.lookup.and.returnValue(returnValue);
    spyObj.add.and.returnValue(returnValue);
    spyObj.remove.and.returnValue(spyObj);
    return spyObj;
}

function getGlobalDiscoveryEntry(domain, interfaceName, newGlobalAddress) {
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
        publicKeyId: ""
    });
}

function assertDiscoveryEntryEquals(expected, actual) {
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

function getDiscoveryEntry(domain, interfaceName) {
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
        publicKeyId: ""
    });
}

describe("libjoynr-js.joynr.capabilities.discovery.CapabilityDiscovery", () => {
    beforeEach(done => {
        let i, discoveryEntry;
        startDateMs = Date.now();
        domain = "myDomain";
        interfaceName = "myInterfaceName";
        address = new ChannelAddress({
            channelId: domain + "TestCapabilityDiscoveryChannel",
            messagingEndpointUrl: "http://testUrl"
        });
        discoveryQos = new DiscoveryQos({
            cacheMaxAge: 0,
            discoveryScope: DiscoveryScope.LOCAL_THEN_GLOBAL
        });

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
                    channelId: "globalCapInfo" + i.toString(),
                    messagingEndpointUrl: "http://testurl"
                })
            );
            globalDiscoveryEntries.push(discoveryEntry);
            globalDiscoveryEntriesReturned.push(
                CapabilitiesUtil.convertToDiscoveryEntryWithMetaInfo(false, discoveryEntry)
            );
        }

        localCapStoreSpy = getSpiedLookupObjWithReturnValue("localCapStoreSpy", []);
        globalCapCacheSpy = getSpiedLookupObjWithReturnValue("globalCapCacheSpy", []);
        globalCapDirSpy = getSpiedLookupObjWithReturnValue("globalCapDirSpy", Promise.resolve({ result: [] }));

        proxyBuilderSpy = jasmine.createSpyObj("proxyBuilderSpy", ["build"]);
        proxyBuilderSpy.build.and.returnValue(Promise.resolve(globalCapDirSpy));
        capabilityDiscovery = new CapabilityDiscovery(
            localCapStoreSpy,
            globalCapCacheSpy,
            messageRouterSpy,
            proxyBuilderSpy,
            "io.joynr"
        );
        capabilityDiscovery.globalAddressReady(address);
        done();
    });

    it("is instantiable, of correct type and has all members", done => {
        expect(capabilityDiscovery).toBeDefined();
        expect(capabilityDiscovery instanceof CapabilityDiscovery).toBeTruthy();
        expect(capabilityDiscovery.lookup).toBeDefined();
        expect(typeof capabilityDiscovery.lookup === "function").toBeTruthy();
        done();
    });

    it("throws when constructor arguments are missing", done => {
        expect(() => new CapabilityDiscovery()).toThrow();
        expect(() => new CapabilityDiscovery(localCapStoreSpy)).toThrow();
        expect(() => new CapabilityDiscovery(messageRouterSpy)).toThrow();
        expect(() => new CapabilityDiscovery(localCapStoreSpy, globalCapCacheSpy, proxyBuilderSpy)).toThrow();
        expect(
            () => new CapabilityDiscovery(localCapStoreSpy, globalCapCacheSpy, messageRouterSpy, proxyBuilderSpy)
        ).toThrow();
        expect(() => new CapabilityDiscovery(localCapStoreSpy, globalCapCacheSpy, messageRouterSpy)).toThrow();
        expect(
            () =>
                new CapabilityDiscovery(
                    localCapStoreSpy,
                    globalCapCacheSpy,
                    messageRouterSpy,
                    proxyBuilderSpy,
                    "domain"
                )
        ).not.toThrow();

        done();
    });

    it("calls local capabilities directory according to discoveryQos.discoveryScope LOCAL_THEN_GLOBAL when local cache provides non-empty result", done => {
        localCapStoreSpy = getSpiedLookupObjWithReturnValue("localCapStoreSpy", discoveryEntries);
        globalCapCacheSpy = getSpiedLookupObjWithReturnValue("globalCapCacheSpy", []);
        capabilityDiscovery = new CapabilityDiscovery(
            localCapStoreSpy,
            globalCapCacheSpy,
            messageRouterSpy,
            proxyBuilderSpy,
            "io.joynr"
        );
        capabilityDiscovery.globalAddressReady(address);

        capabilityDiscovery.lookup([domain], interfaceName, discoveryQos);
        expect(localCapStoreSpy.lookup).toHaveBeenCalledWith({
            domains: [domain],
            interfaceName
        });
        expect(globalCapDirSpy.lookup).not.toHaveBeenCalled();
        expect(globalCapCacheSpy.lookup).not.toHaveBeenCalled();
        done();
    });

    it("calls local and global capabilities directory according to discoveryQos.discoveryScope LOCAL_THEN_GLOBAL when local cache provides empty result", done => {
        discoveryQos.discoveryScope = DiscoveryScope.LOCAL_THEN_GLOBAL;
        capabilityDiscovery.lookup([domain], interfaceName, discoveryQos);
        setTimeout(() => {
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
                interfaceName
            });

            done();
        }, 10);
    });

    it("calls local and not global cache and not global capabilities directory according to discoveryQos.discoveryScope LOCAL_THEN_GLOBAL when local store provides non-empty result", done => {
        localCapStoreSpy.lookup.and.returnValue([getDiscoveryEntry(domain, interfaceName)]);
        capabilityDiscovery.lookup([domain], interfaceName, discoveryQos);
        expect(localCapStoreSpy.lookup).toHaveBeenCalledWith({
            domains: [domain],
            interfaceName
        });
        expect(globalCapCacheSpy.lookup).not.toHaveBeenCalled();
        expect(globalCapDirSpy.lookup).not.toHaveBeenCalled();
        done();
    });

    it("calls local and global cache and global capabilities directory according to discoveryQos.discoveryScope LOCAL_THEN_GLOBAL when local store and global cache provides non-empty result", done => {
        discoveryQos.discoveryScope = DiscoveryScope.LOCAL_THEN_GLOBAL;
        capabilityDiscovery.lookup([domain], interfaceName, discoveryQos);
        waitsFor(
            () => {
                return (
                    localCapStoreSpy.lookup.calls.count() >= 1 &&
                    globalCapCacheSpy.lookup.calls.count() >= 1 &&
                    globalCapDirSpy.lookup.calls.count() >= 1
                );
            },
            "wait for lookups to be done",
            1000
        )
            .then(() => {
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
                    interfaceName
                });
                done();
                return null;
            })
            .catch(done.fail);
    });

    it("calls local capabilities directory according to discoveryQos.discoveryScope LOCAL_ONLY", done => {
        discoveryQos.discoveryScope = DiscoveryScope.LOCAL_ONLY;
        capabilityDiscovery.lookup([domain], interfaceName, discoveryQos);
        expect(localCapStoreSpy.lookup).toHaveBeenCalledWith({
            domains: [domain],
            interfaceName
        });
        expect(globalCapCacheSpy.lookup).not.toHaveBeenCalled();
        expect(globalCapDirSpy.lookup).not.toHaveBeenCalled();
        done();
    });

    it("calls global capabilities directory according to discoveryQos.discoveryScope GLOBAL_ONLY", done => {
        discoveryQos.discoveryScope = DiscoveryScope.GLOBAL_ONLY;
        capabilityDiscovery.lookup([domain], interfaceName, discoveryQos);

        waitsFor(
            () => {
                return globalCapCacheSpy.lookup.calls.count() >= 1 && globalCapDirSpy.lookup.calls.count() >= 1;
            },
            "waiting for globalCapCacheSpy to get called",
            1000
        )
            .then(() => {
                expect(localCapStoreSpy.lookup).not.toHaveBeenCalled();
                expect(globalCapCacheSpy.lookup).toHaveBeenCalledWith({
                    domains: [domain],
                    interfaceName,
                    cacheMaxAge: discoveryQos.cacheMaxAge
                });
                expect(globalCapDirSpy.lookup).toHaveBeenCalledWith({
                    domains: [domain],
                    interfaceName
                });
                done();
                return null;
            })
            .catch(done.fail);
    });

    it("does not call global capabilities directory according to discoveryQos.discoveryScope GLOBAL_ONLY, if global cache is non-empty", done => {
        globalCapCacheSpy.lookup.and.returnValue([getDiscoveryEntry(domain, interfaceName)]);
        discoveryQos.discoveryScope = DiscoveryScope.GLOBAL_ONLY;
        capabilityDiscovery.lookup([domain], interfaceName, discoveryQos);
        expect(localCapStoreSpy.lookup).not.toHaveBeenCalled();
        expect(globalCapCacheSpy.lookup).toHaveBeenCalledWith({
            domains: [domain],
            interfaceName,
            cacheMaxAge: discoveryQos.cacheMaxAge
        });
        expect(globalCapDirSpy.lookup).not.toHaveBeenCalled();
        done();
    });

    function testDiscoveryResult(
        descriptor,
        discoveryScope,
        localdiscoveryEntries,
        globalCapCacheEntries,
        globalCapabilityInfos,
        expectedReturnValue
    ) {
        const localCapStoreSpy = getSpiedLookupObjWithReturnValue(
            "localCapStoreSpy" + descriptor,
            localdiscoveryEntries
        );
        const globalCapCacheSpy = getSpiedLookupObjWithReturnValue(
            "globalCapCacheSpy" + descriptor,
            globalCapCacheEntries
        );
        const globalCapDirSpy = getSpiedLookupObjWithReturnValue(
            "globalCapDirSpy" + descriptor,
            Promise.resolve({
                result: globalCapabilityInfos
            })
        );

        const proxyBuilderSpy = jasmine.createSpyObj("proxyBuilderSpy", ["build"]);
        proxyBuilderSpy.build.and.returnValue(Promise.resolve(globalCapDirSpy));
        const capabilityDiscovery = new CapabilityDiscovery(
            localCapStoreSpy,
            globalCapCacheSpy,
            messageRouterSpy,
            proxyBuilderSpy,
            "io.joynr"
        );
        capabilityDiscovery.globalAddressReady(address);
        const discoveryQos = new DiscoveryQos({
            cacheMaxAge: 0,
            discoveryScope
        });

        return capabilityDiscovery
            .lookup([domain], interfaceName, discoveryQos)
            .then(fulfilledWith => {
                let i;
                const endDateMs = Date.now();
                if (expectedReturnValue === undefined) {
                    fail("no return value was expected");
                } else {
                    expect(fulfilledWith.length).toEqual(expectedReturnValue.length);
                    for (i = 0; i < fulfilledWith.length; i++) {
                        assertDiscoveryEntryEquals(expectedReturnValue[i], fulfilledWith[i]);
                        expect(fulfilledWith[i].lastSeenDateMs >= startDateMs).toBeTruthy();
                        expect(fulfilledWith[i].lastSeenDateMs <= endDateMs).toBeTruthy();
                    }
                }
            })
            .catch(() => {
                if (expectedReturnValue !== undefined) {
                    fail("a return value was expected: " + expectedReturnValue);
                }
            });
    }

    it("discovers correct discoveryEntries according to discoveryScope", done => {
        const promises = [];
        promises.push(testDiscoveryResult("00", DiscoveryScope.LOCAL_THEN_GLOBAL, [], [], [], []));
        promises.push(
            testDiscoveryResult(
                "01",
                DiscoveryScope.LOCAL_THEN_GLOBAL,
                [discoveryEntries[1]],
                [],
                [],
                [discoveryEntries[1]]
            )
        );
        promises.push(
            testDiscoveryResult(
                "02",
                DiscoveryScope.LOCAL_THEN_GLOBAL,
                [],
                [globalDiscoveryEntries[1]],
                [],
                [globalDiscoveryEntriesReturned[1]]
            )
        );
        promises.push(
            testDiscoveryResult(
                "03",
                DiscoveryScope.LOCAL_THEN_GLOBAL,
                [],
                [],
                [globalDiscoveryEntries[2]],
                [globalDiscoveryEntriesReturned[2]]
            )
        );
        promises.push(
            testDiscoveryResult(
                "04",
                DiscoveryScope.LOCAL_THEN_GLOBAL,
                [discoveryEntries[3]],
                [],
                [globalDiscoveryEntries[3]],
                [discoveryEntriesReturned[3]]
            )
        );
        promises.push(
            testDiscoveryResult(
                "05",
                DiscoveryScope.LOCAL_THEN_GLOBAL,
                [discoveryEntries[3]],
                [globalDiscoveryEntries[1]],
                [globalDiscoveryEntries[3]],
                [discoveryEntriesReturned[3]]
            )
        );
        promises.push(testDiscoveryResult("06", DiscoveryScope.LOCAL_ONLY, [], [], [], []));
        promises.push(
            testDiscoveryResult(
                "07",
                DiscoveryScope.LOCAL_ONLY,
                [discoveryEntries[5]],
                [],
                [],
                [discoveryEntriesReturned[5]]
            )
        );
        promises.push(testDiscoveryResult("08", DiscoveryScope.LOCAL_ONLY, [], [], [globalDiscoveryEntries[6]], []));
        promises.push(testDiscoveryResult("09", DiscoveryScope.LOCAL_ONLY, [], [globalDiscoveryEntries[5]], [], []));
        promises.push(
            testDiscoveryResult(
                "10",
                DiscoveryScope.LOCAL_ONLY,
                [],
                [globalDiscoveryEntries[5]],
                [globalDiscoveryEntries[6]],
                []
            )
        );
        promises.push(
            testDiscoveryResult(
                "11",
                DiscoveryScope.LOCAL_ONLY,
                [discoveryEntries[7]],
                [],
                [globalDiscoveryEntries[7]],
                [discoveryEntriesReturned[7]]
            )
        );
        promises.push(
            testDiscoveryResult(
                "12",
                DiscoveryScope.LOCAL_ONLY,
                [discoveryEntries[7]],
                [globalDiscoveryEntries[1]],
                [globalDiscoveryEntries[7]],
                [discoveryEntriesReturned[7]]
            )
        );
        promises.push(testDiscoveryResult("13", DiscoveryScope.GLOBAL_ONLY, [], [], [], []));
        promises.push(
            testDiscoveryResult(
                "14",
                DiscoveryScope.GLOBAL_ONLY,
                [],
                [globalDiscoveryEntries[9]],
                [],
                [globalDiscoveryEntriesReturned[9]]
            )
        );
        promises.push(testDiscoveryResult("15", DiscoveryScope.GLOBAL_ONLY, [discoveryEntries[9]], [], [], []));
        promises.push(
            testDiscoveryResult(
                "16",
                DiscoveryScope.GLOBAL_ONLY,
                [],
                [globalDiscoveryEntries[10]],
                [globalDiscoveryEntries[10]],
                [globalDiscoveryEntriesReturned[10]]
            )
        );
        promises.push(
            testDiscoveryResult(
                "17",
                DiscoveryScope.GLOBAL_ONLY,
                [],
                [],
                [globalDiscoveryEntries[10]],
                [globalDiscoveryEntriesReturned[10]]
            )
        );
        promises.push(
            testDiscoveryResult(
                "18",
                DiscoveryScope.GLOBAL_ONLY,
                [],
                [globalDiscoveryEntries[11]],
                [globalDiscoveryEntries[11]],
                [globalDiscoveryEntriesReturned[11]]
            )
        );
        promises.push(
            testDiscoveryResult(
                "19",
                DiscoveryScope.GLOBAL_ONLY,
                [discoveryEntries[10]],
                [globalDiscoveryEntries[11]],
                [globalDiscoveryEntries[11]],
                [globalDiscoveryEntriesReturned[11]]
            )
        );
        promises.push(testDiscoveryResult("20", DiscoveryScope.LOCAL_AND_GLOBAL, [], [], [], []));
        promises.push(
            testDiscoveryResult(
                "21",
                DiscoveryScope.LOCAL_AND_GLOBAL,
                [discoveryEntries[1]],
                [],
                [],
                [discoveryEntriesReturned[1]]
            )
        );
        promises.push(
            testDiscoveryResult(
                "22",
                DiscoveryScope.LOCAL_AND_GLOBAL,
                [],
                [globalDiscoveryEntries[1]],
                [],
                [globalDiscoveryEntriesReturned[1]]
            )
        );
        promises.push(
            testDiscoveryResult(
                "23",
                DiscoveryScope.LOCAL_AND_GLOBAL,
                [],
                [],
                [globalDiscoveryEntries[2]],
                [globalDiscoveryEntriesReturned[2]]
            )
        );
        promises.push(
            testDiscoveryResult(
                "24",
                DiscoveryScope.LOCAL_AND_GLOBAL,
                [discoveryEntries[3]],
                [globalDiscoveryEntries[4]],
                [],
                [discoveryEntriesReturned[3], globalDiscoveryEntriesReturned[4]]
            )
        );
        promises.push(
            testDiscoveryResult(
                "25",
                DiscoveryScope.LOCAL_AND_GLOBAL,
                [discoveryEntries[3]],
                [],
                [globalDiscoveryEntries[4]],
                [discoveryEntriesReturned[3], globalDiscoveryEntriesReturned[4]]
            )
        );
        promises.push(
            testDiscoveryResult(
                "26",
                DiscoveryScope.LOCAL_AND_GLOBAL,
                [discoveryEntries[3]],
                [globalDiscoveryEntries[1]],
                [globalDiscoveryEntries[3]],
                [discoveryEntriesReturned[3], globalDiscoveryEntriesReturned[1]]
            )
        );

        Promise.all(promises)
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    function getDiscoveryEntryWithScope(scope) {
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
            publicKeyId: ""
        });
    }

    it("calls local cap dir correctly", done => {
        const discoveryEntry = getDiscoveryEntryWithScope(ProviderScope.LOCAL);
        capabilityDiscovery
            .add(discoveryEntry)
            .then(() => {
                expect(localCapStoreSpy.add).toHaveBeenCalled();
                expect(localCapStoreSpy.add).toHaveBeenCalledWith({
                    discoveryEntry,
                    remote: false
                });
                expect(globalCapDirSpy.add).not.toHaveBeenCalledWith(undefined);
                done();
                return null;
            })
            .catch(fail);
    });

    it("calls global cap dir correctly", done => {
        let actualDiscoveryEntry;
        const discoveryEntry = getDiscoveryEntryWithScope(ProviderScope.GLOBAL);
        const expectedDiscoveryEntry = CapabilitiesUtil.discoveryEntry2GlobalDiscoveryEntry(discoveryEntry, address);
        capabilityDiscovery
            .add(discoveryEntry)
            .then(() => {
                expect(globalCapDirSpy.add).toHaveBeenCalled();
                actualDiscoveryEntry = globalCapDirSpy.add.calls.argsFor(0)[0].globalDiscoveryEntry;
                // lastSeenDate is set to Date.now() in CapabilityDiscovery.add
                expectedDiscoveryEntry.lastSeenDateMs = actualDiscoveryEntry.lastSeenDateMs;
                assertDiscoveryEntryEquals(expectedDiscoveryEntry, actualDiscoveryEntry);

                expect(localCapStoreSpy.add).not.toHaveBeenCalledWith();
                done();
                return null;
            })
            .catch(fail);
    });

    it("touch calls global cap dir correctly", done => {
        let actualClusterControllerId;
        let actualTtl;
        const expectedTtl = 1337;
        const expectedClusterControllerId = "testTouchClusterControllerId";
        capabilityDiscovery
            .touch(expectedClusterControllerId, expectedTtl)
            .then(() => {
                expect(globalCapDirSpy.touch).toHaveBeenCalled();
                actualTtl = proxyBuilderSpy.build.calls.argsFor(0)[1].messagingQos.ttl;
                actualClusterControllerId = globalCapDirSpy.touch.calls.argsFor(0)[0].clusterControllerId;
                expect(actualTtl).toEqual(expectedTtl);
                expect(actualClusterControllerId).toEqual(expectedClusterControllerId);
                done();
                return null;
            })
            .catch(fail);
    });

    it("reports error from global cap dir", done => {
        let actualDiscoveryEntry;
        const discoveryEntry = getDiscoveryEntryWithScope(ProviderScope.GLOBAL);
        const expectedDiscoveryEntry = CapabilitiesUtil.discoveryEntry2GlobalDiscoveryEntry(discoveryEntry, address);
        globalCapDirSpy = getSpiedLookupObjWithReturnValue("globalCapDirSpy", Promise.reject(new Error("Some error.")));

        proxyBuilderSpy.build.and.returnValue(Promise.resolve(globalCapDirSpy));
        capabilityDiscovery
            .add(discoveryEntry)
            .then(() => {
                fail("expected an error to have been reported");
                return null;
            })
            .catch(error => {
                expect(globalCapDirSpy.add).toHaveBeenCalled();
                actualDiscoveryEntry = globalCapDirSpy.add.calls.argsFor(0)[0].globalDiscoveryEntry;
                // lastSeenDate is set to Date.now() in CapabilityDiscovery.add
                expectedDiscoveryEntry.lastSeenDateMs = actualDiscoveryEntry.lastSeenDateMs;
                assertDiscoveryEntryEquals(expectedDiscoveryEntry, actualDiscoveryEntry);
                expect(localCapStoreSpy.add).not.toHaveBeenCalledWith();
                expect(Object.prototype.toString.call(error === "[object Error]")).toBeTruthy();
                done();
                return null;
            });
    });

    it("throws on unknown provider scope", done => {
        const discoveryEntry = getDiscoveryEntryWithScope("UnknownScope");
        capabilityDiscovery
            .add(discoveryEntry)
            .then(() => {
                fail("expected an error");
                return null;
            })
            .catch(error => {
                expect(globalCapDirSpy.add).not.toHaveBeenCalled();
                expect(localCapStoreSpy.add).not.toHaveBeenCalledWith();
                expect(Object.prototype.toString.call(error) === "[object Error]").toBeTruthy();
                done();
                return null;
            });
    });

    it("lookup with multiple domains should throw an exception", done => {
        localCapStoreSpy = getSpiedLookupObjWithReturnValue("localCapStoreSpy", discoveryEntries);
        globalCapCacheSpy = getSpiedLookupObjWithReturnValue("globalCapCacheSpy", []);
        capabilityDiscovery = new CapabilityDiscovery(
            localCapStoreSpy,
            globalCapCacheSpy,
            messageRouterSpy,
            proxyBuilderSpy,
            "io.joynr"
        );
        capabilityDiscovery.globalAddressReady(address);
        capabilityDiscovery
            .lookup([domain, domain], interfaceName, discoveryQos)
            .then(fail)
            .catch(done);
    });
});
