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
    echoProxy: undefined,
    initialize: function() {
        if (consumerBase.echoProxy === undefined) {
            console.log("Using domain " + options.domain);
            console.error("Performing " + options.numRuns + " runs");
            var viaClusterController = options.viacc == 'true';
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
                    ttl : timeout
                });
                var EchoProxy = require("../generated-javascript/joynr/tests/performance/EchoProxy.js");
                return loadedJoynr.proxyBuilder.build(EchoProxy, {
                    domain : options.domain,
                    messagingQos : messagingQos
                });
            }).then(function(echoProxy) {
                consumerBase.echoProxy = echoProxy;
                return null;
            }).catch(function(error) {
                throw error;
            });
        } else {
            return Promise.resolve();
        }
    },
    shutdown : function() {
        return joynr.shutdown();
    },
    registerProvider : function() {
        var providerQos = new joynr.types.ProviderQos({
            customParameters : [],
            priority : Date.now(),
            scope : joynr.types.ProviderScope.LOCAL,
            supportsOnChangeSubscriptions : true
        });

        var EchoProvider = require("../generated-javascript/joynr/tests/performance/EchoProvider.js");
        var EchoProviderImpl = require("./EchoProviderImpl.js");
        var echoProvider = joynr.providerBuilder.build(
                EchoProvider,
                EchoProviderImpl.implementation);

        joynr.registration.registerProvider(options.domain, echoProvider, providerQos).then(function() {
            log("provider registered successfully");
        }).catch(function(error) {
            log("error registering provider: " + error.toString());
        });
    },
    executeBenchmark : function(benchmarkName, benchmark, numRuns) {
        var numRuns = numRuns ? numRuns : options.numRuns;
        console.log("call " + benchmarkName +" " + numRuns + " times");
        startTime = Date.now();
        var promises = [];

        for (var i = 1; i <= numRuns; i++) {
            promises.push(benchmark(i));
        }

        return Promise.all(promises).then(function() {
            console.log("all the numRuns were executed");
            var elapsedTimeMs = Date.now() - startTime;

            error(benchmarkName + " took " + elapsedTimeMs + " ms. " + numRuns / (elapsedTimeMs / 1000) + " msgs/s");
            return null;
        });
    },
    echoString : function() {
        var testProcedure = function(i) {
            var args = {
                data : PerformanceUtilities.createString(options.stringLength-2, "x") + "-" + i
            };
            return consumerBase.echoProxy.echoString(args).then(function(returnValues) {
                if (args.data !== returnValues.responseData) {
                    throw new Error("Echo " + JSON.stringify(returnValues.responseData) + " does not match input data: " + JSON.stringify(args.data));
                }
                return returnValues;
            });
        }
        return consumerBase.executeBenchmark("echoString", testProcedure);
    },
    echoComplexStruct : function() {
        var testProcedure = function(i) {
            var args = {
                data : new ComplexStruct({
                    num32 : PerformanceUtilities.createRandomNumber(100000),
                    num64 : PerformanceUtilities.createRandomNumber(1000000),
                    data : PerformanceUtilities.createByteArray(options.byteArrayLength, 1),
                    str : PerformanceUtilities.createString(options.stringLength-2, "x") + "-" + i
                })
            };
            return consumerBase.echoProxy.echoComplexStruct(args).then(function(returnValues) {
                if (args.data.num32 !== returnValues.responseData.num32 ||
                    args.data.num64 !== returnValues.responseData.num64 ||
                    args.data.data.length !== returnValues.responseData.data.length ||
                    args.data.str.length !== returnValues.responseData.str.length) {
                    throw new Error("Echo " + JSON.stringify(returnValues.responseData) + " does not match input data: " + JSON.stringify(args.data));
                }
                return returnValues;
            });
        }
        return consumerBase.executeBenchmark("echoComplexStruct", testProcedure);
    },
    echoByteArray : function(byteArraySizeFactor) {
        var byteArraySizeFactor = byteArraySizeFactor ? byteArraySizeFactor : 1;
        var byteArraySize = byteArraySizeFactor * options.byteArrayLength;
        var testProcedure = function(i) {
            var args = {
                data : PerformanceUtilities.createByteArray(byteArraySize, 1)
            };
            var firstElement = i % 128;
            args.data[0] = firstElement;
            return consumerBase.echoProxy.echoByteArray(args).then(function(returnValues) {
                if (args.data.length !== returnValues.responseData.length ||
                    firstElement !== returnValues.responseData[0]) {
                    throw new Error("Echo " + JSON.stringify(returnValues.responseData) + " does not match input data: " + JSON.stringify(args.data));
                }

                return returnValues;
            });
        }
        // the larger this byteArraySizeFactor is, the longer this test takes
        // in order to mitigate that, we scale the numer of runs by byteArraySizeFactor
        var numRuns = options.numRuns;
        if (byteArraySizeFactor > 1) {
            numRuns = numRuns / (Math.sqrt(byteArraySizeFactor));
        }
        return consumerBase.executeBenchmark("echoByteArray " + byteArraySize, testProcedure, numRuns);
    },
    echoByteArrayWithSizeTimesK : function() {
        if (options.skipByteArraySizeTimesK === undefined || options.skipByteArraySizeTimesK === true) {
            consumerBase.echoByteArray(1000);
        }
    }
};
module.exports = consumerBase;
