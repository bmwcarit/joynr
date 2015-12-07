/*jslint es5: true */

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

define([
            "joynr/capabilities/discovery/CapabilityDiscovery",
            "joynr/types/DiscoveryQos",
            "joynr/types/ArbitrationStrategyCollection",
            "joynr/types/ProviderQos",
            "joynr/types/CustomParameter",
            "joynr/types/ProviderScope",
            "joynr/types/DiscoveryScope",
            "joynr/types/DiscoveryEntry",
            "joynr/types/GlobalDiscoveryEntry",
            "joynr/system/RoutingTypes/ChannelAddress",
            "joynr/types/Version",
            "global/Promise"
        ],
        function(
                CapabilityDiscovery,
                DiscoveryQos,
                ArbitrationStrategyCollection,
                ProviderQos,
                CustomParameter,
                ProviderScope,
                DiscoveryScope,
                DiscoveryEntry,
                GlobalDiscoveryEntry,
                ChannelAddress,
                Version,
                Promise) {

            var domain, interfaceName, discoveryQos, discoveryEntries, globalDiscoveryEntries;
            var capabilityDiscovery, messageRouterSpy, proxyBuilderSpy, address, localCapStoreSpy;
            var globalCapCacheSpy, globalCapDirSpy, capabilityInfo;
            var asyncTimeout = 5000;
            var startDateMs;


            messageRouterSpy = jasmine.createSpyObj("routingTable", [
                "addNextHop",
                "resolveNextHop"
            ]);

            messageRouterSpy.addNextHop.andReturn(Promise.resolve());
            messageRouterSpy.resolveNextHop.andReturn(Promise.resolve());

            function getSpiedLookupObjWithReturnValue(name, returnValue) {
                var spyObj = jasmine.createSpyObj(name, [
                    "lookup",
                    "add",
                    "remove"
                ]);
                spyObj.lookup.andReturn(returnValue);
                spyObj.add.andReturn(returnValue);
                spyObj.remove.andReturn(spyObj);
                return spyObj;
            }

            function getGlobalDiscoveryEntry(domain, interfaceName, newGlobalAddress) {
                return new GlobalDiscoveryEntry({
                    providerVersion : new Version({ majorVersion: 47, minorVersion: 11}),
                    domain : domain,
                    interfaceName : interfaceName,
                    lastSeenDateMs : Date.now(),
                    qos : new ProviderQos({
                        customParameters : [ new CustomParameter({
                            name : "theName",
                            value : "theValue"
                        })
                        ],
                        priority : 1234,
                        scope : discoveryQos.discoveryScope === DiscoveryScope.LOCAL_ONLY
                                ? ProviderScope.LOCAL
                                : ProviderScope.GLOBAL,
                        supportsOnChangeSubscriptions : true
                    }),
                    address : JSON.stringify((newGlobalAddress !== undefined ? newGlobalAddress : address)),
                    participantId : "700",
                    publicKeyId : ""
                });
            }

            function assertDiscoveryEntryEquals(expected, actual) {
                expect(actual.domain).toEqual(expected.domain);
                expect(actual.interfaceName).toEqual(expected.interfaceName);
                expect(actual.participantId).toEqual(expected.participantId);
                expect(actual.qos).toEqual(expected.qos);
                expect(actual.address).toEqual(expected.address);
                expect(actual.publicKeyId).toEqual(expected.publicKeyId);
            }

            function getDiscoveryEntry(domain, interfaceName) {
                return new DiscoveryEntry({
                    providerVersion : new Version({ majorVersion: 47, minorVersion: 11}),
                    domain : domain,
                    interfaceName : interfaceName,
                    qos : new ProviderQos({
                        customParameters : [ new CustomParameter({
                            name : "theName",
                            value : "theValue"
                        })
                        ],
                        priority : 1234,
                        scope : discoveryQos.discoveryScope === DiscoveryScope.LOCAL_ONLY
                                ? ProviderScope.LOCAL
                                : ProviderScope.GLOBAL,
                        supportsOnChangeSubscriptions : true
                    }),
                    participantId : "700",
                    lastSeenDateMs : Date.now(),
                    publicKeyId : ""
                });
            }

            describe(
                    "libjoynr-js.joynr.capabilities.discovery.CapabilityDiscovery",
                    function() {

                        beforeEach(function() {
                            var i;
                            startDateMs = Date.now();
                            domain = "myDomain";
                            interfaceName = "myInterfaceName";
                            address = new ChannelAddress({
                                channelId: domain + "TestCapabilityDiscoveryChannel",
                                messagingEndpointUrl: "http://testUrl"
                            });
                            discoveryQos = new DiscoveryQos({
                                cacheMaxAge : 0,
                                discoveryScope : DiscoveryScope.LOCAL_THEN_GLOBAL
                            });

                            discoveryEntries = [];
                            for (i = 0; i < 12; ++i) {
                                discoveryEntries.push(getDiscoveryEntry(
                                        domain + i.toString(),
                                        interfaceName + i.toString()));
                            }

                            globalDiscoveryEntries = [];
                            for (i = 0; i < 12; ++i) {
                                globalDiscoveryEntries.push(getGlobalDiscoveryEntry(domain + i.toString(), interfaceName  + i.toString(), new ChannelAddress({
                                    channelId: "globalCapInfo" + i.toString(),
                                    messagingEndpointUrl: "http://testurl"
                                })));
                            }

                            localCapStoreSpy =
                                    getSpiedLookupObjWithReturnValue("localCapStoreSpy", []);
                            globalCapCacheSpy =
                                    getSpiedLookupObjWithReturnValue("globalCapCacheSpy", []);
                            globalCapDirSpy =
                                    getSpiedLookupObjWithReturnValue("globalCapDirSpy", Promise.resolve([]));

                            proxyBuilderSpy = jasmine.createSpyObj("proxyBuilderSpy", [
                                "build"
                            ]);
                            proxyBuilderSpy.build.andReturn(Promise.resolve(globalCapDirSpy));
                            capabilityDiscovery =
                                    new CapabilityDiscovery(
                                            localCapStoreSpy,
                                            globalCapCacheSpy,
                                            messageRouterSpy,
                                            proxyBuilderSpy,
                                            "io.joynr");
                            capabilityDiscovery.globalAddressReady(address);
                        });

                        it(
                                "is instantiable, of correct type and has all members",
                                function() {
                                    expect(capabilityDiscovery).toBeDefined();
                                    expect(capabilityDiscovery instanceof CapabilityDiscovery)
                                            .toBeTruthy();
                                    expect(capabilityDiscovery.lookup).toBeDefined();
                                    expect(typeof capabilityDiscovery.lookup === "function")
                                            .toBeTruthy();
                                });

                        it("throws when constructor arguments are missing", function() {
                            expect(function() {
                                var capDisc = new CapabilityDiscovery();
                            }).toThrow();
                            expect(function() {
                                var capDisc = new CapabilityDiscovery(localCapStoreSpy);
                            }).toThrow();
                            expect(function() {
                                var capDisc = new CapabilityDiscovery(messageRouterSpy);
                            }).toThrow();
                            expect(
                                    function() {
                                        var capDisc =
                                                new CapabilityDiscovery(
                                                        localCapStoreSpy,
                                                        globalCapCacheSpy,
                                                        proxyBuilderSpy);
                                    }).toThrow();
                            expect(
                                    function() {
                                        var capDisc =
                                                new CapabilityDiscovery(
                                                        localCapStoreSpy,
                                                        globalCapCacheSpy,
                                                        messageRouterSpy,
                                                        proxyBuilderSpy);
                                    }).toThrow();
                            expect(function() {
                                var capDisc =
                                        new CapabilityDiscovery(
                                                localCapStoreSpy,
                                                globalCapCacheSpy,
                                                messageRouterSpy);
                            }).toThrow();
                            expect(function() {
                                var capDisc =
                                        new CapabilityDiscovery(
                                                localCapStoreSpy,
                                                globalCapCacheSpy,
                                                messageRouterSpy,
                                                proxyBuilderSpy,
                                                "domain");
                            }).not.toThrow();
                        });

                        it(
                                "calls local capabilities directory according to discoveryQos.discoveryScope LOCAL_THEN_GLOBAL when local cache provides non-empty result",
                                function() {
                                    localCapStoreSpy =
                                            getSpiedLookupObjWithReturnValue(
                                                    "localCapStoreSpy",
                                                    discoveryEntries);
                                    globalCapCacheSpy =
                                            getSpiedLookupObjWithReturnValue(
                                                    "globalCapCacheSpy",
                                                    []);
                                    capabilityDiscovery =
                                            new CapabilityDiscovery(
                                                    localCapStoreSpy,
                                                    globalCapCacheSpy,
                                                    messageRouterSpy,
                                                    proxyBuilderSpy,
                                                    "io.joynr");

                                    capabilityDiscovery.lookup([domain], interfaceName, discoveryQos);
                                    expect(localCapStoreSpy.lookup).toHaveBeenCalledWith({
                                        domains : [domain],
                                        interfaceName : interfaceName
                                    });
                                    expect(globalCapDirSpy.lookup).not.toHaveBeenCalled();
                                    expect(globalCapCacheSpy.lookup).not.toHaveBeenCalled();
                                });

                        it(
                                "calls local and global capabilities directory according to discoveryQos.discoveryScope LOCAL_THEN_GLOBAL when local cache provides empty result",
                                function() {
                                    runs(function(){
                                        discoveryQos.discoveryScope = DiscoveryScope.LOCAL_THEN_GLOBAL;
                                        capabilityDiscovery.lookup([domain], interfaceName, discoveryQos);
                                    });

                                    waitsFor(function(){
                                        return globalCapDirSpy.lookup.callCount > 0;
                                    },
                                    "until globalCapDir has been called",
                                    asyncTimeout);

                                    runs(function(){
                                        expect(localCapStoreSpy.lookup).toHaveBeenCalledWith({
                                            domains : [domain],
                                            interfaceName : interfaceName
                                        });
                                        expect(globalCapCacheSpy.lookup).toHaveBeenCalledWith({
                                            domains : [domain],
                                            interfaceName : interfaceName,
                                            cacheMaxAge : discoveryQos.cacheMaxAge
                                        });
                                        expect(globalCapDirSpy.lookup).toHaveBeenCalledWith({
                                            domains : [domain],
                                            interfaceName : interfaceName
                                        });
                                    });
                                });

                        it(
                                "calls local and not global cache and not global capabilities directory according to discoveryQos.discoveryScope LOCAL_THEN_GLOBAL when local store provides non-empty result",
                                function() {
                                    localCapStoreSpy.lookup.andReturn([ getDiscoveryEntry(
                                            domain,
                                            interfaceName)
                                    ]);
                                    capabilityDiscovery.lookup([domain], interfaceName, discoveryQos);
                                    expect(localCapStoreSpy.lookup).toHaveBeenCalledWith({
                                        domains : [domain],
                                        interfaceName : interfaceName
                                    });
                                    expect(globalCapCacheSpy.lookup).not.toHaveBeenCalled();
                                    expect(globalCapDirSpy.lookup).not.toHaveBeenCalled();
                                });

                        it(
                                "calls local and global cache and global capabilities directory according to discoveryQos.discoveryScope LOCAL_THEN_GLOBAL when local store and global cache provides non-empty result",
                                function() {
                                    runs(function(){
                                        discoveryQos.discoveryScope = DiscoveryScope.LOCAL_THEN_GLOBAL;
                                        capabilityDiscovery.lookup([domain], interfaceName, discoveryQos);
                                    });

                                    waitsFor(function(){
                                        return globalCapDirSpy.lookup.callCount > 0;
                                    },
                                    "until globalCapDir has been called",
                                    asyncTimeout);

                                    runs(function(){
                                        expect(localCapStoreSpy.lookup).toHaveBeenCalledWith({
                                            domains : [domain],
                                            interfaceName : interfaceName
                                        });
                                        expect(globalCapCacheSpy.lookup).toHaveBeenCalledWith({
                                            domains : [domain],
                                            interfaceName : interfaceName,
                                            cacheMaxAge : discoveryQos.cacheMaxAge
                                        });
                                        expect(globalCapDirSpy.lookup).toHaveBeenCalledWith({
                                            domains : [domain],
                                            interfaceName : interfaceName
                                        });
                                    });
                                });

                        it(
                                "calls local capabilities directory according to discoveryQos.discoveryScope LOCAL_ONLY",
                                function() {
                                    discoveryQos.discoveryScope = DiscoveryScope.LOCAL_ONLY;
                                    capabilityDiscovery.lookup([domain], interfaceName, discoveryQos);
                                    expect(localCapStoreSpy.lookup).toHaveBeenCalledWith({
                                        domains : [domain],
                                        interfaceName : interfaceName
                                    });
                                    expect(globalCapCacheSpy.lookup).not.toHaveBeenCalled();
                                    expect(globalCapDirSpy.lookup).not.toHaveBeenCalled();
                                });

                        it(
                                "calls global capabilities directory according to discoveryQos.discoveryScope GLOBAL_ONLY",
                                function() {
                                    runs(function(){
                                        discoveryQos.discoveryScope = DiscoveryScope.GLOBAL_ONLY;
                                        capabilityDiscovery.lookup([domain], interfaceName, discoveryQos);
                                    });

                                    waitsFor(function(){
                                        return globalCapDirSpy.lookup.callCount > 0;
                                    },
                                    "until globalCapDir has been called",
                                    asyncTimeout);

                                    runs(function(){
                                        expect(localCapStoreSpy.lookup).not.toHaveBeenCalled();
                                        expect(globalCapCacheSpy.lookup).toHaveBeenCalledWith({
                                            domains : [domain],
                                            interfaceName : interfaceName,
                                            cacheMaxAge : discoveryQos.cacheMaxAge
                                        });
                                        expect(globalCapDirSpy.lookup).toHaveBeenCalledWith({
                                            domains : [domain],
                                            interfaceName : interfaceName
                                        });
                                    });
                                });

                        it(
                                "does not call global capabilities directory according to discoveryQos.discoveryScope GLOBAL_ONLY, if global cache is non-empty",
                                function() {
                                    globalCapCacheSpy.lookup.andReturn([ getDiscoveryEntry(
                                            domain,
                                            interfaceName)
                                    ]);
                                    discoveryQos.discoveryScope = DiscoveryScope.GLOBAL_ONLY;
                                    capabilityDiscovery.lookup([domain], interfaceName, discoveryQos);
                                    expect(localCapStoreSpy.lookup).not.toHaveBeenCalled();
                                    expect(globalCapCacheSpy.lookup).toHaveBeenCalledWith({
                                        domains : [domain],
                                        interfaceName : interfaceName,
                                        cacheMaxAge : discoveryQos.cacheMaxAge
                                    });
                                    expect(globalCapDirSpy.lookup).not.toHaveBeenCalled();
                                });

                        function testDiscoveryResult(
                                descriptor,
                                discoveryScope,
                                localdiscoveryEntries,
                                globalCapCacheEntries,
                                globalCapabilityInfos,
                                expectedReturnValue) {
                            var onFulfilledSpy = jasmine.createSpy("onFulfilled" + descriptor), onRejectedSpy =
                                    jasmine.createSpy("onRejected" + descriptor);
                            var localCapStoreSpy =
                                    getSpiedLookupObjWithReturnValue("localCapStoreSpy"
                                        + descriptor, localdiscoveryEntries);
                            var globalCapCacheSpy =
                                    getSpiedLookupObjWithReturnValue("globalCapCacheSpy"
                                        + descriptor, globalCapCacheEntries);
                            var globalCapDirSpy =
                                    getSpiedLookupObjWithReturnValue(
                                            "globalCapDirSpy" + descriptor,
                                            Promise.resolve({
                                                result : globalCapabilityInfos
                                            }));

                            var proxyBuilderSpy = jasmine.createSpyObj("proxyBuilderSpy", [
                                   "build"
                            ]);
                            proxyBuilderSpy.build.andReturn(Promise.resolve(globalCapDirSpy));
                            var capabilityDiscovery =
                                    new CapabilityDiscovery(
                                            localCapStoreSpy,
                                            globalCapCacheSpy,
                                            messageRouterSpy,
                                            proxyBuilderSpy,
                                            "io.joynr");
                            var discoveryQos = new DiscoveryQos({
                                cacheMaxAge : 0,
                                discoveryScope : discoveryScope
                            });

                            runs(function() {
                                capabilityDiscovery.lookup([domain], interfaceName, discoveryQos)
                                        .then(onFulfilledSpy).catch(onRejectedSpy);
                            });

                            waitsFor(
                                    function() {
                                        return onFulfilledSpy.callCount > 0
                                            || onRejectedSpy.callCount > 0;
                                    },
                                    "until the capability discovery promise is not pending any more",
                                    asyncTimeout);

                            runs(function() {
                                if (expectedReturnValue === undefined) {
                                    expect(onFulfilledSpy).not.toHaveBeenCalled();
                                } else {
                                    var i;
                                    var endDateMs = Date.now();
                                    var fulfilledWith = onFulfilledSpy.calls[0].args[0];
                                    expect(fulfilledWith.length).toEqual(expectedReturnValue.length);
                                    for (i = 0; i<fulfilledWith.length; i++) {
                                        assertDiscoveryEntryEquals(expectedReturnValue[i], fulfilledWith[i]);
                                        expect(fulfilledWith[i].lastSeenDateMs >= startDateMs).toBeTruthy();
                                        expect(fulfilledWith[i].lastSeenDateMs <= endDateMs).toBeTruthy();
                                    }
                                }
                            });
                        }

                        it(
                                "discovers correct discoveryEntries according to discoveryScope",
                                function() {
                                    testDiscoveryResult(
                                            "00",
                                            DiscoveryScope.LOCAL_THEN_GLOBAL,
                                            [],
                                            [],
                                            [],
                                            []);
                                    testDiscoveryResult(
                                            "01",
                                            DiscoveryScope.LOCAL_THEN_GLOBAL,
                                            [ discoveryEntries[1]
                                            ],
                                            [],
                                            [],
                                            [ discoveryEntries[1]
                                            ]);
                                    testDiscoveryResult(
                                            "02",
                                            DiscoveryScope.LOCAL_THEN_GLOBAL,
                                            [],
                                            [ globalDiscoveryEntries[1]
                                            ],
                                            [],
                                            [ globalDiscoveryEntries[1]
                                            ]);
                                    testDiscoveryResult(
                                            "03",
                                            DiscoveryScope.LOCAL_THEN_GLOBAL,
                                            [],
                                            [],
                                            [ globalDiscoveryEntries[2]
                                            ],
                                            [ globalDiscoveryEntries[2]
                                            ]);
                                    testDiscoveryResult(
                                            "04",
                                            DiscoveryScope.LOCAL_THEN_GLOBAL,
                                            [ discoveryEntries[3]
                                            ],
                                            [],
                                            [ globalDiscoveryEntries[3]
                                            ],
                                            [ discoveryEntries[3]
                                            ]);
                                    testDiscoveryResult(
                                            "05",
                                            DiscoveryScope.LOCAL_THEN_GLOBAL,
                                            [ discoveryEntries[3]
                                            ],
                                            [ globalDiscoveryEntries[1]
                                            ],
                                            [ globalDiscoveryEntries[3]
                                            ],
                                            [ discoveryEntries[3]
                                            ]);
                                    testDiscoveryResult(
                                            "06",
                                            DiscoveryScope.LOCAL_ONLY,
                                            [],
                                            [],
                                            [],
                                            []);
                                    testDiscoveryResult(
                                            "07",
                                            DiscoveryScope.LOCAL_ONLY,
                                            [ discoveryEntries[5]
                                            ],
                                            [],
                                            [],
                                            [ discoveryEntries[5]
                                            ]);
                                    testDiscoveryResult(
                                            "08",
                                            DiscoveryScope.LOCAL_ONLY,
                                            [],
                                            [],
                                            [ globalDiscoveryEntries[6]
                                            ],
                                            []);
                                    testDiscoveryResult(
                                            "09",
                                            DiscoveryScope.LOCAL_ONLY,
                                            [],
                                            [ globalDiscoveryEntries[5]
                                            ],
                                            [],
                                            []);
                                    testDiscoveryResult(
                                            "10",
                                            DiscoveryScope.LOCAL_ONLY,
                                            [],
                                            [ globalDiscoveryEntries[5]
                                            ],
                                            [ globalDiscoveryEntries[6]
                                            ],
                                            []);
                                    testDiscoveryResult(
                                            "11",
                                            DiscoveryScope.LOCAL_ONLY,
                                            [ discoveryEntries[7]
                                            ],
                                            [],
                                            [ globalDiscoveryEntries[7]
                                            ],
                                            [ discoveryEntries[7]
                                            ]);
                                    testDiscoveryResult(
                                            "12",
                                            DiscoveryScope.LOCAL_ONLY,
                                            [ discoveryEntries[7]
                                            ],
                                            [ globalDiscoveryEntries[1]
                                            ],
                                            [ globalDiscoveryEntries[7]
                                            ],
                                            [ discoveryEntries[7]
                                            ]);
                                    testDiscoveryResult(
                                            "13",
                                            DiscoveryScope.GLOBAL_ONLY,
                                            [],
                                            [],
                                            [],
                                            []);
                                    testDiscoveryResult(
                                            "14",
                                            DiscoveryScope.GLOBAL_ONLY,
                                            [],
                                            [ globalDiscoveryEntries[9]
                                            ],
                                            [],
                                            [ globalDiscoveryEntries[9]
                                            ]);
                                    testDiscoveryResult(
                                            "15",
                                            DiscoveryScope.GLOBAL_ONLY,
                                            [ discoveryEntries[9]
                                            ],
                                            [],
                                            [],
                                            []);
                                    testDiscoveryResult(
                                            "16",
                                            DiscoveryScope.GLOBAL_ONLY,
                                            [],
                                            [ globalDiscoveryEntries[10]
                                            ],
                                            [ globalDiscoveryEntries[10]
                                            ],
                                            [ globalDiscoveryEntries[10]
                                            ]);
                                    testDiscoveryResult(
                                            "17",
                                            DiscoveryScope.GLOBAL_ONLY,
                                            [],
                                            [],
                                            [ globalDiscoveryEntries[10]
                                            ],
                                            [ globalDiscoveryEntries[10]
                                            ]);
                                    testDiscoveryResult(
                                            "18",
                                            DiscoveryScope.GLOBAL_ONLY,
                                            [],
                                            [ globalDiscoveryEntries[11]
                                            ],
                                            [ globalDiscoveryEntries[11]
                                            ],
                                            [ globalDiscoveryEntries[11]
                                            ]);
                                    testDiscoveryResult(
                                            "19",
                                            DiscoveryScope.GLOBAL_ONLY,
                                            [ discoveryEntries[10]
                                            ],
                                            [ globalDiscoveryEntries[11]
                                            ],
                                            [ globalDiscoveryEntries[11]
                                            ],
                                            [ globalDiscoveryEntries[11]
                                            ]);
                                    testDiscoveryResult(
                                            "20",
                                            DiscoveryScope.LOCAL_AND_GLOBAL,
                                            [],
                                            [],
                                            [],
                                            []);
                                    testDiscoveryResult(
                                            "21",
                                            DiscoveryScope.LOCAL_AND_GLOBAL,
                                            [ discoveryEntries[1]
                                            ],
                                            [],
                                            [],
                                            [ discoveryEntries[1]
                                            ]);
                                    testDiscoveryResult(
                                            "22",
                                            DiscoveryScope.LOCAL_AND_GLOBAL,
                                            [],
                                            [ globalDiscoveryEntries[1]
                                            ],
                                            [],
                                            [ globalDiscoveryEntries[1]
                                            ]);
                                    testDiscoveryResult(
                                            "23",
                                            DiscoveryScope.LOCAL_AND_GLOBAL,
                                            [],
                                            [],
                                            [ globalDiscoveryEntries[2]
                                            ],
                                            [ globalDiscoveryEntries[2]
                                            ]);
                                    testDiscoveryResult(
                                            "24",
                                            DiscoveryScope.LOCAL_AND_GLOBAL,
                                            [ discoveryEntries[3]
                                            ],
                                            [ globalDiscoveryEntries[4]
                                            ],
                                            [],
                                            [
                                                discoveryEntries[3],
                                                globalDiscoveryEntries[4]
                                            ]);
                                    testDiscoveryResult(
                                            "25",
                                            DiscoveryScope.LOCAL_AND_GLOBAL,
                                            [ discoveryEntries[3]
                                            ],
                                            [],
                                            [ globalDiscoveryEntries[4]
                                            ],
                                            [
                                                discoveryEntries[3],
                                                globalDiscoveryEntries[4]
                                            ]);
                                    testDiscoveryResult(
                                            "26",
                                            DiscoveryScope.LOCAL_AND_GLOBAL,
                                            [ discoveryEntries[3]
                                            ],
                                            [ globalDiscoveryEntries[1]
                                            ],
                                            [ globalDiscoveryEntries[3]
                                            ],
                                            [
                                                discoveryEntries[3],
                                                globalDiscoveryEntries[1]
                                            ]);
                                });

                        function getDiscoveryEntryWithScope(scope) {
                            return new DiscoveryEntry({
                                providerVersion : new Version({ majorVersion: 47, minorVersion: 11}),
                                domain : "domain",
                                interfaceName : "interfaceName",
                                qos : new ProviderQos({
                                    customParameters : [ new CustomParameter({
                                        name : "theName",
                                        value : "theValue"
                                    })
                                    ],
                                    priority : 1234,
                                    scope : scope,
                                    supportsOnChangeSubscriptions : true
                                }),
                                participantId : "700",
                                lastSeenDateMs : 123,
                                publicKeyId : ""
                            });
                        }

                        function getGlobalDiscoveryEntryWithScope(scope) {
                            return new GlobalDiscoveryEntry({
                                providerVersion : new Version({ majorVersion: 47, minorVersion: 11}),
                                domain : "domain",
                                interfaceName : "interfaceName",
                                lastSeenDateMs : Date.now(),
                                qos : new ProviderQos({
                                    customParameters : [ new CustomParameter({
                                        name : "theName",
                                        value : "theValue"
                                    })
                                    ],
                                    priority : 1234,
                                    scope : scope,
                                    supportsOnChangeSubscriptions : true
                                }),
                                address : JSON.stringify(address),
                                participantId : "700",
                                publicKeyId : ""
                            });
                        }

                        it("calls local cap dir correctly", function() {
                            var discoveryEntry = getDiscoveryEntryWithScope(ProviderScope.LOCAL);
                            var spy = jasmine.createSpyObj("spy", [
                                "onFulfilled",
                                "onRejected"
                            ]);

                            runs(function() {
                                capabilityDiscovery.add(discoveryEntry).then(
                                        spy.onFulfilled).catch(spy.onRejected);
                            });

                            waitsFor(function() {
                                return spy.onFulfilled.callCount > 0;
                            }, "onFulfilled spy to be invoked", 100);

                            runs(function() {
                                expect(localCapStoreSpy.add).toHaveBeenCalled();
                                expect(localCapStoreSpy.add).toHaveBeenCalledWith({
                                    discoveryEntry : discoveryEntry,
                                    remote : false
                                });
                                expect(globalCapDirSpy.add).not.toHaveBeenCalledWith(undefined);
                                expect(spy.onFulfilled).toHaveBeenCalled();
                                expect(spy.onRejected).not.toHaveBeenCalledWith(undefined);
                            });
                        });

                        /* this test is not valid, as capabilityDiscovery.add() shall be invoked with typed
                         * capability information
                         */
                        //it("types untyped CapInfo", function() {
                        //    var typedCapInfo = getDiscoveryEntryWithScope(ProviderScope.LOCAL);
                        //    var untypedCapInfo = JSON.parse(JSON.stringify(typedCapInfo)); //untype discoveryEntry
                        //    typedCapInfo = Typing.augmentTypes(untypedCapInfo, typeRegistry);
                        //    /*jslint nomen: true */
                        //    delete untypedCapInfo._typeName;
                        //    /*jslint nomen: false */
                        //    capabilityDiscovery.add(untypedCapInfo);
                        //    expect(localCapStoreSpy.add).toHaveBeenCalled();
                        //    expect(localCapStoreSpy.add).toHaveBeenCalledWith(typedCapInfo);
                        //});*/
                        it(
                                "calls global cap dir correctly",
                                function() {
                                    var actualDiscoveryEntry;
                                    var discoveryEntry =
                                            getDiscoveryEntryWithScope(ProviderScope.GLOBAL);
                                    var spy = jasmine.createSpyObj("spy", [
                                        "onFulfilled",
                                        "onRejected"
                                    ]);

                                    runs(function() {
                                        capabilityDiscovery.add(discoveryEntry).then(
                                                spy.onFulfilled).catch(spy.onRejected);
                                    });

                                    waitsFor(function() {
                                        return spy.onFulfilled.callCount > 0;
                                    }, "onFulfilled spy to be invoked", 100);

                                    runs(function() {
                                        expect(globalCapDirSpy.add).toHaveBeenCalled();
                                        actualDiscoveryEntry = globalCapDirSpy.add.calls[0].args[0].globalDiscoveryEntry;
                                        discoveryEntry.address = JSON.stringify(address);
                                        assertDiscoveryEntryEquals(discoveryEntry, actualDiscoveryEntry);

                                        expect(localCapStoreSpy.add).not.toHaveBeenCalledWith();
                                        expect(spy.onFulfilled).toHaveBeenCalled();
                                        expect(spy.onRejected).not.toHaveBeenCalledWith();
                                    });
                                });

                        it(
                                "reports error from global cap dir",
                                function() {
                                    var actualDiscoveryEntry;
                                    var discoveryEntry =
                                            getDiscoveryEntryWithScope(ProviderScope.GLOBAL);
                                    var spy = jasmine.createSpyObj("spy", [
                                        "onFulfilled",
                                        "onRejected"
                                    ]);

                                    globalCapDirSpy =
                                            getSpiedLookupObjWithReturnValue(
                                                    "globalCapDirSpy",
                                                    Promise.reject(new Error("Some error.")));

                                    proxyBuilderSpy.build.andReturn(Promise.resolve(globalCapDirSpy));
                                    runs(function() {
                                        capabilityDiscovery.add(discoveryEntry).then(
                                                spy.onFulfilled).catch(spy.onRejected);
                                    });

                                    waitsFor(function() {
                                        return spy.onRejected.callCount > 0;
                                    }, "onRejected spy to be called", 100);

                                    runs(function() {
                                        expect(globalCapDirSpy.add).toHaveBeenCalled();
                                        actualDiscoveryEntry = globalCapDirSpy.add.calls[0].args[0].globalDiscoveryEntry;
                                        assertDiscoveryEntryEquals(getGlobalDiscoveryEntryWithScope(ProviderScope.GLOBAL), actualDiscoveryEntry);
                                        expect(localCapStoreSpy.add).not.toHaveBeenCalledWith();
                                        expect(spy.onRejected).toHaveBeenCalled();
                                        expect(
                                                Object.prototype.toString
                                                        .call(spy.onRejected.mostRecentCall.args[0]) === "[object Error]")
                                                .toBeTruthy();
                                        expect(spy.onFulfilled).not.toHaveBeenCalled();
                                    });
                                });

                        it(
                                "throws on unknown provider scope",
                                function() {
                                    var discoveryEntry = getDiscoveryEntryWithScope("UnknownScope");
                                    var spy = jasmine.createSpyObj("spy", [
                                        "onFulfilled",
                                        "onRejected"
                                    ]);

                                    runs(function() {
                                        capabilityDiscovery.add(discoveryEntry).then(
                                                spy.onFulfilled).catch(spy.onRejected);
                                    });

                                    waitsFor(function() {
                                        return spy.onRejected.callCount > 0;
                                    }, "onRejected spy to be called", 100);

                                    runs(function() {
                                        expect(globalCapDirSpy.add).not.toHaveBeenCalled();
                                        expect(localCapStoreSpy.add).not.toHaveBeenCalledWith();
                                        expect(spy.onRejected).toHaveBeenCalled();
                                        expect(
                                                Object.prototype.toString
                                                        .call(spy.onRejected.mostRecentCall.args[0]) === "[object Error]")
                                                .toBeTruthy();
                                        expect(spy.onFulfilled).not.toHaveBeenCalled();
                                    });
                                });

                        it(
                                "lookup with multiple domains should throw an exception",
                                function() {
                                    localCapStoreSpy =
                                            getSpiedLookupObjWithReturnValue(
                                                    "localCapStoreSpy",
                                                    discoveryEntries);
                                    globalCapCacheSpy =
                                            getSpiedLookupObjWithReturnValue(
                                                    "globalCapCacheSpy",
                                                    []);
                                    capabilityDiscovery =
                                            new CapabilityDiscovery(
                                                    localCapStoreSpy,
                                                    globalCapCacheSpy,
                                                    messageRouterSpy,
                                                    proxyBuilderSpy,
                                                    "io.joynr");
                                    expect(capabilityDiscovery.lookup([domain, domain], interfaceName, discoveryQos).isRejected()).toBe(true);
                                });

                    });

        }); // require
