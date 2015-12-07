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

define([
    "joynr/provisioning/provisioning_cc",
    "joynr/start/InProcessRuntime"
], function(provisioning, InProcessRuntime) {
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
            var fulfilledSpy = jasmine.createSpy("fulfilledSpy");
            runs(function() {
                provisioning.channelId = "TestInProcessRuntime" + Date.now();
                runtime.start().then(fulfilledSpy).catch(outputPromiseError);
            });

            waitsFor(function() {
                return fulfilledSpy.callCount > 0;
            }, "until libjoynr is started", 3 * provisioning.internalMessagingQos.ttl);

            runs(function() {
                expect(fulfilledSpy).toHaveBeenCalled();
            });
        }

        function shutdownInProcessRuntime() {
            var fulfilledSpy = jasmine.createSpy("fulfilledSpy");
            runs(function() {
                runtime.shutdown().then(fulfilledSpy).catch(outputPromiseError);
            });

            waitsFor(function() {
                return fulfilledSpy.callCount > 0;
            }, "until libjoynr is shut down", 3 * provisioning.internalMessagingQos.ttl);

            runs(function() {
                expect(fulfilledSpy).toHaveBeenCalled();
            });
        }

        beforeEach(function() {
            runtime = new InProcessRuntime(provisioning);
        });

        it("is of correct type and has all members", function() {
            expect(InProcessRuntime).toBeDefined();
            expect(runtime).toBeDefined();
            expect(runtime instanceof InProcessRuntime).toBeTruthy();
            expect(runtime.logging).toBeDefined();
            expect(runtime.typeRegistry).toBeDefined();

            expect(runtime.capabilities).toBeUndefined();
            expect(runtime.registration).toBeUndefined();
            expect(runtime.proxyBuilder).toBeUndefined();

            startInProcessRuntime();

            runs(function() {
                expect(runtime.typeRegistry).toBeDefined();
                expect(runtime.capabilities).toBeDefined();
                expect(runtime.registration).toBeDefined();
                expect(runtime.proxyBuilder).toBeDefined();
            });
        });

        it("can be started and shutdown successfully", function() {
            startInProcessRuntime();
            var log = runtime.logging.getLogger("joynr.start.TestInProcessRuntime");
            log.info("runtime started");
            shutdownInProcessRuntime();
        });

        var nrRestarts = 3;
        it("can be started and shut down successfully " + nrRestarts + " times", function() {
            var i;
            for (i = 0; i < nrRestarts; ++i) {
                startInProcessRuntime();
                shutdownInProcessRuntime();
            }
        });

        it("throws when started in state STARTING", function() {
            var fulfilledSpy = jasmine.createSpy("fulfilledSpy");
            runs(function() {
                provisioning.channelId = "TestInProcessRuntime" + Date.now();
                runtime.start().then(fulfilledSpy).catch(outputPromiseError);
            });

            runs(function() {
                expect(function() {
                    runtime.start();
                }).toThrow();
            });

            waitsFor(function() {
                return fulfilledSpy.callCount > 0;
            }, "until libjoynr is started", 3 * provisioning.internalMessagingQos.ttl);

            runs(function() {
                expect(fulfilledSpy).toHaveBeenCalled();
            });

            shutdownInProcessRuntime();
        });

        it("throws when started in state STARTED", function() {
            startInProcessRuntime();

            runs(function() {
                expect(function() {
                    runtime.start();
                }).toThrow();
            });

            shutdownInProcessRuntime();
        });

        it("throws when shutdown in state STARTING", function() {
            var fulfilledSpy = jasmine.createSpy("fulfilledSpy");
            runs(function() {
                provisioning.channelId = "TestInProcessRuntime" + Date.now();
                runtime.start().then(fulfilledSpy).catch(outputPromiseError);
            });

            expect(function() {
                runtime.shutdown();
            }).toThrow();

            waitsFor(function() {
                return fulfilledSpy.callCount > 0;
            }, "until libjoynr is started", 3 * provisioning.internalMessagingQos.ttl);

            runs(function() {
                expect(fulfilledSpy).toHaveBeenCalled();
            });
            shutdownInProcessRuntime();

        });

        it("throws when shutdown in state SHUTTINGDOWN", function() {
            startInProcessRuntime();

            var fulfilledSpy = jasmine.createSpy("fulfilledSpy");
            runs(function() {
                runtime.shutdown().then(fulfilledSpy).catch(outputPromiseError);
            });

            runs(function() {
                expect(function() {
                    runtime.shutdown();
                }).toThrow();
            });

            waitsFor(function() {
                return fulfilledSpy.callCount > 0;
            }, "until libjoynr is shut down", 3 * provisioning.internalMessagingQos.ttl);

            runs(function() {
                expect(fulfilledSpy).toHaveBeenCalled();
            });
        });

        it("throws when shutdown in state SHUTDOWN", function() {
            startInProcessRuntime();
            shutdownInProcessRuntime();

            runs(function() {
                expect(function() {
                    runtime.shutdown();
                }).toThrow();
            });
        });
    });
});
