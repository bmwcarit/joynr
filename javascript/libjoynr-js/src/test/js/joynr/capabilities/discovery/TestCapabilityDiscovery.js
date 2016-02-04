/*global joynrTestRequire: true */
/*jslint es5: true */

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

joynrTestRequire(
        "joynr/capabilities/discovery/TestCapabilityDiscovery",
        [
            "joynr/capabilities/discovery/CapabilityDiscovery",
            "joynr/types/DiscoveryQos",
            "joynr/types/ArbitrationStrategyCollection",
            "joynr/types/ProviderQos",
            "joynr/types/CustomParameter",
            "joynr/types/ProviderScope",
            "joynr/types/DiscoveryScope",
            "joynr/types/DiscoveryEntry",
            "joynr/types/CapabilityInformation",
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
                CapabilityInformation,
                Promise) {

            var domain, interfaceName, discoveryQos, discoveryEntries, globalCapInfos;
            var capabilityDiscovery, messageRouterSpy, proxyBuilderSpy, channelId, localCapStoreSpy;
            var globalCapCacheSpy, globalCapDirSpy, capabilityInfo;
            var asyncTimeout = 5000;

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

            function getCapInfo(domain, interfaceName, newChannelId) {
                return new CapabilityInformation({
                    domain : domain,
                    interfaceName : interfaceName,
                    providerQos : new ProviderQos({
                        customParameters : [ new CustomParameter({
                            name : "theName",
                            value : "theValue"
                        })
                        ],
                        providerVersion : 123,
                        priority : 1234,
                        scope : discoveryQos.discoveryScope === DiscoveryScope.LOCAL_ONLY
                                ? ProviderScope.LOCAL
                                : ProviderScope.GLOBAL,
                        onChangeSubscriptions : true
                    }),
                    channelId : (newChannelId !== undefined ? newChannelId : channelId),
                    participantId : "700"
                });
            }

            function getDiscoverEntry(domain, interfaceName) {
                return new DiscoveryEntry({
                    domain : domain,
                    interfaceName : interfaceName,
                    qos : new ProviderQos({
                        customParameters : [ new CustomParameter({
                            name : "theName",
                            value : "theValue"
                        })
                        ],
                        providerVersion : 123,
                        priority : 1234,
                        scope : discoveryQos.discoveryScope === DiscoveryScope.LOCAL_ONLY
                                ? ProviderScope.LOCAL
                                : ProviderScope.GLOBAL,
                        onChangeSubscriptions : true
                    }),
                    participantId : "700",
                    connections : []
                });
            }

            describe(
                    "libjoynr-js.joynr.capabilities.discovery.CapabilityDiscovery",
                    function() {

                        beforeEach(function() {
                            var i;
                            domain = "myDomain";
                            interfaceName = "myInterfaceName";
                            channelId = domain + "TestCapabilityDiscoveryChannel";
                            discoveryQos = new DiscoveryQos({
                                cacheMaxAge : 0,
                                discoveryScope : DiscoveryScope.LOCAL_THEN_GLOBAL
                            });

                            discoveryEntries = [];
                            for (i = 0; i < 12; ++i) {
                                discoveryEntries.push(getDiscoverEntry(
                                        domain + i.toString(),
                                        interfaceName + i.toString()));
                            }

                            globalCapInfos = [];
                            for (i = 0; i < 12; ++i) {
                                globalCapInfos.push(getCapInfo(domain + i.toString(), interfaceName
                                    + i.toString(), "globalCapInfo" + i.toString()));
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
                                            channelId,
                                            "io.joynr");
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
                                                        messageRouterSpy,
                                                        proxyBuilderSpy,
                                                        channelId);
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
                                                messageRouterSpy,
                                                channelId);
                            }).toThrow();
                            expect(function() {
                                var capDisc =
                                        new CapabilityDiscovery(
                                                localCapStoreSpy,
                                                globalCapCacheSpy,
                                                messageRouterSpy,
                                                proxyBuilderSpy,
                                                channelId);
                            }).toThrow();
                            expect(function() {
                                var capDisc =
                                        new CapabilityDiscovery(
                                                localCapStoreSpy,
                                                globalCapCacheSpy,
                                                messageRouterSpy,
                                                proxyBuilderSpy,
                                                channelId,
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
                                                    channelId,
                                                    "io.joynr");

                                    capabilityDiscovery.lookup(domain, interfaceName, discoveryQos);
                                    expect(localCapStoreSpy.lookup).toHaveBeenCalledWith({
                                        domain : domain,
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
                                        capabilityDiscovery.lookup(domain, interfaceName, discoveryQos);
                                    });

                                    waitsFor(function(){
                                        return globalCapDirSpy.lookup.callCount > 0;
                                    },
                                    "until globalCapDir has been called",
                                    asyncTimeout);

                                    runs(function(){
                                        expect(localCapStoreSpy.lookup).toHaveBeenCalledWith({
                                            domain : domain,
                                            interfaceName : interfaceName
                                        });
                                        expect(globalCapCacheSpy.lookup).toHaveBeenCalledWith({
                                            domain : domain,
                                            interfaceName : interfaceName,
                                            cacheMaxAge : discoveryQos.cacheMaxAge
                                        });
                                        expect(globalCapDirSpy.lookup).toHaveBeenCalledWith({
                                            domain : domain,
                                            interfaceName : interfaceName
                                        });
                                    });
                                });

                        it(
                                "calls local and not global cache and not global capabilities directory according to discoveryQos.discoveryScope LOCAL_THEN_GLOBAL when local store provides non-empty result",
                                function() {
                                    localCapStoreSpy.lookup.andReturn([ getDiscoverEntry(
                                            domain,
                                            interfaceName)
                                    ]);
                                    capabilityDiscovery.lookup(domain, interfaceName, discoveryQos);
                                    expect(localCapStoreSpy.lookup).toHaveBeenCalledWith({
                                        domain : domain,
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
                                        capabilityDiscovery.lookup(domain, interfaceName, discoveryQos);
                                    });

                                    waitsFor(function(){
                                        return globalCapDirSpy.lookup.callCount > 0;
                                    },
                                    "until globalCapDir has been called",
                                    asyncTimeout);

                                    runs(function(){
                                        expect(localCapStoreSpy.lookup).toHaveBeenCalledWith({
                                            domain : domain,
                                            interfaceName : interfaceName
                                        });
                                        expect(globalCapCacheSpy.lookup).toHaveBeenCalledWith({
                                            domain : domain,
                                            interfaceName : interfaceName,
                                            cacheMaxAge : discoveryQos.cacheMaxAge
                                        });
                                        expect(globalCapDirSpy.lookup).toHaveBeenCalledWith({
                                            domain : domain,
                                            interfaceName : interfaceName
                                        });
                                    });
                                });

                        it(
                                "calls local capabilities directory according to discoveryQos.discoveryScope LOCAL_ONLY",
                                function() {
                                    discoveryQos.discoveryScope = DiscoveryScope.LOCAL_ONLY;
                                    capabilityDiscovery.lookup(domain, interfaceName, discoveryQos);
                                    expect(localCapStoreSpy.lookup).toHaveBeenCalledWith({
                                        domain : domain,
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
                                        capabilityDiscovery.lookup(domain, interfaceName, discoveryQos);
                                    });

                                    waitsFor(function(){
                                        return globalCapDirSpy.lookup.callCount > 0;
                                    },
                                    "until globalCapDir has been called",
                                    asyncTimeout);

                                    runs(function(){
                                        expect(localCapStoreSpy.lookup).not.toHaveBeenCalled();
                                        expect(globalCapCacheSpy.lookup).toHaveBeenCalledWith({
                                            domain : domain,
                                            interfaceName : interfaceName,
                                            cacheMaxAge : discoveryQos.cacheMaxAge
                                        });
                                        expect(globalCapDirSpy.lookup).toHaveBeenCalledWith({
                                            domain : domain,
                                            interfaceName : interfaceName
                                        });
                                    });
                                });

                        it(
                                "does not call global capabilities directory according to discoveryQos.discoveryScope GLOBAL_ONLY, if global cache is non-empty",
                                function() {
                                    globalCapCacheSpy.lookup.andReturn([ getDiscoverEntry(
                                            domain,
                                            interfaceName)
                                    ]);
                                    discoveryQos.discoveryScope = DiscoveryScope.GLOBAL_ONLY;
                                    capabilityDiscovery.lookup(domain, interfaceName, discoveryQos);
                                    expect(localCapStoreSpy.lookup).not.toHaveBeenCalled();
                                    expect(globalCapCacheSpy.lookup).toHaveBeenCalledWith({
                                        domain : domain,
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
                                            channelId,
                                            "io.joynr");
                            var discoveryQos = new DiscoveryQos({
                                cacheMaxAge : 0,
                                discoveryScope : discoveryScope
                            });

                            runs(function() {
                                capabilityDiscovery.lookup(domain, interfaceName, discoveryQos)
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
                                    expect(onFulfilledSpy)
                                            .toHaveBeenCalledWith(expectedReturnValue);
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
                                            [ globalCapInfos[1]
                                            ],
                                            [],
                                            [ discoveryEntries[1]
                                            ]);
                                    testDiscoveryResult(
                                            "03",
                                            DiscoveryScope.LOCAL_THEN_GLOBAL,
                                            [],
                                            [],
                                            [ globalCapInfos[2]
                                            ],
                                            [ discoveryEntries[2]
                                            ]);
                                    testDiscoveryResult(
                                            "04",
                                            DiscoveryScope.LOCAL_THEN_GLOBAL,
                                            [ discoveryEntries[3]
                                            ],
                                            [],
                                            [ globalCapInfos[3]
                                            ],
                                            [ discoveryEntries[3]
                                            ]);
                                    testDiscoveryResult(
                                            "05",
                                            DiscoveryScope.LOCAL_THEN_GLOBAL,
                                            [ discoveryEntries[3]
                                            ],
                                            [ globalCapInfos[1]
                                            ],
                                            [ globalCapInfos[3]
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
                                            [ globalCapInfos[6]
                                            ],
                                            []);
                                    testDiscoveryResult(
                                            "09",
                                            DiscoveryScope.LOCAL_ONLY,
                                            [],
                                            [ globalCapInfos[5]
                                            ],
                                            [],
                                            []);
                                    testDiscoveryResult(
                                            "10",
                                            DiscoveryScope.LOCAL_ONLY,
                                            [],
                                            [ globalCapInfos[5]
                                            ],
                                            [ globalCapInfos[6]
                                            ],
                                            []);
                                    testDiscoveryResult(
                                            "11",
                                            DiscoveryScope.LOCAL_ONLY,
                                            [ discoveryEntries[7]
                                            ],
                                            [],
                                            [ globalCapInfos[7]
                                            ],
                                            [ discoveryEntries[7]
                                            ]);
                                    testDiscoveryResult(
                                            "12",
                                            DiscoveryScope.LOCAL_ONLY,
                                            [ discoveryEntries[7]
                                            ],
                                            [ globalCapInfos[1]
                                            ],
                                            [ globalCapInfos[7]
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
                                            [ globalCapInfos[9]
                                            ],
                                            [],
                                            [ discoveryEntries[9]
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
                                            [ globalCapInfos[10]
                                            ],
                                            [ globalCapInfos[10]
                                            ],
                                            [ discoveryEntries[10]
                                            ]);
                                    testDiscoveryResult(
                                            "17",
                                            DiscoveryScope.GLOBAL_ONLY,
                                            [],
                                            [],
                                            [ globalCapInfos[10]
                                            ],
                                            [ discoveryEntries[10]
                                            ]);
                                    testDiscoveryResult(
                                            "18",
                                            DiscoveryScope.GLOBAL_ONLY,
                                            [],
                                            [ globalCapInfos[11]
                                            ],
                                            [ globalCapInfos[11]
                                            ],
                                            [ discoveryEntries[11]
                                            ]);
                                    testDiscoveryResult(
                                            "19",
                                            DiscoveryScope.GLOBAL_ONLY,
                                            [ discoveryEntries[10]
                                            ],
                                            [ globalCapInfos[11]
                                            ],
                                            [ globalCapInfos[11]
                                            ],
                                            [ discoveryEntries[11]
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
                                            [ globalCapInfos[1]
                                            ],
                                            [],
                                            [ discoveryEntries[1]
                                            ]);
                                    testDiscoveryResult(
                                            "23",
                                            DiscoveryScope.LOCAL_AND_GLOBAL,
                                            [],
                                            [],
                                            [ globalCapInfos[2]
                                            ],
                                            [ discoveryEntries[2]
                                            ]);
                                    testDiscoveryResult(
                                            "24",
                                            DiscoveryScope.LOCAL_AND_GLOBAL,
                                            [ discoveryEntries[3]
                                            ],
                                            [ globalCapInfos[4]
                                            ],
                                            [],
                                            [
                                                discoveryEntries[3],
                                                discoveryEntries[4]
                                            ]);
                                    testDiscoveryResult(
                                            "25",
                                            DiscoveryScope.LOCAL_AND_GLOBAL,
                                            [ discoveryEntries[3]
                                            ],
                                            [],
                                            [ globalCapInfos[4]
                                            ],
                                            [
                                                discoveryEntries[3],
                                                discoveryEntries[4]
                                            ]);
                                    testDiscoveryResult(
                                            "26",
                                            DiscoveryScope.LOCAL_AND_GLOBAL,
                                            [ discoveryEntries[3]
                                            ],
                                            [ globalCapInfos[1]
                                            ],
                                            [ globalCapInfos[3]
                                            ],
                                            [
                                                discoveryEntries[3],
                                                discoveryEntries[1]
                                            ]);
                                });

                        function getDiscoveryEntryWithScope(scope) {
                            return new DiscoveryEntry({
                                domain : "domain",
                                interfaceName : "interfaceName",
                                qos : new ProviderQos({
                                    customParameters : [ new CustomParameter({
                                        name : "theName",
                                        value : "theValue"
                                    })
                                    ],
                                    providerVersion : 123,
                                    priority : 1234,
                                    scope : scope,
                                    onChangeSubscription : true
                                }),
                                participantId : "700"
                            });
                        }

                        function getCapInfoWithScope(scope) {
                            return new CapabilityInformation({
                                domain : "domain",
                                interfaceName : "interfaceName",
                                providerQos : new ProviderQos({
                                    customParameters : [ new CustomParameter({
                                        name : "theName",
                                        value : "theValue"
                                    })
                                    ],
                                    providerVersion : 123,
                                    priority : 1234,
                                    scope : scope,
                                    onChangeSubscription : true
                                }),
                                channelId : channelId,
                                participantId : "700"
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
                                        expect(
                                                globalCapDirSpy.add.calls[0].args[0].capability.domain)
                                                .toEqual(discoveryEntry.domain);
                                        expect(
                                                globalCapDirSpy.add.calls[0].args[0].capability.interfaceName)
                                                .toEqual(discoveryEntry.interfaceName);
                                        expect(
                                                globalCapDirSpy.add.calls[0].args[0].capability.participantId)
                                                .toEqual(discoveryEntry.participantId);
                                        expect(
                                                globalCapDirSpy.add.calls[0].args[0].capability.providerQos)
                                                .toEqual(discoveryEntry.qos);
                                        expect(
                                                globalCapDirSpy.add.calls[0].args[0].capability.channelId)
                                                .toEqual(channelId);
                                        expect(localCapStoreSpy.add).not.toHaveBeenCalledWith();
                                        expect(spy.onFulfilled).toHaveBeenCalled();
                                        expect(spy.onRejected).not.toHaveBeenCalledWith();
                                    });
                                });

                        it(
                                "reports error from global cap dir",
                                function() {
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
                                        expect(globalCapDirSpy.add).toHaveBeenCalledWith({
                                            capability : getCapInfoWithScope(ProviderScope.GLOBAL)
                                        });
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

                    });

        }); // require
