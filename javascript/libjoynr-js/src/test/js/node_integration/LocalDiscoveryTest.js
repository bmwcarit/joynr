/*jslint es5: true, node: true, nomen: true */

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

var joynr = require("joynr"),
    RadioProxy = require("../../generated/joynr/vehicle/RadioProxy"),
    TestWithVersionProvider = require("../../generated/joynr/tests/TestWithVersionProvider"),
    TestWithVersionProxy = require("../../generated/joynr/tests/TestWithVersionProxy"),
    IntegrationUtils = require("./IntegrationUtils"),
    provisioning = require("../../resources/joynr/provisioning/provisioning_cc"),
    DiscoveryQos = require("../../../../src/main/js/joynr/proxy/DiscoveryQos"),
    JoynrException = require("../../../main/js/joynr/exceptions/JoynrException");

describe("libjoynr-js.integration.localDiscoveryTest", function() {
    var radioProxy;
    var provisioningSuffix;
    var domain;
    var childId;
    var MyTestWithVersionProvider = function() {};

    afterEach(function(done) {
        IntegrationUtils.shutdownLibjoynr()
            .then(function() {
                done();
                return null;
            })
            .catch(function() {
                throw new Error("shutdown ChildProcess and Libjoynr failed");
            });
    });

    beforeEach(function(done) {
        radioProxy = undefined;

        provisioningSuffix = "LocalDiscoveryTest" + "-" + Date.now();
        domain = provisioningSuffix;

        provisioning.channelId = provisioningSuffix;
        joynr.loaded = false;
        joynr.selectRuntime("inprocess");

        joynr
            .load(provisioning)
            .then(function(newjoynr) {
                joynr = newjoynr;
                IntegrationUtils.initialize(joynr);
                done();
                return null;
            })
            .catch(function(error) {
                if (error instanceof JoynrException) {
                    done.fail("error in beforeEach: " + error.detailMessage);
                } else {
                    done.fail("error in beforeEach: " + error);
                }
            });
    });

    function registerGlobalDiscoveryEntry() {
        return IntegrationUtils.initializeChildProcess(
            "TestEnd2EndCommProviderProcess",
            provisioningSuffix,
            domain
        ).then(function(newChildId) {
            childId = newChildId;
            return IntegrationUtils.startChildProcess(childId);
        });
    }

    function unregisterGlobalDiscoveryEntry() {
        return IntegrationUtils.shutdownChildProcess(childId);
    }

    function buildProxyForGlobalDiscoveryEntry() {
        return joynr.proxyBuilder.build(RadioProxy, {
            domain: domain,
            messagingQos: new joynr.messaging.MessagingQos(),
            discoveryQos: new DiscoveryQos()
        });
    }

    it("local discovery entry is forwarded to proxy", function(done) {
        var providerQos = new joynr.types.ProviderQos({
            customParameters: [],
            priority: Date.now(),
            scope: joynr.types.ProviderScope.GLOBAL,
            supportsOnChangeSubscriptions: true
        });

        var testWithVersionProviderImpl = new MyTestWithVersionProvider();
        var testWithVersionProvider = joynr.providerBuilder.build(TestWithVersionProvider, testWithVersionProviderImpl);
        joynr.registration
            .registerProvider(domain, testWithVersionProvider, providerQos)
            .then(function() {
                return joynr.proxyBuilder.build(TestWithVersionProxy, {
                    domain: domain,
                    messagingQos: new joynr.messaging.MessagingQos(),
                    discoveryQos: new DiscoveryQos()
                });
            })
            .then(function(testWithVersionProxy) {
                expect(testWithVersionProxy.providerDiscoveryEntry.isLocal).toBeDefined();
                expect(testWithVersionProxy.providerDiscoveryEntry.isLocal).toBe(true);
                joynr.registration.unregisterProvider(domain, testWithVersionProvider).then(function() {
                    done();
                    return null;
                });
            });
    });

    it("global discovery entry is forwarded to proxy", function(done) {
        registerGlobalDiscoveryEntry()
            .then(function() {
                return buildProxyForGlobalDiscoveryEntry();
            })
            .then(function(radioProxy) {
                expect(radioProxy.providerDiscoveryEntry.isLocal).toBeDefined();
                expect(radioProxy.providerDiscoveryEntry.isLocal).toBe(false);
                return unregisterGlobalDiscoveryEntry();
            })
            .then(function() {
                done();
                return null;
            });
    });

    it("cached global discovery entry is forwarded to proxy", function(done) {
        registerGlobalDiscoveryEntry()
            .then(function() {
                // build proxy to fill global capabilities cache
                return buildProxyForGlobalDiscoveryEntry();
            })
            .then(function() {
                // build proxy from global capabilities cache
                return buildProxyForGlobalDiscoveryEntry();
            })
            .then(function(radioProxy) {
                expect(radioProxy.providerDiscoveryEntry.isLocal).toBeDefined();
                expect(radioProxy.providerDiscoveryEntry.isLocal).toBe(false);
                return unregisterGlobalDiscoveryEntry();
            })
            .then(function() {
                done();
                return null;
            });
    });
}); // describe
