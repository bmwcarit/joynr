/*jslint es5: true, node: true, node: true */
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
var CapabilityDiscovery = require("../../../../classes/joynr/capabilities/discovery/CapabilityDiscovery");
var DiscoveryQos = require("../../../../classes/joynr/types/DiscoveryQos");
var ArbitrationStrategyCollection = require("../../../../classes/joynr/types/ArbitrationStrategyCollection");
var ProviderQos = require("../../../../classes/joynr/types/ProviderQos");
var CustomParameter = require("../../../../classes/joynr/types/CustomParameter");
var ProviderScope = require("../../../../classes/joynr/types/ProviderScope");
var DiscoveryScope = require("../../../../classes/joynr/types/DiscoveryScope");
var DiscoveryEntry = require("../../../../classes/joynr/types/DiscoveryEntry");
var GlobalDiscoveryEntry = require("../../../../classes/joynr/types/GlobalDiscoveryEntry");
var ChannelAddress = require("../../../../classes/joynr/system/RoutingTypes/ChannelAddress");
var Version = require("../../../../classes/joynr/types/Version");
var Promise = require("../../../../classes/global/Promise");
var waitsFor = require("../../../../test-classes/global/WaitsFor");
var CapabilitiesUtil = require("../../../../classes/joynr/util/CapabilitiesUtil");

var domain, interfaceName, discoveryQos;
var discoveryEntries, discoveryEntriesReturned, globalDiscoveryEntries, globalDiscoveryEntriesReturned;
var capabilityDiscovery, messageRouterSpy, proxyBuilderSpy, address, localCapStoreSpy;
var globalCapCacheSpy, globalCapDirSpy, capabilityInfo;
var asyncTimeout = 5000;
var startDateMs;

messageRouterSpy = jasmine.createSpyObj("routingTable", ["addNextHop", "resolveNextHop"]);

messageRouterSpy.addNextHop.and.returnValue(Promise.resolve());
messageRouterSpy.resolveNextHop.and.returnValue(Promise.resolve());

function getSpiedLookupObjWithReturnValue(name, returnValue) {
    var spyObj = jasmine.createSpyObj(name, ["lookup", "add", "remove", "touch"]);
    spyObj.lookup.and.returnValue(returnValue);
    spyObj.add.and.returnValue(returnValue);
    spyObj.remove.and.returnValue(spyObj);
    return spyObj;
}

