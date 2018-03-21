/*jslint node: true */

/*
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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

var JoynrLogger = require("../../../classes/joynr/system/JoynrLogger");

describe("libjoynr-js.joynr.system.JoynrLogger", function() {
    var loggerInstance, loggingSpy;
    var loggerName = "joynrLogger";
    var message = "some message";

    beforeEach(function() {
        loggingSpy = jasmine.createSpy("loggingSpy");
        JoynrLogger.setOutput(loggingSpy);
        loggerInstance = new JoynrLogger(loggerName);
        jasmine.clock().install();
        jasmine.clock().mockDate();
    });

    afterEach(function() {
        jasmine.clock().uninstall();
    });

    it("uses the expected defaults without a config", function() {
        expect(loggerInstance.name).toEqual(loggerName);
        expect(loggerInstance.level).toEqual("off");
        expect(loggerInstance.isDebugEnabled()).toBeFalsy();

        loggerInstance.fatal(message);
        expect(loggingSpy).not.toHaveBeenCalled();
    });

    function loglevelHelper(level) {
        loggerInstance[level](message);
        expect(loggingSpy).not.toHaveBeenCalled();
        JoynrLogger.setLogLevel(level);
        loggerInstance[level](message);
        expect(loggingSpy).toHaveBeenCalled();
        loggingSpy.calls.reset();
    }

    it("works with different logLevels", function() {
        loglevelHelper(JoynrLogger.LogLevel.FATAL);
        loglevelHelper(JoynrLogger.LogLevel.ERROR);
        loglevelHelper(JoynrLogger.LogLevel.WARN);
        loglevelHelper(JoynrLogger.LogLevel.INFO);
        loglevelHelper(JoynrLogger.LogLevel.DEBUG);
        loglevelHelper(JoynrLogger.LogLevel.TRACE);
    });

    function logEventHelper(level) {
        loggerInstance[level](message);
        expect(loggingSpy).toHaveBeenCalledWith({ level: { name: level }, messages: [message] });
        loggingSpy.calls.reset();
    }

    it("adds the logLevel to the logEvent", function() {
        JoynrLogger.setLogLevel(JoynrLogger.LogLevel.TRACE);
        logEventHelper(JoynrLogger.LogLevel.TRACE);
        logEventHelper(JoynrLogger.LogLevel.DEBUG);
        logEventHelper(JoynrLogger.LogLevel.INFO);
        logEventHelper(JoynrLogger.LogLevel.WARN);
        logEventHelper(JoynrLogger.LogLevel.ERROR);
        logEventHelper(JoynrLogger.LogLevel.FATAL);
    });

    function getDateString() {
        var date = new Date();
        return date.getHours() + ":" + date.getMinutes() + ":" + date.getSeconds() + "," + date.getMilliseconds();
    }

    it("works with simple formatting", function() {
        JoynrLogger.setLogLevel("trace");
        JoynrLogger.setFormatting("%d%c%p%m");
        loggerInstance.debug(message);
        var formattedMessage = getDateString() + loggerName + "debug" + message;
        expect(loggingSpy).toHaveBeenCalledWith({ level: { name: "debug" }, messages: [formattedMessage] });
    });

    it("works with complicated formatting", function() {
        JoynrLogger.setLogLevel("trace");
        JoynrLogger.setFormatting("[%d{HH:mm:ss,SSS}][%c][%p] %m{2}");
        loggerInstance.debug(message);
        var formattedMessage = "[" + getDateString() + "][" + loggerName + "][debug] " + message;
        expect(loggingSpy).toHaveBeenCalledWith({ level: { name: "debug" }, messages: [formattedMessage] });
    });
});

/// pattern : "[%d{HH:mm:ss,SSS}][%c][%p] %m{2}"
