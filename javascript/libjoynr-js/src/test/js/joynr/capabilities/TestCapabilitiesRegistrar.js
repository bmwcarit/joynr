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

//TODO: some of this relies on the dummy implementation, change accordingly when implementating
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
                        var authToken;
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

                        beforeEach(function() {

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

                            spyOn(provider, "checkImplementation").andCallThrough();

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
                            authToken = "authToken";
                            participantId = "myParticipantId";
                            participantIdStorageSpy =
                                    jasmine.createSpyObj(
                                            "participantIdStorage",
                                            [ "getParticipantId"
                                            ]);
                            participantIdStorageSpy.getParticipantId.andReturn(participantId);
                            requestReplyManagerSpy =
                                    jasmine.createSpyObj(
                                            "RequestReplyManager",
                                            [ "addRequestCaller"
                                            ]);
                            discoveryStubSpy = jasmine.createSpyObj("discoveryStub", [ "add"
                            ]);
                            discoveryStubSpy.add.andReturn(Promise.resolve());
                            messageRouterSpy = jasmine.createSpyObj("messageRouter", [ "addNextHop"
                            ]);

                            messageRouterSpy.addNextHop.andReturn(Promise.resolve());
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
                        });

                        it("is instantiable", function() {
                            expect(capabilitiesRegistrar).toBeDefined();
                            expect(capabilitiesRegistrar instanceof CapabilitiesRegistrar)
                                    .toBeTruthy();
                        });

                        it("is has all members", function() {
                            expect(capabilitiesRegistrar.registerProvider).toBeDefined();
                            expect(typeof capabilitiesRegistrar.registerProvider === "function")
                                    .toBeTruthy();
                        });

                        it("is checks the provider's implementation", function() {
                            capabilitiesRegistrar.registerProvider(
                                    domain,
                                    provider,
                                    providerQos);
                            expect(provider.checkImplementation).toHaveBeenCalled();
                        });

                        it(
                                "is checks the provider's implementation, and throws if incomplete",
                                function() {
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

                                });

                        it("fetches participantId from the participantIdStorage", function() {
                            capabilitiesRegistrar.registerProvider(
                                    domain,
                                    provider,
                                    providerQos);
                            expect(participantIdStorageSpy.getParticipantId).toHaveBeenCalled();
                            expect(participantIdStorageSpy.getParticipantId).toHaveBeenCalledWith(
                                    domain,
                                    provider);
                        });

                        it("registers next hop with routing table", function() {
                            capabilitiesRegistrar.registerProvider(
                                    domain,
                                    provider,
                                    providerQos);
                            expect(messageRouterSpy.addNextHop).toHaveBeenCalled();
                            expect(messageRouterSpy.addNextHop).toHaveBeenCalledWith(
                                    participantId,
                                    libjoynrMessagingAddress);
                        });

                        it("registers provider at RequestReplyManager", function() {
                            capabilitiesRegistrar.registerProvider(
                                    domain,
                                    provider,
                                    providerQos);
                            expect(requestReplyManagerSpy.addRequestCaller).toHaveBeenCalled();
                            expect(requestReplyManagerSpy.addRequestCaller).toHaveBeenCalledWith(
                                    participantId,
                                    provider);
                        });

                        it(
                                "registers a provider with PublicationManager if it has an attribute",
                                function() {
                                    capabilitiesRegistrar.registerProvider(
                                            domain,
                                            provider,
                                            providerQos);
                                    expect(publicationManagerSpy.addPublicationProvider)
                                            .toHaveBeenCalled();
                                    expect(publicationManagerSpy.addPublicationProvider)
                                            .toHaveBeenCalledWith(participantId, provider);
                                });

                        it("registers capability at capabilities stub", function() {
                            var actualDiscoveryEntry;
                            var upperBound;
                            var lowerBound = Date.now();
                            capabilitiesRegistrar.registerProvider(
                                    domain,
                                    provider,
                                    providerQos);
                            upperBound = Date.now();
                            expect(discoveryStubSpy.add).toHaveBeenCalled();
                            actualDiscoveryEntry = discoveryStubSpy.add.calls[0].args[0];
                            expect(actualDiscoveryEntry.domain).toEqual(domain);
                            expect(actualDiscoveryEntry.interfaceName).toEqual(provider.interfaceName);
                            expect(actualDiscoveryEntry.participantId).toEqual(participantId);
                            expect(actualDiscoveryEntry.qos).toEqual(providerQos);
                            expect(actualDiscoveryEntry.lastSeenDateMs).not.toBeLessThan(lowerBound);
                            expect(actualDiscoveryEntry.lastSeenDateMs).not.toBeGreaterThan(upperBound);
                            expect(actualDiscoveryEntry.providerVersion.majorVersion).toEqual(provider.constructor.MAJOR_VERSION);
                            expect(actualDiscoveryEntry.providerVersion.minorVersion).toEqual(provider.constructor.MINOR_VERSION);
                        });

                        it("registers logging context with the ContextManager", function() {
                            var loggingContext = {
                                myContext : "myContext"
                            };
                            capabilitiesRegistrar.registerProvider(
                                    domain,
                                    provider,
                                    providerQos,
                                    loggingContext);
                            expect(loggingManagerSpy.setLoggingContext).toHaveBeenCalled();
                            expect(loggingManagerSpy.setLoggingContext).toHaveBeenCalledWith(
                                    participantId,
                                    loggingContext);
                        });

                        it("returns the promise success from capabilites stub", function() {
                            var spy = jasmine.createSpyObj("spy", [
                                "onFulfilled",
                                "onRejected"
                            ]);

                            runs(function() {
                                capabilitiesRegistrar.registerProvider(
                                        domain,
                                        provider,
                                        providerQos).then(spy.onFulfilled).catch(spy.onRejected);
                            });

                            waitsFor(function() {
                                return spy.onFulfilled.callCount > 0;
                            }, "onFulfilled spy to be invoked", 100);

                            runs(function() {
                                expect(spy.onFulfilled).toHaveBeenCalled();
                                expect(spy.onFulfilled).toHaveBeenCalledWith([
                                    undefined,
                                    undefined
                                ]);
                                expect(spy.onRejected).not.toHaveBeenCalled();
                            });
                        });

                        it(
                                "returns the promise onRejected from capabilites stub",
                                function() {
                                    var spy = jasmine.createSpyObj("spy", [
                                        "onFulfilled",
                                        "onRejected"
                                    ]);
                                    discoveryStubSpy.add.andReturn(Promise.reject(new Error("Some error.")));

                                    runs(function() {
                                        capabilitiesRegistrar.registerProvider(
                                                domain,
                                                provider,
                                                providerQos).then(spy.onFulfilled).catch(spy.onRejected);
                                    });

                                    waitsFor(function() {
                                        return spy.onRejected.callCount > 0;
                                    }, "onRejected spy to be called", 100);

                                    runs(function() {
                                        expect(spy.onFulfilled).not.toHaveBeenCalled();
                                        expect(spy.onRejected).toHaveBeenCalled();
                                        expect(
                                                Object.prototype.toString
                                                        .call(spy.onRejected.mostRecentCall.args[0]) === "[object Error]")
                                                .toBeTruthy();
                                    });
                                });

                        it("passes calls to deprecated registerCapabilitiy to registerProvider without authToken", function() {
                            spyOn(capabilitiesRegistrar, "registerProvider");
                            capabilitiesRegistrar.registerCapability(
                                    authToken,
                                    domain,
                                    provider,
                                    providerQos);
                            expect(capabilitiesRegistrar.registerProvider).toHaveBeenCalledWith(domain, provider, providerQos);
                        });
                    });

        }); // require
