/*jslint node: true */

/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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

var joynr = require("joynr");
var ComplexStruct = require("../generated-javascript/joynr/tests/performance/Types/ComplexStruct.js");
var testbase = require("test-base");
var PerformanceUtilities = require("./performanceutilities.js");
var Promise = require("bluebird").Promise;

var error = testbase.logging.error;
var log = testbase.logging.log;
var options = PerformanceUtilities.getCommandLineOptionsOrDefaults(process.env);
var timeout = 600000;

var consumerBase = {
    echoProxy : undefined,
    initialize: function() {
        if (consumerBase.echoProxy === undefined) {
            console.log("Using domain " + options.domain);
            console.error("Performing " + options.numRuns + " runs");
            error ("test runs: " + options.testRuns);
            var viaClusterController = options.viacc == "true";
            console.log("Via cluster-contoller: " + viaClusterController);
            var provisioning = testbase.provisioning_common;
            if (viaClusterController) {
                provisioning.ccAddress.host = options.cchost;
                provisioning.ccAddress.port = options.ccport;
                joynr.selectRuntime("websocket.libjoynr");
            } else {
                provisioning.bounceProxyBaseUrl = options.bounceProxyBaseUrl;
                provisioning.brokerUri = options.brokerUri;
                provisioning.bounceProxyUrl = provisioning.bounceProxyBaseUrl + "/bounceproxy/";
                joynr.selectRuntime("inprocess");
            }
            provisioning.logging.configuration.loggers.root.level = "error";

            return joynr.load(provisioning).then(function(loadedJoynr) {
                joynr = loadedJoynr;
                log("joynr started");
                if (!viaClusterController) {
                    consumerBase.registerProvider();
                }
                return loadedJoynr;
            }).then(function(loadedJoynr) {
                var messagingQos = new loadedJoynr.messaging.MessagingQos({
                    ttl: timeout
                });
                var EchoProxy = require("../generated-javascript/joynr/tests/performance/EchoProxy.js");
                return loadedJoynr.proxyBuilder.build(EchoProxy, {
                    domain      : options.domain,
                    messagingQos: messagingQos
                });
            }).then(function(echoProxy) {
                consumerBase.echoProxy = echoProxy;
                return null;
            }).catch(function(e) {
                throw e;
            });
        } else {
            return Promise.resolve();
        }
    },

    shutdown: function() {
        return joynr.shutdown();
    },
    registerProvider: function() {
        var providerQos = new joynr.types.ProviderQos({
            customParameters             : [],
            priority                     : Date.now(),
            scope                        : joynr.types.ProviderScope.LOCAL,
            supportsOnChangeSubscriptions: true
        });

        var EchoProvider = require("../generated-javascript/joynr/tests/performance/EchoProvider.js");
        var EchoProviderImpl = require("./EchoProviderImpl.js");
        var echoProvider = joynr.providerBuilder.build( EchoProvider, EchoProviderImpl.implementation);

        joynr.registration.registerProvider(options.domain, echoProvider, providerQos).then(function() {
            log("provider registered successfully");
        }).catch(function(e) {
            error("error registering provider: " + e.toString());
        });
    },

    executeBenchmark: function(benchmarkName, benchmarkData, benchmark){
        var numRuns = benchmarkData.length;
        log("call " + benchmarkName + " " + numRuns + " times");
        var startTime = Date.now();

        return Promise.map(benchmarkData, function(data){
            return benchmark(data);
        }).then(function(){
            log("all the numRuns were executed");
            var elapsedTimeMs = Date.now() - startTime;

            error(benchmarkName + " runs: " + numRuns + " took " + elapsedTimeMs + " ms. " + numRuns / (elapsedTimeMs / 1000) + " msgs/s");
            return elapsedTimeMs;
        });
    },

    excecuteMultipleBenchmarks: function(benchmarkName, generateBenchmarkData, benchmark) {
        PerformanceUtilities.forceGC();
        var numRuns = options.numRuns;
        var testRuns = options.testRuns ? Number.parseInt(options.testRuns) : 1;
        var totalRuns = numRuns * testRuns;
        var totalTime = 0;
        var testIndex = 0;
        var dummyArray = new Array(testRuns);
        var runsTime = [];
        var measureMemory = options.measureMemory == "true";
        var memInterval;
        var memSum = 0;
        var memTests = 0;

        if (measureMemory){
            memInterval = setInterval(function () {
                var memoryUsage = process.memoryUsage();
                memSum += memoryUsage.rss;
                memTests++;
            }, 1000);
        }

        return Promise.map(dummyArray, function(){
            var data = [];
            for (var j = 0; j < numRuns; j++){
                data.push(generateBenchmarkData(j));
            }
            testIndex++;
            var name = benchmarkName + " Test: " + testIndex;
            return consumerBase.executeBenchmark(name, data, benchmark )
                .then(function(time){
                    totalTime += time;
                    runsTime.push(time);
                });
        }, { concurrency: 1 })
            .then(function () {
                var averageTime = totalRuns / (totalTime / 1000);
                var variance = 0;
                runsTime.map(function(time){
                    return numRuns / (time / 1000);
                }).forEach(function (time) {
                    variance += Math.pow(time - averageTime, 2);
                });
                variance /= runsTime.length;
                var deviation = Math.sqrt(variance);
                error("the total runtime was: " + totalTime + " runs: " + totalRuns + " msgs/s: " + averageTime + " +/- " + deviation);

                if (measureMemory){
                    var memav = memSum / memTests;
                    var mb = (memav / 1048576.0).toFixed(2);
                    error("test used on average: " + mb + " MB memory");
                    clearInterval(memInterval);
                }
            });
    },
    echoString: function() {
        var generateData = function(i) {
            return {
                data: PerformanceUtilities.createString(options.stringLength - 2, "x") + "-" + i
            };
        };
        var testProcedure = function(args) {
            return consumerBase.echoProxy.echoString(args).then(function(returnValues) {
                if (args.data !== returnValues.responseData) {
                    throw new Error("Echo " + JSON.stringify(returnValues.responseData) + " does not match input data: " + JSON.stringify(args.data));
                }
                return returnValues;
            });
        };
        return consumerBase.excecuteMultipleBenchmarks("echoString", generateData, testProcedure);
    },

    echoComplexStruct: function() {
        var generateData = function(i){
            return {
                data: new ComplexStruct({
                    num32: PerformanceUtilities.createRandomNumber(100000),
                    num64: PerformanceUtilities.createRandomNumber(1000000),
                    data : PerformanceUtilities.createByteArray(options.byteArrayLength, 1),
                    str  : PerformanceUtilities.createString(options.stringLength - 2, "x") + "-" + i
                })
            };
        };
        var testProcedure = function(args) {
            return consumerBase.echoProxy.echoComplexStruct(args).then(function(returnValues) {
                if (args.data.num32 !== returnValues.responseData.num32 ||
                args.data.num64 !== returnValues.responseData.num64 ||
                args.data.data.length !== returnValues.responseData.data.length ||
                args.data.str.length !== returnValues.responseData.str.length) {
                    throw new Error("Echo " + JSON.stringify(returnValues.responseData) + " does not match input data: " + JSON.stringify(args.data));
                }
                return returnValues;
            });
        };
        return consumerBase.excecuteMultipleBenchmarks("echoComplexStruct", generateData, testProcedure);
    },
    echoByteArray: function(byteArraySizeFactor) {
        byteArraySizeFactor = byteArraySizeFactor || 1;
        var byteArraySize = byteArraySizeFactor * options.byteArrayLength;
        var generateData = function(i){
            var args = {
                data: PerformanceUtilities.createByteArray(byteArraySize, 1)
            };
            var firstElement = i % 128;
            args.data[0] = firstElement;
            return args;
        };
        var testProcedure = function(args) {
            return consumerBase.echoProxy.echoByteArray(args).then(function(returnValues, i) {
                var firstElement = args.data[0];
                if (args.data.length !== returnValues.responseData.length ||
                firstElement !== returnValues.responseData[0]) {
                    throw new Error("Echo " + JSON.stringify(returnValues.responseData) + " does not match input data: " + JSON.stringify(args.data));
                }

                return returnValues;
            });
        };
        // the larger this byteArraySizeFactor is, the longer this test takes
        // in order to mitigate that, we scale the numer of runs by byteArraySizeFactor
        var numRuns = options.numRuns;
        if (byteArraySizeFactor > 1) {
            numRuns = numRuns / Math.sqrt(byteArraySizeFactor);
        }
        return consumerBase.excecuteMultipleBenchmarks("echoByteArray " + byteArraySize, generateData, testProcedure, numRuns);
    },
    echoByteArrayWithSizeTimesK: function() {
        if (options.skipByteArraySizeTimesK === undefined || options.skipByteArraySizeTimesK === true) {
            consumerBase.echoByteArray(1000);
        }
    }
};
module.exports = consumerBase;
