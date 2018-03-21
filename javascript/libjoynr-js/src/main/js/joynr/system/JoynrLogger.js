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
var trace, debug, info, warn, error, fatal;

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
        this.log(message, level);
    };
}

function createLogs() {
    trace = createLog(JoynrLogger.LogLevel.TRACE);
    debug = createLog(JoynrLogger.LogLevel.DEBUG);
    info = createLog(JoynrLogger.LogLevel.INFO);
    warn = createLog(JoynrLogger.LogLevel.WARN);
    error = createLog(JoynrLogger.LogLevel.ERROR);
    fatal = createLog(JoynrLogger.LogLevel.FATAL);
}

createLogs();

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
function logWithoutFormatting(message, level) {
    this.output({ level: { name: level }, messages: [message] });
}

function logWithFormatting(message, level) {
    var formattedMessage = this.format(message, level);
    this.output({ level: { name: level }, messages: [formattedMessage] });
}

JoynrLogger.prototype.log = logWithoutFormatting;

JoynrLogger.prototype.isDebugEnabled = function() {
    return this.level === JoynrLogger.LogLevel.DEBUG || this.level === JoynrLogger.LogLevel.TRACE;
};

function curryAddString(string) {
    return function(output, settings) {
        return output + string;
    };
}

function addDate(output, settings) {
    var date = new Date();
    return output + date.getHours() + ":" + date.getMinutes() + ":" + date.getSeconds() + "," + date.getMilliseconds();
}

function addLoggerName(output, settings) {
    return output + settings.name;
}

function addLogLevel(output, settings) {
    return output + settings.level;
}

function addMessage(output, settings) {
    return output + settings.message;
}

function createFormattingFunctionFromSteps(steps) {
    return function(message, level) {
        var i,
            output = "";
        var length = steps.length;
        var settings = {
            message: message,
            level: level,
            name: this.name
        };
        for (i = 0; i < length; i++) {
            output = steps[i](output, settings);
        }
        return output;
    };
}

JoynrLogger.setFormatting = function(pattern) {
    var steps = [];
    // pattern : "[%d{HH:mm:ss,SSS}][%c][%p] %m{2}"

    function checkRest(restString) {
        if (restString !== "") {
            steps.push(curryAddString(restString));
        }
    }

    var regexPattern = new RegExp("{[^}]*}", "g");
    // too complicated patterns are not supported
    var patternWithoutArguments = pattern.replace(regexPattern, "");

    var splitString = patternWithoutArguments.split("%");
    var firstElement = splitString.shift();
    if (firstElement !== "") {
        steps.push(curryAddString(firstElement));
    }
    while (splitString.length) {
        var subString = splitString.shift();
        var command = subString.charAt(0);
        subString = subString.slice(1);
        switch (command) {
            case "d":
                steps.push(addDate);
                checkRest(subString);
                break;
            case "c": // logger name
                steps.push(addLoggerName);
                checkRest(subString);
                break;
            case "p": // log level
                steps.push(addLogLevel);
                checkRest(subString);
                break;
            case "m": // message
                steps.push(addMessage);
                checkRest(subString);
                break;
            default:
                break;
        }
    }

    JoynrLogger.prototype.format = createFormattingFunctionFromSteps(steps);
    JoynrLogger.prototype.log = logWithFormatting;
};

module.exports = JoynrLogger;
