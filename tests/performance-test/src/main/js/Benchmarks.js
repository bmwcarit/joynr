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

var ComplexStruct = require("../generated-javascript/joynr/tests/performance/Types/ComplexStruct.js");
var PerformanceUtilities = require("./performanceutilities.js");
var options = PerformanceUtilities.getCommandLineOptionsOrDefaults(process.env);

function Benchmarks(proxy, loadedJoynr) {
    return {
        echoString: {
            generateData: function(i) {
                return {
                    data: PerformanceUtilities.createString(options.stringLength - 2, "x") + "-" + i
                };
            },
            testProcedure: function(args) {
                return proxy.echoString(args).then(function(returnValues) {
                    if (args.data !== returnValues.responseData) {
                        throw new Error(
                            "Echo " +
                                JSON.stringify(returnValues.responseData) +
                                " does not match input data: " +
                                JSON.stringify(args.data)
                        );
                    }
                    return returnValues;
                });
            }
        },

        attributeString: {
            generateData: function(i) {
                return {
                    data: PerformanceUtilities.createString(options.stringLength - 2, "x") + "-" + i
                };
            },
            testProcedure: function(args) {
                return proxy.simpleAttribute.set(args);
            }
        },

        echoComplexStruct: {
            generateData: function(i) {
                return {
                    data: new ComplexStruct({
                        num32: PerformanceUtilities.createRandomNumber(100000),
                        num64: PerformanceUtilities.createRandomNumber(1000000),
                        data: PerformanceUtilities.createByteArray(options.byteArrayLength, 1),
                        str: PerformanceUtilities.createString(options.stringLength - 2, "x") + "-" + i
                    })
                };
            },
            testProcedure: function(args) {
                return proxy.echoComplexStruct(args).then(function(returnValues) {
                    if (
                        args.data.num32 !== returnValues.responseData.num32 ||
                        args.data.num64 !== returnValues.responseData.num64 ||
                        args.data.data.length !== returnValues.responseData.data.length ||
                        args.data.str.length !== returnValues.responseData.str.length
                    ) {
                        throw new Error(
                            "Echo " +
                                JSON.stringify(returnValues.responseData) +
                                " does not match input data: " +
                                JSON.stringify(args.data)
                        );
                    }
                    return returnValues;
                });
            }
        },
        echoByteArray: {
            generateData: function(i) {
                var byteArraySize = options.byteArrayLength;
                var args = {
                    data: PerformanceUtilities.createByteArray(byteArraySize, 1)
                };
                var firstElement = i % 128;
                args.data[0] = firstElement;
                return args;
            },
            testProcedure: function(args) {
                return proxy.echoByteArray(args).then(function(returnValues, i) {
                    var firstElement = args.data[0];
                    if (
                        args.data.length !== returnValues.responseData.length ||
                        firstElement !== returnValues.responseData[0]
                    ) {
                        throw new Error(
                            "Echo " +
                                JSON.stringify(returnValues.responseData) +
                                " does not match input data: " +
                                JSON.stringify(args.data)
                        );
                    }

                    return returnValues;
                });
            }
        },
        registerPlentyOfConsumers: {
            generateData: function(i) {
                var data = {
                    data: PerformanceUtilities.createString(options.stringLength - 2, "x") + "-" + i
                };
                var number = i;
                return { data, number };
            },
            testProcedure: function(args) {
                var messagingQos = new loadedJoynr.messaging.MessagingQos({
                    ttl: 3600000
                });
                var EchoProxy = require("../generated-javascript/joynr/tests/performance/EchoProxy.js");
                return loadedJoynr.proxyBuilder
                    .build(EchoProxy, {
                        domain: options.domain,
                        messagingQos: messagingQos
                    })
                    .then(proxy => proxy.simpleAttribute.set(args.data));
            }
        }
    };
}

module.exports = Benchmarks;
