/*global joynrTestRequire: true */
/*jslint es5: true, nomen: true */

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

joynrTestRequire(
        "integration/TestProxyErrors",
        [
            "global/Promise",
            "joynr",
            "joynr/provisioning/provisioning_cc"
        ],
        function(
                Promise,
                joynr,
                provisioning) {

            var domain = "domain";

            var TestVersionProvider = function TestVersionProvider() {
                this.checkImplementation = function() {
                    return true;
                };
                this.interfaceName = "tests/proxy/Version";
                this.id = 1;
                return Object.freeze(this);
            };

            TestVersionProvider.MAJOR_VERSION = 1;
            TestVersionProvider.MINOR_VERSION = 0;

            var TestVersionProxy = function TestVersionProxy() {
                this.checkImplementation = function() {
                    return true;
                };
                this.interfaceName = "tests/proxy/Version";
                this.id = 2;
                return this;
            };
            TestVersionProxy.getUsedDatatypes = function getUsedDatatypes(){
                return [];
            };
            TestVersionProxy.MAJOR_VERSION = 2;
            TestVersionProxy.MINOR_VERSION = 0;

            describe(
                    "libjoynr-js.integration.proxy.errors",
                    function() {
                        beforeEach(function() {
                            var provider;
                            var initialized = false;
                            var testProvisioning = null;
                            var providerQos;
                            runs(function() {
                                joynr.load(provisioning).then(function(newjoynr) {
                                    joynr = newjoynr;
                                    providerQos = new joynr.types.ProviderQos({
                                        scope : joynr.types.ProviderScope.LOCAL
                                    });
                                    initialized = true;
                                }).then(function() {
                                    provider = joynr.providerBuilder.build(TestVersionProvider, {});
                                    joynr.registration.registerProvider(domain, provider, providerQos);
                                });
                            });
                            waitsFor(function() {
                                return initialized === true;
                            }, "joynr to be created", 5000);
                        });

                        it("throws a NoCompatibleProviderFoundException if provider version does not match proxy version", function() {
                            var finished = false;
                            var caughtError;
                            runs(function() {
                                joynr.proxyBuilder.build(TestVersionProxy, {
                                    domain : domain,
                                    messagingQos : new joynr.messaging.MessagingQos(),
                                    discoveryQos : new joynr.proxy.DiscoveryQos({
                                        discoveryTimeoutMs : 100
                                    })
                                }).then(function() {
                                    throw("the proxy builder should fail with an error");
                                }).catch(function(error) {
                                    caughtError = error;
                                    finished = true;
                                });
                            });
                            waitsFor(function() {
                                return finished === true;
                            }, "exception was caught", 5000);
                            runs(function() {
                                expect(caughtError instanceof Error).toBeTruthy();
                                expect(caughtError instanceof joynr.exceptions.NoCompatibleProviderFoundException).toBeTruthy();
                                expect(caughtError.discoveredVersions.length).toEqual(1);
                                expect(caughtError.discoveredVersions[0].majorVersion).toEqual(1);
                                expect(caughtError.discoveredVersions[0].minorVersion).toEqual(0);
                            });
                        });

                        afterEach(function() {
                            var shutdown;
                            runs(function() {
                                joynr.shutdown().then(function() {
                                    shutdown = true;
                                });
                            });
                            waitsFor(function() {
                                return shutdown;
                            }, "libjoynr to be shut down", 5000);
                        });
                    });
        });
