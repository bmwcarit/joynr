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
var ConsoleAppender = require("./ConsoleAppenderNode");
var JoynrLogger = require("./JoynrLogger");

var logLevelChangedCallbacks = [];
var LoggingManager = {};

LoggingManager.reset = function reset() {
    JoynrLogger.setOutput(ConsoleAppender.prototype.append);
};

/**
 * @function LoggingManager#getLogger
 * @param {String} name - the name of the logger
 *
 * @returns {JoynrLogger} a new Instance of JoynrLogger with the respective name.
 */
LoggingManager.getLogger = function getLogger(name) {
    return new JoynrLogger(name);
};

/**
 * configure takes a configuration object that conforms to a subset of the log4j2
 * json configuration
 *
 * @function LoggingManager#configure
 * @param {Config} settings - log4j2-style JSON config, but as JavaScript object
 *            (i.e. already parsed)
 */
LoggingManager.configure = function configure(settings) {
    if (settings.configuration) {
        if (
            settings.configuration.loggers &&
            settings.configuration.loggers.root &&
            settings.configuration.loggers.root.level
        ) {
            var level = settings.configuration.loggers.root.level;
            JoynrLogger.setLogLevel(level);
            logLevelChangedCallbacks.forEach(function(callback) {
                callback(level);
            });
        }
    }
    if (settings.appenderClasses && Object.keys(settings.appenderClasses).length > 0) {
        var appenderClassKey = Object.keys(settings.appenderClasses)[0];
        JoynrLogger.setOutput(settings.appenderClasses[appenderClassKey].prototype.append);
    }
};

LoggingManager.reset();

LoggingManager.registerForLogLevelChanged = function(callback) {
    logLevelChangedCallbacks.push(callback);
};

/**
 * @memberof LoggingManager
 * @readonly
 * @enum {String}
 */
LoggingManager.LogLevel = JoynrLogger.LogLevel;

module.exports = LoggingManager;