function getGlobalDiscoveryEntry(domain, interfaceName, newGlobalAddress) {
    return new GlobalDiscoveryEntry({
        providerVersion: new Version({ majorVersion: 47, minorVersion: 11 }),
        domain: domain,
        interfaceName: interfaceName,
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
        domain: domain,
        interfaceName: interfaceName,
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

describe("libjoynr-js.joynr.capabilities.discovery.CapabilityDiscovery", function() {
    beforeEach(function(done) {
        var i, discoveryEntry;
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

    it("is instantiable, of correct type and has all members", function(done) {
        expect(capabilityDiscovery).toBeDefined();
        expect(capabilityDiscovery instanceof CapabilityDiscovery).toBeTruthy();
        expect(capabilityDiscovery.lookup).toBeDefined();
        expect(typeof capabilityDiscovery.lookup === "function").toBeTruthy();
        done();
    });

    it("throws when constructor arguments are missing", function(done) {
        expect(function() {
            var capDisc = new CapabilityDiscovery();
        }).toThrow();
        expect(function() {
            var capDisc = new CapabilityDiscovery(localCapStoreSpy);
        }).toThrow();
        expect(function() {
            var capDisc = new CapabilityDiscovery(messageRouterSpy);
        }).toThrow();
        expect(function() {
            var capDisc = new CapabilityDiscovery(localCapStoreSpy, globalCapCacheSpy, proxyBuilderSpy);
        }).toThrow();
        expect(function() {
            var capDisc = new CapabilityDiscovery(
                localCapStoreSpy,
                globalCapCacheSpy,
                messageRouterSpy,
                proxyBuilderSpy
            );
        }).toThrow();
        expect(function() {
            var capDisc = new CapabilityDiscovery(localCapStoreSpy, globalCapCacheSpy, messageRouterSpy);
        }).toThrow();
        expect(function() {
            var capDisc = new CapabilityDiscovery(
                localCapStoreSpy,
                globalCapCacheSpy,
                messageRouterSpy,
                proxyBuilderSpy,
                "domain"
            );
        }).not.toThrow();
        done();
    });

    it("calls local capabilities directory according to discoveryQos.discoveryScope LOCAL_THEN_GLOBAL when local cache provides non-empty result", function(
        done
    ) {
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
            interfaceName: interfaceName
        });
        expect(globalCapDirSpy.lookup).not.toHaveBeenCalled();
        expect(globalCapCacheSpy.lookup).not.toHaveBeenCalled();
        done();
    });

    it("calls local and global capabilities directory according to discoveryQos.discoveryScope LOCAL_THEN_GLOBAL when local cache provides empty result", function(
        done
    ) {
        discoveryQos.discoveryScope = DiscoveryScope.LOCAL_THEN_GLOBAL;
        capabilityDiscovery.lookup([domain], interfaceName, discoveryQos);
        setTimeout(function() {
            expect(localCapStoreSpy.lookup).toHaveBeenCalledWith({
                domains: [domain],
                interfaceName: interfaceName
            });
            expect(globalCapCacheSpy.lookup).toHaveBeenCalledWith({
                domains: [domain],
                interfaceName: interfaceName,
                cacheMaxAge: discoveryQos.cacheMaxAge
            });
            expect(globalCapDirSpy.lookup).toHaveBeenCalledWith({
                domains: [domain],
                interfaceName: interfaceName
            });

            done();
        }, 10);
    });

    it("calls local and not global cache and not global capabilities directory according to discoveryQos.discoveryScope LOCAL_THEN_GLOBAL when local store provides non-empty result", function(
        done
    ) {
        localCapStoreSpy.lookup.and.returnValue([getDiscoveryEntry(domain, interfaceName)]);
        capabilityDiscovery.lookup([domain], interfaceName, discoveryQos);
        expect(localCapStoreSpy.lookup).toHaveBeenCalledWith({
            domains: [domain],
            interfaceName: interfaceName
        });
        expect(globalCapCacheSpy.lookup).not.toHaveBeenCalled();
        expect(globalCapDirSpy.lookup).not.toHaveBeenCalled();
        done();
    });

    it("calls local and global cache and global capabilities directory according to discoveryQos.discoveryScope LOCAL_THEN_GLOBAL when local store and global cache provides non-empty result", function(
        done
    ) {
        discoveryQos.discoveryScope = DiscoveryScope.LOCAL_THEN_GLOBAL;
        capabilityDiscovery.lookup([domain], interfaceName, discoveryQos);
        waitsFor(
            function() {
                return (
                    localCapStoreSpy.lookup.calls.count() >= 1 &&
                    globalCapCacheSpy.lookup.calls.count() >= 1 &&
                    globalCapDirSpy.lookup.calls.count() >= 1
                );
            },
            "wait for lookups to be done",
            1000
        )
            .then(function() {
                expect(localCapStoreSpy.lookup).toHaveBeenCalledWith({
                    domains: [domain],
                    interfaceName: interfaceName
                });
                expect(globalCapCacheSpy.lookup).toHaveBeenCalledWith({
                    domains: [domain],
                    interfaceName: interfaceName,
                    cacheMaxAge: discoveryQos.cacheMaxAge
                });
                expect(globalCapDirSpy.lookup).toHaveBeenCalledWith({
                    domains: [domain],
                    interfaceName: interfaceName
                });
                done();
                return null;
            })
            .catch(done.fail);
    });

    it("calls local capabilities directory according to discoveryQos.discoveryScope LOCAL_ONLY", function(done) {
        discoveryQos.discoveryScope = DiscoveryScope.LOCAL_ONLY;
        capabilityDiscovery.lookup([domain], interfaceName, discoveryQos);
        expect(localCapStoreSpy.lookup).toHaveBeenCalledWith({
            domains: [domain],
            interfaceName: interfaceName
        });
        expect(globalCapCacheSpy.lookup).not.toHaveBeenCalled();
        expect(globalCapDirSpy.lookup).not.toHaveBeenCalled();
        done();
    });

    it("calls global capabilities directory according to discoveryQos.discoveryScope GLOBAL_ONLY", function(done) {
        discoveryQos.discoveryScope = DiscoveryScope.GLOBAL_ONLY;
        capabilityDiscovery.lookup([domain], interfaceName, discoveryQos);

        waitsFor(
            function() {
                return globalCapCacheSpy.lookup.calls.count() >= 1 && globalCapDirSpy.lookup.calls.count() >= 1;
            },
            "waiting for globalCapCacheSpy to get called",
            1000
        )
            .then(function() {
                expect(localCapStoreSpy.lookup).not.toHaveBeenCalled();
                expect(globalCapCacheSpy.lookup).toHaveBeenCalledWith({
                    domains: [domain],
                    interfaceName: interfaceName,
                    cacheMaxAge: discoveryQos.cacheMaxAge
                });
                expect(globalCapDirSpy.lookup).toHaveBeenCalledWith({
                    domains: [domain],
                    interfaceName: interfaceName
                });
                done();
                return null;
            })
            .catch(done.fail);
    });

    it("does not call global capabilities directory according to discoveryQos.discoveryScope GLOBAL_ONLY, if global cache is non-empty", function(
        done
    ) {
        globalCapCacheSpy.lookup.and.returnValue([getDiscoveryEntry(domain, interfaceName)]);
        discoveryQos.discoveryScope = DiscoveryScope.GLOBAL_ONLY;
        capabilityDiscovery.lookup([domain], interfaceName, discoveryQos);
        expect(localCapStoreSpy.lookup).not.toHaveBeenCalled();
        expect(globalCapCacheSpy.lookup).toHaveBeenCalledWith({
            domains: [domain],
            interfaceName: interfaceName,
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
        var onFulfilledSpy = jasmine.createSpy("onFulfilled" + descriptor),
            onRejectedSpy = jasmine.createSpy("onRejected" + descriptor);
        var localCapStoreSpy = getSpiedLookupObjWithReturnValue("localCapStoreSpy" + descriptor, localdiscoveryEntries);
        var globalCapCacheSpy = getSpiedLookupObjWithReturnValue(
            "globalCapCacheSpy" + descriptor,
            globalCapCacheEntries
        );
        var globalCapDirSpy = getSpiedLookupObjWithReturnValue(
            "globalCapDirSpy" + descriptor,
            Promise.resolve({
                result: globalCapabilityInfos
            })
        );

        var proxyBuilderSpy = jasmine.createSpyObj("proxyBuilderSpy", ["build"]);
        proxyBuilderSpy.build.and.returnValue(Promise.resolve(globalCapDirSpy));
        var capabilityDiscovery = new CapabilityDiscovery(
            localCapStoreSpy,
            globalCapCacheSpy,
            messageRouterSpy,
            proxyBuilderSpy,
            "io.joynr"
        );
        capabilityDiscovery.globalAddressReady(address);
        var discoveryQos = new DiscoveryQos({
            cacheMaxAge: 0,
            discoveryScope: discoveryScope
        });

        return capabilityDiscovery
            .lookup([domain], interfaceName, discoveryQos)
            .then(function(fulfilledWith) {
                var i;
                var endDateMs = Date.now();
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
            .catch(function(error) {
                if (expectedReturnValue !== undefined) {
                    fail("a return value was expected: " + expectedReturnValue);
                }
            });
    }

    it("discovers correct discoveryEntries according to discoveryScope", function(done) {
        var promises = [];
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
            .then(function() {
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
                scope: scope,
                supportsOnChangeSubscriptions: true
            }),
            participantId: "700",
            lastSeenDateMs: 123,
            publicKeyId: ""
        });
    }

    function getGlobalDiscoveryEntryWithScope(scope) {
        return new GlobalDiscoveryEntry({
            providerVersion: new Version({ majorVersion: 47, minorVersion: 11 }),
            domain: "domain",
            interfaceName: "interfaceName",
            lastSeenDateMs: Date.now(),
            qos: new ProviderQos({
                customParameters: [
                    new CustomParameter({
                        name: "theName",
                        value: "theValue"
                    })
                ],
                priority: 1234,
                scope: scope,
                supportsOnChangeSubscriptions: true
            }),
            address: JSON.stringify(address),
            participantId: "700",
            publicKeyId: ""
        });
    }

    it("calls local cap dir correctly", function(done) {
        var discoveryEntry = getDiscoveryEntryWithScope(ProviderScope.LOCAL);
        capabilityDiscovery
            .add(discoveryEntry)
            .then(function() {
                expect(localCapStoreSpy.add).toHaveBeenCalled();
                expect(localCapStoreSpy.add).toHaveBeenCalledWith({
                    discoveryEntry: discoveryEntry,
                    remote: false
                });
                expect(globalCapDirSpy.add).not.toHaveBeenCalledWith(undefined);
                done();
                return null;
            })
            .catch(fail);
    });

    it("calls global cap dir correctly", function(done) {
        var actualDiscoveryEntry;
        var discoveryEntry = getDiscoveryEntryWithScope(ProviderScope.GLOBAL);
        var expectedDiscoveryEntry = CapabilitiesUtil.discoveryEntry2GlobalDiscoveryEntry(discoveryEntry, address);
        capabilityDiscovery
            .add(discoveryEntry)
            .then(function() {
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

    it("touch calls global cap dir correctly", function(done) {
        var actualClusterControllerId;
        var actualTtl;
        var expectedTtl = 1337;
        var expectedClusterControllerId = "testTouchClusterControllerId";
        capabilityDiscovery
            .touch(expectedClusterControllerId, expectedTtl)
            .then(function() {
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

    it("reports error from global cap dir", function(done) {
        var actualDiscoveryEntry;
        var discoveryEntry = getDiscoveryEntryWithScope(ProviderScope.GLOBAL);
        var expectedDiscoveryEntry = CapabilitiesUtil.discoveryEntry2GlobalDiscoveryEntry(discoveryEntry, address);
        globalCapDirSpy = getSpiedLookupObjWithReturnValue("globalCapDirSpy", Promise.reject(new Error("Some error.")));

        proxyBuilderSpy.build.and.returnValue(Promise.resolve(globalCapDirSpy));
        capabilityDiscovery
            .add(discoveryEntry)
            .then(function() {
                fail("expected an error to have been reported");
                return null;
            })
            .catch(function(error) {
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

    it("throws on unknown provider scope", function(done) {
        var discoveryEntry = getDiscoveryEntryWithScope("UnknownScope");
        capabilityDiscovery
            .add(discoveryEntry)
            .then(function() {
                fail("expected an error");
                return null;
            })
            .catch(function(error) {
                expect(globalCapDirSpy.add).not.toHaveBeenCalled();
                expect(localCapStoreSpy.add).not.toHaveBeenCalledWith();
                expect(Object.prototype.toString.call(error) === "[object Error]").toBeTruthy();
                done();
                return null;
            });
    });

    it("lookup with multiple domains should throw an exception", function(done) {
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
            .then(function() {
                fail("unexpected success");
            })
            .catch(function(error) {
                done();
                return null;
            });
    });
});
