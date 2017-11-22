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

define([
    "joynr",
    "joynr/vehicle/RadioProxy",
    "joynr/tests/TestWithVersionProvider",
    "joynr/tests/TestWithVersionProxy",
    "integration/IntegrationUtils",
    "joynr/provisioning/provisioning_cc",
    "joynr/proxy/DiscoveryQos",
    "joynr/exceptions/JoynrException"
],
function(
joynr,
RadioProxy,
TestWithVersionProvider,
TestWithVersionProxy,
IntegrationUtils,
provisioning,
DiscoveryQos,
JoynrException) {

    describe(
    "libjoynr-js.integration.localDiscoveryTest",
    function() {

        var radioProxy;
        var provisioningSuffix;
        var domain;
        var workerId;
        var subscriptionLength = 2000;
        var safetyTimeout = 200;
        var MyTestWithVersionProvider = function() {};

        afterEach(function(done) {
            IntegrationUtils.shutdownLibjoynr()
            .then(function() {
                done();
                return null;
            }).catch(function() {
                throw new Error("shutdown Webworker and Libjoynr failed");
            });
        });

        beforeEach(function(done) {

            var testProvisioning = null;
            radioProxy = undefined;

            provisioningSuffix = "LocalDiscoveryTest" + "-" + Date.now();
            domain = provisioningSuffix;
            testProvisioning =
            IntegrationUtils.getProvisioning(
            provisioning,
            provisioningSuffix);

            joynr.load(testProvisioning)
            .then(function(newjoynr) {
                joynr = newjoynr;
                IntegrationUtils.initialize(joynr);
                done();
                return null;
            }).catch(function(error) {
                if (error instanceof JoynrException) {
                    done.fail("error in beforeEach: " + error.detailMessage);
                } else {
                    done.fail("error in beforeEach: " + error);
                }
            });
        });

        function registerGlobalDiscoveryEntry() {
            return IntegrationUtils.initializeWebWorker(
            "TestEnd2EndCommProviderWorker",
            provisioningSuffix,
            domain)
            .then(function(newWorkerId) {
                workerId = newWorkerId;
                return IntegrationUtils.startWebWorker(workerId);
            });
        }

        function unregisterGlobalDiscoveryEntry() {
            return IntegrationUtils.shutdownWebWorker(workerId);
        }

        function buildProxyForGlobalDiscoveryEntry() {
            return joynr.proxyBuilder.build(RadioProxy, {
                domain : domain,
                messagingQos : new joynr.messaging.MessagingQos(),
                discoveryQos : new DiscoveryQos()
            });
        }

        it("local discovery entry is forwarded to proxy", function(done) {
            var providerQos = new joynr.types.ProviderQos({
                customParameters : [],
                priority : Date.now(),
                scope : joynr.types.ProviderScope.GLOBAL,
                supportsOnChangeSubscriptions : true
            });

            var testWithVersionProviderImpl = new MyTestWithVersionProvider();
            var testWithVersionProvider = joynr.providerBuilder.build(
            TestWithVersionProvider,
            testWithVersionProviderImpl);
            joynr.registration.registerProvider(domain, testWithVersionProvider, providerQos)
            .then(function() {
                return joynr.proxyBuilder.build(TestWithVersionProxy, {
                    domain : domain,
                    messagingQos : new joynr.messaging.MessagingQos(),
                    discoveryQos : new DiscoveryQos()
                });
            }).then(function(testWithVersionProxy) {
                expect(testWithVersionProxy.providerDiscoveryEntry.isLocal).toBeDefined();
                expect(testWithVersionProxy.providerDiscoveryEntry.isLocal).toBe(true);
                joynr.registration.unregisterProvider(domain, testWithVersionProvider)
                .then(function() {
                    done();
                    return null;
                });
            });
        });

        it("global discovery entry is forwarded to proxy", function(done) {
            registerGlobalDiscoveryEntry()
            .then(function() {
                return buildProxyForGlobalDiscoveryEntry();
            }).then(function(radioProxy) {
                expect(radioProxy.providerDiscoveryEntry.isLocal).toBeDefined();
                expect(radioProxy.providerDiscoveryEntry.isLocal).toBe(false);
                return unregisterGlobalDiscoveryEntry();
            }).then(function() {
                done();
                return null;
            });
        });

        it("cached global discovery entry is forwarded to proxy", function(done) {
            registerGlobalDiscoveryEntry()
            .then(function() {
                // build proxy to fill global capabilities cache
                return buildProxyForGlobalDiscoveryEntry();
            }).then(function() {
                // build proxy from global capabilities cache
                return buildProxyForGlobalDiscoveryEntry();
            }).then(function(radioProxy) {
                expect(radioProxy.providerDiscoveryEntry.isLocal).toBeDefined();
                expect(radioProxy.providerDiscoveryEntry.isLocal).toBe(false);
                return unregisterGlobalDiscoveryEntry();
            }).then(function() {
                done();
                return null;
            });
        });

    }); // describe
}); // define
