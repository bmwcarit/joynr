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
var noop = function() {};

function JoynrLogger(name) {
    this.name = name;
}

JoynrLogger.LogLevel = {
    TRACE: "trace",
    DEBUG: "debug",
    INFO: "info",
    WARN: "warn",
    ERROR: "error",
    FATAL: "fatal",
    OFF: "off"
};

function createLog(level) {
    return function(message) {
        this.output({ level: { name: level }, messages: [message] });
    };
}

var trace = createLog(JoynrLogger.LogLevel.TRACE);
var debug = createLog(JoynrLogger.LogLevel.DEBUG);
var info = createLog(JoynrLogger.LogLevel.INFO);
var warn = createLog(JoynrLogger.LogLevel.WARN);
var error = createLog(JoynrLogger.LogLevel.ERROR);
var fatal = createLog(JoynrLogger.LogLevel.FATAL);

JoynrLogger.setLogLevel = function(level) {
    JoynrLogger.prototype.trace = noop;
    JoynrLogger.prototype.debug = noop;
    JoynrLogger.prototype.info = noop;
    JoynrLogger.prototype.warn = noop;
    JoynrLogger.prototype.error = noop;
    JoynrLogger.prototype.fatal = noop;

    // jslint doesn't allow fallthrough and can't be disabled
    switch (level) {
        case JoynrLogger.LogLevel.TRACE:
            JoynrLogger.prototype.trace = trace;
            JoynrLogger.prototype.debug = debug;
            JoynrLogger.prototype.info = info;
            JoynrLogger.prototype.warn = warn;
            JoynrLogger.prototype.error = error;
            JoynrLogger.prototype.fatal = fatal;
            break;
        case JoynrLogger.LogLevel.DEBUG:
            JoynrLogger.prototype.debug = debug;
            JoynrLogger.prototype.info = info;
            JoynrLogger.prototype.warn = warn;
            JoynrLogger.prototype.error = error;
            JoynrLogger.prototype.fatal = fatal;
            break;
        case JoynrLogger.LogLevel.INFO:
            JoynrLogger.prototype.info = info;
            JoynrLogger.prototype.warn = warn;
            JoynrLogger.prototype.error = error;
            JoynrLogger.prototype.fatal = fatal;
            break;
        case JoynrLogger.LogLevel.WARN:
            JoynrLogger.prototype.warn = warn;
            JoynrLogger.prototype.error = error;
            JoynrLogger.prototype.fatal = fatal;
            break;
        case JoynrLogger.LogLevel.ERROR:
            JoynrLogger.prototype.error = error;
            JoynrLogger.prototype.fatal = fatal;
            break;
        case JoynrLogger.LogLevel.FATAL:
            JoynrLogger.prototype.fatal = fatal;
            break;
        case JoynrLogger.LogLevel.OFF:
            break;
        default:
            throw new Error("invalid log level " + level);
    }
    JoynrLogger.prototype.level = level;
};

JoynrLogger.prototype.name = "root";
JoynrLogger.prototype.level = "off";

JoynrLogger.prototype.trace = noop;
JoynrLogger.prototype.debug = noop;
JoynrLogger.prototype.info = noop;
JoynrLogger.prototype.warn = noop;
JoynrLogger.prototype.error = noop;
JoynrLogger.prototype.fatal = noop;

JoynrLogger.setOutput = function(outputFunction) {
    this.prototype.output = outputFunction;
};

JoynrLogger.prototype.output = noop;

JoynrLogger.prototype.isDebugEnabled = function() {
    return this.level === JoynrLogger.LogLevel.DEBUG || this.level === JoynrLogger.LogLevel.TRACE;
};

module.exports = JoynrLogger;
