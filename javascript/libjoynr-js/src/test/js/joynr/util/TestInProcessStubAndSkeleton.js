/*global joynrTestRequire: true */

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
        "joynr/util/TestInProcessStubAndSkeleton",
        [
            "joynr/types/CapabilityInformation",
            "joynr/types/ProviderQos",
            "joynr/types/ProviderScope",
            "joynr/types/CustomParameter",
            "joynr/types/Version",
            "joynr/messaging/inprocess/InProcessAddress",
            "joynr/util/InProcessStub",
            "joynr/util/InProcessSkeleton"
        ],
        function(
                CapabilityInformation,
                ProviderQos,
                ProviderScope,
                CustomParameter,
                Version,
                InProcessAddress,
                InProcessStub,
                InProcessSkeleton) {

            describe("libjoynr-js.joynr.util.InProcessStubAndSkeleton", function() {
                it("InProcessSkeleton is instantiable", function() {
                    expect(new InProcessSkeleton()).toBeDefined();
                    expect(new InProcessSkeleton() instanceof InProcessSkeleton).toBeTruthy();
                });
            });

            describe("libjoynr-js.joynr.util.InProcessStubAndSkeleton", function() {
                it("InProcessStub is instantiable", function() {
                    expect(new InProcessStub(new InProcessSkeleton())).toBeDefined();
                    expect(new InProcessStub(new InProcessSkeleton()) instanceof InProcessStub)
                            .toBeTruthy();
                });
            });

            var capability = {
                discoveryEntry : new CapabilityInformation({
                    providerVersion : new Version({
                        majorVersion : 47,
                        minorVersion : 11
                    }),
                    domain : "KeywordmyDomain",
                    interfaceName : "myInterfaceName",
                    providerQos : new ProviderQos({
                        customParameters : [ new CustomParameter({
                            name : "theName",
                            value : "theValue"
                        })
                        ],
                        priority : 1,
                        scope : ProviderScope.LOCAL,
                        supportsOnChangeSubscriptions : true
                    }),
                    channelId : "InProcessAddress",
                    participantId : "1"
                })
            };

            var arrayOfCapabilities = {
                discoveryEntries : [
                    new CapabilityInformation({
                        providerVersion : new Version({
                            majorVersion : 47,
                            minorVersion : 11
                        }),
                        domain : "KeywordmyDomain",
                        interfaceName : "myInterfaceName",
                        providerQos : new ProviderQos({
                            customParameters : [ new CustomParameter({
                                name : "theName",
                                value : "theValue"
                            })
                            ],
                            priority : 1,
                            scope : ProviderScope.LOCAL,
                            supportsOnChangeSubscriptions : true
                        }),
                        channelId : "InProcessAddress",
                        participantId : "1"
                    }),
                    new CapabilityInformation({
                        providerVersion : new Version({
                            majorVersion : 47,
                            minorVersion : 11
                        }),
                        domain : "myDomain",
                        interfaceName : "myInterfaceName",
                        providerQos : new ProviderQos({
                            customParameters : [ new CustomParameter({
                                name : "theName",
                                value : "theValue"
                            })
                            ],
                            priority : 4,
                            scope : ProviderScope.LOCAL,
                            supportsOnChangeSubscriptions : true
                        }),
                        channelId : "InProcessAddress",
                        participantId : "1"
                    }),
                    new CapabilityInformation({
                        providerVersion : new Version({
                            majorVersion : 47,
                            minorVersion : 11
                        }),
                        domain : "myWithKeywordDomain",
                        interfaceName : "myInterfaceName",
                        providerQos : new ProviderQos({
                            customParameters : [ new CustomParameter({
                                name : "theName",
                                value : "theValue"
                            })
                            ],
                            priority : 3,
                            scope : ProviderScope.LOCAL,
                            supportsOnChangeSubscriptions : true
                        }),
                        channelId : "InProcessAddress",
                        participantId : "1"
                    }),
                    new CapabilityInformation({
                        providerVersion : new Version({
                            majorVersion : 47,
                            minorVersion : 11
                        }),
                        domain : "myDomain",
                        interfaceName : "myInterfaceNameKeyword",
                        providerQos : new ProviderQos({
                            customParameters : [ new CustomParameter({
                                name : "theName",
                                value : "theValue"
                            })
                            ],
                            priority : 5,
                            scope : ProviderScope.LOCAL,
                            supportsOnChangeSubscriptions : true
                        }),
                        channelId : "InProcessAddress",
                        participantId : "1"
                    }),
                    new CapabilityInformation({
                        providerVersion : new Version({
                            majorVersion : 47,
                            minorVersion : 11
                        }),
                        domain : "myDomain",
                        interfaceName : "myInterfaceName",
                        providerQos : new ProviderQos({
                            customParameters : [ new CustomParameter({
                                name : "theName",
                                value : "theValue"
                            })
                            ],
                            priority : 2,
                            scope : ProviderScope.LOCAL,
                            supportsOnChangeSubscriptions : true
                        }),
                        channelId : "InProcessAddress",
                        participantId : "1"
                    })
                ]
            };

            var providerQos = new ProviderQos({
                qos : [ {
                    name : "theName",
                    value : "theValue",
                    type : "QosString"
                }
                ],
                version : 123,
                priority : 1234
            });

            var myDomain = "myDomain";

            var myInterfaceName = "myInterfaceName";

            var myChannelId = "myChannelId";

            var participantId = {
                participandId : "participantId"
            };

            var participantIds = {
                participantIds : [
                    participantId,
                    "participantId2"
                ]
            };

            describe(
                    "libjoynr-js.joynr.util.InProcessStubAndSkeleton",
                    function() {
                        function check(stub, spy) {
                            var lookupTest = {
                                domain : myDomain,
                                interfaceName : myInterfaceName,
                                providerQos : providerQos
                            };
                            // make calls on the stub
                            stub.remove(participantId);
                            stub.remove(participantIds);
                            stub.add(capability);
                            stub.add(arrayOfCapabilities);
                            stub.lookup(lookupTest);
                            stub.lookup(participantId);

                            // check if calls are going through to the mocked object
                            expect(spy.remove.calls[0].args[0]).toEqual(participantId);
                            expect(spy.remove.calls[1].args[0]).toEqual(participantIds);
                            expect(spy.add.calls[0].args[0]).toEqual(capability);
                            expect(spy.add.calls[1].args[0]).toEqual(arrayOfCapabilities);
                            expect(spy.lookup.calls[0].args[0]).toEqual(lookupTest);
                            expect(spy.lookup.calls[1].args[0]).toEqual(participantId);
                        }

                        var spy;
                        beforeEach(function() { // create mock object for capabilities directory
                            spy = jasmine.createSpyObj("capabilitiesDirectory", [
                                "remove",
                                "add",
                                "lookup"
                            ]);
                        });

                        it(
                                "all methods get called through stub and skeleton correctly",
                                function() {
                                    var stub = new InProcessStub(new InProcessSkeleton(spy));
                                    check(stub, spy);
                                });

                        it(
                                "all methods get called through stub and skeleton correctly with late initialization",
                                function() {
                                    var stub = new InProcessStub();
                                    stub.setSkeleton(new InProcessSkeleton(spy));
                                    check(stub, spy);
                                });

                        it(
                                "all methods get called through stub and skeleton correctly after overwrite",
                                function() {
                                    var stub = new InProcessStub();
                                    stub.setSkeleton(new InProcessSkeleton({
                                        someKey : "someValue"
                                    }));
                                    stub.setSkeleton(new InProcessSkeleton(spy));
                                    check(stub, spy);
                                });

                        var objects = [
                            {
                                key : "value"
                            },
                            0,
                            -1,
                            1234,
                            "",
                            "a strting",
                            new InProcessStub(new InProcessSkeleton({}))
                        ];

                        function testValue(obj) {
                            expect(function() {
                                var o = new InProcessStub(obj);
                            }).toThrow();
                            expect(function() {
                                var o = new InProcessStub().setSkeleton(obj);
                            }).toThrow();
                        }

                        it(
                                "throws when Stub receives object which is not of type InProcessSkeleton",
                                function() {
                                    var i;
                                    for (i = 0; i < objects.length; ++i) {
                                        testValue(objects[i]);
                                    }
                                    expect(function() {
                                        var o = new InProcessStub();
                                    }).not.toThrow();
                                });

                        it("throws when inProcessSkeleton is undefined or null ", function() {
                            expect(function() {
                                new InProcessStub().setSkeleton(undefined);
                            }).toThrow();
                            expect(function() {
                                new InProcessStub().setSkeleton(null);
                            }).toThrow();
                        });

                    });

        }); // require
