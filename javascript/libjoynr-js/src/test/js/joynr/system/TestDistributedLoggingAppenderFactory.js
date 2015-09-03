/*global joynrTestRequire: true */

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

joynrTestRequire(
        "joynr/system/TestDistributedLoggingAppenderFactory",
        [
            "global/Promise",
            "joynr/system/LoggingManager",
            "joynr/system/DistributedLoggingAppenderConstructorFactory",
            "joynr/system/DistributedLoggingAppender",
            "joynr/system/LoggerFactory"
        ],
        function(
                Promise,
                LoggingManager,
                DistributedLoggingAppenderConstructorFactory,
                DistributedLoggingAppender,
                LoggerFactory) {
            var log =
                    LoggerFactory
                            .getLogger("joynr.system.TestDistributedLoggingAppenderConstructorFactory");

            var configLogging = {
                configuration : {
                    name : "TestLogging",
                    appenders : {
                        Console : {
                            name : "STDOUT",
                            PatternLayout : {
                                pattern : "%m%n"
                            }
                        },
                        DistributedLogging : {
                            name : "DISTRIBUTED",
                            ThresholdFilter : {
                                level : "debug"
                            }
                        }
                    },
                    loggers : {
                        root : {
                            level : "debug",
                            "AppenderRef" : [
                                {
                                    "ref" : "STDOUT",
                                    "level" : "warn"
                                },
                                {
                                    "ref" : "DISTRIBUTED",
                                    "level" : "debug"
                                }
                            ]
                        }
                    }
                }
            };

            describe(
                    "libjoynr-js.joynr.system.DistributedLoggingAppenderConstructorFactory",
                    function() {

                        it("is instantiable", function() {
                            expect(DistributedLoggingAppenderConstructorFactory).toBeDefined();
                        });

                        it(
                                "creates a constructor of type DistributedLoggingAppender",
                                function() {
                                    var proxyBuilder, messagingQos, DistributedLoggingAppenderConstructor, newAppender;

                                    proxyBuilder = {
                                        build : function() {}
                                    };

                                    messagingQos = {};

                                    DistributedLoggingAppenderConstructor =
                                            DistributedLoggingAppenderConstructorFactory.build(
                                                    proxyBuilder,
                                                    messagingQos);
                                    newAppender = new DistributedLoggingAppenderConstructor();
                                    expect(newAppender.constructor).toEqual(
                                            DistributedLoggingAppender.prototype.constructor);

                                });

                        it(
                                "creates a loggingProxy and sets the proxy on the distributedLoggingAppender",
                                function() {
                                    var proxyBuilder, resolve, messagingQos, DistributedLoggingAppenderConstructor, newAppender, newProxy =
                                            {};

                                    proxyBuilder = {
                                        build : function() {}
                                    };
                                    spyOn(proxyBuilder, "build").andReturn(
                                            new Promise(function(internalResolve) {
                                                resolve = internalResolve;
                                            }));

                                    messagingQos = {};

                                    DistributedLoggingAppenderConstructor =
                                            DistributedLoggingAppenderConstructorFactory.build(
                                                    proxyBuilder,
                                                    messagingQos);
                                    newAppender = new DistributedLoggingAppenderConstructor();
                                    expect(newAppender.constructor).toEqual(
                                            DistributedLoggingAppender.prototype.constructor);

                                    spyOn(newAppender, "setProxy");
                                    expect(proxyBuilder.build).toHaveBeenCalled();

                                    resolve(newProxy);
                                    expect(newAppender.setProxy).toHaveBeenCalledWith(newProxy);
                                });

                    });
        });