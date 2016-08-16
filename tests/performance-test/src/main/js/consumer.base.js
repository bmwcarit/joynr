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
            console.log("Short-circuit-test: " + viaClusterController);
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

        joynr.capabilities.registerCapability("", options.domain, echoProvider, providerQos).then(function() {
            log("provider registered successfully");
        }).catch(function(error) {
            log("error registering provider: " + error.toString());
        });
    },
    executeBenchmark : function(benchmarkName, benchmark) {
        console.log("call " + benchmarkName +" " + options.numRuns + " times");
        startTime = Date.now();
        var promises = [];

        for (var i = 1; i <= options.numRuns; i++) {
            promises.push(benchmark(i));
        }

        return Promise.all(promises).then(function() {
            console.log("all the options.numRuns were executed");
            var elapsedTimeMs = Date.now() - startTime;

            error(benchmarkName + " took " + elapsedTimeMs + " ms. " + options.numRuns / (elapsedTimeMs / 1000) + " msgs/s");
            return null;
        });
    },
    echoString : function(echoProxy) {
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
    echoComplexStruct : function(echoProxy) {
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
                if (JSON.stringify(args.data) !== JSON.stringify(returnValues.responseData)) {
                    throw new Error("Echo " + JSON.stringify(returnValues.responseData) + " does not match input data: " + JSON.stringify(args.data));
                }
                return returnValues;
            });
        }
        return consumerBase.executeBenchmark("echoComplexStruct", testProcedure);
    },
    echoByteArray : function(echoProxy) {
        var args = {
            data : PerformanceUtilities.createByteArray(options.byteArrayLength, 1)
        };

        var testProcedure = function(i) {
            var firstElement = PerformanceUtilities.createRandomNumber(256) - 128;
            args.data[0] = firstElement;
            return consumerBase.echoProxy.echoByteArray(args).then(function(returnValues) {
                if (args.data.length !== returnValues.responseData.length ||
                    firstElement !== returnValues.responseData[0]) {
                    throw new Error("Echo " + JSON.stringify(returnValues.responseData) + " does not match input data: " + JSON.stringify(args.data));
                }

                return returnValues;
            });
        }
        return consumerBase.executeBenchmark("echoByteArray", testProcedure);
    }
};
module.exports = consumerBase;
