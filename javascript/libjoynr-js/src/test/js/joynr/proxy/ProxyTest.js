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
var Promise = require('../../../classes/global/Promise');
var RadioProxy = require('../../../test-classes/joynr/vehicle/RadioProxy');
var RadioStation = require('../../../test-classes/joynr/vehicle/radiotypes/RadioStation');
var ProxyAttribute = require('../../../classes/joynr/proxy/ProxyAttribute');
var ProxyOperation = require('../../../classes/joynr/proxy/ProxyOperation');
var ProxyEvent = require('../../../classes/joynr/proxy/ProxyEvent');
var TypeRegistrySingleton = require('../../../classes/joynr/types/TypeRegistrySingleton');
var DiscoveryQos = require('../../../classes/joynr/proxy/DiscoveryQos');
var MessagingQos = require('../../../classes/joynr/messaging/MessagingQos');
var TestWithVersionProxy = require('../../../test-classes/joynr/tests/TestWithVersionProxy');
var TestWithoutVersionProxy = require('../../../test-classes/joynr/tests/TestWithoutVersionProxy');

            describe(
                    "libjoynr-js.joynr.proxy.Proxy",
                    function() {

                        var settings, dependencies, radioProxy;
                        var typeRegistry = TypeRegistrySingleton.getInstance();

                        beforeEach(function(done) {
                            settings = {
                                domain : "",
                                interfaceName : "",
                                discoveryQos : new DiscoveryQos(),
                                messagingQos : new MessagingQos(),
                                proxyElementTypes : {
                                    ProxyAttribute : ProxyAttribute,
                                    ProxyOperation : ProxyOperation,
                                    ProxyEvent : ProxyEvent
                                },
                                dependencies : {
                                    subscriptionManager : {}
                                }
                            };
                            radioProxy = new RadioProxy(settings);
                            done();
                        });

                        it("version is set correctly", function(done) {
                            expect(TestWithVersionProxy.MAJOR_VERSION).toBeDefined();
                            expect(TestWithVersionProxy.MAJOR_VERSION).toEqual(47);
                            expect(TestWithVersionProxy.MINOR_VERSION).toBeDefined();
                            expect(TestWithVersionProxy.MINOR_VERSION).toEqual(11);
                            done();
                        });

                        it("default version is set correctly", function(done) {
                            expect(TestWithoutVersionProxy.MAJOR_VERSION).toBeDefined();
                            expect(TestWithoutVersionProxy.MAJOR_VERSION).toEqual(0);
                            expect(TestWithoutVersionProxy.MINOR_VERSION).toBeDefined();
                            expect(TestWithoutVersionProxy.MINOR_VERSION).toEqual(0);
                            done();
                        });

                        it("RadioProxy is instantiable", function(done) {
                            expect(radioProxy).toBeDefined();
                            expect(radioProxy).not.toBeNull();
                            expect(typeof radioProxy === "object").toBeTruthy();
                            expect(radioProxy instanceof RadioProxy).toBeTruthy();
                            done();
                        });

                        it("RadioProxy provides API to access used datatypes", function(done) {
                            expect(RadioProxy.getUsedDatatypes).toBeDefined();
                            done();
                        });

                        it(
                                "RadioProxy.getUsedDatatype can be used to synchronize to the successful registration of all used datatypes",
                                function(done) {
                                    var datatypePromises;
                                    var allDatatypesRegistered;
                                    allDatatypesRegistered = false;
                                    expect(RadioProxy.getUsedDatatypes).toBeDefined();
                                    datatypePromises =
                                            RadioProxy.getUsedDatatypes().map(
                                                    function(datatype) {
                                                        return typeRegistry
                                                                .getTypeRegisteredPromise(
                                                                        datatype,
                                                                        1000);
                                                    });
                                    Promise.all(datatypePromises).then(function() {
                                        done();
                                        return null;
                                    }).catch(function(error) {
                                        fail("failed to register all datatypes at the typeRegistry");
                                        return null;
                                    });
                                });

                        it("RadioProxy saves settings object", function(done) {
                            expect(radioProxy.settings).toEqual(settings);
                            done();
                        });

                        it("RadioProxy has all members", function(done) {
                            expect(radioProxy.isOn).toBeDefined();
                            expect(radioProxy.addFavoriteStation).toBeDefined();
                            expect(typeof radioProxy.addFavoriteStation === "function")
                                    .toBeTruthy();
                            expect(radioProxy.weakSignal).toBeDefined();
                            expect(radioProxy.weakSignal instanceof ProxyEvent).toBeTruthy();
                            done();
                        });

                    });
