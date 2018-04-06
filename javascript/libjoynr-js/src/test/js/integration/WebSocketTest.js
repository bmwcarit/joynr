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
    "joynr/vehicle/RadioProvider",
    "joynr/provisioning/provisioning_libjoynr",
    "integration/IntegrationUtils"
], function(joynr, RadioProxy, RadioProvider, provisioning, IntegrationUtils) {
    describe("libjoynr-js.integration.websocket", function() {
        beforeEach(function(done) {
            provisioning.ccAddress = {
                protocol : "ws",
                host : "localhost",
                port : 4242,
                path : "/"
            };

            joynr.load(provisioning).then(function(loadedJoynr){
                IntegrationUtils.initialize(loadedJoynr);
                IntegrationUtils.messagingQos.ttl = provisioning.ttl = 30000;
                done();
            }).catch(function(error){
                throw error;
            });
        });

        afterEach(function(done) {
            IntegrationUtils.shutdownLibjoynr().then(function() {
                done();
                return null;
            }).catch(function(error) {
                return null;
            });
        });

        it("communicates with the websocket server", function(done) {
            var radioProxy;
            var domain = "WebSocketTest-"+Date.now();
            IntegrationUtils.buildProxy(RadioProxy, domain).then(function(newRadioProxy) {
                radioProxy = newRadioProxy;
                return radioProxy.isOn.set({
                    value : true
                });
            }).then(function() {
                return radioProxy.isOn.get();
            }).then(function(value) {
                expect(value).toBe(true);
                return radioProxy.isOn.set({
                    value : false
                });
            }).then(function() {
                radioProxy.isOn.get();
            }).then(function(value) {
                expect(value).toBe(false);
                done();
                return null;
            }).catch(fail);
        });
    });
});
