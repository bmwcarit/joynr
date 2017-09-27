/*jslint node: true */

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
var log4javascript = require('../../lib/log4javascriptNode');
var ConsoleAppender = require('./ConsoleAppenderNode');

            /**
             * @name LoggingManager
             * @class
             */
            function LoggingManager() {
                var appenderTypes, rootLogger, appenders, loggingContexts, that;
                that = this;

                appenders = {};
                loggingContexts = {};

                appenderTypes = {};

                this.reset = function reset() {
                    log4javascript.resetConfiguration();
                    rootLogger = log4javascript.getRootLogger();
                    rootLogger.removeAllAppenders();
                    appenderTypes = {};
                    this.registerAppenderClass("Console", ConsoleAppender);
                };

                /**
                 * convert from a string representation of the log level to a type used by the
                 * logging system
                 *
                 * @function LoggingManager#getLogLevel
                 * @param {String}
                 *            logLevel - one of trace, debug, info, warn, error, fatal
                 * @returns {log4javascript.Level} the Level matching the given logLevel string, or
                 *            DEBUG if no match.
                 */
                this.getLogLevel = function getLogLevel(logLevel) {
                    switch (logLevel.toLowerCase()) {
                        case LoggingManager.LogLevel.TRACE:
                            return log4javascript.Level.TRACE;
                        case LoggingManager.LogLevel.DEBUG:
                            return log4javascript.Level.DEBUG;
                        case LoggingManager.LogLevel.INFO:
                            return log4javascript.Level.INFO;
                        case LoggingManager.LogLevel.WARN:
                            return log4javascript.Level.WARN;
                        case LoggingManager.LogLevel.ERROR:
                            return log4javascript.Level.ERROR;
                        case LoggingManager.LogLevel.FATAL:
                            return log4javascript.Level.FATAL;
                        default:
                            return log4javascript.Level.DEBUG;
                    }
                };

                /**
                 * The config may also contain other attributes specific to the appender type.
                 * For this reason the whole config object is also passed to the appender
                 * constructor.
                 *
                 * @function LoggingManager#createAppender
                 * @param {String} config.type - a previously registered type for the appender
                 * @param {String} config.name - the name for the newly created appender
                 * @param {String} [config.PatternLayout.pattern] - log4j pattern layout for the
                 *            logged messages
                 * @param {String} [config.ThresholdFilter.level] - which log level to log at for
                 *            this appender (once logger level has been applied).
                 *
                 * @returns {Appender} the created appender instance
                 */
                this.createAppender =
                        function createAppender(config) {
                            var AppenderConstructor, appender, layout;

                            AppenderConstructor = appenderTypes[config.type];
                            if (AppenderConstructor === undefined) {
                                throw new Error("unknown constructor for appender type "
                                    + config.type);
                            }

                            appender = new AppenderConstructor(config, loggingContexts);
                            if (config.ThresholdFilter !== undefined
                                && config.ThresholdFilter.level !== undefined) {
                                appender.setThreshold(this
                                        .getLogLevel(config.ThresholdFilter.level));
                            }

                            if (config.PatternLayout !== undefined
                                && config.PatternLayout.pattern !== undefined) {
                                layout =
                                        new log4javascript.PatternLayout(
                                                config.PatternLayout.pattern);
                                appender.setLayout(layout);
                            }

                            appenders[config.name] = appender;
                            return appender;
                        };

                /**
                 * @function LoggingManager#getAppender
                 */
                this.getAppender = function getAppender(name) {
                    return appenders[name];
                };

                /**
                 * @function LoggingManager#createLogger
                 * @param {String} config.name - The logger's name
                 * @param {String} config.level - base level for the logger
                 * @param {String} [config.additivity] - whether child loggers inherit their
                 *            parent's appenders
                 * @param {Object|Object[]} [config.AppenderRef] - an object or array of objects
                 *            containing the name(s) of previously created appender(s)
                 *            {String} [config.AppenderRef.ref] - the name of a previously created
                 *            appender
                 */
                this.createLogger =
                        function createLogger(config) {
                            var i, logger, appenderRefs, appenderRef, appender;

                            if (config.name.toLowerCase() === "root") {
                                logger = rootLogger;
                            } else {
                                logger = this.getLogger(config.name);
                            }

                            if (config.additivity !== undefined) {
                                logger.setAdditivity(config.additivity.toLowerCase() === "true");
                            }

                            if (config.level !== undefined) {
                                logger.setLevel(this.getLogLevel(config.level));
                            }

                            // appenderRef is either an array of refs, or a single ref under
                            // the key "ref"
                            appenderRefs = [];
                            if (config.AppenderRef !== undefined) {
                                if (config.AppenderRef.ref !== undefined) {
                                    appenderRefs.push(config.AppenderRef);
                                } else if (Object.prototype.toString.call(config.AppenderRef) === "[object Array]") {
                                    appenderRefs = config.AppenderRef;
                                }

                                for (i = 0; i < appenderRefs.length; i++) {
                                    appenderRef = appenderRefs[i];
                                    appender = appenders[appenderRef.ref];
                                    logger.addAppender(appender);
                                }
                            }
                        };

                /**
                 * @function LoggingManager#registerAppenderClass
                 * @param {String} appenderTypeName - the name of the appender type
                 * @param {function} constructor - a function that returns an instance of the given
                 *            appender type
                 */
                this.registerAppenderClass =
                        function registerAppenderClass(appenderTypeName, constructor) {
                            var appendFunction = constructor.prototype.append;
                            var toStringFunction = constructor.prototype.toString;
                            constructor.prototype = new log4javascript.Appender();
                            constructor.prototype.append = appendFunction;
                            constructor.prototype.toString = toStringFunction;
                            appenderTypes[appenderTypeName] = constructor;
                        };

                /**
                 * @function LoggingManager#getLogger
                 * @param {String} name - the name of the logger
                 *
                 * @returns {Object} the logger (creates a new one if it did not previously exist)
                 */
                this.getLogger = function getLogger(name) {
                    return log4javascript.getLogger(name);
                };

                /**
                 * This function adds an appender of the given type to a named logger
                 *
                 * @function LoggingManager#addAppenderToLogger
                 * @param {String}
                 *            logName - the name of the log to which the appender should be added
                 * @param {String}
                 *            appenderRef - the name of the appender as previously created
                 *
                 */
                this.addAppenderToLogger = function addAppender(logName, appenderRef) {
                    if (appenders[appenderRef] !== undefined) {
                        this.getLogger(logName).addAppender(appenders[appenderRef]);
                    }
                };

                /**
                 * @function LoggingManager#setLoggingContext
                 * @param {String}
                 *            participantId - the participantId is used by the logging appenders to
                 *            lookup the context
                 * @param {Object}
                 *            loggingContext - any object (ie key/value pairs) that are to be
                 *            appended to log messages.
                 */
                this.setLoggingContext = function setLoggingContext(participantId, loggingContext) {
                    loggingContexts[participantId] = loggingContext;
                };

                /**
                 * @function LoggingManager#deleteLoggingContext
                 * @param {String}
                 *            participantId - the participantId is used by the logging appenders to
                 *            lookup the context
                 */
                this.deleteLoggingContext = function deleteLoggingContext(participantId) {
                    delete loggingContexts[participantId];
                };

                /**
                 * get the appender array configs.
                 *
                 * @function LoggingManager#createConfiguredAppenders
                 * @param {Config} configuration - either in an array called appender
                 *            (within the appenders object), or stored as keys of the appenders
                 *            object.
                 *
                 * as shown in the test config from log4j:
                 * https://github.com/apache/logging-log4j2/blob/trunk/log4j-core/src/test/resources/log4j-routing2.json
                 *
                 */
                function createConfiguredAppenders(configuration) {
                    var i, appenderConfigs, keyConfig, config, appenderKey, appenders, appender;

                    appenderConfigs = configuration.appender || [];
                    for (appenderKey in configuration) {
                        if (configuration.hasOwnProperty(appenderKey)) {
                            if (appenderKey !== "appender") {
                                keyConfig = configuration[appenderKey];
                                keyConfig.type = appenderKey;
                                appenderConfigs.push(keyConfig);
                            }
                        }
                    }

                    appenders = {};
                    for (i = 0; i < appenderConfigs.length; i++) {
                        config = appenderConfigs[i];
                        appender = that.createAppender(config);
                        appenders[config.name] = appender;
                    }

                    return appenders;
                }

                /**
                 * get the loggers array, as shown in the test config from log4j:
                 * see {@link https://github.com/apache/logging-log4j2/blob/trunk/log4j-core/src/test/resources/log4j-routing2.json}
                 *
                 * @function LoggingManager#createConfiguredLoggers
                 */
                function createConfiguredLoggers(configuration) {
                    var i, loggerConfigs, keyConfig, config, loggerKey;

                    if (Object.prototype.toString.call(configuration.logger) === "[object Array]") {
                        loggerConfigs = configuration.logger;
                    } else {
                        loggerConfigs = [];
                    }

                    for (loggerKey in configuration) {
                        if (configuration.hasOwnProperty(loggerKey)) {
                            if (loggerKey !== "logger") {
                                keyConfig = configuration[loggerKey];
                                keyConfig.name = loggerKey;
                                loggerConfigs.push(keyConfig);
                            }
                        }
                    }

                    for (i = 0; i < loggerConfigs.length; i++) {
                        config = loggerConfigs[i];
                        that.createLogger(config);
                    }
                }

                /**
                 * configure takes a configuration object that conforms to a subset of the log4j2
                 * json configuration
                 *
                 * @function LoggingManager#configure
                 * @param {Config} settings - log4j2-style JSON config, but as JavaScript object
                 *            (i.e. already parsed)
                 */
                this.configure =
                        function configure(settings) {
                            if (settings.appenderClasses !== undefined) {
                                var appenderClassKey;
                                for (appenderClassKey in settings.appenderClasses) {
                                    if (settings.appenderClasses.hasOwnProperty(appenderClassKey)) {
                                        this.registerAppenderClass(
                                                appenderClassKey,
                                                settings.appenderClasses[appenderClassKey]);
                                    }
                                }
                            }
                            appenders =
                                    createConfiguredAppenders(settings.configuration.appenders
                                        || {});
                            createConfiguredLoggers(settings.configuration.loggers);
                        };

                this.shutdown = function() {
                    var i, appender;
                    for (i = 0; i < appenders.length; i++) {
                        appender = appenders[i];
                        if (appender !== undefined && appender.shtudown !== undefined) {
                            appender.shutdown();
                        }
                    }
                };

                this.reset();
            }

            /**
             * @memberof LoggingManager
             * @readonly
             * @enum {String}
             */
            LoggingManager.LogLevel = {
                TRACE : "trace",
                DEBUG : "debug",
                INFO : "info",
                WARN : "warn",
                ERROR : "error",
                FATAL : "fatal"
            };

            LoggingManager.Appender = log4javascript.Appender;
            LoggingManager.NullLayout = log4javascript.NullLayout;

module.exports = LoggingManager;
