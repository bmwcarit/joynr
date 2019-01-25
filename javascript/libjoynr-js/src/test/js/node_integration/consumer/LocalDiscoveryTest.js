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

let joynr = require("../../../../main/js/joynr");
const RadioProxy = require("../../../generated/joynr/vehicle/RadioProxy");
const TestWithVersionProvider = require("../../../generated/joynr/tests/TestWithVersionProvider");
const TestWithVersionProxy = require("../../../generated/joynr/tests/TestWithVersionProxy");
const IntegrationUtils = require("../IntegrationUtils");
const provisioning = require("../../../resources/joynr/provisioning/provisioning_cc");
const DiscoveryQos = require("../../../../main/js/joynr/proxy/DiscoveryQos");
const JoynrException = require("../../../../main/js/joynr/exceptions/JoynrException");

describe("libjoynr-js.integration.localDiscoveryTest", () => {
    let provisioningSuffix;
    let domain;
    let childId;
    const MyTestWithVersionProvider = function() {};

    afterEach(done => {
        IntegrationUtils.shutdownLibjoynr()
            .then(() => {
                done();
                return null;
            })
            .catch(() => {
                throw new Error("shutdown ChildProcess and Libjoynr failed");
            });
    });

    beforeEach(done => {
        provisioningSuffix = `LocalDiscoveryTest-${Date.now()}`;
        domain = provisioningSuffix;

        provisioning.channelId = provisioningSuffix;
        joynr.loaded = false;
        joynr.selectRuntime("inprocess");

        joynr
            .load(provisioning)
            .then(newjoynr => {
                joynr = newjoynr;
                IntegrationUtils.initialize(joynr);
                done();
                return null;
            })
            .catch(error => {
                if (error instanceof JoynrException) {
                    done.fail(`error in beforeEach: ${error.detailMessage}`);
                } else {
                    done.fail(`error in beforeEach: ${error}`);
                }
            });
    });

    function registerGlobalDiscoveryEntry() {
        return IntegrationUtils.initializeChildProcess(
            "TestEnd2EndCommProviderProcess",
            provisioningSuffix,
            domain
        ).then(newChildId => {
            childId = newChildId;
            return IntegrationUtils.startChildProcess(childId);
        });
    }

    function unregisterGlobalDiscoveryEntry() {
        return IntegrationUtils.shutdownChildProcess(childId);
    }

    function buildProxyForGlobalDiscoveryEntry() {
        return joynr.proxyBuilder.build(RadioProxy, {
            domain,
            messagingQos: new joynr.messaging.MessagingQos(),
            discoveryQos: new DiscoveryQos()
        });
    }

    it("local discovery entry is forwarded to proxy", done => {
        const providerQos = new joynr.types.ProviderQos({
            customParameters: [],
            priority: Date.now(),
            scope: joynr.types.ProviderScope.GLOBAL,
            supportsOnChangeSubscriptions: true
        });

        const testWithVersionProviderImpl = new MyTestWithVersionProvider();
        const testWithVersionProvider = joynr.providerBuilder.build(
            TestWithVersionProvider,
            testWithVersionProviderImpl
        );
        joynr.registration
            .registerProvider(domain, testWithVersionProvider, providerQos)
            .then(() => {
                return joynr.proxyBuilder.build(TestWithVersionProxy, {
                    domain,
                    messagingQos: new joynr.messaging.MessagingQos(),
                    discoveryQos: new DiscoveryQos()
                });
            })
            .then(testWithVersionProxy => {
                expect(testWithVersionProxy.providerDiscoveryEntry.isLocal).toBeDefined();
                expect(testWithVersionProxy.providerDiscoveryEntry.isLocal).toBe(true);
                joynr.registration.unregisterProvider(domain, testWithVersionProvider).then(() => {
                    done();
                    return null;
                });
            });
    });

    it("global discovery entry is forwarded to proxy", done => {
        registerGlobalDiscoveryEntry()
            .then(() => {
                return buildProxyForGlobalDiscoveryEntry();
            })
            .then(radioProxy => {
                expect(radioProxy.providerDiscoveryEntry.isLocal).toBeDefined();
                expect(radioProxy.providerDiscoveryEntry.isLocal).toBe(false);
                return unregisterGlobalDiscoveryEntry();
            })
            .catch(e => {
                return unregisterGlobalDiscoveryEntry()
                    .then(() => {
                        throw e;
                    })
                    .catch(e2 => {
                        throw new Error(`Error1: ${e}\n Error2: ${e2}`);
                    });
            })
            .then(() => {
                done();
                return null;
            });
    });

    it("cached global discovery entry is forwarded to proxy", done => {
        registerGlobalDiscoveryEntry()
            .then(() => {
                // build proxy to fill global capabilities cache
                return buildProxyForGlobalDiscoveryEntry();
            })
            .then(() => {
                // build proxy from global capabilities cache
                return buildProxyForGlobalDiscoveryEntry();
            })
            .then(radioProxy => {
                expect(radioProxy.providerDiscoveryEntry.isLocal).toBeDefined();
                expect(radioProxy.providerDiscoveryEntry.isLocal).toBe(false);
                return unregisterGlobalDiscoveryEntry();
            })
            .catch(e => {
                return unregisterGlobalDiscoveryEntry()
                    .then(() => {
                        throw e;
                    })
                    .catch(e2 => {
                        throw new Error(`Error1: ${e}\n Error2: ${e2}`);
                    });
            })
            .then(() => {
                done();
                return null;
            });
    });
}); // describe
