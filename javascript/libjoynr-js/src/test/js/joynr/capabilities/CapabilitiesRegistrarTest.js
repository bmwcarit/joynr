/*jslint es5: true */
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

define([
            "global/Promise",
            "joynr/capabilities/CapabilitiesRegistrar",
            "joynr/types/ProviderQos",
            "joynr/types/GlobalDiscoveryEntry",
            "joynr/provider/ProviderAttributeNotifyReadWrite",
            "joynr/types/DiscoveryEntry",
            "joynr/types/ProviderScope",
            "joynr/types/Version",
            "uuid",
        ],
        function(
                Promise,
                CapabilitiesRegistrar,
                ProviderQos,
                GlobalDiscoveryEntry,
                ProviderAttributeNotifyReadWrite,
                DiscoveryEntry,
                ProviderScope,
                Version,
                uuid) {
            describe(
                    "libjoynr-js.joynr.capabilities.CapabilitiesRegistrar",
                    function() {
                        var capabilitiesRegistrar;
                        var requestReplyManagerSpy;
                        var publicationManagerSpy;
                        var participantId;
                        var domain;
                        var participantIdStorageSpy;
                        var discoveryStubSpy;
                        var messageRouterSpy;
                        var loggingManagerSpy;
                        var libjoynrMessagingAddress;
                        var provider;
                        var capability;
                        var localChannelId;
                        var providerQos;
                        var address;
                        var checkImplementation;
                        var TestProvider;

                        beforeEach(function(done) {

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

                            providerQos =
                                    new ProviderQos([], 1, Date.now(), ProviderScope.GLOBAL, true);

                            provider.myAttribute = new ProviderAttributeNotifyReadWrite(provider, {
                                dependencies : {
                                    publicationManager : publicationManagerSpy
                                }
                            }, "myAttribute", "Boolean");

                            localChannelId = "localChannelId";
                            domain = "testdomain";
                            address = "address";
                            participantId = "myParticipantId";
                            participantIdStorageSpy =
                                    jasmine.createSpyObj(
                                            "participantIdStorage",
                                            [ "getParticipantId"
                                            ]);
                            participantIdStorageSpy.getParticipantId.and.returnValue(participantId);
                            requestReplyManagerSpy =
                                    jasmine.createSpyObj(
                                            "RequestReplyManager",
                                            [ "addRequestCaller"
                                            ]);
                            discoveryStubSpy = jasmine.createSpyObj("discoveryStub", [ "add"
                            ]);
                            discoveryStubSpy.add.and.returnValue(Promise.resolve());
                            messageRouterSpy = jasmine.createSpyObj("messageRouter", [ "addNextHop"
                            ]);

                            messageRouterSpy.addNextHop.and.returnValue(Promise.resolve());
                            libjoynrMessagingAddress = {
                                "someKey" : "someValue",
                                "toBe" : "a",
                                "object" : {}
                            };
                            loggingManagerSpy =
                                    jasmine.createSpyObj("loggingManager", [ "setLoggingContext"
                                    ]);

                            capabilitiesRegistrar = new CapabilitiesRegistrar({
                                discoveryStub : discoveryStubSpy,
                                messageRouter : messageRouterSpy,
                                participantIdStorage : participantIdStorageSpy,
                                libjoynrMessagingAddress : libjoynrMessagingAddress,
                                requestReplyManager : requestReplyManagerSpy,
                                publicationManager : publicationManagerSpy,
                                loggingManager : loggingManagerSpy
                            });

                            capability = new GlobalDiscoveryEntry({
                                providerVersion : new Version({ majorVersion: 47, minorVersion: 11}),
                                domain : domain,
                                interfaceName : provider.interfaceName,
                                qos : providerQos,
                                channelId : localChannelId,
                                participantId : participantId,
                                publicKeyId: "",
                                address : address
                            });
                            done();
                        });

                        it("is instantiable", function(done) {
                            expect(capabilitiesRegistrar).toBeDefined();
                            expect(capabilitiesRegistrar instanceof CapabilitiesRegistrar)
                                    .toBeTruthy();
                            done();
                        });

                        it("is has all members", function(done) {
                            expect(capabilitiesRegistrar.registerProvider).toBeDefined();
                            expect(typeof capabilitiesRegistrar.registerProvider === "function")
                                    .toBeTruthy();
                            done();
                        });

                        it("is checks the provider's implementation", function(done) {
                            capabilitiesRegistrar.registerProvider(
                                    domain,
                                    provider,
                                    providerQos).then(function() {
                                return null;
                            }).catch(function() {
                                return null;
                            });
                            expect(provider.checkImplementation).toHaveBeenCalled();
                            done();
                        });

                        it(
                                "is checks the provider's implementation, and throws if incomplete",
                                function(done) {
                                    provider.checkImplementation = function() {
                                        return [ "Operation:addFavoriteStation"
                                        ];
                                    };

                                    expect(
                                            function() {
                                                capabilitiesRegistrar.registerProvider(
                                                        domain,
                                                        provider,
                                                        providerQos);
                                            }).toThrow(
                                            new Error("provider: "
                                                + domain
                                                + "/"
                                                + provider.interfaceName
                                                + " is missing: Operation:addFavoriteStation"));
                                    done();
                                });

                        it("fetches participantId from the participantIdStorage", function(done) {
                            capabilitiesRegistrar.registerProvider(
                                    domain,
                                    provider,
                                    providerQos).then(function() {
                               return null;
                            }).catch(function() {
                               return null;
                            });
                            expect(participantIdStorageSpy.getParticipantId).toHaveBeenCalled();
                            expect(participantIdStorageSpy.getParticipantId).toHaveBeenCalledWith(
                                    domain,
                                    provider);
                            done();
                        });

                        it("registers next hop with routing table", function(done) {
                            capabilitiesRegistrar.registerProvider(
                                    domain,
                                    provider,
                                    providerQos).then(function() {
                               return null;
                            }).catch(function() {
                               return null;
                            });
                            expect(messageRouterSpy.addNextHop).toHaveBeenCalled();
                            expect(messageRouterSpy.addNextHop).toHaveBeenCalledWith(
                                    participantId,
                                    libjoynrMessagingAddress);
                            done();
                        });

                        it("registers provider at RequestReplyManager", function(done) {
                            capabilitiesRegistrar.registerProvider(
                                    domain,
                                    provider,
                                    providerQos).then(function() {
                               return null;
                            }).catch(function() {
                               return null;
                            });
                            expect(requestReplyManagerSpy.addRequestCaller).toHaveBeenCalled();
                            expect(requestReplyManagerSpy.addRequestCaller).toHaveBeenCalledWith(
                                    participantId,
                                    provider);
                            done();
                        });

                        it(
                                "registers a provider with PublicationManager if it has an attribute",
                                function(done) {
                                    capabilitiesRegistrar.registerProvider(
                                            domain,
                                            provider,
                                            providerQos).then(function() {
                                        return null;
                                    }).catch(function() {
                                        return null;
                                    });
                                    expect(publicationManagerSpy.addPublicationProvider)
                                            .toHaveBeenCalled();
                                    expect(publicationManagerSpy.addPublicationProvider)
                                            .toHaveBeenCalledWith(participantId, provider);
                                    done();
                                });

                        it("registers capability at capabilities stub", function(done) {
                            var actualDiscoveryEntry;
                            var upperBound;
                            var lowerBound = Date.now();
                            capabilitiesRegistrar.registerProvider(
                                    domain,
                                    provider,
                                    providerQos).then(function() {
                                return null;
                            }).catch(function() {
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

                        it("registers logging context with the ContextManager", function(done) {
                            var expiryDateMs = -1;
                            var loggingContext = {
                                myContext : "myContext"
                            };
                            capabilitiesRegistrar.registerProvider(
                                    domain,
                                    provider,
                                    providerQos,
                                    expiryDateMs,
                                    loggingContext).then(function() {
                                return null;
                            }).catch(function() {
                                return null;
                            });
                            expect(loggingManagerSpy.setLoggingContext).toHaveBeenCalled();
                            expect(loggingManagerSpy.setLoggingContext).toHaveBeenCalledWith(
                                    participantId,
                                    loggingContext);
                            done();
                        });

                        it("returns the provider participant ID", function(done) {
                            capabilitiesRegistrar.registerProvider(
                                    domain,
                                    provider,
                                    providerQos).then(function(result) {
                                        expect(result).toEqual(participantId);
                                        done();
                                        return null;
                                    }).catch(function(error) {
                                        fail("unexpected error: " + error);
                                        return null;
                                    });
                        });

                        it(
                                "returns the promise onRejected from capabilites stub",
                                function(done) {
                                    discoveryStubSpy.add.and.returnValue(Promise.reject(new Error("Some error.")));

                                    capabilitiesRegistrar.registerProvider(
                                            domain,
                                            provider,
                                            providerQos).then(function() {
                                                fail("expected an error");
                                                return null;
                                            }).catch(function(error) {
                                                expect(
                                                        Object.prototype.toString
                                                        .call(error) === "[object Error]")
                                                        .toBeTruthy();
                                                done();
                                                return null;
                                            });
                        });

                        it(
                                "CapabilitiesRegistrar throws exception when called while shut down",
                                function(done) {
                                    capabilitiesRegistrar.shutdown();
                                    expect(function() {
                                        capabilitiesRegistrar.registerProvider(
                                                domain,
                                                provider,
                                                providerQos);
                                    }).toThrow();
                                    expect(function() {
                                        capabilitiesRegistrar.unregisterProvider(domain, provider);
                                    }).toThrow();
                                    done();
                                });

                    });

        }); // require
