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
require("../../node-unit-test-helper");
var LoggingManager = require("../../../../main/js/joynr/system/LoggingManager");
var JoynrLogger = require("../../../../main/js/joynr/system/JoynrLogger");

describe("libjoynr-js.joynr.util.LoggingManager", function() {
    beforeEach(function() {
        spyOn(JoynrLogger, "setLogLevel").and.callThrough();
        spyOn(JoynrLogger, "setFormatting").and.callThrough();
        spyOn(JoynrLogger, "setOutput").and.callThrough();
    });

    function expectNoCalls() {
        expect(JoynrLogger.setLogLevel).not.toHaveBeenCalled();
        expect(JoynrLogger.setFormatting).not.toHaveBeenCalled();
        expect(JoynrLogger.setOutput).not.toHaveBeenCalled();
    }

    it("will do nothing when the configuration is missing", function() {
        LoggingManager.configure({});
        expectNoCalls();

        LoggingManager.configure({ configuration: {} });
        expectNoCalls();

        LoggingManager.configure({ configuration: { loggers: {} } });
        expectNoCalls();

        LoggingManager.configure({ configuration: { loggers: { root: {} } } });
        expectNoCalls();

        LoggingManager.configure({ configuration: { appenders: {} } });
        expectNoCalls();

        LoggingManager.configure({ configuration: { appenders: { appender: [] } } });
        expectNoCalls();

        LoggingManager.configure({ configuration: { appenders: { appender: [{}] } } });
        expectNoCalls();

        LoggingManager.configure({ configuration: { appenders: { appender: [{ PatternLayout: {} }] } } });
        expectNoCalls();

        LoggingManager.configure({ appenderClasses: {} });
        expectNoCalls();
    });

    it("sets the loglevel of JoynrLogger when loglevel of the root logger is set", function() {
        LoggingManager.configure({ configuration: { loggers: { root: { level: JoynrLogger.LogLevel.DEBUG } } } });
        expect(JoynrLogger.setLogLevel).toHaveBeenCalledWith(JoynrLogger.LogLevel.DEBUG);
    });

    it("sets the formatting of JoynrLogger when a patternLayout is set", function() {
        var pattern = "some pattern %m";
        LoggingManager.configure({
            configuration: { appenders: { appender: [{ PatternLayout: { pattern: pattern } }] } }
        });
        expect(JoynrLogger.setFormatting).toHaveBeenCalledWith(pattern);
    });

    it("won't set the formatting of JoynrLogger when the patternLayout is %m", function() {
        var pattern = "%m";
        LoggingManager.configure({ configuration: { appenders: [{ PatternLayout: { pattern: pattern } }] } });
        expect(JoynrLogger.setFormatting).not.toHaveBeenCalled();
    });

    it("sets the output of JoynrLogger when an appenderClass is registered", function() {
        function SomeObject() {}
        SomeObject.prototype.append = function() {};
        LoggingManager.configure({ appenderClasses: { someName: SomeObject } });
        expect(JoynrLogger.setOutput).toHaveBeenCalledWith(SomeObject.prototype.append);
    });
});
