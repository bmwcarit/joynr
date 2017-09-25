/*jslint es5: true, node: true, node: true */
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
var Promise = require('../../../classes/global/Promise');
var LoggingManager = require('../../../classes/joynr/system/LoggingManager');
var DistributedLoggingAppenderConstructorFactory = require('../../../classes/joynr/system/DistributedLoggingAppenderConstructorFactory');
var DistributedLoggingAppender = require('../../../classes/joynr/system/DistributedLoggingAppender');
var LoggerFactory = require('../../../classes/joynr/system/LoggerFactory');
var WaitsFor = require('../../../test-classes/global/WaitsFor');
module.exports = (function (Promise, LoggingManager, DistributedLoggingAppenderConstructorFactory, DistributedLoggingAppender, LoggerFactory, waitsFor) {
            var asyncTimeout = 5000;

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

                        it("is instantiable", function(done) {
                            expect(DistributedLoggingAppenderConstructorFactory).toBeDefined();
                            done();
                        });

                        it(
                                "creates a constructor of type DistributedLoggingAppender",
                                function(done) {
                                    var proxyBuilder, messagingQos, DistributedLoggingAppenderConstructor, newAppender;

                                    proxyBuilder = {
                                        build : function() {
                                            return Promise.resolve();
                                        }
                                    };

                                    messagingQos = {};

                                    DistributedLoggingAppenderConstructor =
                                            DistributedLoggingAppenderConstructorFactory.build(
                                                    proxyBuilder,
                                                    messagingQos);
                                    newAppender = new DistributedLoggingAppenderConstructor();
                                    expect(newAppender.constructor).toEqual(
                                            DistributedLoggingAppender.prototype.constructor);
                                    done();
                                });

                        it(
                                "creates a loggingProxy and sets the proxy on the distributedLoggingAppender",
                                function(done) {
                                    var proxyBuilder, resolve, messagingQos, DistributedLoggingAppenderConstructor, newAppender, newProxy =
                                            {};

                                    proxyBuilder = {
                                        build : function() { }
                                    };
                                    spyOn(proxyBuilder, "build").and
                                            .returnValue(new Promise(
                                                    function(internalResolve) {
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

                                    waitsFor(function() {
                                        return newAppender.setProxy.calls.count() > 0;
                                    }, "setProxy has been called", asyncTimeout).then(function() {
                                        expect(newAppender.setProxy).toHaveBeenCalledWith(newProxy);
                                        done();
                                        return null;
                                    }).catch(fail);
                                });

                    });
}(Promise, LoggingManager, DistributedLoggingAppenderConstructorFactory, DistributedLoggingAppender, LoggerFactory, WaitsFor));