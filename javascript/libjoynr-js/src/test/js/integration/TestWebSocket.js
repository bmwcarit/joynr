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

joynrTestRequire("integration/TestWebSocket", [
    "joynr",
    "joynr/vehicle/RadioProxy",
    "joynr/vehicle/RadioProvider",
    "joynr/provisioning/provisioning_libjoynr",
    "integration/IntegrationUtils"
], function(joynr, RadioProxy, RadioProvider, provisioning, IntegrationUtils) {
    describe("libjoynr-js.integration.websocket", function() {
        var radioProxy, runtime, discoveryTimeout;

        beforeEach(function() {
            var provisioningSuffix = "-" + Date.now(), libJoynrStarted = false;
            discoveryTimeout = 4000;

            provisioning.ccAddress = {
                protocol : "ws",
                host : "localhost",
                port : 4242,
                path : "/"
            };

            runs(function() {
                joynr.load(provisioning).then(function(loadedJoynr){
                    IntegrationUtils.initialize(loadedJoynr);
                    IntegrationUtils.messagingQos.ttl = provisioning.ttl = 30000;
                    libJoynrStarted = true;
                }).catch(function(error){
                    throw error;
                });
            });

            waitsFor(function() {
                return libJoynrStarted;
            }, "libjoynr has to be started ", provisioning.ttl);

        });

        function shutdownLibJoynr() {
            var shutDownLibJoynr;

            runs(function() {
                IntegrationUtils.shutdownLibjoynr().then(function() {
                    shutDownLibJoynr = true;
                });
            });

            waitsFor(function() {
                return shutDownLibJoynr;
            }, "Libjoynr needs to be shut down", 5000);
        }

        afterEach(function() {
            shutdownLibJoynr();
        });

        it("communicates with the websocket server", function() {
            var promise = null;
            radioProxy = undefined;
            runs(function() {
                IntegrationUtils.buildProxy(RadioProxy).then(function(newRadioProxy) {
                    radioProxy = newRadioProxy;
                });
            });

            waitsFor(function() {
                return radioProxy !== undefined;
            }, "proxy to be resolved", provisioning.ttl);

            runs(function() {
                promise = radioProxy.isOn.set({
                    value : true
                });
            });

            waitsFor(function() {
                return promise.state() !== "pending";
            }, "attribute is set to true", provisioning.ttl);

            runs(function() {
                promise = radioProxy.isOn.get().then(function(value) {
                    expect(value).toBe(true);
                }).catch(IntegrationUtils.outputPromiseError);
            });

            waitsFor(function() {
                return promise.state() !== "pending";
            }, "attribute is received", provisioning.ttl);

            runs(function() {
                promise = radioProxy.isOn.set({
                    value : false
                });
            });

            waitsFor(function() {
                return promise.state() !== "pending";
            }, "attribute is set", provisioning.ttl);

            runs(function() {
                promise = radioProxy.isOn.get().then(function(value) {
                    expect(value).toBe(false);
                }).catch(IntegrationUtils.outputPromiseError);
            });

            waitsFor(function() {
                return promise.state() !== "pending";
            }, "attribute is received", provisioning.ttl);

            runs(function() {
                expect(promise.state()).toEqual("resolved");
            });

        });
    });
});