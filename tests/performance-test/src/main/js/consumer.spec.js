/*jslint node: true */

/*
 * #%L
 * %%
 * Copyright (C) 2016 BMW Car IT GmbH
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

var Promise = require("bluebird").Promise;
var joynr = require("joynr");
var testbase = require("test-base");
var PerformanceUtilities = require("./performanceutilities.js");
var ComplexStruct = require("../generated-javascript/joynr/tests/performance/Types/ComplexStruct.js");

var prettyLog = testbase.logging.prettyLog;
var error = testbase.logging.error;
var log = testbase.logging.log;
var timeout = 600000;

jasmine.getEnv().addReporter(new testbase.TestReporter());

var options = PerformanceUtilities.getCommandLineOptionsOrDefaults(process.env);

console.log("Using domain " + options.domain);
console.error("Performing " + options.numRuns + " runs");

//disable log
console.log = function() {};

var registerProvider = function(joynr) {
    var providerQos = new joynr.types.ProviderQos({
        customParameters : [],
        priority : Date.now(),
        scope : joynr.types.ProviderScope.LOCAL,
        onChangeSubscriptions : true
    });

    var EchoProvider = require("../generated-javascript/joynr/tests/performance/EchoProvider.js");
    var EchoProviderImpl = require("./EchoProviderImpl.js");
    var echoProvider = joynr.providerBuilder.build(
            EchoProvider,
            EchoProviderImpl.implementation);

    joynr.capabilities.registerCapability("", options.domain, echoProvider, providerQos).then(function() {
        log("provider registered successfully");
    }).catch(function(error) {
        log("error registering provider: " + error.toString());
    });
}

describe("js consumer performance test", function() {

    var initialized = false;
    var echoProxy;

    beforeEach(function() {
        var ready = false;
        var spy = jasmine.createSpyObj("spy", [ "callback"]);

        if (initialized === false) {

            runs(function() {
                console.log("Environment not yet setup");
                var viaClusterController = options.viacc == 'true';
                var provisioning = testbase.provisioning_common;
                if (viaClusterController) {
                    provisioning.ccAddress.host = options.cchost;
                    provisioning.ccAddress.port = options.ccport;
                    joynr.selectRuntime("websocket.libjoynr");
                } else {
                    provisioning.bounceProxyBaseUrl = options.bounceProxyBaseUrl;
                    provisioning.bounceProxyUrl = provisioning.bounceProxyBaseUrl + "/bounceproxy/";
                    joynr.selectRuntime("inprocess");
                }
                joynr.load(provisioning).then(function(loadedJoynr) {
                    log("joynr started");
                    joynr = loadedJoynr;
                    if (!viaClusterController) {
                        registerProvider(joynr);
                    }
                    var messagingQos = new joynr.messaging.MessagingQos({
                        ttl : timeout
                    });
                    var EchoProxy = require("../generated-javascript/joynr/tests/performance/EchoProxy.js");
                    joynr.proxyBuilder.build(EchoProxy, {
                        domain : options.domain,
                        messagingQos : messagingQos
                    }).then(function(newEchoProxy) {
                        echoProxy = newEchoProxy;
                        expect(echoProxy).toBeDefined();

                        ready = true;
                    }).catch(function(error) {
                        log("Error building echoProxy: " + error);
                    });
                    return loadedJoynr;
                }).catch(function(error) {
                    throw error;
                });
            });

            waitsFor(function() {
                return ready;
            }, "joynr proxy build", options.timeout);

            runs(function() {
                initialized = true;
                // The warmup run is supposed to ensure that the initialization process finished
                console.log("Performing warmup run");
                echoProxy.echoString( { data: "Init string" } ).then(spy.callback).catch(spy.callback);
            });

            waitsFor(function() {
                return spy.callback.callCount >= 0;
            }, "Warmup run", options.timeout);
        } else {
            console.log("Environment already setup");
        }
    });

    var executeBenchmark = function(benchmarkName, benchmark) {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        var startTime;
        spy.onFulfilled.reset();
        spy.onError.reset();
        runs(function() {
            log("call echoComplexStruct " + options.numRuns + " times");
            startTime = Date.now();
            for (var i = 1; i <= options.numRuns; i++) {
                benchmark(i).then(spy.onFulfilled).catch(spy.onError);
            }
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount + spy.onError.callCount >= options.numRuns;
        }, "callEchoComplexStruct", timeout);

        runs(function() {
            expect(spy.onFulfilled.callCount+ '').toEqual(options.numRuns);
            expect(spy.onError.callCount).toEqual(0);
            var elapsedTimeMs = Date.now() - startTime;

            error(benchmarkName + " took " + elapsedTimeMs + " ms. " + options.numRuns / (elapsedTimeMs / 1000) + " msgs/s");
        });
    };

    it("EchoString", function() {
        var testProcedure = function(i) {
            var args = {
                data : PerformanceUtilities.createString(options.stringLength-2, "x") + "-" + i
            };
            return echoProxy.echoString(args).then(function(returnValues) {
                if (args.data !== returnValues.responseData) {
                    throw new Error("Echo " + returnValues.responseData + " does not match input data: " + args.data);
                }
                return returnValues;
            });
        }
        executeBenchmark("echoString", testProcedure);
    });

    it("EchoComplexStruct", function() {
        var testProcedure = function(i) {
            var args = {
                data : new ComplexStruct({
                    num32 : PerformanceUtilities.createRandomNumber(100000),
                    num64 : PerformanceUtilities.createRandomNumber(1000000),
                    data : PerformanceUtilities.createByteArray(options.byteArrayLength, 1),
                    str : PerformanceUtilities.createString(options.stringLength-2, "x") + "-" + i
                })
            };
            return echoProxy.echoComplexStruct(args).then(function(returnValues) {
                if (JSON.stringify(args.data) !== JSON.stringify(returnValues.responseData)) {
                    throw new Error("Echo " + returnValues.responseData + " does not match input data: " + args.data);
                }
                return returnValues;
            });
        }
        executeBenchmark("echoComplexStruct", testProcedure);
    });

    it("EchoByteArray", function() {
        var args = {
            data : PerformanceUtilities.createByteArray(options.byteArrayLength, 1)
        };

        var testProcedure = function(i) {
            var firstElement = PerformanceUtilities.createRandomNumber(256);
            args.data[0] = firstElement;
            return echoProxy.echoByteArray(args).then(function(returnValues) {
                if (args.data.length !== returnValues.responseData.length ||
                    firstElement !== returnValues.responseData[0]) {
                    throw new Error("Echo " + returnValues.responseData + " does not match input data: " + args.data);
                }

                return returnValues;
            });
        }
        executeBenchmark("echoByteArray", testProcedure);
    });
});
