/*jslint es5: true */
/*global fail: true, xit: true */

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

define(
    ["joynr/system/LoggingManager", "joynr/system/WebWorkerMessagingAppender", "joynr/system/LoggerFactory"],
    function(LoggingManager, WebWorkerMessagingAppender, LoggerFactory) {
        var log = LoggerFactory.getLogger("joynr.util.LoggingManager");
        var configWithArrays = {
            appenderClasses: {
                WebWorker: WebWorkerMessagingAppender
            },
            configuration: {
                appenders: {
                    appender: [
                        {
                            type: "Console",
                            name: "STDOUT",
                            PatternLayout: {
                                pattern: "%m%n"
                            }
                        },
                        {
                            type: "WebWorker",
                            name: "WEBWORKER",
                            ThresholdFilter: {
                                level: "debug"
                            }
                        }
                    ]
                },
                loggers: {
                    logger: [
                        {
                            name: "TestLogger1",
                            level: "info",
                            additivity: "false",
                            AppenderRef: {
                                ref: "STDOUT"
                            }
                        },
                        {
                            name: "TestLogger2",
                            level: "error",
                            additivity: "false",
                            AppenderRef: {
                                ref: "STDOUT"
                            }
                        }
                    ],
                    root: {
                        level: "error",
                        AppenderRef: {
                            ref: "STDOUT"
                        }
                    }
                }
            }
        };

        var configureWithKeys = {
            appenderClasses: {
                WebWorker: WebWorkerMessagingAppender
            },
            configuration: {
                name: "Test2",
                appenders: {
                    Console: {
                        name: "STDOUT",
                        PatternLayout: {
                            pattern: "%m%n"
                        }
                    },
                    WebWorker: {
                        name: "WEBWORKER",
                        ThresholdFilter: {
                            level: "debug"
                        }
                    }
                },
                loggers: {
                    logger: {
                        name: "TestLogger1",
                        level: "info",
                        additivity: "false",
                        AppenderRef: {
                            ref: "WEBWORKER"
                        }
                    },
                    TestLogger1: {
                        level: "info",
                        additivity: "false",
                        AppenderRef: {
                            ref: "STDOUT"
                        }
                    },
                    TestLogger2: {
                        level: "error",
                        additivity: "false",
                        AppenderRef: {
                            ref: "STDOUT"
                        }
                    },
                    root: {
                        level: "error",
                        AppenderRef: {
                            ref: "STDOUT"
                        }
                    }
                }
            }
        };

        describe("libjoynr-js.joynr.util.LoggingManager", function() {
            it("is instantiable", function(done) {
                expect(new LoggingManager()).toBeDefined();
                done();
            });

            it("is of correct type", function(done) {
                var loggingManager = new LoggingManager();
                expect(loggingManager).toBeDefined();
                expect(loggingManager).not.toBeNull();
                expect(typeof loggingManager === "object").toBeTruthy();
                expect(loggingManager instanceof LoggingManager).toEqual(true);
                done();
            });

            it("adds a console appender", function(done) {
                var loggingManager = new LoggingManager();
                var config = {
                    type: "Console",
                    name: "STDOUT"
                };

                var appender = loggingManager.createAppender(config);
                expect(loggingManager.getAppender(config.name)).toEqual(appender);
                // disabled: LoggerFactory.BrowserConsoleAppender does not exist
                //expect(
                //        loggingManager.getAppender(config.name) instanceof LoggerFactory.BrowserConsoleAppender)
                //        .toEqual(true);
                done();
            });

            it("adds an appender of registered type", function(done) {
                var loggingManager = new LoggingManager();
                var config = {
                    type: "MyAppenderType",
                    name: "MyAppender"
                };

                var MyAppenderType = function() {};

                loggingManager.registerAppenderClass("MyAppenderType", MyAppenderType);
                var appender = loggingManager.createAppender(config);
                expect(loggingManager.getAppender(config.name)).toEqual(appender);
                expect(loggingManager.getAppender(config.name) instanceof MyAppenderType).toEqual(true);
                done();
            });

            it("loads loggers and appenders as defined in a config with arrays", function(done) {
                var loggingManager = new LoggingManager();

                loggingManager.configure(configWithArrays);
                //expect(
                //        loggingManager.getAppender("STDOUT") instanceof LoggerFactory.BrowserConsoleAppender)
                //        .toEqual(true);
                expect(loggingManager.getAppender("WEBWORKER") instanceof WebWorkerMessagingAppender).toEqual(true);
                expect(loggingManager.getLogger("TestLogger1").getLevel()).toEqual(loggingManager.getLogLevel("info"));
                expect(loggingManager.getLogger("TestLogger2").getLevel()).toEqual(loggingManager.getLogLevel("error"));
                done();
            });

            it("loads loggers and appenders as defined in a config with object keys", function(done) {
                var loggingManager = new LoggingManager();

                loggingManager.configure(configureWithKeys);
                //expect(
                //        loggingManager.getAppender("STDOUT") instanceof LoggerFactory.BrowserConsoleAppender)
                //        .toEqual(true);
                expect(loggingManager.getAppender("WEBWORKER") instanceof WebWorkerMessagingAppender).toEqual(true);
                expect(loggingManager.getLogger("TestLogger1").getLevel()).toEqual(loggingManager.getLogLevel("info"));
                expect(loggingManager.getLogger("TestLogger2").getLevel()).toEqual(loggingManager.getLogLevel("error"));
                done();
            });

            xit("configures loggers with multiple appenders", function(done) {
                var configMultipleAppenderRefs = {
                    configuration: {
                        name: "TestLogging",
                        appenders: {
                            AppenderType1: {
                                name: "APPENDER1",
                                PatternLayout: {
                                    pattern: "%m%n"
                                }
                            },
                            AppenderType2: {
                                name: "APPENDER2",
                                ThresholdFilter: {
                                    level: "debug"
                                }
                            }
                        },
                        loggers: {
                            root: {
                                level: "debug",
                                AppenderRef: [
                                    {
                                        ref: "APPENDER1"
                                    },
                                    {
                                        ref: "APPENDER2"
                                    }
                                ]
                            }
                        }
                    }
                };

                var AppenderType1 = function() {};
                AppenderType1.prototype = new LoggerFactory.BrowserConsoleAppender();
                var AppenderType2 = function() {};
                AppenderType2.prototype = new LoggerFactory.BrowserConsoleAppender();

                spyOn(AppenderType1.prototype, "append");
                spyOn(AppenderType2.prototype, "append");

                var loggingManager = new LoggingManager();

                loggingManager.registerAppenderClass("AppenderType1", AppenderType1);
                loggingManager.registerAppenderClass("AppenderType2", AppenderType2);

                loggingManager.configure(configMultipleAppenderRefs);
                expect(loggingManager.getAppender("APPENDER1") instanceof AppenderType1).toEqual(true);
                expect(loggingManager.getAppender("APPENDER2") instanceof AppenderType2).toEqual(true);

                log = LoggerFactory.getRootLogger();
                expect(log.getLevel()).toEqual(loggingManager.getLogLevel("debug"));
                log.info("test1");

                expect(AppenderType1.prototype.append).toHaveBeenCalled();
                //expect(AppenderType2.prototype.append).toHaveBeenCalled();
                done();
            });
        });
    }
);
