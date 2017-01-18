/*jslint es5: true */
/*global fail: true */

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
    "joynr",
    "integration/IntegrationUtils",
    "joynr/provisioning/provisioning_cc",
    "integration/provisioning_end2end_common"
], function(joynr, IntegrationUtils, provisioning, provisioning_end2end) {
    describe("libjoynr-js.libjoynr", function() {
        var workerId;
        var testProvisioning = null;
        var provisioningSuffix;
        var ready;

        function testMutability(object, member, mutable) {
            var testObj;
            var oldValue;

            testObj = {
                myKey : "myValue"
            };
            oldValue = object[member];
            if (oldValue === undefined) {
                throw new Error("object to be tested for mutability is undefined: " + member);
            }
            object[member] = testObj;
            if (mutable) {
                expect(object[member]).toEqual(testObj);
                // restore old value
                object[member] = oldValue;
            } else {
                expect(object[member]).toEqual(oldValue);
            }
        }

        function shutdownWebWorkerAndLibJoynr() {
            return IntegrationUtils.shutdownWebWorker(workerId).then(IntegrationUtils.shutdownLibjoynr);
        }

        beforeEach(function(done) {
            provisioningSuffix = "LibJoynrTest-" + Date.now();
            testProvisioning = IntegrationUtils.getProvisioning(provisioning, provisioningSuffix);
            joynr.load(testProvisioning).then(function(newJoynr) {
                joynr = newJoynr;
                IntegrationUtils.initialize(joynr);
                done();
                return null;
            });
        });

        afterEach(function(done) {
            shutdownWebWorkerAndLibJoynr().then(function() {
                done();
                return null;
            }).catch(fail);
        });

        it("joynr namespace is available", function(done) {
            expect(joynr).toBeDefined();
            done();
        });

        it("public modules are available", function(done) {
            expect(joynr.proxy.PeriodicSubscriptionQos).toBeDefined();
            expect(joynr.proxy.OnChangeWithKeepAliveSubscriptionQos).toBeDefined();
            expect(joynr.proxy.OnChangeSubscriptionQos).toBeDefined();
            expect(joynr.messaging.MessagingQos).toBeDefined();
            expect(joynr.proxy.DiscoveryQos).toBeDefined();
            expect(joynr.types.ArbitrationStrategyCollection).toBeDefined();
            done();
        });

        it("public namespaces are immutable", function(done) {
            testMutability(joynr, "proxy");
            testMutability(joynr.proxy, "PeriodicSubscriptionQos");
            testMutability(joynr.proxy, "OnChangeWithKeepAliveSubscriptionQos");
            testMutability(joynr.proxy, "OnChangeSubscriptionQos");
            testMutability(joynr, "messaging");
            testMutability(joynr.messaging, "MessagingQos");
            testMutability(joynr, "types");
            testMutability(joynr.proxy, "DiscoveryQos");
            testMutability(joynr.types, "ProviderQos");
            testMutability(joynr.types, "ArbitrationStrategyCollection");
            done();
        });

        it("public objects are immutable", function(done) {
            var libJoynrStarted;
            testMutability(joynr, "start");
            // testMutability(joynr, "shutdown");
            testMutability(joynr, "typeRegistry");
            testMutability(joynr.typeRegistry, "addType");
            testMutability(joynr.typeRegistry, "getConstructor");
            testMutability(joynr, "capabilities");
            testMutability(joynr.capabilities, "registerCapability");
            testMutability(joynr.registration, "registerProvider");
            // testMutability(joynr, "proxyBuilder");
            testMutability(joynr.proxyBuilder, "build");
            done();
        });
        it("provisioning is still mutable", function(done) {
            testMutability(provisioning, "bounceProxyBaseUrl", true);
            done();
        });

        it("proxy is immutable", function(done) {
            var radioProxy;

            require([ "joynr/vehicle/RadioProxy"
            ], function(RadioProxy) {
                var domain = "LibJoynrTest-" + Date.now();
                IntegrationUtils.initializeWebWorker(
                        "TestEnd2EndCommProviderWorker",
                        provisioningSuffix,
                        domain).then(function(newWorkerId) {
                    workerId = newWorkerId;
                    return IntegrationUtils.startWebWorker(workerId);
                }).then(function() {
                    return IntegrationUtils.buildProxy(RadioProxy, domain);
                }).then(function(newRadioProxy) {
                    radioProxy = newRadioProxy;
                    testMutability(radioProxy, "isOn");
                    testMutability(radioProxy.isOn, "get");
                    testMutability(radioProxy.isOn, "set");
                    testMutability(radioProxy.isOn, "subscribe");
                    testMutability(radioProxy.isOn, "unsubscribe");

                    testMutability(radioProxy, "addFavoriteStation");

                    testMutability(radioProxy, "weakSignal");
                    testMutability(radioProxy.weakSignal, "subscribe");
                    testMutability(radioProxy.weakSignal, "unsubscribe");
                    done();
                    return null;
                });
            });
        });

        it("provider is immutable", function(done) {
            var radioProvider;

            require([ "joynr/vehicle/RadioProvider"
            ], function(RadioProvider) {
                /* ensure all required datatypes are loaded once the RadioProvider is resolved */
                /*jslint nomen: true */
                var untypedObject = {
                    _typeName : "joynr.vehicle.radiotypes.DatatypeForTestLibjoynr",
                    name : "untypedObject"
                };
                /*jslint nomen: false */
                joynr.util.Util.ensureTypedValues(untypedObject, joynr.typeRegistry);
                radioProvider = joynr.providerBuilder.build(RadioProvider, {});
                testMutability(radioProvider, "isOn");
                testMutability(radioProvider.isOn, "registerGetter");
                testMutability(radioProvider.isOn, "registerSetter");
                testMutability(radioProvider.isOn, "get");
                testMutability(radioProvider.isOn, "set");
                testMutability(radioProvider.isOn, "valueChanged");
                testMutability(radioProvider.isOn, "registerObserver");
                testMutability(radioProvider.isOn, "unregisterObserver");

                testMutability(radioProvider, "addFavoriteStation");
                testMutability(radioProvider.addFavoriteStation, "registerOperation");
                testMutability(radioProvider.addFavoriteStation, "callOperation");

                testMutability(radioProvider, "weakSignal");
                testMutability(radioProvider.weakSignal, "fire");
                done();
            });
        });
    });
});
