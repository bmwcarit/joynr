/*jslint node: true */

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

describe("js consumer performance test", function() {
    var joynrModule = require("joynr");

    var PerformanceUtilities = require("./performanceutilities.js");
    var ComplexStruct = require("../generated-javascript/joynr/tests/performance/Types/ComplexStruct.js");

    var initialized = false;
    var echoProxy;
    var joynr;

    var options = PerformanceUtilities.getCommandLineOptionsOrDefaults(process.env);

    console.log("Using domain " + options.domain);
    console.error("Performing " + options.numRuns + " runs");

    beforeEach(function() {
        var ready = false;

        if (initialized === false) {
            var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);

            spy.onFulfilled.reset();
            spy.onError.reset();

            runs(function() {
                console.log("Environment not yet setup");

                var provisioning = require("./provisioning_common.js");

                joynrModule.load(provisioning).then(function(loadedJoynr) {
                	console.log("Joynr started");

                    joynr = loadedJoynr;

                    var messagingQos = new joynr.messaging.MessagingQos({ ttl : 1200000 });
                    var EchoProxy = require("../generated-javascript/joynr/tests/performance/EchoProxy.js");

                    joynr.proxyBuilder.build(EchoProxy, {
                        domain : options.domain,
                        messagingQos : messagingQos
                    }).then(function(newEchoProxy) {
                        echoProxy = newEchoProxy;
                        expect(echoProxy).toBeDefined();

                        ready = true;
                    }).catch(function(error) {
                        console.log("Error building echoProxy: " + error);
                    });

                }).catch(function(error) {
                    throw error;
                });
            });

            waitsFor(function() {
                return ready;
            }, "Joynr proxy build", 15000);

            runs(function() {
                initialized = true;

                // The warmup run is supposed to ensure that the initialization process finished
                console.log("Performing warmup run");
                echoProxy.echoString( { data: "Init string" } ).then(spy.onFulfilled).catch(spy.onError);
            });

            waitsFor(function() {
                return spy.onFulfilled.callCount + spy.onError.callCount >= 0;
            }, "Warmup run", options.timeout);
        } else {
            console.log("Environment already setup");
        }
    });

    it("EchoString", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        var startTime;

        spy.onFulfilled.reset();
        spy.onError.reset();

        runs(function() {
            var args = { data: PerformanceUtilities.createString(options.stringLength, "x") };
            startTime = new Date().getTime();

            for(var i = 0; i < options.numRuns ; i++) {
                echoProxy.echoString(args).then(spy.onFulfilled).catch(spy.onError);
            }
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount + spy.onError.callCount >= options.numRuns;
        }, "echoString", options.timeout);

        runs(function() {
            var elapsedTime = new Date().getTime() - startTime;

            // Write the result to stderr, because this way we can collect all required data easier.
            console.error("EchoString took " + elapsedTime + " ms. " + options.numRuns / (elapsedTime / 1000) + " msgs/s");
        });
    });

    it("EchoStruct", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        var startTime;

        spy.onFulfilled.reset();
        spy.onError.reset();

        runs(function() {
            var inputStruct = new ComplexStruct();

            inputStruct.num32 = 1234;
            inputStruct.num64 = 42;
            inputStruct.data = PerformanceUtilities.createByteArray(options.byteArrayLength, 1);
            inputStruct.str = PerformanceUtilities.createString(options.stringLength, "x");

            var args = { data: inputStruct };
            startTime = new Date().getTime();

            for(var i = 0; i < options.numRuns ; i++) {
                echoProxy.echoComplexStruct(args).then(spy.onFulfilled).catch(spy.onError);
            }
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount + spy.onError.callCount >= options.numRuns;
        }, "echoComplexStruct", options.timeout);

        runs(function() {
            var elapsedTime = new Date().getTime() - startTime;

            // Write the result to stderr, because this way we can collect all required data easier.
            console.error("echoComplexStruct test case took " + elapsedTime + " ms. " + options.numRuns / (elapsedTime / 1000) + " msgs/s");
        });
    });

    it("EchoByteArray", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        var startTime;

        spy.onFulfilled.reset();
        spy.onError.reset();

        runs(function() {
            var args = { data: PerformanceUtilities.createByteArray(options.byteArrayLength, 1) };
            startTime = new Date().getTime();

            for(var i = 0; i < options.numRuns ; i++) {
                echoProxy.echoByteArray(args).then(spy.onFulfilled).catch(spy.onError);
            }
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount + spy.onError.callCount >= options.numRuns;
        }, "echoByteArray", options.timeout);

        runs(function() {
            var elapsedTime = new Date().getTime() - startTime;

            // Write the result to stderr, because this way we can collect all required data easier.
            console.error("echoByteArray took " + elapsedTime + " ms. " + options.numRuns / (elapsedTime / 1000) + " msgs/s");
        });
    });
});
