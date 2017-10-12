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

var provisioning = require('../../../test-classes/joynr/provisioning/provisioning_cc');
var InProcessRuntime = require('../../../classes/joynr/start/InProcessRuntime');
var Promise = require('../../../classes/global/Promise');
var waitsFor = require('../../../test-classes/global/WaitsFor');

    function outputPromiseError(error) {
        expect(error.toString()).toBeFalsy();
    }

    // type that usually goes into proxy generation
    function RadioStation(name, station, source) {
        if (!(this instanceof RadioStation)) {
            // in case someone calls constructor without new keyword (e.g. var c = Constructor({..}))
            return new RadioStation(name, station, source);
        }
        this.name = name;
        this.station = station;
        this.source = source;

        Object.defineProperty(this, "_typeName", {
            configurable : false,
            writable : false,
            enumerable : true,
            value : ".vehicle.RadioStation"
        });
    }

    describe("libjoynr-js.joynr.start.TestInProcessRuntime", function() {
        var runtime;

        function startInProcessRuntime() {
            return runtime.start().catch(outputPromiseError);
        }

        function shutdownInProcessRuntime() {
            return runtime.shutdown().catch(outputPromiseError);
        }

        beforeEach(function(done) {
            runtime = new InProcessRuntime(provisioning);
            done();
        });

        it("is of correct type and has all members", function(done) {
            expect(InProcessRuntime).toBeDefined();
            expect(runtime).toBeDefined();
            expect(runtime instanceof InProcessRuntime).toBeTruthy();
            expect(runtime.logging).toBeDefined();
            expect(runtime.typeRegistry).toBeDefined();

            expect(runtime.registration).toBeUndefined();
            expect(runtime.proxyBuilder).toBeUndefined();

            startInProcessRuntime().then(function() {
                expect(runtime.typeRegistry).toBeDefined();
                expect(runtime.registration).toBeDefined();
                expect(runtime.proxyBuilder).toBeDefined();
                done();
                return null;
            }).catch(fail);
        });

        it("can be started and shutdown successfully", function(done) {
            var log = runtime.logging.getLogger("joynr.start.TestInProcessRuntime");
            startInProcessRuntime().then(shutdownInProcessRuntime).then(function() {
                done();
                return null;
            }).catch(fail);
        });

        var nrRestarts = 3;
        it("can be started and shut down successfully " + nrRestarts + " times", function(done) {
            var i;

            function createFunc(promiseChain) {
                return promiseChain.then(shutdownInProcessRuntime).then(startInProcessRuntime);
            }

            var promiseChain = startInProcessRuntime();
            for (i = 1; i < nrRestarts; ++i) {
                promiseChain = createFunc(promiseChain);
            }
            promiseChain.then(shutdownInProcessRuntime).then(function() {
                done();
                return null;
            }).catch(fail);
        });

        it("throws when started in state STARTED", function(done) {
            startInProcessRuntime().then(function() {
                expect(function() {
                    runtime.start();
                }).toThrow();
                return shutdownInProcessRuntime();
            }).then(function() {
                done();
                return null;
            }).catch(fail);
        });

        it("throws when shutdown in state SHUTDOWN", function(done) {
            startInProcessRuntime().then(shutdownInProcessRuntime).then(function() {
                expect(function() {
                    runtime.shutdown();
                }).toThrow();
                done();
                return null;
            }).catch(fail);
        });
    });